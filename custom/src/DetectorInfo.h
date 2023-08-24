#pragma once

#include "QGCLoggingCategory.h"
#include "TunnelProtocol.h"
#include "QGCMAVLink.h"

#include <QObject>
#include <QTimer>

Q_DECLARE_LOGGING_CATEGORY(DetectorInfoLog)

class DetectorInfo : public QObject
{
    Q_OBJECT

public:
    DetectorInfo(uint32_t tagId, const QString& tagLabel, uint32_t intraPulseMsecs, uint32_t k, QObject* parent = nullptr);
    ~DetectorInfo();

    Q_PROPERTY(int      tagId           MEMBER _tagId           CONSTANT)
    Q_PROPERTY(QString  tagLabel        MEMBER _tagLabel        CONSTANT)
    Q_PROPERTY(bool     heartbeatLost   MEMBER _heartbeatLost   NOTIFY heartbeatLostChanged)
    Q_PROPERTY(double   lastPulseSNR    MEMBER _lastPulseSNR    NOTIFY lastPulseSNRChanged)
    Q_PROPERTY(bool     lastPulseStale  MEMBER _lastPulseStale  NOTIFY lastPulseStaleChanged)

    void        handleTunnelPulse   (const mavlink_tunnel_t& tunnel);
    void        resetMaxSNR         ()                              { _maxSNR = 0.0; }
    void        resetHeartbeatCount ()                              { _heartbeatCount = 0; }
    void        resetPulseGroupCount()                              { _pulseGroupGrount = 0; }
    double      maxSNR              () const                        { return _maxSNR; }
    uint32_t    heartbeatCount      () const                        { return _heartbeatCount; }
    uint32_t    pulseGroupGrount    () const                        { return _pulseGroupGrount; }

signals:
    void heartbeatLostChanged   ();
    void lastPulseSNRChanged    ();
    void lastPulseStaleChanged  ();

private:
    uint32_t        _tagId                  = 0;
    QString         _tagLabel;
    uint32_t        _intraPulseMsecs        = 0;
    uint32_t        _k                      = 0;
    bool            _heartbeatLost          = true;
    double          _lastPulseSNR           = 0.0;
    int             _lastPulseGroupSeqCtr   = -1;
    bool            _lastPulseStale         = true;
    uint32_t        _heartbeatTimerInterval = 0;
    QTimer          _heartbeatTimeoutTimer;
    QTimer          _stalePulseSNRTimer;
    bool            _trackingMaxSNR         = false;
    double          _maxSNR                 = 0.0;
    uint32_t        _heartbeatCount         = 0;
    uint32_t        _pulseGroupGrount       = 0;
};
