#include "DetectorInfo.h"
#include "Vehicle.h"
#include "CustomSettings.h"
#include "QGCApplication.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "FlyViewSettings.h"
#include "QmlComponentInfo.h"
#include "TunnelProtocol.h"
#include "PulseInfoList.h"

#include <QDebug>
#include <QPointF>
#include <QLineF>
#include <QQmlEngine>

using namespace TunnelProtocol;

QGC_LOGGING_CATEGORY(DetectorInfoLog, "DetectorInfoLog")

DetectorInfo::DetectorInfo(uint32_t tagId, const QString& tagLabel, uint32_t intraPulseMsecs, uint32_t k, QObject* parent)
    : QObject           (parent)
    , _tagId            (tagId)
    , _tagLabel         (tagLabel)
    , _intraPulseMsecs  (intraPulseMsecs)
    , _k                (k)
{
    _heartbeatTimerInterval = ((_k + 1) * intraPulseMsecs) + 1000;

    qDebug() << "DetectorInfo::DetectorInfo" << _tagId << _tagLabel << _intraPulseMsecs << _k << _heartbeatTimerInterval;

    _heartbeatTimeoutTimer.setSingleShot(true);
    _heartbeatTimeoutTimer.setInterval(_heartbeatTimerInterval);
    _heartbeatTimeoutTimer.callOnTimeout([this]() {
        _heartbeatLost = true;
        emit heartbeatLostChanged();
    });

    _stalePulseStrengthTimer.setSingleShot(true);
    _stalePulseStrengthTimer.setInterval(_heartbeatTimerInterval);
    _stalePulseStrengthTimer.callOnTimeout([this]() {
        _lastPulseStale = true;
        emit lastPulseStaleChanged();
    });
}

DetectorInfo::~DetectorInfo()
{

}

void DetectorInfo::handleTunnelPulse(const mavlink_tunnel_t& tunnel)
{
    if (tunnel.payload_length != sizeof(PulseInfo_t)) {
        qWarning() << "handleTunnelPulse Received incorrectly sized PulseInfo payload expected:actual" <<  sizeof(PulseInfo_t) << tunnel.payload_length;
    }

    PulseInfo_t pulseInfo;
    memcpy(&pulseInfo, tunnel.payload, sizeof(pulseInfo));

    bool isDetectorHeartbeat = pulseInfo.frequency_hz == 0;

    if (pulseInfo.tag_id == _tagId) {
        if (isDetectorHeartbeat) {
            _heartbeatLost = false;
            _heartbeatCount++;
            _heartbeatTimeoutTimer.start();
            emit heartbeatLostChanged();
            qCDebug(DetectorInfoLog) << "HEARTBEAT from Detector id" << _tagId;
        } else if (pulseInfo.confirmed_status) {
            qCDebug(DetectorInfoLog) << "CONFIRMED tag_id:frequency_hz:seq_ctr:snr:stft_score:noise_psd" <<
                                        pulseInfo.tag_id <<
                                        pulseInfo.frequency_hz <<
                                        pulseInfo.group_seq_counter <<
                                        pulseInfo.snr <<
                                        pulseInfo.stft_score <<
                                        pulseInfo.noise_psd;

            // We track the max pulse in each K group
            if (_lastPulseGroupSeqCtr != pulseInfo.group_seq_counter) {
                _lastPulseGroupSeqCtr = pulseInfo.group_seq_counter;
                _pulseGroupGrount++;
                _lastPulseStrength = pulseInfo.snr;
            } else {
                _lastPulseStrength = std::max(pulseInfo.snr, _lastPulseStrength);
            }
            _lastPulseStale = false;

            emit lastPulseStrengthChanged();
            emit lastPulseStaleChanged();
            _stalePulseStrengthTimer.start();

            _maxStrength = qMax(_maxStrength, pulseInfo.snr);
        }
    } 
}
