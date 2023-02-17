#include "CustomPlugin.h"
#include "Vehicle.h"
#include "CustomSettings.h"
#include "QGCApplication.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "FlyViewSettings.h"
#include "QmlComponentInfo.h"
#include "TunnelProtocol.h"

#include <QDebug>
#include <QPointF>
#include <QLineF>

using namespace TunnelProtocol;

QGC_LOGGING_CATEGORY(CustomPluginLog, "CustomPluginLog")

CustomPlugin::CustomPlugin(QGCApplication *app, QGCToolbox* toolbox)
    : QGCCorePlugin         (app, toolbox)
    , _vehicleStateIndex    (0)
    , _flightMachineActive  (false)
    , _vehicleFrequency     (0)
    , _lastPulseSendIndex   (-1)
    , _missedPulseCount     (0)
    , _tagInfoLoader        (this)
{
    static_assert(TunnelProtocolValidateSizes, "TunnelProtocolValidateSizes failed");

    _delayTimer.setSingleShot(true);
    _targetValueTimer.setSingleShot(true);
    _tunnelCommandAckTimer.setSingleShot(true);
    _tunnelCommandAckTimer.setInterval(1000);

    connect(&_delayTimer,               &QTimer::timeout, this, &CustomPlugin::_delayComplete);
    connect(&_targetValueTimer,         &QTimer::timeout, this, &CustomPlugin::_targetValueFailed);
    connect(&_tunnelCommandAckTimer,    &QTimer::timeout, this, &CustomPlugin::_tunnelCommandAckFailed);
}

CustomPlugin::~CustomPlugin()
{
    if (_pulseLogFile.isOpen()) {
        _pulseLogFile.close();
    }
}

void CustomPlugin::setToolbox(QGCToolbox* toolbox)
{
    QGCCorePlugin::setToolbox(toolbox);
    _customSettings = new CustomSettings(nullptr);
    _customOptions = new CustomOptions(this, nullptr);

    connect(toolbox->multiVehicleManager(), &MultiVehicleManager::activeVehicleChanged, this, &CustomPlugin::_activeVehicleChanged);

    _tagInfoLoader.loadTags();
}

void CustomPlugin::_activeVehicleChanged(Vehicle* activeVehicle)
{
    // PX4 firmware streams DEBUG_FLOAT_ARRAY messages for some reason. This seems to be the only reliable way to get the message to
    // flow through the Skynode UDP connection out a SiK radio. We need to up the rate so we don't lose messages.
    // When connected over non-SiK radio this causes duplicates to be sent. So we need to remove those later.
    disconnect(_toolbox->multiVehicleManager(), &MultiVehicleManager::activeVehicleChanged, this, &CustomPlugin::_activeVehicleChanged);
    activeVehicle->sendCommand(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_SET_MESSAGE_INTERVAL, true /* showError */, MAVLINK_MSG_ID_DEBUG_FLOAT_ARRAY, 100000 /* disable */, 0 /* flight stack */);
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

bool CustomPlugin::mavlinkMessage(Vehicle*, LinkInterface*, mavlink_message_t message)
{
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
        }

        return false;
    } else {
        return true;
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
            }
        } else {
            _say(QStringLiteral("%1 command failed").arg(_tunnelCommandIdToText(ack.command)));
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

    memcpy(&_lastPulseInfo, tunnel.payload, sizeof(_lastPulseInfo));

    qCDebug(CustomPluginLog) << Qt::fixed << qSetRealNumberPrecision(2) <<
                                "PULSE tag_id:frequency_hz:time:snr:confirmed" <<
                                _lastPulseInfo.tag_id <<
                                _lastPulseInfo.frequency_hz <<
                                _lastPulseInfo.start_time_seconds <<
                                _lastPulseInfo.snr <<
                                _lastPulseInfo.confirmed_status;
    emit pulseReceived();

    _rgPulseValues.append(_lastPulseInfo.snr);
}

