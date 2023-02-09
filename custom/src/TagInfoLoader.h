#pragma once

#include "QGCMAVLink.h"
#include "TunnelProtocol.h"

#include <QObject>

class TagInfoLoader : public QObject
{
    Q_OBJECT

public:
    TagInfoLoader(QObject* parent = NULL);
    ~TagInfoLoader();

    bool loadTags(void);

    QList<TunnelProtocol::TagInfo_t> tagList;
};
