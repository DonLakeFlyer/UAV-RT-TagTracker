#include "CustomPlugin.h"
#include "Vehicle.h"
#include "CustomSettings.h"
#include "QGCApplication.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "FlyViewSettings.h"
#include "QmlComponentInfo.h"
#include "TunnelProtocol.h"
#include "PulseInfoList.h"
#include "FTPManager.h"

#include <QDebug>
#include <QPointF>
#include <QLineF>

using namespace TunnelProtocol;

QGC_LOGGING_CATEGORY(CustomPluginLog, "CustomPluginLog")

CustomPlugin::CustomPlugin(QGCApplication *app, QGCToolbox* toolbox)
#ifdef HERELINK_BUILD
    : HerelinkCorePlugin    (app, toolbox)
#else
    : QGCCorePlugin         (app, toolbox)
#endif
    , _vehicleStateIndex    (0)
    , _flightStateMachineActive  (false)
    , _vehicleFrequency     (0)
    , _lastPulseSendIndex   (-1)
    , _missedPulseCount     (0)
{
    static_assert(TunnelProtocolValidateSizes, "TunnelProtocolValidateSizes failed");

    _vehicleStateTimeoutTimer.setSingleShot(true);
    _tunnelCommandAckTimer.setSingleShot(true);
    _tunnelCommandAckTimer.setInterval(2000);
    _controllerHeartbeatTimer.setSingleShot(true);
    _controllerHeartbeatTimer.setInterval(6000);    // We should get heartbeats every 5 seconds

    connect(&_vehicleStateTimeoutTimer,     &QTimer::timeout, this, &CustomPlugin::_vehicleStateWaitFailed);
    connect(&_tunnelCommandAckTimer,        &QTimer::timeout, this, &CustomPlugin::_tunnelCommandAckFailed);
    connect(&_controllerHeartbeatTimer,     &QTimer::timeout, this, &CustomPlugin::_controllerHeartbeatFailed);
}

CustomPlugin::~CustomPlugin()
{
    _csvStopFullPulseLog();
    _csvStopRotationPulseLog();
}

void CustomPlugin::setToolbox(QGCToolbox* toolbox)
{
#ifdef HERELINK_BUILD
    HerelinkCorePlugin::setToolbox(toolbox);
#else
    QGCCorePlugin::setToolbox(toolbox);
#endif

    _customSettings = new CustomSettings(nullptr);
    _customOptions = new CustomOptions(this, nullptr);

    _tagInfoList.checkForTagFile();

    _csvClearPrevRotationLogs();
}

QVariantList& CustomPlugin::settingsPages(void)
{
    if(_settingsPages.size() == 0) {
        _settingsPages = QGCCorePlugin::settingsPages();

        QmlComponentInfo* customPage = new QmlComponentInfo(tr("Tags"),
                                                  QUrl::fromUserInput("qrc:/qml/CustomSettings.qml"),
                                                  QUrl::fromUserInput("qrc:/res/gear-white.svg"));
        _settingsPages.append(QVariant::fromValue(reinterpret_cast<QmlComponentInfo*>(customPage)));

    }

    return _settingsPages;
}

bool CustomPlugin::mavlinkMessage(Vehicle* vehicle, LinkInterface* linkInterface, mavlink_message_t message)
{
    #if 0
#ifdef HERELINK_BUILD
    if (!HerelinkCorePlugin::mavlinkMessage(vehicle, linkInterface, message)) {
        return false;
    }
#endif
#endif

    if (message.msgid == MAVLINK_MSG_ID_TUNNEL) {
        mavlink_tunnel_t tunnel;

        mavlink_msg_tunnel_decode(&message, &tunnel);

        HeaderInfo_t header;
        memcpy(&header, tunnel.payload, sizeof(header));

        switch (header.command) {
        case COMMAND_ID_ACK:
            _handleTunnelCommandAck(tunnel);
            break;
        case COMMAND_ID_PULSE:
            _handleTunnelPulse(tunnel);
            break;
        case COMMAND_ID_HEARTBEAT:
            _handleTunnelHeartbeat(tunnel);
            break;
        }

        return false;
    } else {
        return true;
    }
}

void CustomPlugin::_handleTunnelHeartbeat(const mavlink_tunnel_t& tunnel)
{
    static uint32_t counter = 0;

    Heartbeat_t heartbeat;

    memcpy(&heartbeat, tunnel.payload, sizeof(heartbeat));

    switch (heartbeat.system_id) {
    case HEARTBEAT_SYSTEM_MAVLINKCONTROLLER:
        qCDebug(CustomPluginLog) << "HEARTBEAT from MavlinkTagController" << counter++;
        _controllerHeartbeat = true;
        emit controllerHeartbeatChanged();
        _controllerHeartbeatTimer.start();
        break;
    case HEARTBEAT_SYSTEM_CHANNELIZER:
        qCDebug(CustomPluginLog) << "HEARTBEAT from Channelizer";
        break;
    }
}

