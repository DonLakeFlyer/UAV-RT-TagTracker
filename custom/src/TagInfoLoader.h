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

    typedef QList<TunnelProtocol::TagInfo_t> TagList_t;

    bool                        loadTags    (void);
    TagList_t                   getTagList  (void);
    TunnelProtocol::TagInfo_t   getTagInfo  (uint32_t tag_id) { return _tagInfoMap[tag_id]; }

private:
    typedef QMap<uint32_t, TunnelProtocol::TagInfo_t> TagInfoMap_t;

    TagInfoMap_t _tagInfoMap;
};
