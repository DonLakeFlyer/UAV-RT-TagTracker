#pragma once

#ifdef HERELINK_BUILD
    #include "HerelinkCorePlugin.h"
#else
    #include "QGCCorePlugin.h"
#endif
#include "QmlObjectListModel.h"
#include "CustomOptions.h"
#include "CustomSettings.h"
#include "FactSystem.h"
#include "TunnelProtocol.h"
#include "PulseInfo.h"
#include "DetectorInfoListModel.h"
#include "TagDatabase.h"

#include <QElapsedTimer>
#include <QGeoCoordinate>
#include <QTimer>
#include <QLoggingCategory>
#include <QFile>

Q_DECLARE_LOGGING_CATEGORY(CustomPluginLog)

class CustomPlugin : 
#ifdef HERELINK_BUILD
    public HerelinkCorePlugin
#else
    public QGCCorePlugin
#endif
{
    Q_OBJECT

public:
    CustomPlugin(QGCApplication* app, QGCToolbox* toolbox);
    ~CustomPlugin();

    // IMPORTANT: This enum must match the heartbeat status values in TunnelProtocol.h
    Q_ENUMS(ControllerStatus)
    enum ControllerStatus {
        ControllerStatusIdle           = HEARTBEAT_STATUS_IDLE,
        ControllerStatusReceivingTags  = HEARTBEAT_STATUS_RECEIVING_TAGS,
        ControllerStatusHasTags        = HEARTBEAT_STATUS_HAS_TAGS,
        ControllerStatusDetecting      = HEARTBEAT_STATUS_DETECTING,
        ControllerStatusCapture        = HEARTBEAT_STATUS_CAPTURE
    };

    Q_PROPERTY(CustomSettings*      customSettings          READ    customSettings              CONSTANT)
    Q_PROPERTY(QList<QList<double>> angleRatios             MEMBER  _rgAngleRatios              NOTIFY angleRatiosChanged)
    Q_PROPERTY(QList<double>        calcedBearings          MEMBER  _rgCalcedBearings           NOTIFY calcedBearingsChanged)
    Q_PROPERTY(bool                 flightMachineActive     MEMBER  _flightStateMachineActive   NOTIFY flightMachineActiveChanged)
    Q_PROPERTY(bool                 controllerLostHeartbeat MEMBER  _controllerLostHeartbeat    NOTIFY controllerLostHeartbeatChanged)
    Q_PROPERTY(int                  controllerStatus        MEMBER  _controllerStatus           NOTIFY controllerStatusChanged)
    Q_PROPERTY(QmlObjectListModel*  detectorInfoList        READ    detectorInfoList            CONSTANT)
    Q_PROPERTY(TagDatabase*         tagDatabase             MEMBER  _tagDatabase                CONSTANT)

    CustomSettings*     customSettings  () { return _customSettings; }
    QmlObjectListModel* detectorInfoList() { return dynamic_cast<QmlObjectListModel*>(&_detectorInfoListModel); }

    Q_INVOKABLE void startRotation      (void);
    Q_INVOKABLE void cancelAndReturn    (void);
    Q_INVOKABLE void sendTags           (void);
    Q_INVOKABLE void startDetection     (void);
    Q_INVOKABLE void stopDetection      (void);
    Q_INVOKABLE void rawCapture         (void);

    // Overrides from QGCCorePlugin
    bool                mavlinkMessage          (Vehicle* vehicle, LinkInterface* link, mavlink_message_t message) final;
    QGCOptions*         options                 (void) final { return qobject_cast<QGCOptions*>(_customOptions); }
    bool                adjustSettingMetaData   (const QString& settingsGroup, FactMetaData& metaData) final;
    QmlObjectListModel* customMapItems          (void) final;
    const QVariantList& toolBarIndicators       (void) final;

    // Overrides from QGCTool
    void setToolbox(QGCToolbox* toolbox) final;

signals:
    void angleRatiosChanged             (void);
    void calcedBearingsChanged          (void);
    void flightMachineActiveChanged     (bool flightMachineActive);
    void pulseInfoListsChanged          (void);
    void controllerLostHeartbeatChanged ();
    void controllerStatusChanged        ();

private slots:
    void _vehicleStateRawValueChanged   (QVariant rawValue);
    void _advanceStateMachine           (void);
    void _vehicleStateTimeout        (void);
    void _updateFlightMachineActive     (bool flightMachineActive);
    void _mavCommandResult              (int vehicleId, int component, int command, int result, bool noResponseFromVehicle);
    void _tunnelCommandAckFailed        (void);
    void _controllerHeartbeatFailed     (void);

private:
    typedef enum {
        CommandTakeoff,
        CommandSetHeading,
        CommandWaitForHeartbeats,
    } VehicleStateCommand_t;

    typedef struct VehicleState_t {
        VehicleStateCommand_t   command;
        Fact*                   fact;
        int                     targetValueWaitMsecs;
        double                  targetValue;
        double                  targetVariance;
    } VehicleState_t;

    typedef struct HeartbeatInfo_t {
        bool    heartbeatLost               { true };
        uint    heartbeatTimerInterval      { 0 };
        QTimer  timer;
        uint    rotationDelayHeartbeatCount { 0 };
    } HeartbeatInfo_t;

    void    _handleTunnelCommandAck     (const mavlink_tunnel_t& tunnel);
    void    _handleTunnelPulse          (const mavlink_tunnel_t& tunnel);
    void    _handleTunnelHeartbeat      (const mavlink_tunnel_t& tunnel);
    void    _rotateVehicle              (Vehicle* vehicle, double headingDegrees);
    void    _say                        (QString text);
    bool    _armVehicleAndValidate      (Vehicle* vehicle);
    bool    _setRTLFlightModeAndValidate(Vehicle* vehicle);
    void    _sendCommandAndVerify       (Vehicle* vehicle, MAV_CMD command, double param1 = 0.0, double param2 = 0.0, double param3 = 0.0, double param4 = 0.0, double param5 = 0.0, double param6 = 0.0, double param7 = 0.0);
    void    _takeoff                    (Vehicle* vehicle, double takeoffAltRel);
    void    _resetStateAndRTL           (void);
    int     _rawPulseToPct              (double rawPulse);
    void    _sendTunnelCommand          (uint8_t* payload, size_t payloadSize);
    QString _tunnelCommandIdToText      (uint32_t command);
    double  _pulseTimeSeconds           (void) { return _lastPulseInfo.start_time_seconds; }
    double  _pulseSNR                   (void) { return _lastPulseInfo.snr; }
    bool    _pulseConfirmed             (void) { return _lastPulseInfo.confirmed_status; }
    void    _sendNextTag                (void);
    void    _sendEndTags                (void);
    void    _setupDelayForSteadyCapture (void);
    void    _rotationDelayComplete      (void);
    QString _csvLogFilePath             (void);
    void    _csvStartFullPulseLog       (void);
    void    _csvStopFullPulseLog        (void);
    void    _csvClearPrevRotationLogs   (void);
    void    _csvStartRotationPulseLog   (int rotationCount);
    void    _csvStopRotationPulseLog    (bool calcBearing);
    void    _csvLogPulse                (QFile& csvFile, const TunnelProtocol::PulseInfo_t& pulseInfo);
    void    _csvLogRotationStartStop    (QFile& csvFile, bool startRotation);

    QVariantList            _settingsPages;
    QVariantList            _instrumentPages;
    int                     _vehicleStateIndex;
    QList<VehicleState_t>   _vehicleStates;
    QList<QList<double>>    _rgAngleStrengths;
    QList<QList<double>>    _rgAngleRatios;
    QList<double>           _rgCalcedBearings;
    bool                    _flightStateMachineActive;
    int                     _currentSlice;
    int                     _cSlice;
    int                     _detectionStatus    = -1;
    bool                    _retryRotation      = false;
    int                     _controllerStatus   = ControllerStatusIdle;
    int                     _nextTagIndexToSend = 0;

    QTimer                  _vehicleStateTimeoutTimer;
    QTimer                  _tunnelCommandAckTimer;
    uint32_t                _tunnelCommandAckExpected;
    CustomOptions*          _customOptions;
    CustomSettings*         _customSettings;
    int                     _vehicleFrequency;
    int                     _lastPulseSendIndex;
    int                     _missedPulseCount;
    QmlObjectListModel      _customMapItems;
    QFile                   _csvFullPulseLogFile;
    QFile                   _csvRotationPulseLogFile;
    int                     _csvRotationCount = 1;
    TunnelProtocol::PulseInfo_t _lastPulseInfo;

    DetectorInfoListModel   _detectorInfoListModel;

    TagDatabase*             _tagDatabase = nullptr;

    bool                    _controllerLostHeartbeat { true };
    QTimer                  _controllerHeartbeatTimer;
};

class PulseRoseMapItem : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString          url             MEMBER _url             CONSTANT)
    Q_PROPERTY(int              rotationIndex   MEMBER _rotationIndex   CONSTANT)
    Q_PROPERTY(QGeoCoordinate   rotationCenter  MEMBER _rotationCenter  CONSTANT)

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

class PulseMapItem : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString          url         MEMBER _url         CONSTANT)
    Q_PROPERTY(QGeoCoordinate   coordinate  MEMBER _coordinate  CONSTANT)
    Q_PROPERTY(uint             tagId       MEMBER _tagId       CONSTANT)
    Q_PROPERTY(double           snr         MEMBER _snr         CONSTANT)

public:
    PulseMapItem(QUrl& itemUrl, QGeoCoordinate coordinate, uint tagId, double snr, QObject* parent)
        : QObject       (parent)
        , _url          (itemUrl.toString())
        , _coordinate   (coordinate)
        , _tagId        (tagId)
        , _snr          (snr)
    { }

private:
    QString         _url;
    QGeoCoordinate  _coordinate;
    uint            _tagId;
    double          _snr;
};