void CustomPlugin::_handleTunnelCommandAck(const mavlink_tunnel_t& tunnel)
{
    AckInfo_t ack;

    memcpy(&ack, tunnel.payload, sizeof(ack));

    if (ack.command == _tunnelCommandAckExpected) {
        _tunnelCommandAckExpected = 0;
        _tunnelCommandAckTimer.stop();

        qCDebug(CustomPluginLog) << "Tunnel command ack received - command:result" << _tunnelCommandIdToText(ack.command) << ack.result;
        if (ack.result == COMMAND_RESULT_SUCCESS) {
            switch (ack.command) {
            case COMMAND_ID_START_TAGS:
            case COMMAND_ID_TAG:
                _sendNextTag();
                break;
            case COMMAND_ID_END_TAGS:
                _resetPulseLog();
                break;
            case COMMAND_ID_START_DETECTION:
                _csvStartFullPulseLog();
                break;
            case COMMAND_ID_STOP_DETECTION:
                _csvStopFullPulseLog();
                break;
            }
        } else {
            QString message = QStringLiteral("%1 command failed").arg(_tunnelCommandIdToText(ack.command));

            _say(message);
            qgcApp()->showAppMessage(message);

        }

    } else {
        qWarning() << "_handleTunnelCommandAck: Received unexpected command id ack expected:actual" <<
                      _tunnelCommandIdToText(_tunnelCommandAckExpected) <<
                      _tunnelCommandIdToText(ack.command);
    }
}

void CustomPlugin::_handleTunnelPulse(const mavlink_tunnel_t& tunnel)
{
    if (tunnel.payload_length != sizeof(PulseInfo_t)) {
        qWarning() << "_handleTunnelPulse Received incorrectly sized PulseInfo payload expected:actual" <<  sizeof(PulseInfo_t) << tunnel.payload_length;
    }

    if (_tagInfoList.empty()) {
        // Vehicle sending still sending pulses but QGC has not yet loaded any.
        // This can happen if you restart QGC while the vehicle is still running.
        qCDebug(CustomPluginLog) << "No tags loaded, ignoring pulse";
        return;
    }

    PulseInfo_t pulseInfo;
    memcpy(&pulseInfo, tunnel.payload, sizeof(pulseInfo));

    if (pulseInfo.confirmed_status || pulseInfo.frequency_hz == 0) {
        bool extTagInfoExists   = false;
        auto evenTagId          = pulseInfo.tag_id - (pulseInfo.tag_id % 2);
        auto extTagInfo         = _tagInfoList.getTagInfo(evenTagId, extTagInfoExists);
        bool rate2Tag           = pulseInfo.tag_id % 2;

        if (!extTagInfoExists) {
            qWarning() << "_handleTunnelPulse: Received pulse for unknown tag_id" << pulseInfo.tag_id;
            return;
        }

        if (_flightStateMachineActive) {
            _rotationPulseInfoMap[pulseInfo.tag_id].append(pulseInfo);
        }

        QString tagRateChar(rate2Tag ? extTagInfo.ip_msecs_2_id : extTagInfo.ip_msecs_1_id);

        if (pulseInfo.frequency_hz != 0) {
            qCDebug(CustomPluginLog) << Qt::fixed << qSetRealNumberPrecision(2) <<
                                        "CONFIRMED tag_id:frequency_hz:seq_ctr:snr" <<
                                        pulseInfo.tag_id <<
                                        pulseInfo.frequency_hz <<
                                        pulseInfo.group_seq_counter <<
                                        pulseInfo.snr;
            _csvLogPulse(_csvFullPulseLogFile, pulseInfo);
            _csvLogPulse(_csvRotationPulseLogFile, pulseInfo);
        } else {
            tagRateChar = "H" + tagRateChar;
            qCDebug(CustomPluginLog) << "HEARTBEAT from Detector" << pulseInfo.tag_id;
            _updateDetectorHeartbeat(pulseInfo.tag_id);
        }

        // Find the tag in the pulse log
        bool foundTag = false;
        for (int i=0; i<_pulseLog.count(); i++) {
            auto listModel = qvariant_cast<PulseInfoList*>(_pulseLog[i]);
            if (listModel->tagId() == evenTagId) {
                auto pInfo = new PulseInfo(pulseInfo, extTagInfo.name, tagRateChar);
                listModel->insert(0, pInfo);
                foundTag = true;
                break;
            }
        }
        if (!foundTag) {
            qWarning() << "_handleTunnelPulse: Unable to find tag in pulse log" << evenTagId;
        }

        // Add pulse to map
        if (_customSettings->showPulseOnMap()->rawValue().toBool() && pulseInfo.snr != 0) {
            QUrl url = QUrl::fromUserInput("qrc:/qml/PulseMapItem.qml");
            PulseMapItem* mapItem = new PulseMapItem(url, QGeoCoordinate(pulseInfo.position_x, pulseInfo.position_y), pulseInfo.tag_id, pulseInfo.snr, this);
            _customMapItems.append(mapItem);
        }
    } else {
        qCDebug(CustomPluginLog) << Qt::fixed << qSetRealNumberPrecision(2) <<
                                    "UNCONFIRMED tag_id" <<
                                    pulseInfo.tag_id;
    }

}

QString CustomPlugin::_csvLogFilePath(void)
{
    return qgcApp()->toolbox()->settingsManager()->appSettings()->logSavePath();
}

