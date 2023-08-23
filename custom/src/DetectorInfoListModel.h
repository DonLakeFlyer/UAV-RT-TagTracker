#pragma once

#include "QmlObjectListModel.h"
#include "TagInfoList.h"
#include "QGCMAVLink.h"

class DetectorInfoListModel : public QmlObjectListModel
{
    Q_OBJECT

public:
    DetectorInfoListModel(const TagInfoList& tagInfoList, QObject* parent = nullptr);
    ~DetectorInfoListModel();

    void    handleTunnelPulse           (const mavlink_tunnel_t& tunnel);
    void    resetMaxSNR                 ();
    void    resetPulseGroupCount        ();
    double  maxSNR                      () const;
    bool    allHeartbeatCountsReached   (uint32_t targetHeartbeatCount) const;
    bool    allPulseGroupCountsReached  (uint32_t targetPulseGroupCount) const;
};