void CustomPlugin::_logPulseToFile(const mavlink_tunnel_t& tunnel)
{
#if 0
    if (!_pulseLogFile.isOpen()) {
        _pulseLogFile.setFileName(QString("%1/Pulse-%2.csv").arg(qgcApp()->toolbox()->settingsManager()->appSettings()->logSavePath(),
                                                                 QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss-zzz").toLocal8Bit().data()));
        qCDebug(CustomPluginLog) << "Pulse logging to:" << _pulseLogFile;
        if (!_pulseLogFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qCWarning(CustomPluginLog) << "Unable to open pulse log file";
            return;
        }
    }

    _pulseLogFile.write(QString("1,%1,").arg(debug_float_array.time_usec).toUtf8());
    for (int i=0; i <= static_cast<int>(PulseIndex::PulseIndexLast); i++) {
        _pulseLogFile.write(QString("%1,").arg(static_cast<double>(debug_float_array.data[i]), 0, 'f', 6).toUtf8());
    }
    _pulseLogFile.write("\n");
#endif
}

void CustomPlugin::_logRotateStartStopToFile(bool start)
{
    if (!_pulseLogFile.isOpen()) {
        qCWarning(CustomPluginLog) << "_logRotateStartStopToFile called before detection started";
        return;
    }

    Vehicle* vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
    if (!vehicle) {
        return;
    }

    QGeoCoordinate coord = vehicle->coordinate();
    _pulseLogFile.write(QString("%1,%2,%3,%4\n").arg(start ? 2 : 3)
                        .arg(coord.latitude(), 0, 'f', 6)
                        .arg(coord.longitude(), 0, 'f', 6)
                        .arg(vehicle->altitudeAMSL()->rawValue().toDouble(), 0, 'f', 6)
                        .toUtf8());
}

void CustomPlugin::_updateFlightMachineActive(bool flightMachineActive)
{
    _flightMachineActive = flightMachineActive;
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

    _logRotateStartStopToFile(true /* start */);

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
    _vehicleStateIndex = 0;

    // Rotate
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

        vehicleState.command =              CommandDelay;
        vehicleState.fact =                 nullptr;
        vehicleState.targetValueWaitSecs =  10;
        _vehicleStates.append(vehicleState);
    }

    _nextVehicleState();
}

void CustomPlugin::startDetection(void)
{
    StartDetectionInfo_t startDetectionInfo;

    startDetectionInfo.header.command = COMMAND_ID_START_DETECTION;
    _sendTunnelCommand((uint8_t*)&startDetectionInfo, sizeof(startDetectionInfo));
}

void CustomPlugin::stopDetection(void)
{
    StopDetectionInfo_t stopDetectionInfo;

    stopDetectionInfo.header.command = COMMAND_ID_STOP_DETECTION;
    _sendTunnelCommand((uint8_t*)&stopDetectionInfo, sizeof(stopDetectionInfo));
}

void CustomPlugin::airspyHFCapture(void)
{
    HeaderInfo_t airspyHFCaptureInfo;

    airspyHFCaptureInfo.command = COMMAND_ID_AIRSPY_HF;
    _sendTunnelCommand((uint8_t*)&airspyHFCaptureInfo, sizeof(airspyHFCaptureInfo));
}

void CustomPlugin::airspyMiniCapture(void)
{
    HeaderInfo_t airspyMiniCaptureInfo;

    airspyMiniCaptureInfo.command = COMMAND_ID_AIRSPY_MINI;
    _sendTunnelCommand((uint8_t*)&airspyMiniCaptureInfo, sizeof(airspyMiniCaptureInfo));
}

void CustomPlugin::sendTags(void)
{
    if (_tagInfoLoader.tagList.count() == 0) {
        qgcApp()->showAppMessage(("No tags are available to send."));
        return;
    }

    _nextTagToSend = 0;

    StartTagsInfo_t startTagsInfo;

    startTagsInfo.header.command = COMMAND_ID_START_TAGS;
    _sendTunnelCommand((uint8_t*)&startTagsInfo, sizeof(startTagsInfo));
}

void CustomPlugin::_sendNextTag(void)
{
    if (_nextTagToSend >= _tagInfoLoader.tagList.count()) {
        _sendEndTags();
    } else {
        TagInfo_t tagInfo = _tagInfoLoader.tagList[_nextTagToSend++];
        _sendTunnelCommand((uint8_t*)&tagInfo, sizeof(tagInfo));
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

    if (!_flightMachineActive) {
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
    } else if (currentState.command == CommandSetHeading && command == MAV_CMD_DO_REPOSITION) {
        disconnect(vehicle, &Vehicle::mavCommandResult, this, &CustomPlugin::_mavCommandResult);
        if (noResponseFromVehicle) {
            _say(QStringLiteral("Vehicle did not response to Rotate command. Flight cancelled. Vehicle returning."));
            _resetStateAndRTL();
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
    _sendCommandAndVerify(
                vehicle,
                MAV_CMD_DO_REPOSITION,
                -1,                                     // no change in ground speed
                MAV_DO_REPOSITION_FLAGS_CHANGE_MODE,    // switch to guided mode
                0,                                      //reserved
                qDegreesToRadians(headingDegrees),      // change heading
                qQNaN(), qQNaN(), qQNaN());             // no change lat, lon, alt
}

void CustomPlugin::_nextVehicleState(void)
{
    Vehicle* vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();

    if (!vehicle) {
        return;
    }

    if (_vehicleStateIndex != 0 && vehicle->flightMode() != "Takeoff" && vehicle->flightMode() != "Hold") {
        // User cancel
        _updateFlightMachineActive(false);
        _logRotateStartStopToFile(false /* stop */);
        return;
    }

    if (_vehicleStateIndex >= _vehicleStates.count()) {
        _say(QStringLiteral("Collection complete."));
        _updateFlightMachineActive(false);
        _logRotateStartStopToFile(false /* stop */);
        return;
    }

    const VehicleState_t& currentState = _vehicleStates[_vehicleStateIndex];

    switch (currentState.command) {
    case CommandTakeoff:
        // Takeoff to specified altitude
        _say(QStringLiteral("Waiting for takeoff to %1 %2.").arg(FactMetaData::metersToAppSettingsVerticalDistanceUnits(currentState.targetValue).toDouble()).arg(FactMetaData::appSettingsVerticalDistanceUnitsString()));
        _takeoff(vehicle, currentState.targetValue);
        break;
    case CommandSetHeading:
        _say(QStringLiteral("Waiting for rotate to %1 degrees.").arg(qRound(currentState.targetValue)));
        _rotateVehicle(vehicle, currentState.targetValue);
        break;
    case CommandDelay:
        _say(QStringLiteral("Collecting data for %1 seconds.").arg(currentState.targetValueWaitSecs));
        _vehicleStateIndex++;
        _rgPulseValues.clear();
        _delayTimer.setInterval(currentState.targetValueWaitSecs * 1000);
        _delayTimer.start();
        break;
    }

    if (currentState.fact) {
        _targetValueTimer.setInterval(currentState.targetValueWaitSecs * 1000);
        _targetValueTimer.start();
        connect(currentState.fact, &Fact::rawValueChanged, this, &CustomPlugin::_vehicleStateRawValueChanged);
        _vehicleStateRawValueChanged(currentState.fact->rawValue());
    }
}

void CustomPlugin::_vehicleStateRawValueChanged(QVariant rawValue)
{
    if (!_flightMachineActive) {
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
        _targetValueTimer.stop();
        disconnect(currentState.fact, &Fact::rawValueChanged, this, &CustomPlugin::_vehicleStateRawValueChanged);
        _vehicleStateIndex++;
        _nextVehicleState();
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

void CustomPlugin::_delayComplete(void)
{
    double maxPulse = 0;
    foreach(double pulse, _rgPulseValues) {
        maxPulse = qMax(maxPulse, pulse);
    }
    qCDebug(CustomPluginLog) << "_delayComplete" << maxPulse << _rgPulseValues;
    _rgAngleStrengths.last()[_nextSlice] = maxPulse;

    if (++_nextSlice >= _cSlice) {
        _nextSlice = 0;
    }

    maxPulse = 0;
    for (int i=0; i<_cSlice; i++) {
        if (_rgAngleStrengths.last()[i] > maxPulse) {
            maxPulse = _rgAngleStrengths.last()[i];
        }
    }

    for (int i=0; i<_cSlice; i++) {
        double angleStrength = _rgAngleStrengths.last()[i];
        if (!qIsNaN(angleStrength)) {
            _rgAngleRatios.last()[i] = _rgAngleStrengths.last()[i] / maxPulse;
        }
    }
    emit angleRatiosChanged();

    _nextVehicleState();
}

void CustomPlugin::_targetValueFailed(void)
{
    _say("Failed to reach target.");
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
    _delayTimer.stop();
    _targetValueTimer.stop();

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
}

bool CustomPlugin::adjustSettingMetaData(const QString& settingsGroup, FactMetaData& metaData)
{
    if (settingsGroup == AppSettings::settingsGroup && metaData.name() == AppSettings::batteryPercentRemainingAnnounceName) {
        metaData.setRawDefaultValue(20);
    } else if (settingsGroup == FlyViewSettings::settingsGroup && metaData.name() == FlyViewSettings::showSimpleCameraControlName) {
        metaData.setRawDefaultValue(false);
    }

    return true;
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
    case COMMAND_ID_AIRSPY_HF:
        return QStringLiteral("airspy hf");
    case COMMAND_ID_AIRSPY_MINI:
        return QStringLiteral("airspy mini");
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
    _say(QStringLiteral("%1 failed. no response from vehicle.").arg(_tunnelCommandIdToText(_tunnelCommandAckExpected)));
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

