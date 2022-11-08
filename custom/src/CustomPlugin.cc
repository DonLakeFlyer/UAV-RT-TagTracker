#include "CustomPlugin.h"
#include "Vehicle.h"
#include "CustomSettings.h"
#include "QGCApplication.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "FlyViewSettings.h"
#include "QmlComponentInfo.h"

#include <QDebug>
#include <QPointF>
#include <QLineF>

// DEBUG_FLOAT_ARRAY
//  time_usec - send index (used to detect lost telemetry)
//  array_id - command id

QGC_LOGGING_CATEGORY(CustomPluginLog, "CustomPluginLog")

CustomPlugin::CustomPlugin(QGCApplication *app, QGCToolbox* toolbox)
    : QGCCorePlugin         (app, toolbox)
    , _vehicleStateIndex    (0)
    , _flightMachineActive  (false)
    , _beepStrength         (0)
    , _temp                 (0)
    , _bpm                  (0)
    , _simulate             (false)
    , _vehicleFrequency     (0)
    , _lastPulseSendIndex   (-1)
    , _missedPulseCount     (0)
{
    _delayTimer.setSingleShot(true);
    _targetValueTimer.setSingleShot(true);
    _vhfCommandAckTimer.setSingleShot(true);
    _vhfCommandAckTimer.setInterval(1000);

    connect(&_delayTimer,           &QTimer::timeout, this, &CustomPlugin::_delayComplete);
    connect(&_targetValueTimer,     &QTimer::timeout, this, &CustomPlugin::_targetValueFailed);
    connect(&_simPulseTimer,        &QTimer::timeout, this, &CustomPlugin::_simulatePulse);
    connect(&_vhfCommandAckTimer,   &QTimer::timeout, this, &CustomPlugin::_vhfCommandAckFailed);
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
    if (message.msgid == MAVLINK_MSG_ID_DEBUG_FLOAT_ARRAY) {
        mavlink_debug_float_array_t debug_float_array;

        mavlink_msg_debug_float_array_decode(&message, &debug_float_array);

        CommandID commandId = static_cast<CommandID>(debug_float_array.array_id);

        switch (commandId) {
        case CommandID::CommandIDAck:
            _handleVHFCommandAck(debug_float_array);
            break;
        case CommandID::CommandIDPulse:
            _handleVHFPulse(debug_float_array);
            break;
#if 0
        case CommandID::CommandIDDetectionStatus:
            _handleDetectionStatus(debug_float_array);
            break;
#endif
        default:
            // Not all commands handled
            break;
        }

        return false;
    } else {
        return true;
    }
}

void CustomPlugin::_handleVHFCommandAck(const mavlink_debug_float_array_t& debug_float_array)
{
    CommandID vhfCommand    = static_cast<CommandID>(debug_float_array.data[static_cast<uint32_t>(AckIndex::AckIndexCommand)]);
    uint32_t result         = static_cast<uint32_t>(debug_float_array.data[static_cast<uint32_t>(AckIndex::AckIndexResult)]);

    if (vhfCommand == _vhfCommandAckExpected) {
        _vhfCommandAckExpected = static_cast<CommandID>(0);
        _vhfCommandAckTimer.stop();
        qCDebug(CustomPluginLog) << "VHF command ack received - command:result" << _vhfCommandIdToText(vhfCommand) << result;
        if (result != 1) {
            _say(QStringLiteral("%1 command failed").arg(_vhfCommandIdToText(vhfCommand)));
        }
    } else {
        qWarning() << "_handleVHFCommandAck: Received unexpected command id ack expected:actual" << _vhfCommandIdToText(_vhfCommandAckExpected) << _vhfCommandIdToText(vhfCommand);
    }
}

