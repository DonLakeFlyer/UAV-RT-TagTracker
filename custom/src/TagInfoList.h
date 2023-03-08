#pragma once

#include <cstdint>
#include "TunnelProtocol.h"

#include <QObject>
#include <QList>

typedef struct {
    TunnelProtocol::TagInfo_t   tagInfo;
    QString                     name;
    QString                     ip_msecs_1_id;
    QString                     ip_msecs_2_id;
} ExtendedTagInfo_t;

class TagInfoList : public QList<ExtendedTagInfo_t>
{
public:
    TagInfoList();

    bool                loadTags        (void);
    ExtendedTagInfo_t   getTagInfo      (uint32_t tag_id);
    uint32_t            radioCenterHz   () { return _radioCenterHz; }

private:
    bool _channelizerTuner  ();
    void _printChannelMap   (const uint32_t centerFreqHz, const QVector<uint32_t>& wrappedRequestedFreqsHz);
    int  _firstChannelFreqHz(const int centerFreq);

    uint32_t            _radioCenterHz      = 0;
    QVector<uint32_t>   _channelBucketCenters;

    static const uint32_t   _sampleRateHz       = 375000;
    static const uint       _nChannels          = 100;
    static const uint32_t   _fullBwHz           = _sampleRateHz;
    static const uint32_t   _halfBwHz           = _fullBwHz / 2;
    static const uint32_t   _channelBwHz        = _sampleRateHz / _nChannels;
    static const uint32_t   _halfChannelBwHz    = _channelBwHz / 2;
};
