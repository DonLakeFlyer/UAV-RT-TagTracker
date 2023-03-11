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

Q_DECLARE_LOGGING_CATEGORY(CustomPluginLog)

class CustomPlugin : public QGCCorePlugin
{
    Q_OBJECT

public:
    CustomPlugin(QGCApplication* app, QGCToolbox* toolbox);
    ~CustomPlugin();

    Q_PROPERTY(CustomSettings*  customSettings      MEMBER  _customSettings         CONSTANT)
    Q_PROPERTY(QList<QList<double>> angleRatios     MEMBER  _rgAngleRatios          NOTIFY angleRatiosChanged)
    Q_PROPERTY(bool             flightMachineActive MEMBER  _flightMachineActive    NOTIFY flightMachineActiveChanged)
    Q_PROPERTY(QVariantList     pulseLog            MEMBER  _pulseLog               NOTIFY pulseLogChanged)

    Q_INVOKABLE void startRotation      (void);
    Q_INVOKABLE void cancelAndReturn    (void);
    Q_INVOKABLE void sendTags           (void);
    Q_INVOKABLE void startDetection     (void);
    Q_INVOKABLE void stopDetection      (void);
    Q_INVOKABLE void airspyHFCapture    (void);
    Q_INVOKABLE void airspyMiniCapture  (void);

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

private slots:
    void _vehicleStateRawValueChanged   (QVariant rawValue);
    void _nextVehicleState              (void);
    void _delayComplete                 (void);
    void _targetValueFailed             (void);
    void _updateFlightMachineActive     (bool flightMachineActive);
    void _mavCommandResult              (int vehicleId, int component, int command, int result, bool noResponseFromVehicle);
    void _tunnelCommandAckFailed        (void);
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

    QTimer                  _delayTimer;
    QTimer                  _targetValueTimer;
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

    QVariantList            _pulseLog;
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
