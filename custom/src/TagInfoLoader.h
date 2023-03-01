#pragma once

#include "QGCMAVLink.h"
#include "TunnelProtocol.h"

#include <QObject>
#include <QMap>

class TagInfoLoader : public QObject
{
    Q_OBJECT

public:
    TagInfoLoader(QObject* parent = NULL);
    ~TagInfoLoader();

    typedef struct {
        TunnelProtocol::TagInfo_t   tagInfo;
        QString                     name;
        QString                     ip_msecs_1_id;
        QString                     ip_msecs_2_id;
    } ExtendedTagInfo_t;

    typedef QList<ExtendedTagInfo_t> TagList_t;

    bool                loadTags    (void);
    TagList_t           getTagList  (void);
    ExtendedTagInfo_t   getTagInfo  (uint32_t tag_id) { return _tagInfoMap[tag_id]; }

private:
    typedef QMap<uint32_t, ExtendedTagInfo_t> TagInfoMap_t;

    TagInfoMap_t _tagInfoMap;
};