void CustomPlugin::_csvStartFullPulseLog(void)
{
    if (_csvFullPulseLogFile.isOpen()) {
        qgcApp()->showAppMessage("Unabled to open full pulse csv log file - csvFile already open");
        return;
    }

    _csvFullPulseLogFile.setFileName(QString("%1/Pulse-%2.csv").arg(_csvLogFilePath(), QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss-zzz").toLocal8Bit().data()));
    qCDebug(CustomPluginLog) << "Full CSV Pulse logging to:" << _csvFullPulseLogFile;
    if (!_csvFullPulseLogFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qgcApp()->showAppMessage(QString("Open of full pulse csv log file failed: %1").arg(_csvFullPulseLogFile.errorString()));
        return;
    }
}

void CustomPlugin::_csvStopFullPulseLog(void)
{
    if (_csvFullPulseLogFile.isOpen()) {
        _csvFullPulseLogFile.close();
    }
}

void CustomPlugin::_csvClearPrevRotationLogs(void)
{
    QDir csvLogDir(_csvLogFilePath(), {"Rotation-*.csv"});
    for (const QString & filename: csvLogDir.entryList()){
        csvLogDir.remove(filename);
    }
}

void CustomPlugin::_csvStartRotationPulseLog(int rotationCount)
{
    if (_csvRotationPulseLogFile.isOpen()) {
        qgcApp()->showAppMessage("Unabled to open rotation pulse csv log file - csvFile already open");
        return;
    }

    _csvRotationPulseLogFile.setFileName(QString("%1/Rotation-%2.csv").arg(_csvLogFilePath()).arg(rotationCount));
    qCDebug(CustomPluginLog) << "Rotation CSV Pulse logging to:" << _csvRotationPulseLogFile;
    if (!_csvRotationPulseLogFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qgcApp()->showAppMessage(QString("Open of rotation pulse csv log file failed: %1").arg(_csvRotationPulseLogFile.errorString()));
        return;
    }
}

void CustomPlugin::_csvStopRotationPulseLog(void)
{
    if (_csvRotationPulseLogFile.isOpen()) {
        _csvLogRotationStartStop(_csvRotationPulseLogFile, false /* startRotation */);
        _csvRotationPulseLogFile.close();
    }
}

void CustomPlugin::_csvLogPulse(QFile& csvFile, const PulseInfo_t& pulseInfo)
{
    if (csvFile.isOpen()) {
        csvFile.write(QString("%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12,%13,%14,%15,%16,%17,%18,%19\n")
            .arg(COMMAND_ID_PULSE)
            .arg(pulseInfo.tag_id)
            .arg(pulseInfo.frequency_hz)
            .arg(pulseInfo.start_time_seconds,          0, 'f', 6)
            .arg(pulseInfo.predict_next_start_seconds,  0, 'f', 6)
            .arg(pulseInfo.snr,                         0, 'f', 6)
            .arg(pulseInfo.stft_score,                  0, 'f', 6)
            .arg(pulseInfo.group_seq_counter)
            .arg(pulseInfo.group_ind)
            .arg(pulseInfo.group_snr,                   0, 'f', 6)
            .arg(pulseInfo.detection_status)
            .arg(pulseInfo.confirmed_status)
            .arg(pulseInfo.position_x,                  0, 'f', 6)
            .arg(pulseInfo.position_y,                  0, 'f', 6)
            .arg(pulseInfo.position_z,                  0, 'f', 6)
            .arg(pulseInfo.orientation_x,               0, 'f', 6)
            .arg(pulseInfo.orientation_y,               0, 'f', 6)
            .arg(pulseInfo.orientation_z,               0, 'f', 6)
            .arg(pulseInfo.orientation_w,               0, 'f', 6)
            .toUtf8());
    }
}

void CustomPlugin::_csvLogRotationStartStop(QFile& csvFile, bool startRotation)
{
    Vehicle* vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
    if (!vehicle) {
        qCWarning(CustomPluginLog) << "INTERNAL ERROR: _csvLogRotationStartStop - no vehicle available";
        return;
    }

    if (csvFile.isOpen()) {
        QGeoCoordinate coord = vehicle->coordinate();
        csvFile.write(QString("%1,%2,%3,%4\n").arg(startRotation ? COMMAND_ID_START_ROTATION : COMMAND_ID_STOP_ROTATION)
                      .arg(coord.latitude(), 0, 'f', 6)
                      .arg(coord.longitude(), 0, 'f', 6)
                      .arg(vehicle->altitudeAMSL()->rawValue().toDouble(), 0, 'f', 6)
                      .toUtf8());
    }
}

void CustomPlugin::_updateFlightMachineActive(bool flightMachineActive)
{
    _flightStateMachineActive = flightMachineActive;
    emit flightMachineActiveChanged(flightMachineActive);
}

void CustomPlugin::cancelAndReturn(void)
{
    _say("Cancelling flight.");
    _resetStateAndRTL();
}

void CustomPlugin::startRotation(void)
{
    qCDebug(CustomPluginLog) << "startRotation";

    Vehicle* vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();

    if (!vehicle) {
        return;
    }

    _csvStartRotationPulseLog(_csvRotationCount++);
    _csvLogRotationStartStop(_csvFullPulseLogFile, true /* startRotation */);
    _csvLogRotationStartStop(_csvRotationPulseLogFile, true /* startRotation */);

    _updateFlightMachineActive(true);

    // Setup new rotation data
    _rgPulseValues.clear();
    _rgAngleStrengths.append(QList<double>());
    _rgAngleRatios.append(QList<double>());

    QList<double>&  angleStrengths =    _rgAngleStrengths.last();
    QList<double>&  angleRatios =       _rgAngleRatios.last();

    // Prime angle strengths with no values
    _cSlice = _customSettings->divisions()->rawValue().toInt();
    for (int i=0; i<_cSlice; i++) {
        angleStrengths.append(qQNaN());
        angleRatios.append(qQNaN());
    }
    emit angleRatiosChanged();

    // Create compass rose ui on map
    QUrl url = QUrl::fromUserInput("qrc:/qml/CustomPulseRoseMapItem.qml");
    PulseRoseMapItem* mapItem = new PulseRoseMapItem(url, _rgAngleStrengths.count() - 1, vehicle->coordinate(), this);
    _customMapItems.append(mapItem);

    // First heading is adjusted to be at the center of the closest subdivision
    double vehicleHeading = vehicle->heading()->rawValue().toDouble();
    double divisionDegrees = 360.0 / _cSlice;
    _firstSlice = _nextSlice = static_cast<int>((vehicleHeading + (divisionDegrees / 2.0)) / divisionDegrees);
    _firstHeading = divisionDegrees * _firstSlice;

    _vehicleStates.clear();

    // Build rotation state machine entries
    double headingIncrement = 360.0 / _cSlice;
    double nextHeading = _firstHeading - headingIncrement;
    for (int i=0; i<_cSlice; i++) {
        VehicleState_t vehicleState;

        nextHeading += headingIncrement;
        if (nextHeading >= 360) {
            nextHeading -= 360;
        }

        vehicleState.command =              CommandSetHeading;
        vehicleState.fact =                 vehicle->heading();
        vehicleState.targetValueWaitSecs =  10;
        vehicleState.targetValue =          nextHeading;
        vehicleState.targetVariance =       1;
        _vehicleStates.append(vehicleState);

        vehicleState.command =              CommandWaitForHeartbeats;
        vehicleState.fact =                 nullptr;
        vehicleState.targetValueWaitSecs =  30; // Should be enough for 3 k=3 groupings at 2 second intra-pulse interval
        _vehicleStates.append(vehicleState);
    }

    _captureRotationPulses  = true;
    _vehicleStateIndex      = -1;
    _retryRotation          = false;
    _advanceStateMachine();
}

void CustomPlugin::startDetection(void)
{
    StartDetectionInfo_t startDetectionInfo;

    memset(&startDetectionInfo, 0, sizeof(startDetectionInfo));

    startDetectionInfo.header.command               = COMMAND_ID_START_DETECTION;
    startDetectionInfo.radio_center_frequency_hz    = _tagInfoList.radioCenterHz();
    startDetectionInfo.sdr_type                     = _customSettings->sdrType()->rawValue().toUInt();

    _sendTunnelCommand((uint8_t*)&startDetectionInfo, sizeof(startDetectionInfo));

    _setupDetectorHeartbeats();
}

void CustomPlugin::stopDetection(void)
{
    StopDetectionInfo_t stopDetectionInfo;

    stopDetectionInfo.header.command = COMMAND_ID_STOP_DETECTION;
    _sendTunnelCommand((uint8_t*)&stopDetectionInfo, sizeof(stopDetectionInfo));

    _clearDetectorHeartbeats();
}

void CustomPlugin::rawCapture(void)
{
    RawCapture_t rawCapture;

    rawCapture.header.command   = COMMAND_ID_RAW_CAPTURE;
    rawCapture.sdr_type         = _customSettings->sdrType()->rawValue().toUInt();

    _sendTunnelCommand((uint8_t*)&rawCapture, sizeof(rawCapture));
}

void CustomPlugin::sendTags(void)
{    
    _tagInfoList.loadTags(_customSettings->sdrType()->rawValue().toUInt());

    if (_tagInfoList.size() == 0) {
        qgcApp()->showAppMessage(("No tags are available to send."));
        return;
    }
    _nextTagToSend = _tagInfoList.begin();

    StartTagsInfo_t startTagsInfo;

    startTagsInfo.header.command    = COMMAND_ID_START_TAGS;
    startTagsInfo.sdr_type          = _customSettings->sdrType()->rawValue().toUInt();

    _sendTunnelCommand((uint8_t*)&startTagsInfo, sizeof(startTagsInfo));
}

void CustomPlugin::_sendNextTag(void)
{
    // Don't send tags too fast
    QGC::SLEEP::msleep(100);

    if (_nextTagToSend == _tagInfoList.end()) {
        _sendEndTags();
    } else {
        auto refExtTagInfo = *_nextTagToSend;
        ExtendedTagInfo_t extTagInfo = refExtTagInfo;   // make a copy

        _sendTunnelCommand((uint8_t*)&extTagInfo.tagInfo, sizeof(extTagInfo.tagInfo));

        _nextTagToSend++;
    }
}

void CustomPlugin::_sendEndTags(void)
{
    EndTagsInfo_t endTagsInfo;

    endTagsInfo.header.command = COMMAND_ID_END_TAGS;
    _sendTunnelCommand((uint8_t*)&endTagsInfo, sizeof(endTagsInfo));
}

void CustomPlugin::_sendCommandAndVerify(Vehicle* vehicle, MAV_CMD command, double param1, double param2, double param3, double param4, double param5, double param6, double param7)
{
    connect(vehicle, &Vehicle::mavCommandResult, this, &CustomPlugin::_mavCommandResult);
    vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1,
                            command,
                            false /* showError */,
                            static_cast<float>(param1),
                            static_cast<float>(param2),
                            static_cast<float>(param3),
                            static_cast<float>(param4),
                            static_cast<float>(param5),
                            static_cast<float>(param6),
                            static_cast<float>(param7));
}

void CustomPlugin::_mavCommandResult(int vehicleId, int component, int command, int result, bool noResponseFromVehicle)
{
    Q_UNUSED(vehicleId);
    Q_UNUSED(component);

    Vehicle* vehicle = dynamic_cast<Vehicle*>(sender());
    if (!vehicle) {
        qWarning() << "Dynamic cast failed!";
        return;
    }

    if (!_flightStateMachineActive) {
        disconnect(vehicle, &Vehicle::mavCommandResult, this, &CustomPlugin::_mavCommandResult);
        return;
    }

    const VehicleState_t& currentState = _vehicleStates[_vehicleStateIndex];

    if (currentState.command == CommandTakeoff && command == MAV_CMD_NAV_TAKEOFF) {
        disconnect(vehicle, &Vehicle::mavCommandResult, this, &CustomPlugin::_mavCommandResult);
        if (noResponseFromVehicle) {
            _say(QStringLiteral("Vehicle did not respond to takeoff command"));
            _updateFlightMachineActive(false);
        } else if (result != MAV_RESULT_ACCEPTED) {
            _say(QStringLiteral("takeoff command failed"));
            _updateFlightMachineActive(false);
        }
    } else if (currentState.command == CommandSetHeading && command == (vehicle->px4Firmware() ? MAV_CMD_DO_REPOSITION : MAV_CMD_CONDITION_YAW)) {
        disconnect(vehicle, &Vehicle::mavCommandResult, this, &CustomPlugin::_mavCommandResult);
        if (noResponseFromVehicle) {
            if (_retryRotation) {
                _retryRotation = false;
                _say(QStringLiteral("Vehicle did not response to Rotate command. Retrying."));
                _rotateVehicle(vehicle, _vehicleStates[_vehicleStateIndex].targetValue);
            } else {
                _say(QStringLiteral("Vehicle did not respond to Rotate command. Flight cancelled. Vehicle returning."));
                _resetStateAndRTL();
            }
        } else if (result != MAV_RESULT_ACCEPTED) {
            _say(QStringLiteral("Rotate command failed. Flight cancelled. Vehicle returning."));
            _resetStateAndRTL();
        }
    }
}

void CustomPlugin::_takeoff(Vehicle* vehicle, double takeoffAltRel)
{
    double vehicleAltitudeAMSL = vehicle->altitudeAMSL()->rawValue().toDouble();
    if (qIsNaN(vehicleAltitudeAMSL)) {
        qgcApp()->informationMessageBoxOnMainThread(tr("Error"), tr("Unable to takeoff, vehicle position not known."));
        return;
    }

    double takeoffAltAMSL = takeoffAltRel + vehicleAltitudeAMSL;

    _sendCommandAndVerify(
                vehicle,
                MAV_CMD_NAV_TAKEOFF,
                -1,                             // No pitch requested
                0, 0,                           // param 2-4 unused
                qQNaN(), qQNaN(), qQNaN(),      // No yaw, lat, lon
                takeoffAltAMSL);                // AMSL altitude
}



void CustomPlugin::_rotateVehicle(Vehicle* vehicle, double headingDegrees)
{
    if (vehicle->px4Firmware()) {
        _sendCommandAndVerify(
            vehicle,
            MAV_CMD_DO_REPOSITION,
            -1,                                     // no change in ground speed
            MAV_DO_REPOSITION_FLAGS_CHANGE_MODE,    // switch to guided mode
            0,                                      // reserved
            qDegreesToRadians(headingDegrees),      // change heading
            qQNaN(), qQNaN(), qQNaN());             // no change lat, lon, alt
    } else if (vehicle->apmFirmware()){
        _sendCommandAndVerify(
            vehicle,
            MAV_CMD_CONDITION_YAW,
            headingDegrees,
            0,                                      // Use default angular speed
            1,                                      // Rotate clockwise
            0,                                      // heading specified as absolute angle
            0, 0, 0);                               // Unused
    }
}


void CustomPlugin::_setupDelayForHeartbeats(void)
{
    // Start all heartbeat counts at zero
    for (auto& info: _detectorHeartbeatInfoMap) {
        info.heartbeatCount = 0;
    }

    // Clear previous rotation pulse info
    _rotationPulseInfoMap.clear();

    _rgPulseValues.clear();
}

void CustomPlugin::_advanceStateMachine(void)
{
    Vehicle* vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();

    if (!vehicle) {
        return;
    }

    // Clear previous state
    if (_vehicleStateIndex > 0) {
        const VehicleState_t& previousState = _vehicleStates[_vehicleStateIndex];

        if (previousState.targetValueWaitSecs) {
            _vehicleStateTimeoutTimer.stop();
            if (previousState.fact) {
                disconnect(previousState.fact, &Fact::rawValueChanged, this, &CustomPlugin::_vehicleStateRawValueChanged);
            }
        }

        _rotationPulseInfoMap.clear();
        _retryRotation = false;
    }

    if (_vehicleStateIndex == _vehicleStates.count() - 1) {
        // State machine complete
        _say(QStringLiteral("Collection complete."));
        _updateFlightMachineActive(false);
        _csvStopRotationPulseLog();
        return;
    }

    const VehicleState_t& currentState = _vehicleStates[++_vehicleStateIndex];

    QString holdFlightMode(vehicle->px4Firmware() ? "Hold" : "Guided");
    if (currentState.command != CommandTakeoff && vehicle->flightMode() != "Takeoff" && vehicle->flightMode() != holdFlightMode) {
        // User cancel
        _say(QStringLiteral("Collection cancelled."));
        _updateFlightMachineActive(false);
        _csvStopRotationPulseLog();
        return;
    }

    switch (currentState.command) {
    case CommandTakeoff:
        // Takeoff to specified altitude
        _say(QStringLiteral("Waiting for takeoff to %1 %2.").arg(FactMetaData::metersToAppSettingsVerticalDistanceUnits(currentState.targetValue).toDouble()).arg(FactMetaData::appSettingsVerticalDistanceUnitsString()));
        _takeoff(vehicle, currentState.targetValue);
        break;
    case CommandSetHeading:
        _say(QStringLiteral("Waiting for rotate to %1 degrees.").arg(qRound(currentState.targetValue)));
        _retryRotation = true;
        _rotateVehicle(vehicle, currentState.targetValue);
        break;
    case CommandWaitForHeartbeats:
        _say(QStringLiteral("Collecting data for %1 seconds max").arg(currentState.targetValueWaitSecs));
        _setupDelayForHeartbeats();
        break;
    }

    if (currentState.targetValueWaitSecs) {
        _vehicleStateTimeoutTimer.setInterval(currentState.targetValueWaitSecs * 1000);
        _vehicleStateTimeoutTimer.start();
        if (currentState.fact) {
            connect(currentState.fact, &Fact::rawValueChanged, this, &CustomPlugin::_vehicleStateRawValueChanged);
            _vehicleStateRawValueChanged(currentState.fact->rawValue());
        }
    }
}

// This will advance the state machine if the value reaches the target value
void CustomPlugin::_vehicleStateRawValueChanged(QVariant rawValue)
{
    if (!_flightStateMachineActive) {
        Fact* fact = dynamic_cast<Fact*>(sender());
        if (!fact) {
            qWarning() << "Dynamic cast failed!";
            return;
        }
        disconnect(fact, &Fact::rawValueChanged, this, &CustomPlugin::_vehicleStateRawValueChanged);
    }

    const VehicleState_t& currentState = _vehicleStates[_vehicleStateIndex];

    //qCDebug(CustomPluginLog) << "Waiting for value actual:wait:variance" << rawValue.toDouble() << currentState.targetValue << currentState.targetVariance;

    if (qAbs(rawValue.toDouble() - currentState.targetValue) <= currentState.targetVariance) {
        // Target value reached
        _advanceStateMachine();
    }
}

void CustomPlugin::_say(QString text)
{
    qCDebug(CustomPluginLog) << "_say" << text;
    _toolbox->audioOutput()->say(text.toLower());
}

int CustomPlugin::_rawPulseToPct(double rawPulse)
{
    double maxPossiblePulse = static_cast<double>(_customSettings->maxPulse()->rawValue().toDouble());
    return static_cast<int>(100.0 * (rawPulse / maxPossiblePulse));
}

void CustomPlugin::_rotationDelayComplete(void)
{
    // Find the strongest pulse in the current slice
    double maxSNR = 0;
    for (const auto& pulseList: _rotationPulseInfoMap) {
        for (const auto& pulseInfo: pulseList) {
            if (pulseInfo.snr > maxSNR) {
                maxSNR = pulseInfo.snr;
            }
        }
    }
    qCDebug(CustomPluginLog) << "_rotationDelayComplete: max snr" << maxSNR;
    _rgAngleStrengths.last()[_nextSlice] = maxSNR;

    // Adjust the angle ratios to this new information
    maxSNR = 0;
    for (int i=0; i<_cSlice; i++) {
        if (_rgAngleStrengths.last()[i] > maxSNR) {
            maxSNR = _rgAngleStrengths.last()[i];
        }
    }
    for (int i=0; i<_cSlice; i++) {
        double angleStrength = _rgAngleStrengths.last()[i];
        if (!qIsNaN(angleStrength)) {
            _rgAngleRatios.last()[i] = _rgAngleStrengths.last()[i] / maxSNR;
        }
    }
    emit angleRatiosChanged();

    // Advance to next slice
    if (++_nextSlice >= _cSlice) {
        _nextSlice = 0;
    }

    _advanceStateMachine();
}

void CustomPlugin::_vehicleStateWaitFailed(void)
{
    if (_vehicleStates[_vehicleStateIndex].command == CommandWaitForHeartbeats) {
        _say("Failed to wait for pulses.");
    } else {
        _say("Failed to reach target.");
    }
    cancelAndReturn();
}

bool CustomPlugin::_armVehicleAndValidate(Vehicle* vehicle)
{
    if (vehicle->armed()) {
        return true;
    }

    bool armedChanged = false;

    // We try arming 3 times
    for (int retries=0; retries<3; retries++) {
        vehicle->setArmed(true, false /* showError */);

        // Wait for vehicle to return armed state
        for (int i=0; i<10; i++) {
            if (vehicle->armed()) {
                armedChanged = true;
                break;
            }
            QGC::SLEEP::msleep(100);
            qgcApp()->processEvents(QEventLoop::ExcludeUserInputEvents);
        }
        if (armedChanged) {
            break;
        }
    }

    if (!armedChanged) {
        _say("Vehicle failed to arm");
    }

    return armedChanged;
}

bool CustomPlugin::_setRTLFlightModeAndValidate(Vehicle* vehicle)
{
    QString rtlFlightMode = vehicle->rtlFlightMode();

    if (vehicle->flightMode() == rtlFlightMode) {
        return true;
    }

    bool flightModeChanged = false;

    // We try 3 times
    for (int retries=0; retries<3; retries++) {
        vehicle->setFlightMode(rtlFlightMode);

        // Wait for vehicle to return flight mode
        for (int i=0; i<10; i++) {
            if (vehicle->flightMode() == rtlFlightMode) {
                flightModeChanged = true;
                break;
            }
            QGC::SLEEP::msleep(100);
            qgcApp()->processEvents(QEventLoop::ExcludeUserInputEvents);
        }
        if (flightModeChanged) {
            break;
        }
    }

    if (!flightModeChanged) {
        _say("Vehicle failed to respond to Return command");
    }

    return flightModeChanged;
}

void CustomPlugin::_resetStateAndRTL(void)
{
    qCDebug(CustomPluginLog) << "_resetStateAndRTL";
    _vehicleStateTimeoutTimer.stop();

    Vehicle* vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
    if (vehicle) {
        disconnect(vehicle, &Vehicle::mavCommandResult, this, &CustomPlugin::_mavCommandResult);
    }

    for (const VehicleState_t& vehicleState: _vehicleStates) {
        if (vehicleState.fact) {
            disconnect(vehicleState.fact, &Fact::rawValueChanged, this, &CustomPlugin::_vehicleStateRawValueChanged);
        }
    }

    _setRTLFlightModeAndValidate(vehicle);

    _updateFlightMachineActive(false);

    _csvStopFullPulseLog();
    _csvStopRotationPulseLog();
}

bool CustomPlugin::adjustSettingMetaData(const QString& settingsGroup, FactMetaData& metaData)
{
    if (settingsGroup == AppSettings::settingsGroup && metaData.name() == AppSettings::batteryPercentRemainingAnnounceName) {
        metaData.setRawDefaultValue(20);
    } else if (settingsGroup == FlyViewSettings::settingsGroup && metaData.name() == FlyViewSettings::showSimpleCameraControlName) {
        metaData.setRawDefaultValue(false);
    }

#ifdef HERELINK_BUILD
    return HerelinkCorePlugin::adjustSettingMetaData(settingsGroup, metaData);
#else
    return true;
#endif
}

QString CustomPlugin::_tunnelCommandIdToText(uint32_t vhfCommandId)
{
    switch (vhfCommandId) {
    case COMMAND_ID_TAG:
        return QStringLiteral("tag");
    case COMMAND_ID_START_TAGS:
        return QStringLiteral("start tags");
    case COMMAND_ID_END_TAGS:
        return QStringLiteral("end tags");
    case COMMAND_ID_PULSE:
        return QStringLiteral("pulse");
    case COMMAND_ID_RAW_CAPTURE:
        return QStringLiteral("raw capture");
    case COMMAND_ID_START_DETECTION:
        return QStringLiteral("start detection");
    case COMMAND_ID_STOP_DETECTION:
        return QStringLiteral("stop detection");
    default:
        return QStringLiteral("unknown command:%1").arg(vhfCommandId);
    }
}

void CustomPlugin::_tunnelCommandAckFailed(void)
{
    QString message = QStringLiteral("%1 failed. no response from vehicle.").arg(_tunnelCommandIdToText(_tunnelCommandAckExpected));

    _say(QStringLiteral("%1 failed. no response from vehicle.").arg(_tunnelCommandIdToText(_tunnelCommandAckExpected)));
    qgcApp()->showAppMessage(message);

    _tunnelCommandAckExpected = 0;
}

void CustomPlugin::_sendTunnelCommand(uint8_t* payload, size_t payloadSize)
{
    Vehicle* vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
    if (!vehicle) {
        qCDebug(CustomPluginLog) << "_sendTunnelCommand called with no vehicle active";
        return;
    }

    WeakLinkInterfacePtr    weakPrimaryLink     = vehicle->vehicleLinkManager()->primaryLink();

    if (!weakPrimaryLink.expired()) {
        SharedLinkInterfacePtr  sharedLink  = weakPrimaryLink.lock();
        MAVLinkProtocol*        mavlink     = qgcApp()->toolbox()->mavlinkProtocol();
        mavlink_message_t       msg;
        mavlink_tunnel_t        tunnel;

        memset(&tunnel, 0, sizeof(tunnel));

        HeaderInfo_t tunnelHeader;
        memcpy(&tunnelHeader, payload, sizeof(tunnelHeader));

        _tunnelCommandAckExpected = tunnelHeader.command;
        _tunnelCommandAckTimer.start();

        memcpy(tunnel.payload, payload, payloadSize);

        tunnel.target_system    = vehicle->id();
        tunnel.target_component = MAV_COMP_ID_ONBOARD_COMPUTER;
        tunnel.payload_type     = MAV_TUNNEL_PAYLOAD_TYPE_UNKNOWN;
        tunnel.payload_length   = payloadSize;

        mavlink_msg_tunnel_encode_chan(
                    static_cast<uint8_t>(mavlink->getSystemId()),
                    static_cast<uint8_t>(mavlink->getComponentId()),
                    sharedLink->mavlinkChannel(),
                    &msg,
                    &tunnel);

        vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
    }


}

QmlObjectListModel* CustomPlugin::customMapItems(void)
{
    return &_customMapItems;
}

void CustomPlugin::_resetPulseLog(void)
{
    _pulseLog.clear();

    for (auto& extTagInfo: _tagInfoList) {
        auto listModel = new PulseInfoList(extTagInfo.tagInfo.id, extTagInfo.name, extTagInfo.tagInfo.frequency_hz, this);
        _pulseLog.append(QVariant::fromValue(listModel));
    }

    emit pulseLogChanged();
}

void CustomPlugin::_ftpDownloadComplete(const QString& file, const QString& errorMsg)
{
    qgcApp()->showAppMessage(QString("Download Complete %1").arg(errorMsg), file);
}

void CustomPlugin::_ftpCommandError(const QString& msg)
{
    qgcApp()->showAppMessage(msg);
}

void CustomPlugin::downloadLogs()
{
    Vehicle* vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
    if (!vehicle) {
        qCDebug(CustomPluginLog) << "downloadLogs called with no vehicle active";
        return;
    }

    auto ftpManager = vehicle->ftpManager();

    connect(ftpManager, &FTPManager::downloadComplete,  this, &CustomPlugin::_ftpDownloadComplete);
    connect(ftpManager, &FTPManager::commandError,      this, &CustomPlugin::_ftpCommandError);

    bool success = ftpManager->download(
                        MAV_COMP_ID_ONBOARD_COMPUTER,
                        "/home/vmware/MavlinkTagController.log",
                        _csvLogFilePath());
    if (!success) {
        qCDebug(CustomPluginLog) << "FTP download failed to start";
    }
}

void CustomPlugin::_controllerHeartbeatFailed()
{
    _controllerHeartbeat = false;
    emit controllerHeartbeatChanged();
}

void CustomPlugin::_clearDetectorHeartbeats(void)
{
    for (auto detectorHeartbeatInfo: _detectorHeartbeatInfoMap) {
        delete detectorHeartbeatInfo.pTimer;
    }
    _detectorHeartbeatInfoMap.clear();
    _detectorHeartbeats.clear();

    emit detectorHeartbeatsChanged();
}

void CustomPlugin::_setupDetectorHeartbeatInfo(DetectorHeartbeatInfo_t& detectorHeartbeatInfo, ExtendedTagInfo_t& extTagInfo, bool rate1)
{
    uint32_t k = _customSettings->k()->rawValue().toUInt();

    detectorHeartbeatInfo.heartbeatTimerInterval    = (k + 1) * (rate1 ?  extTagInfo.tagInfo.intra_pulse1_msecs : extTagInfo.tagInfo.intra_pulse2_msecs);
    detectorHeartbeatInfo.pTimer                    = new QTimer(this);
    detectorHeartbeatInfo.pTimer->setSingleShot(true);
    detectorHeartbeatInfo.pTimer->setInterval(detectorHeartbeatInfo.heartbeatTimerInterval);
    detectorHeartbeatInfo.pTimer->callOnTimeout([this, &detectorHeartbeatInfo]() {
        detectorHeartbeatInfo.heartbeat = false;
        _rebuildDetectorHeartbeats();
    });
}

void CustomPlugin::_setupDetectorHeartbeats(void)
{
    _clearDetectorHeartbeats();

    for (auto& extTagInfo: _tagInfoList) {
        _setupDetectorHeartbeatInfo(_detectorHeartbeatInfoMap[extTagInfo.tagInfo.id], extTagInfo, true /* rate1 */);

        if (extTagInfo.tagInfo.intra_pulse2_msecs != 0) {
            _setupDetectorHeartbeatInfo(_detectorHeartbeatInfoMap[extTagInfo.tagInfo.id + 1], extTagInfo, false /* rate1 */);
        }
    }

    _rebuildDetectorHeartbeats();
}

void  CustomPlugin::_updateDetectorHeartbeat(int tagId)
{
    // Rebuild heartbeat status list
    DetectorHeartbeatInfo_t& detectorHeartbeatInfo  = _detectorHeartbeatInfoMap[tagId];
    detectorHeartbeatInfo.heartbeat                 = true;
    detectorHeartbeatInfo.heartbeatCount++;
    detectorHeartbeatInfo.pTimer->start(detectorHeartbeatInfo.heartbeatTimerInterval);
    _rebuildDetectorHeartbeats();

    // If we are waiting on heartbeats for a rotation pause, check whether we've seen them all
    if (_flightStateMachineActive && _vehicleStates[_vehicleStateIndex].command == CommandWaitForHeartbeats) {
        for (auto& info: _detectorHeartbeatInfoMap) {
            if (info.heartbeatCount < 3) {
                return;
            }
        }

        // We have seen all the heartbeats we were waiting for, we can advance the state machine
        _rotationDelayComplete();
    }
}

void CustomPlugin::_rebuildDetectorHeartbeats(void)
{
    _detectorHeartbeats.clear();

    for (auto& extTagInfo: _tagInfoList) {
        DetectorHeartbeatInfo_t& detectorHeartbeatInfo = _detectorHeartbeatInfoMap[extTagInfo.tagInfo.id];
        _detectorHeartbeats.append(QVariant::fromValue(detectorHeartbeatInfo.heartbeat));

        if (extTagInfo.tagInfo.intra_pulse2_msecs != 0) {
            DetectorHeartbeatInfo_t& detectorHeartbeatInfo = _detectorHeartbeatInfoMap[extTagInfo.tagInfo.id + 1];
            _detectorHeartbeats.append(QVariant::fromValue(detectorHeartbeatInfo.heartbeat));
        }
    }

    emit detectorHeartbeatsChanged();
}
