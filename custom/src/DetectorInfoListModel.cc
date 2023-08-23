#include "DetectorInfoListModel.h"
#include "DetectorInfo.h"

#include <QDebug>
#include <QPointF>
#include <QLineF>
#include <QQmlEngine>

DetectorInfoListModel::DetectorInfoListModel(const TagInfoList& tagInfoList, QObject* parent)
    : QmlObjectListModel(parent)
{
    for (auto& extTagInfo: tagInfoList) {
        DetectorInfo* detectorInfo = new DetectorInfo(extTagInfo.tagInfo.id, extTagInfo.ip_msecs_1_id, extTagInfo.tagInfo.intra_pulse1_msecs, extTagInfo.tagInfo.k, this);
        append(detectorInfo);

        if (extTagInfo.tagInfo.intra_pulse2_msecs != 0) {
            DetectorInfo* detectorInfo = new DetectorInfo(extTagInfo.tagInfo.id + 1, extTagInfo.ip_msecs_2_id, extTagInfo.tagInfo.intra_pulse2_msecs, extTagInfo.tagInfo.k, this);
            append(detectorInfo);
        }
    }
}

DetectorInfoListModel::~DetectorInfoListModel()
{

}

void DetectorInfoListModel::handleTunnelPulse(const mavlink_tunnel_t& tunnel)
{
    for (int i=0; i<count(); i++) {
        DetectorInfo* detectorInfo = qobject_cast<DetectorInfo*>(get(i));
        detectorInfo->handleTunnelPulse(tunnel);
    }
}

void DetectorInfoListModel::resetMaxSNR()
{
    for (int i=0; i<count(); i++) {
        DetectorInfo* detectorInfo = qobject_cast<DetectorInfo*>(get(i));
        detectorInfo->resetMaxSNR();
    }
}

void DetectorInfoListModel::resetPulseGroupCount()
{
    for (int i=0; i<count(); i++) {
        DetectorInfo* detectorInfo = qobject_cast<DetectorInfo*>((*this)[i]);
        detectorInfo->resetPulseGroupCount();
    }
}

double DetectorInfoListModel::maxSNR() const
{
    double maxSNR = 0.0;
    for (int i=0; i<count(); i++) {
        const DetectorInfo* detectorInfo = qobject_cast<const DetectorInfo*>((*this)[i]);
        maxSNR = std::max(maxSNR, detectorInfo->maxSNR());
    }

    return maxSNR;
}

bool DetectorInfoListModel::allHeartbeatCountsReached(uint32_t targetHeartbeatCount) const
{
    for (int i=0; i<count(); i++) {
        const DetectorInfo* detectorInfo = qobject_cast<const DetectorInfo*>((*this)[i]);
        if (detectorInfo->heartbeatCount() < targetHeartbeatCount) {
            return false;
        }
    }

    return true;
}

bool DetectorInfoListModel::allPulseGroupCountsReached(uint32_t targetPulseGroupCount) const
{
    for (int i=0; i<count(); i++) {
        const DetectorInfo* detectorInfo = qobject_cast<const DetectorInfo*>((*this)[i]);
        if (detectorInfo->pulseGroupGrount() < targetPulseGroupCount) {
            return false;
        }
    }

    return true;
}
