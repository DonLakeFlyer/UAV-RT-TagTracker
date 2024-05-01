#pragma once

#include "QmlObjectListModel.h"
#include "QGCMAVLink.h"

class TagDatabase;

class DetectorInfoListModel : public QmlObjectListModel
{
    Q_OBJECT

public:
    DetectorInfoListModel(QObject* parent = nullptr);
    ~DetectorInfoListModel();

    void    setupFromTags               (TagDatabase* tagDB);
    void    handleTunnelPulse           (const mavlink_tunnel_t& tunnel);
    void    resetMaxStrength            ();
    void    resetPulseGroupCount        ();
    double  maxStrength                 () const;
    bool    allHeartbeatCountsReached   (uint32_t targetHeartbeatCount) const;
    bool    allPulseGroupCountsReached  (uint32_t targetPulseGroupCount) const;
};
