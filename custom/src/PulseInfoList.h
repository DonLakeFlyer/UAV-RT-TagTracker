#pragma once

#include "QmlObjectListModel.h"

class PulseInfoList : public QmlObjectListModel
{
    Q_OBJECT

public:
    PulseInfoList(uint tagId, QString& tagName, uint tagFreqHz, QObject* parent = NULL);
    ~PulseInfoList();

    Q_PROPERTY(uint     tagId     READ      tagId       CONSTANT)
    Q_PROPERTY(QString  tagName   MEMBER    _tagName   CONSTANT)
    Q_PROPERTY(uint     tagFreqHz MEMBER    _tagFreqHz CONSTANT)

    uint tagId() const { return _tagId; }

private:
    uint    _tagId;
    QString _tagName;
    uint    _tagFreqHz;
};