void CustomPlugin::_handleVHFPulse(const mavlink_debug_float_array_t& debug_float_array)
{
    double pulseStrength = static_cast<double>(debug_float_array.data[static_cast<uint32_t>(PulseIndex::PulseIndexStrengthLEGACY)]);

    _logPulseToFile(debug_float_array);

    if (pulseStrength < 0 || pulseStrength > 100) {
        qCDebug(CustomPluginLog) << "PULSE outside range";
    }
    pulseStrength = qMax(0.0, qMin(100.0, pulseStrength));

    _beepStrength = pulseStrength;
    emit beepStrengthChanged(_beepStrength);

    _pulseTimeSeconds   = *((double *)&debug_float_array.data[0]);
    _pulseSNR           = debug_float_array.data[static_cast<uint32_t>(PulseIndex::PulseIndexSNR)];
    _pulseConfirmed     = debug_float_array.data[static_cast<uint32_t>(PulseIndex::PulseIndexConfirmedStatus)] > 0;

    if (qFuzzyCompare(_lastPulseTime, _pulseTimeSeconds)) {
        // Remove duplicates cause by double forwarding or detection boundary case
        return;
    } else {
        qCDebug(CustomPluginLog) << Qt::fixed << qSetRealNumberPrecision(2) << "PULSE time:snr" << _pulseTimeSeconds << _pulseSNR << _pulseConfirmed;
        emit pulseReceived();
    }
    _lastPulseTime = _pulseTimeSeconds;

    _rgPulseValues.append(_beepStrength);
    if (_beepStrength == 0) {
        _elapsedTimer.invalidate();
    } else {
        if (_elapsedTimer.isValid()) {
            qint64 elapsed = _elapsedTimer.elapsed();
            if (elapsed > 0) {
                _bpm = (60 * 1000) / _elapsedTimer.elapsed();
                emit bpmChanged(_bpm);
            }
        }
        _elapsedTimer.restart();
    }
}

void CustomPlugin::_logPulseToFile(const mavlink_debug_float_array_t& debug_float_array)
{
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

#if 0
void CustomPlugin::_handleDetectionStatus(const mavlink_debug_float_array_t& debug_float_array)
{
    _detectionStatus = static_cast<int>(debug_float_array.data[DETECTION_STATUS_IDX_STATUS]);

    emit detectionStatusChanged(_detectionStatus);
}
#endif

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
    Vehicle* vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
    if (!vehicle) {
        qCDebug(CustomPluginLog) << "startDetection called with no vehicle active";
        return;
    }

    mavlink_message_t           msg;
    mavlink_debug_float_array_t debug_float_array;
    MAVLinkProtocol*            mavlink             = qgcApp()->toolbox()->mavlinkProtocol();
    WeakLinkInterfacePtr        weakPrimaryLink     = vehicle->vehicleLinkManager()->primaryLink();

    if (!weakPrimaryLink.expired()) {
        SharedLinkInterfacePtr sharedLink = weakPrimaryLink.lock();

        memset(&debug_float_array, 0, sizeof(debug_float_array));

        debug_float_array.array_id = static_cast<uint16_t>(CommandID::CommandIDStart);

        mavlink_msg_debug_float_array_encode_chan(
                    static_cast<uint8_t>(mavlink->getSystemId()),
                    static_cast<uint8_t>(mavlink->getComponentId()),
                    sharedLink->mavlinkChannel(),
                    &msg,
                    &debug_float_array);
        _sendVHFCommand(vehicle, sharedLink.get(), CommandID::CommandIDStart, msg);
    }
}

void CustomPlugin::stopDetection(void)
{
    Vehicle* vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
    if (!vehicle) {
        qCDebug(CustomPluginLog) << "stopDetection called with no vehicle active";
        return;
    }

    mavlink_message_t           msg;
    mavlink_debug_float_array_t debug_float_array;
    MAVLinkProtocol*            mavlink             = qgcApp()->toolbox()->mavlinkProtocol();
    WeakLinkInterfacePtr        weakPrimaryLink     = vehicle->vehicleLinkManager()->primaryLink();

    if (!weakPrimaryLink.expired()) {
        SharedLinkInterfacePtr sharedLink = weakPrimaryLink.lock();

        memset(&debug_float_array, 0, sizeof(debug_float_array));

        debug_float_array.array_id = static_cast<uint16_t>(CommandID::CommandIDStop);

        mavlink_msg_debug_float_array_encode_chan(
                    static_cast<uint8_t>(mavlink->getSystemId()),
                    static_cast<uint8_t>(mavlink->getComponentId()),
                    sharedLink->mavlinkChannel(),
                    &msg,
                    &debug_float_array);
        _sendVHFCommand(vehicle, sharedLink.get(), CommandID::CommandIDStop, msg);
    }
}

