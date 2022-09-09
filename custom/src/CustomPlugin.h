#pragma once

#include "QGCCorePlugin.h"
#include "QmlObjectListModel.h"
#include "CustomOptions.h"
#include "FactSystem.h"

#include <QElapsedTimer>
#include <QGeoCoordinate>
#include <QTimer>
#include <QLoggingCategory>

class CustomSettings;

Q_DECLARE_LOGGING_CATEGORY(CustomPluginLog)

class CustomPlugin : public QGCCorePlugin
{
    Q_OBJECT

public:
    CustomPlugin(QGCApplication* app, QGCToolbox* toolbox);
    ~CustomPlugin();

    Q_PROPERTY(CustomSettings*  customSettings         MEMBER _customSettings             CONSTANT)
    Q_PROPERTY(qreal            beepStrength        MEMBER _beepStrength            NOTIFY beepStrengthChanged)
    Q_PROPERTY(qreal            temp                MEMBER _temp                    NOTIFY tempChanged)
    Q_PROPERTY(int              bpm                 MEMBER _bpm                     NOTIFY bpmChanged)
    Q_PROPERTY(QVariantList     angleRatios         MEMBER _rgAngleRatios           NOTIFY angleRatiosChanged)
    Q_PROPERTY(int              strongestAngle      MEMBER _strongestAngle          NOTIFY strongestAngleChanged)
    Q_PROPERTY(int              strongestPulsePct   MEMBER _strongestPulsePct       NOTIFY strongestPulsePctChanged)
    Q_PROPERTY(bool             flightMachineActive MEMBER _flightMachineActive     NOTIFY flightMachineActiveChanged)
    Q_PROPERTY(int              vehicleFrequency    MEMBER _vehicleFrequency        NOTIFY vehicleFrequencyChanged)
    Q_PROPERTY(int              missedPulseCount    MEMBER _missedPulseCount        NOTIFY missedPulseCountChanged)

    Q_INVOKABLE void start          (void);
    Q_INVOKABLE void cancelAndReturn(void);
    Q_INVOKABLE void sendTag        (void);

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
    void strongestAngleChanged      (int strongestAngle);
    void strongestPulsePctChanged   (int strongestPulsePct);
    void flightMachineActiveChanged (bool flightMachineActive);
    void vehicleFrequencyChanged    (int vehicleFrequency);
    void missedPulseCountChanged    (int missedPulseCount);

private slots:
    void _vehicleStateRawValueChanged   (QVariant rawValue);
    void _nextVehicleState              (void);
    void _detectComplete                (void);
    void _delayComplete                 (void);
    void _targetValueFailed             (void);
    void _updateFlightMachineActive     (bool flightMachineActive);
    void _mavCommandResult              (int vehicleId, int component, int command, int result, bool noResponseFromVehicle);
    void _simulatePulse                 (void);
    void _vhfCommandAckFailed           (void);

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
    void _rotateVehicle                 (Vehicle* vehicle, double headingDegrees);
    void _say                           (QString text);
    bool _armVehicleAndValidate         (Vehicle* vehicle);
    bool _setRTLFlightModeAndValidate   (Vehicle* vehicle);
    void _sendCommandAndVerify          (Vehicle* vehicle, MAV_CMD command, double param1 = 0.0, double param2 = 0.0, double param3 = 0.0, double param4 = 0.0, double param5 = 0.0, double param6 = 0.0, double param7 = 0.0);
    void _takeoff                       (Vehicle* vehicle, double takeoffAltRel);
    void _resetStateAndRTL              (void);
    int  _rawPulseToPct                 (double rawPulse);
    void _sendVHFCommand                (Vehicle* vehicle, LinkInterface* link, uint32_t vhfCommandId, const mavlink_message_t& msg);
    void _handleSimulatedTagCommand     (const mavlink_debug_float_array_t& debug_float_array);
    void _handleSimulatedStartDetection (const mavlink_debug_float_array_t& debug_float_array);
    void _handleSimulatedStopDetection  (const mavlink_debug_float_array_t& debug_float_array);
    QString _vhfCommandIdToText         (uint32_t vhfCommandId);
    void _sendSimulatedVHFCommandAck    (uint32_t vhfCommandId);
    void _startDetection                (void);
    void _startFlight                   (void);

    QVariantList            _settingsPages;
    QVariantList            _instrumentPages;
    int                     _vehicleStateIndex;
    QList<VehicleState_t>   _vehicleStates;
    QList<double>           _rgPulseValues;
    QList<double>           _rgAngleStrengths;
    QVariantList            _rgAngleRatios;
    QStringList             _rgStringAngleStrengths;
    int                     _strongestAngle;
    int                     _strongestPulsePct;
    bool                    _flightMachineActive;
    double                  _firstHeading;
    int                     _firstSlice;
    int                     _nextSlice;
    int                     _cSlice;

    qreal                   _beepStrength;
    qreal                   _temp;
    int                     _bpm;
    QElapsedTimer           _elapsedTimer;
    QTimer                  _delayTimer;
    QTimer                  _targetValueTimer;
    bool                    _simulate;
    QTimer                  _simPulseTimer;
    QTimer                  _vhfCommandAckTimer;
    uint32_t                _vhfCommandAckExpected;
    CustomOptions*          _customOptions;
    CustomSettings*         _customSettings;
    int                     _vehicleFrequency;
    int                     _lastPulseSendIndex;
    int                     _missedPulseCount;
    QmlObjectListModel      _customMapItems;

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

    Q_PROPERTY(QString url MEMBER _url)

public:
    PulseRoseMapItem(QUrl& itemUrl, QObject* parent) : QObject(parent), _url(itemUrl.toString()) { }

private:
    QString _url;
};
