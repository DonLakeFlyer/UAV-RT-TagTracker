#pragma once

#include "QGCCorePlugin.h"
#include "QmlObjectListModel.h"
#include "CustomOptions.h"
#include "FactSystem.h"

#include "uavrt_interfaces/qgc_enum_class_definitions.hpp"
using namespace uavrt_interfaces;

#include <QElapsedTimer>
#include <QGeoCoordinate>
#include <QTimer>
#include <QLoggingCategory>
#include <QFile>

class CustomSettings;

Q_DECLARE_LOGGING_CATEGORY(CustomPluginLog)

class CustomPlugin : public QGCCorePlugin
{
    Q_OBJECT

public:
    CustomPlugin(QGCApplication* app, QGCToolbox* toolbox);
    ~CustomPlugin();

    Q_PROPERTY(CustomSettings*  customSettings      MEMBER _customSettings          CONSTANT)
    Q_PROPERTY(qreal            beepStrength        MEMBER _beepStrength            NOTIFY beepStrengthChanged)
    Q_PROPERTY(double           pulseTimeSeconds    MEMBER _pulseTimeSeconds        NOTIFY pulseReceived)
    Q_PROPERTY(double           pulseSNR            MEMBER _pulseSNR                NOTIFY pulseReceived)
    Q_PROPERTY(bool             pulseConfirmed      MEMBER _pulseConfirmed          NOTIFY pulseReceived)
    Q_PROPERTY(qreal            temp                MEMBER _temp                    NOTIFY tempChanged)
    Q_PROPERTY(int              bpm                 MEMBER _bpm                     NOTIFY bpmChanged)
    Q_PROPERTY(QList<QList<double>> angleRatios         MEMBER _rgAngleRatios           NOTIFY angleRatiosChanged)
    Q_PROPERTY(bool             flightMachineActive MEMBER _flightMachineActive     NOTIFY flightMachineActiveChanged)
    Q_PROPERTY(int              vehicleFrequency    MEMBER _vehicleFrequency        NOTIFY vehicleFrequencyChanged)
    Q_PROPERTY(int              missedPulseCount    MEMBER _missedPulseCount        NOTIFY missedPulseCountChanged)
    Q_PROPERTY(int              detectionStatus     MEMBER _detectionStatus         NOTIFY detectionStatusChanged)

    Q_INVOKABLE void startRotation  (void);
    Q_INVOKABLE void cancelAndReturn(void);
    Q_INVOKABLE void sendTag        (void);
    Q_INVOKABLE void startDetection (void);
    Q_INVOKABLE void stopDetection  (void);

    // Overrides from QGCCorePlugin
    QVariantList&       settingsPages           (void) final;
    bool                mavlinkMessage          (Vehicle* vehicle, LinkInterface* link, mavlink_message_t message) final;
    QGCOptions*         options                 (void) final { return qobject_cast<QGCOptions*>(_customOptions); }
    bool                adjustSettingMetaData   (const QString& settingsGroup, FactMetaData& metaData) final;
    QmlObjectListModel* customMapItems          (void) final;

    // Overrides from QGCTool
    void setToolbox(QGCToolbox* toolbox) final;

signals:
    void beepStrengthChanged        (qreal beepStrength);
    void tempChanged                (qreal temp);
    void bpmChanged                 (int bpm);
    void angleRatiosChanged         (void);
    void flightMachineActiveChanged (bool flightMachineActive);
    void vehicleFrequencyChanged    (int vehicleFrequency);
    void missedPulseCountChanged    (int missedPulseCount);
    void detectionStatusChanged     (int detectionStatus);
    void pulseReceived              (void);

private slots:
    void _vehicleStateRawValueChanged   (QVariant rawValue);
    void _nextVehicleState              (void);
    void _delayComplete                 (void);
    void _targetValueFailed             (void);
    void _updateFlightMachineActive     (bool flightMachineActive);
    void _mavCommandResult              (int vehicleId, int component, int command, int result, bool noResponseFromVehicle);
    void _simulatePulse                 (void);
    void _vhfCommandAckFailed           (void);
    void _activeVehicleChanged          (Vehicle* activeVehicle);

private:
    typedef enum {
        CommandTakeoff,
        CommandSetHeading,
        CommandDelay
    } VehicleStateCommand_t;

