#pragma once

#include "QGCMAVLink.h"
#include "TunnelProtocol.h"

#include <QObject>

class PulseInfo : public QObject
{
    Q_OBJECT

public:
    PulseInfo(TunnelProtocol::PulseInfo_t pulseInfo, QString& name, QString& rateChar, QObject* parent = NULL);
    ~PulseInfo();

    Q_PROPERTY(uint     tag_id              READ tag_id             CONSTANT)
    Q_PROPERTY(QString  name                READ name               CONSTANT)
    Q_PROPERTY(QString  rateChar            READ rateChar               CONSTANT)
    Q_PROPERTY(uint     frequency_hz        READ frequency_hz       CONSTANT)
    Q_PROPERTY(double   start_time_seconds  READ start_time_seconds CONSTANT)
    Q_PROPERTY(double   snr                 READ snr                CONSTANT)
    Q_PROPERTY(uint     group_ind           READ group_ind          CONSTANT)
    Q_PROPERTY(bool     confirmed           READ confirmed          CONSTANT)

    uint    tag_id              (void) { return _pulseInfo.tag_id; }
    QString name                (void) { return _name; }
    QString rateChar            (void) { return _rateChar; }
    uint    frequency_hz        (void) { return _pulseInfo.frequency_hz; }
    double  start_time_seconds  (void) { return _pulseInfo.start_time_seconds; }
    double  snr                 (void) { return _pulseInfo.snr; }
    uint    group_ind           (void) { return _pulseInfo.group_ind; }
    bool    confirmed           (void) { return _pulseInfo.confirmed_status; } 

private:
    TunnelProtocol::PulseInfo_t _pulseInfo;
    QString                     _name;
    QString                     _rateChar;
};
