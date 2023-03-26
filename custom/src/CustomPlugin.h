#pragma once

#include "QGCCorePlugin.h"
#include "QmlObjectListModel.h"
#include "CustomOptions.h"
#include "FactSystem.h"
#include "TunnelProtocol.h"
#include "TagInfoList.h"
#include "PulseInfo.h"

#include <QElapsedTimer>
#include <QGeoCoordinate>
#include <QTimer>
#include <QLoggingCategory>
#include <QFile>

class CustomSettings;

Q_MOC_INCLUDE("CustomSettings.h")

Q_DECLARE_LOGGING_CATEGORY(CustomPluginLog)

class CustomPlugin : public QGCCorePlugin
{
    Q_OBJECT

public:
    CustomPlugin(QGCApplication* app, QGCToolbox* toolbox);
    ~CustomPlugin();

    Q_PROPERTY(CustomSettings*  customSettings      READ    customSettings          CONSTANT)
    Q_PROPERTY(QList<QList<double>> angleRatios     MEMBER  _rgAngleRatios          NOTIFY angleRatiosChanged)
    Q_PROPERTY(bool             flightMachineActive MEMBER  _flightStateMachineActive    NOTIFY flightMachineActiveChanged)
    Q_PROPERTY(QVariantList     pulseLog            MEMBER  _pulseLog               NOTIFY pulseLogChanged)
    Q_PROPERTY(bool             controllerHeartbeat MEMBER  _controllerHeartbeat    NOTIFY controllerHeartbeatChanged)
    Q_PROPERTY(QVariantList     detectorHeartbeats  MEMBER  _detectorHeartbeats     NOTIFY detectorHeartbeatsChanged)

    CustomSettings* customSettings() { return _customSettings; }

    Q_INVOKABLE void startRotation      (void);
    Q_INVOKABLE void cancelAndReturn    (void);
    Q_INVOKABLE void sendTags           (void);
    Q_INVOKABLE void startDetection     (void);
    Q_INVOKABLE void stopDetection      (void);
    Q_INVOKABLE void airspyHFCapture    (void);
    Q_INVOKABLE void airspyMiniCapture  (void);
    Q_INVOKABLE void downloadLogs       (void);

    // Overrides from QGCCorePlugin
    QVariantList&       settingsPages           (void) final;
    bool                mavlinkMessage          (Vehicle* vehicle, LinkInterface* link, mavlink_message_t message) final;
    QGCOptions*         options                 (void) final { return qobject_cast<QGCOptions*>(_customOptions); }
    bool                adjustSettingMetaData   (const QString& settingsGroup, FactMetaData& metaData) final;
    QmlObjectListModel* customMapItems          (void) final;

    // Overrides from QGCTool
    void setToolbox(QGCToolbox* toolbox) final;

signals:
    void angleRatiosChanged         (void);
    void flightMachineActiveChanged (bool flightMachineActive);
    void pulseInfoListsChanged      (void);
    void pulseLogChanged            ();
    void controllerHeartbeatChanged ();
    void detectorHeartbeatsChanged  ();

private slots:
    void _vehicleStateRawValueChanged   (QVariant rawValue);
    void _advanceStateMachine           (void);
    void _vehicleStateWaitFailed        (void);
    void _updateFlightMachineActive     (bool flightMachineActive);
    void _mavCommandResult              (int vehicleId, int component, int command, int result, bool noResponseFromVehicle);
    void _tunnelCommandAckFailed        (void);
    void _activeVehicleChanged          (Vehicle* activeVehicle);
    void _ftpDownloadComplete           (const QString& file, const QString& errorMsg);
    void _ftpCommandError               (const QString& msg);
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
        int                     targetValueWaitSecs;
        double                  targetValue;
        double                  targetVariance;
    } VehicleState_t;

    typedef struct DetectorHeartbeatInfo_t {
        bool    heartbeat               { false };
        uint    heartbeatTimerInterval  { 0 };
        QTimer* pTimer                  { NULL };
        uint    heartbeatCount          { 0 };
    } DetectorHeartbeatInfo_t;

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
    void    _logPulseToFile             (const mavlink_tunnel_t& tunnel);
    void    _logRotateStartStopToFile   (bool start);
    double  _pulseTimeSeconds           (void) { return _lastPulseInfo.start_time_seconds; }
    double  _pulseSNR                   (void) { return _lastPulseInfo.snr; }
    bool    _pulseConfirmed             (void) { return _lastPulseInfo.confirmed_status; }
    void    _sendNextTag                (void);
    void    _sendEndTags                (void);
    void    _resetPulseLog              (void);
    void    _clearDetectorHeartbeats    (void);
    void    _setupDetectorHeartbeats    (void);
    void    _updateDetectorHeartbeat    (int tagId);
    void    _rebuildDetectorHeartbeats  (void);
    void    _setupDetectorHeartbeatInfo (DetectorHeartbeatInfo_t& detectorHeartbeatInfo, ExtendedTagInfo_t& extTagInfo, bool rate1);
    void    _resetRotationPauseCounts   (void);
    void    _setupDelayForHeartbeats    (void);
    void    _rotationDelayComplete      (void);

    QVariantList            _settingsPages;
    QVariantList            _instrumentPages;
    int                     _vehicleStateIndex;
    QList<VehicleState_t>   _vehicleStates;
    QList<double>           _rgPulseValues;
    QList<QList<double>>    _rgAngleStrengths;
    QList<QList<double>>    _rgAngleRatios;
    bool                    _flightStateMachineActive;
    double                  _firstHeading;
    int                     _firstSlice;
    int                     _nextSlice;
    int                     _cSlice;
    int                     _detectionStatus = -1;

    QTimer                  _vehicleStateTimeoutTimer;
    QTimer                  _tunnelCommandAckTimer;
    uint32_t                _tunnelCommandAckExpected;
    CustomOptions*          _customOptions;
    CustomSettings*         _customSettings;
    int                     _vehicleFrequency;
    int                     _lastPulseSendIndex;
    int                     _missedPulseCount;
    QmlObjectListModel      _customMapItems;
    QFile                   _pulseLogFile;
    TunnelProtocol::PulseInfo_t _lastPulseInfo;

    TagInfoList             _tagInfoList;
    TagInfoList::const_iterator _nextTagToSend;

    bool                    _controllerHeartbeat { false };
    QTimer                  _controllerHeartbeatTimer;

    QVariantList                            _detectorHeartbeats;
    QMap<uint32_t, DetectorHeartbeatInfo_t> _detectorHeartbeatInfoMap;

    bool                                               _captureRotationPulses { false };
    QMap<uint32_t, QList<TunnelProtocol::PulseInfo_t>> _rotationPulseInfoMap;

    QVariantList            _pulseLog;
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
