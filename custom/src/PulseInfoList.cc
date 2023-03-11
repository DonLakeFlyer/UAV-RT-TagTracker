#include "PulseInfoList.h"

PulseInfoList::PulseInfoList(uint tagId, QString& tagName, uint tagFreqHz, QObject* parent)
    : QmlObjectListModel(parent)
    , _tagId    (tagId)
    , _tagName  (tagName)
    , _tagFreqHz(tagFreqHz)
{

}

PulseInfoList::~PulseInfoList()
{

}