void CustomPlugin::airspyHFCapture(void)
{
    Vehicle* vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
    if (!vehicle) {
        qCDebug(CustomPluginLog) << "stopDetection called with no vehicle active";
        return;
    }

    mavlink_message_t           msg;
    mavlink_debug_float_array_t debug_float_array;
    MAVLinkProtocol*            mavlink             = qgcApp()->toolbox()->mavlinkProtocol();
    WeakLinkInterfacePtr        weakPrimaryLink     = vehicle->vehicleLinkManager()->primaryLink();

    if (!weakPrimaryLink.expired()) {
        SharedLinkInterfacePtr sharedLink = weakPrimaryLink.lock();

        memset(&debug_float_array, 0, sizeof(debug_float_array));

        debug_float_array.array_id = 6;

        mavlink_msg_debug_float_array_encode_chan(
                    static_cast<uint8_t>(mavlink->getSystemId()),
                    static_cast<uint8_t>(mavlink->getComponentId()),
                    sharedLink->mavlinkChannel(),
                    &msg,
                    &debug_float_array);
        _sendVHFCommand(vehicle, sharedLink.get(), CommandID::CommandIDStop, msg);
    }
}

void CustomPlugin::airspyMiniCapture(void)
{
    Vehicle* vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
    if (!vehicle) {
        qCDebug(CustomPluginLog) << "stopDetection called with no vehicle active";
        return;
    }

    mavlink_message_t           msg;
    mavlink_debug_float_array_t debug_float_array;
    MAVLinkProtocol*            mavlink             = qgcApp()->toolbox()->mavlinkProtocol();
    WeakLinkInterfacePtr        weakPrimaryLink     = vehicle->vehicleLinkManager()->primaryLink();

    if (!weakPrimaryLink.expired()) {
        SharedLinkInterfacePtr sharedLink = weakPrimaryLink.lock();

        memset(&debug_float_array, 0, sizeof(debug_float_array));

        debug_float_array.array_id = 7;

        mavlink_msg_debug_float_array_encode_chan(
                    static_cast<uint8_t>(mavlink->getSystemId()),
                    static_cast<uint8_t>(mavlink->getComponentId()),
                    sharedLink->mavlinkChannel(),
                    &msg,
                    &debug_float_array);
        _sendVHFCommand(vehicle, sharedLink.get(), CommandID::CommandIDStop, msg);
    }
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

void CustomPlugin::_simulatePulse(void)
{
    Vehicle* vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();

    if (vehicle) {
        double heading = vehicle->heading()->rawValue().toDouble();

        // Strongest pulse is at 90 degrees
        heading -= 90;
        if (heading < 0) {
            heading += 360;
        }

        double pulseRatio;
        if (heading <= 180) {
            pulseRatio = (180.0 - heading) / 180.0;
        } else {
            heading -= 180;
            pulseRatio = heading / 180.0;
        }
        double pulse = 100.0 * pulseRatio;

        double currentAltRel = vehicle->altitudeRelative()->rawValue().toDouble();
        double maxAlt = _customSettings->altitude()->rawValue().toDouble();
        double altRatio = currentAltRel / maxAlt;
        pulse *= altRatio;

        //qDebug() << heading << pulseRatio << pulse;

        WeakLinkInterfacePtr weakPrimaryLink = vehicle->vehicleLinkManager()->primaryLink();
        if (!weakPrimaryLink.expired()) {
            SharedLinkInterfacePtr sharedLink = weakPrimaryLink.lock();

            mavlink_message_t           msg;
            mavlink_debug_float_array_t debug_float_array;

            debug_float_array.array_id = static_cast<uint16_t>(CommandID::CommandIDPulse);
            debug_float_array.data[static_cast<uint32_t>(PulseIndex::PulseIndexStrengthLEGACY)] = pulse;
            mavlink_msg_debug_float_array_encode(static_cast<uint8_t>(vehicle->id()), MAV_COMP_ID_AUTOPILOT1, &msg, &debug_float_array);

            mavlinkMessage(vehicle, sharedLink.get(), msg);
        }
    }
}

void CustomPlugin::_handleSimulatedTagCommand(const mavlink_debug_float_array_t& debug_float_array)
{
    _simulatorTagId                 = static_cast<uint32_t>(debug_float_array.data[static_cast<uint32_t>(TagIndex::TagIndexID)]);
    _simulatorFrequency             = static_cast<uint32_t>(debug_float_array.data[static_cast<uint32_t>(TagIndex::TagIndexFrequency)]);
    _simulatorPulseDuration         = static_cast<uint32_t>(debug_float_array.data[static_cast<uint32_t>(TagIndex::TagIndexDurationMSecs)]);
    _simulatorIntraPulse1           = static_cast<uint32_t>(debug_float_array.data[static_cast<uint32_t>(TagIndex::TagIndexIntraPulse1MSecs)]);
    _simulatorIntraPulse2           = static_cast<uint32_t>(debug_float_array.data[static_cast<uint32_t>(TagIndex::TagIndexIntraPulse2MSecs)]);
    _simulatorIntraPulseUncertainty = static_cast<uint32_t>(debug_float_array.data[static_cast<uint32_t>(TagIndex::TagIndexIntraPulseUncertainty)]);
    _simulatorIntraPulseJitter      = static_cast<uint32_t>(debug_float_array.data[static_cast<uint32_t>(TagIndex::TagIndexIntraPulseJitter)]);
    _simulatorMaxPulse              = debug_float_array.data[static_cast<uint32_t>(TagIndex::TagIndexMaxPulse)];
}

void CustomPlugin::_handleSimulatedStartDetection(const mavlink_debug_float_array_t&)
{
    _simPulseTimer.start(2000);
}

void CustomPlugin::_handleSimulatedStopDetection(const mavlink_debug_float_array_t&)
{
    _simPulseTimer.stop();
}

QString CustomPlugin::_vhfCommandIdToText(CommandID vhfCommandId)
{
    switch (vhfCommandId) {
    case CommandID::CommandIDTag:
        return QStringLiteral("tag");
    case CommandID::CommandIDStart:
        return QStringLiteral("start detection");
    case CommandID::CommandIDStop:
        return QStringLiteral("stop detection");
    default:
        return QStringLiteral("unknown command:%1").arg(static_cast<uint32_t>(vhfCommandId));
    }
}


void CustomPlugin::_vhfCommandAckFailed(void)
{
    _say(QStringLiteral("%1 failed. no response from vehicle.").arg(_vhfCommandIdToText(_vhfCommandAckExpected)));
    _vhfCommandAckExpected = static_cast<CommandID>(0);
}

void CustomPlugin::_sendSimulatedVHFCommandAck(CommandID vhfCommandId)
{
    Vehicle*                    vehicle             = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
    mavlink_message_t           msg;
    mavlink_debug_float_array_t debug_float_array;
    MAVLinkProtocol*            mavlink             = qgcApp()->toolbox()->mavlinkProtocol();
    WeakLinkInterfacePtr        weakPrimaryLink     = vehicle->vehicleLinkManager()->primaryLink();

    if (!weakPrimaryLink.expired()) {
        SharedLinkInterfacePtr sharedLink = weakPrimaryLink.lock();

        memset(&debug_float_array, 0, sizeof(debug_float_array));

        debug_float_array.array_id                                                  = static_cast<uint16_t>(CommandID::CommandIDAck);
        debug_float_array.data[static_cast<uint32_t>(AckIndex::AckIndexCommand)]    = static_cast<float>(vhfCommandId);
        debug_float_array.data[static_cast<uint32_t>(AckIndex::AckIndexResult)]     = 1;

        mavlink_msg_debug_float_array_encode_chan(
                    static_cast<uint8_t>(mavlink->getSystemId()),
                    static_cast<uint8_t>(mavlink->getComponentId()),
                    sharedLink->mavlinkChannel(),
                    &msg,
                    &debug_float_array);

        mavlinkMessage(vehicle, sharedLink.get(), msg);
    }
}

void CustomPlugin::_sendVHFCommand(Vehicle* vehicle, LinkInterface* link, CommandID vhfCommandId, const mavlink_message_t& msg)
{
    _vhfCommandAckExpected = vhfCommandId;
    _vhfCommandAckTimer.start();

    if (_simulate) {
        mavlink_debug_float_array_t debug_float_array;

        mavlink_msg_debug_float_array_decode(&msg, &debug_float_array);

        switch (static_cast<CommandID>(debug_float_array.array_id)) {
        case CommandID::CommandIDTag:
            _handleSimulatedTagCommand(debug_float_array);
            _sendSimulatedVHFCommandAck(CommandID::CommandIDTag);
            break;
        case CommandID::CommandIDStart:
            _handleSimulatedStartDetection(debug_float_array);
            _sendSimulatedVHFCommandAck(CommandID::CommandIDStart);
            break;
        case CommandID::CommandIDStop:
            _handleSimulatedStopDetection(debug_float_array);
            _sendSimulatedVHFCommandAck(CommandID::CommandIDStop);
            break;
        default:
            qWarning() << "Internal error: Unknown command id" << debug_float_array.array_id;
            break;
        }
    } else {
        vehicle->sendMessageOnLinkThreadSafe(link, msg);
    }
}

void CustomPlugin::sendTag(void)
{
    qCDebug(CustomPluginLog) << "sendTag";

    Vehicle*                    vehicle             = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
    mavlink_message_t           msg;
    mavlink_debug_float_array_t debug_float_array;
    MAVLinkProtocol*            mavlink             = qgcApp()->toolbox()->mavlinkProtocol();
    WeakLinkInterfacePtr        weakPrimaryLink     = vehicle->vehicleLinkManager()->primaryLink();

    if (!weakPrimaryLink.expired()) {
        SharedLinkInterfacePtr sharedLink = weakPrimaryLink.lock();

        memset(&debug_float_array, 0, sizeof(debug_float_array));

        debug_float_array.array_id                                                              = static_cast<uint16_t>(CommandID::CommandIDTag);
        debug_float_array.data[static_cast<uint32_t>(TagIndex::TagIndexID)]                     = _customSettings->tagId()->rawValue().toFloat();
        debug_float_array.data[static_cast<uint32_t>(TagIndex::TagIndexFrequency)]              = _customSettings->frequency()->rawValue().toFloat() -  _customSettings->frequencyDelta()->rawValue().toFloat();
        debug_float_array.data[static_cast<uint32_t>(TagIndex::TagIndexDurationMSecs)]          = _customSettings->pulseDuration()->rawValue().toFloat();
        debug_float_array.data[static_cast<uint32_t>(TagIndex::TagIndexIntraPulse1MSecs)]       = _customSettings->intraPulse1()->rawValue().toFloat();
        debug_float_array.data[static_cast<uint32_t>(TagIndex::TagIndexIntraPulse2MSecs)]       = _customSettings->intraPulse2()->rawValue().toFloat();
        debug_float_array.data[static_cast<uint32_t>(TagIndex::TagIndexIntraPulseUncertainty)]  = _customSettings->intraPulseUncertainty()->rawValue().toFloat();
        debug_float_array.data[static_cast<uint32_t>(TagIndex::TagIndexIntraPulseJitter)]       = _customSettings->intraPulseJitter()->rawValue().toFloat();
        debug_float_array.data[static_cast<uint32_t>(TagIndex::TagIndexMaxPulse)]               = _customSettings->maxPulse()->rawValue().toFloat();

        mavlink_msg_debug_float_array_encode_chan(
                    static_cast<uint8_t>(mavlink->getSystemId()),
                    static_cast<uint8_t>(mavlink->getComponentId()),
                    sharedLink->mavlinkChannel(),
                    &msg,
                    &debug_float_array);
        _sendVHFCommand(vehicle, sharedLink.get(), CommandID::CommandIDTag, msg);
    }
}

QmlObjectListModel* CustomPlugin::customMapItems(void)
{
    return &_customMapItems;
}
