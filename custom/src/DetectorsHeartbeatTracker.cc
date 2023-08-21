#include "DetectorsHeartbeatTracker.h"
#include "TagInfoList.h"

#include <QDebug>

DetectorsHeartbeatTracker::DetectorsHeartbeatTracker(QObject* parent)
    : QObject(parent)
{

}

DetectorsHeartbeatTracker::~DetectorsHeartbeatTracker()
{
    _clear();
}

void DetectorsHeartbeatTracker::_setupDetectorHeartbeatInfo(HeartbeatInfo_t& detectorHeartbeatInfo, ExtendedTagInfo_t& extTagInfo, bool rate1)
{
    detectorHeartbeatInfo.heartbeatTimerInterval    = (extTagInfo.tagInfo.k + 1) * (rate1 ?  extTagInfo.tagInfo.intra_pulse1_msecs : extTagInfo.tagInfo.intra_pulse2_msecs);
    detectorHeartbeatInfo.pTimer                    = new QTimer(this);

    detectorHeartbeatInfo.pTimer->setSingleShot(true);
    detectorHeartbeatInfo.pTimer->setInterval(detectorHeartbeatInfo.heartbeatTimerInterval);
    detectorHeartbeatInfo.pTimer->callOnTimeout([this, &detectorHeartbeatInfo]() {
        detectorHeartbeatInfo.heartbeatLost = true;
        emit detectorsLostHeartbeatChanged();
    });
}

void DetectorsHeartbeatTracker::setupFromTags(TagInfoList& tagInfoList)
{
    _clear();

    for (auto& extTagInfo: tagInfoList) {
        _setupDetectorHeartbeatInfo(_detectorHeartbeatInfoMap[extTagInfo.tagInfo.id], extTagInfo, true /* rate1 */);

        if (extTagInfo.tagInfo.intra_pulse2_msecs != 0) {
            _setupDetectorHeartbeatInfo(_detectorHeartbeatInfoMap[extTagInfo.tagInfo.id + 1], extTagInfo, false /* rate1 */);
        }
    }

    emit detectorsLostHeartbeatChanged();
}

void  DetectorsHeartbeatTracker::heartbeatReceived(int tagId)
{
    if (!_detectorHeartbeatInfoMap.contains(tagId)) {
        qCritical() << "detectorHeartbeatReceived call with tagId which is not in _detectorHeartbeatInfoMap - tagId:" << tagId;
        return;
    }

    HeartbeatInfo_t& detectorHeartbeatInfo  = _detectorHeartbeatInfoMap[tagId];
    bool previousHeartbeatLost              = detectorHeartbeatInfo.heartbeatLost;
    detectorHeartbeatInfo.heartbeatLost     = false;
    detectorHeartbeatInfo.heartbeatCount++;
    detectorHeartbeatInfo.pTimer->start(detectorHeartbeatInfo.heartbeatTimerInterval);

    if (previousHeartbeatLost != detectorHeartbeatInfo.heartbeatLost) {
        emit detectorsLostHeartbeatChanged();
    }
}

void DetectorsHeartbeatTracker::_clear(void)
{
    for (auto& info: _detectorHeartbeatInfoMap) {
        delete info.pTimer;
    }
    _detectorHeartbeatInfoMap.clear();
    emit detectorsLostHeartbeatChanged();
}

void DetectorsHeartbeatTracker::startHeartbeatWait()
{
    _heartbeatWaitActive = true;
    for (auto& info: _detectorHeartbeatInfoMap) {
        info.heartbeatCount = 0;
    }
}

bool DetectorsHeartbeatTracker::isHeartbeatWaitComplete(uint32_t heartbeatCount)
{
    for (auto& info: _detectorHeartbeatInfoMap) {
        if (info.heartbeatCount < heartbeatCount) {
            return false;
        }
    }

    return true;
}

QVariantList  DetectorsHeartbeatTracker::detectorsLostHeartbeatList()
{
    QVariantList heartbeatLostList;

    for (auto& info: _detectorHeartbeatInfoMap) {
        heartbeatLostList.append(QVariant::fromValue(info.heartbeatLost));
    }

    return heartbeatLostList;
}
