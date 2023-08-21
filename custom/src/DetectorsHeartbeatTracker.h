#pragma once

#include "TagInfoList.h"

#include <QObject>
#include <QMap>
#include <QTimer>

class DetectorsHeartbeatTracker : public QObject
{
    Q_OBJECT

public:
    DetectorsHeartbeatTracker(QObject* parent = nullptr);
    ~DetectorsHeartbeatTracker();

    void            setupFromTags               (TagInfoList& tagInfoList);
    void            heartbeatReceived           (int tagId);
    bool            isHeartbeatWaitActive         ()                        { return _heartbeatWaitActive; }
    void            startHeartbeatWait          ();
    void            endHeartbeatWait            ()                          { _heartbeatWaitActive = false; }
    bool            isHeartbeatWaitComplete     (uint32_t heartbeatCount);
    QVariantList    detectorsLostHeartbeatList  ();

signals:
    void detectorsLostHeartbeatChanged();

private:
    typedef struct HeartbeatInfo_t {
        bool        heartbeatLost               { true };
        uint32_t    heartbeatCount              { 0 };
        uint        heartbeatTimerInterval      { 0 };
        QTimer*     pTimer                      { nullptr };
        uint        rotationDelayHeartbeatCount { 0 };
    } HeartbeatInfo_t;

    void _setupDetectorHeartbeatInfo(HeartbeatInfo_t& heartbeatInfo, ExtendedTagInfo_t& extTagInfo, bool rate1);
    void _clear                     (void);

    bool                            _heartbeatWaitActive           { false };
    QMap<uint32_t, HeartbeatInfo_t> _detectorHeartbeatInfoMap;
};
