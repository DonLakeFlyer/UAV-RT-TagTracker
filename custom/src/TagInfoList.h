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

    void                checkForTagFile     (void);
    bool                loadTags            (uint32_t sdrType);
    ExtendedTagInfo_t   getTagInfo          (uint32_t tag_id, bool& exists);
    uint32_t            radioCenterHz       () { return _radioCenterHz; }
    uint32_t            maxIntraPulseMsecs  (uint32_t& k);

private:
    void    _setupTunerVars     (uint32_t sdrType);
    bool    _channelizerTuner   ();
    void    _printChannelMap    (const uint32_t centerFreqHz, const QVector<uint32_t>& wrappedRequestedFreqsHz);
    int     _firstChannelFreqHz (const int centerFreq);
    QString _tagInfoFilePath    ();
    bool    _generateThresholds ();

    uint32_t            _radioCenterHz      = 0;
    QVector<uint32_t>   _channelBucketCenters;

    uint32_t   _sampleRateHz;
    uint32_t   _fullBwHz;
    uint32_t   _halfBwHz;
    uint32_t   _channelBwHz;
    uint32_t   _halfChannelBwHz;

    static const uint       _nChannels          = 100;
};