    typedef struct {
        VehicleStateCommand_t   command;
        Fact*                   fact;
        int                     targetValueWaitSecs;
        double                  targetValue;
        double                  targetVariance;
    } VehicleState_t;

    void _handleVHFCommandAck           (const mavlink_debug_float_array_t& debug_float_array);
    void _handleVHFPulse                (const mavlink_debug_float_array_t& debug_float_array);
    //void _handleDetectionStatus         (const mavlink_debug_float_array_t& debug_float_array);
    void _rotateVehicle                 (Vehicle* vehicle, double headingDegrees);
    void _say                           (QString text);
    bool _armVehicleAndValidate         (Vehicle* vehicle);
    bool _setRTLFlightModeAndValidate   (Vehicle* vehicle);
    void _sendCommandAndVerify          (Vehicle* vehicle, MAV_CMD command, double param1 = 0.0, double param2 = 0.0, double param3 = 0.0, double param4 = 0.0, double param5 = 0.0, double param6 = 0.0, double param7 = 0.0);
    void _takeoff                       (Vehicle* vehicle, double takeoffAltRel);
    void _resetStateAndRTL              (void);
    int  _rawPulseToPct                 (double rawPulse);
    void _sendVHFCommand                (Vehicle* vehicle, LinkInterface* link, CommandID vhfCommandId, const mavlink_message_t& msg);
    void _handleSimulatedTagCommand     (const mavlink_debug_float_array_t& debug_float_array);
    void _handleSimulatedStartDetection (const mavlink_debug_float_array_t& debug_float_array);
    void _handleSimulatedStopDetection  (const mavlink_debug_float_array_t& debug_float_array);
    QString _vhfCommandIdToText         (CommandID vhfCommandId);
    void _sendSimulatedVHFCommandAck    (CommandID vhfCommandId);
    void _logPulseToFile                (const mavlink_debug_float_array_t& debug_float_array);
    void _logRotateStartStopToFile      (bool start);

    QVariantList            _settingsPages;
    QVariantList            _instrumentPages;
    int                     _vehicleStateIndex;
    QList<VehicleState_t>   _vehicleStates;
    QList<double>           _rgPulseValues;
    QList<QList<double>>    _rgAngleStrengths;
    QList<QList<double>>    _rgAngleRatios;
    bool                    _flightMachineActive;
    double                  _firstHeading;
    int                     _firstSlice;
    int                     _nextSlice;
    int                     _cSlice;
    int                     _detectionStatus = -1;
    double                  _lastPulseTime = 0;

    qreal                   _beepStrength;
    qreal                   _temp;
    int                     _bpm;
    QElapsedTimer           _elapsedTimer;
    QTimer                  _delayTimer;
    QTimer                  _targetValueTimer;
    bool                    _simulate;
    QTimer                  _simPulseTimer;
    QTimer                  _vhfCommandAckTimer;
    CommandID               _vhfCommandAckExpected;
    CustomOptions*          _customOptions;
    CustomSettings*         _customSettings;
    int                     _vehicleFrequency;
    int                     _lastPulseSendIndex;
    int                     _missedPulseCount;
    QmlObjectListModel      _customMapItems;
    QFile                   _pulseLogFile;

    double                  _pulseTimeSeconds;
    double                  _pulseSNR;
    bool                    _pulseConfirmed;

    // Simulator values
    uint32_t    _simulatorTagId                 = 0;
    uint32_t    _simulatorFrequency;
    uint32_t    _simulatorPulseDuration;
    uint32_t    _simulatorIntraPulse1;
    uint32_t    _simulatorIntraPulse2;
    uint32_t    _simulatorIntraPulseUncertainty;
    uint32_t    _simulatorIntraPulseJitter;
    float       _simulatorMaxPulse;

};

class PulseRoseMapItem : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString          url             MEMBER _url)
    Q_PROPERTY(int              rotationIndex   MEMBER _rotationIndex)
    Q_PROPERTY(QGeoCoordinate   rotationCenter  MEMBER _rotationCenter)

public:
    PulseRoseMapItem(QUrl& itemUrl, int rotationIndex, QGeoCoordinate rotationCenter, QObject* parent)
        : QObject(parent)
        , _url(itemUrl.toString())
        , _rotationIndex(rotationIndex)
        , _rotationCenter(rotationCenter)
    { }

private:
    QString         _url;
    int             _rotationIndex;
    QGeoCoordinate  _rotationCenter;
};
