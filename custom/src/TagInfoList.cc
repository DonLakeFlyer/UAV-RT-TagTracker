#include "TagInfoList.h"
#include "QGCApplication.h"
#include "SettingsManager.h"
#include "AppSettings.h"

#include <QFile>
#include <QTextStream>

using namespace TunnelProtocol;

bool TagInfoList::loadTags(void)
{
    clear();

    TagInfoList newTagInfoList;

    QString tagFilename = QString::asprintf("%s/TagInfo.txt",
                                            qgcApp()->toolbox()->settingsManager()->appSettings()->parameterSavePath().toStdString().c_str());
    QFile   tagFile(tagFilename);

    if (!tagFile.open(QIODevice::ReadOnly)) {
        qgcApp()->showAppMessage(QStringLiteral("TagInfoList: Unable to open tag file: %1").arg(tagFilename));
        return false;
    }


    QString     tagLine;
    QTextStream tagStream(&tagFile);
    uint32_t    lineCount = 0;
    QString     lineFormat = "id, name, freq_hz, ip_msecs_1, ip_msecs_1_id, ip_msecs_2, ip_msecs_2_id, pulse_width_msecs, ip_uncertainty_msecs, ip_jitter_msecs, k, false_alarm";
    while (tagStream.readLineInto(&tagLine)) {
        lineCount++;
        if (tagLine.startsWith("#")) {
            continue;
        }
        if (tagLine.length() == 0) {
            continue;
        }

        int         valueCount = lineFormat.split(",").count();
        QStringList tagValues = tagLine.split(",");
        if (tagValues.count() != valueCount) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoList: Line #%1 Does not contain %2 values").arg(lineCount).arg(valueCount));
            return false;
        }

        bool                ok;
        ExtendedTagInfo_t   extTagInfo;
        QString             tagValueString;
        int                 tagValuePosition = 0;

        extTagInfo.tagInfo.header.command   = COMMAND_ID_TAG;
        extTagInfo.tagInfo.channelizer_bin  = 0;

        // id, freq_hz, ip_msecs, pulse_width_msecs, ip_uncertainty_msecs, ip_jitter_msecs, k, false_alarm

        tagValueString          = tagValues[tagValuePosition++];
        extTagInfo.tagInfo.id   = tagValueString.toUInt(&ok);
        if (!ok) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoList: Line #%1 Value:'%2'. Unable to convert id to uint.").arg(lineCount).arg(tagValueString));
            return false;
        }
        if (extTagInfo.tagInfo.id <= 1) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoList: Line #%1 Value:'%2'. Tag ids must be greater than 1").arg(lineCount).arg(tagValueString));
            return false;
        }
        if (extTagInfo.tagInfo.id % 2) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoList: Line #%1 Value:'%2'. Tag ids must be even numbers").arg(lineCount).arg(tagValueString));
            return false;
        }

        extTagInfo.name = tagValues[tagValuePosition++];
        if (extTagInfo.name.length() == 0) {
            extTagInfo.name.setNum(extTagInfo.tagInfo.id);
        }

        tagValueString          = tagValues[tagValuePosition++];
        extTagInfo.tagInfo.frequency_hz    = tagValueString.toUInt(&ok);
        if (!ok) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoList: Line #%1 Value:'%2'. Unable to convert freq_hz to uint.").arg(lineCount).arg(tagValueString));
            return false;
        }

        tagValueString              = tagValues[tagValuePosition++];
        extTagInfo.tagInfo.intra_pulse1_msecs  = tagValueString.toUInt(&ok);
        if (!ok) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoList: Line #%1 Value:'%2'. Unable to convert ip_msecs_1 to uint.").arg(lineCount).arg(tagValueString));
            return false;
        }
        if (extTagInfo.tagInfo.intra_pulse1_msecs == 0) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoList: Line #%1 Value:'%2'. ip_msecs_1 value cannot be 0").arg(lineCount).arg(tagValueString));
            return false;
        }

        extTagInfo.ip_msecs_1_id = tagValues[tagValuePosition++];
        if (extTagInfo.ip_msecs_1_id.length() == 0) {
            extTagInfo.ip_msecs_1_id = "-";
        }

        tagValueString              = tagValues[tagValuePosition++];
        extTagInfo.tagInfo.intra_pulse2_msecs  = tagValueString.toUInt(&ok);
        if (!ok) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoList: Line #%1 Value:'%2'. Unable to convert ip_msecs_2 to uint.").arg(lineCount).arg(tagValueString));
            return false;
        }

        extTagInfo.ip_msecs_2_id = tagValues[tagValuePosition++];
        if (extTagInfo.ip_msecs_2_id.length() == 0) {
            extTagInfo.ip_msecs_2_id = "-";
        }

        tagValueString              = tagValues[tagValuePosition++];
        extTagInfo.tagInfo.pulse_width_msecs   = tagValueString.toUInt(&ok);
        if (!ok) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoList: Line #%1 Value:'%2'. Unable to convert pulse_width_msecs to uint.").arg(lineCount).arg(tagValueString));
            return false;
        }
        if (extTagInfo.tagInfo.pulse_width_msecs == 0) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoList: Line #%1 Value:'%2'. pulse_width_msecs value cannot be 0").arg(lineCount).arg(tagValueString));
            return false;
        }

        tagValueString                          = tagValues[tagValuePosition++];
        extTagInfo.tagInfo.intra_pulse_uncertainty_msecs   = tagValueString.toUInt(&ok);
        if (!ok) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoList: Line #%1 Value:'%2'. Unable to convert ip_uncertainty_msecs to uint.").arg(lineCount).arg(tagValueString));
            return false;
        }
        if (extTagInfo.tagInfo.intra_pulse_uncertainty_msecs == 0) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoList: Line #%1 Value:'%2'. ip_uncertainty_msecs value cannot be 0").arg(lineCount).arg(tagValueString));
            return false;
        }

        tagValueString                      = tagValues[tagValuePosition++];
        extTagInfo.tagInfo.intra_pulse_jitter_msecs    = tagValueString.toUInt(&ok);
        if (!ok) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoList: Line #%1 Value:'%2'. Unable to convert ip_jitter_msecs to uint.").arg(lineCount).arg(tagValueString));
            return false;
        }
        if (extTagInfo.tagInfo.intra_pulse_jitter_msecs == 0) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoList: Line #%1 Value:'%2'. ip_jitter_msecs value cannot be 0").arg(lineCount).arg(tagValueString));
            return false;
        }

        tagValueString  = tagValues[tagValuePosition++];
        extTagInfo.tagInfo.k       = tagValueString.toUInt(&ok);
        if (!ok) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoList: Line #%1 Value:'%2'. Unable to convert k to uint.").arg(lineCount).arg(tagValueString));
            return false;
        }
        if (extTagInfo.tagInfo.k == 0) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoList: Line #%1 Value:'%2'. k value cannot be 0").arg(lineCount).arg(tagValueString));
            return false;
        }

        tagValueString                  = tagValues[tagValuePosition++];
        extTagInfo.tagInfo.false_alarm_probability = tagValueString.toDouble(&ok);
        if (!ok) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoList: Line #%1 Value:'%2'. Unable to convert false_alarm to uint.").arg(lineCount).arg(tagValueString));
            return false;
        }
        if (extTagInfo.tagInfo.false_alarm_probability == 0) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoList: Line #%1 Value:'%2'. false_alarm value cannot be 0").arg(lineCount).arg(tagValueString));
            return false;
        }

        newTagInfoList.push_back(extTagInfo);
    }

    (*this) = newTagInfoList;

    if (!_channelizerTuner()) {
        qgcApp()->showAppMessage(QStringLiteral("TagInfoList: Unable to tune channelizer"));
        return false;
    }

    return true;
}

ExtendedTagInfo_t TagInfoList::getTagInfo(uint32_t id) 
{ 
    return *std::find_if(begin(), end(), [id](const ExtendedTagInfo_t extTagInfo) { return extTagInfo.tagInfo.id == id; });
}

// Find the best center frequency for the requested frequencies such that the there is only one frequency per channel
// bin. And the requested frequested is as close to center within the channel bin as possible. Frequencies are not
// allowed to be within a shoulder area of the channel bin. Once the best center frequency is found, the channel bin
// values are updated for each tag.
//
// The channel bin structure follows the functionality of Matlab dsp.channelizer. The channel bins are 1-based to follow
// Matlab's 1-based vector/array indexing. Channel 1 is the centered at the radio center frequency Then remainder of the 
// channels follow and wrap around back to the beginning of the bandwidth.
bool TagInfoList::_channelizerTuner()
{
    // Build the list of requested frequencies
    QVector<int> freqListHz(size());
    std::transform(begin(), end(), freqListHz.begin(), [](const ExtendedTagInfo_t& tag) { return tag.tagInfo.frequency_hz; });

    int freqMaxHz          = *std::max_element(freqListHz.begin(), freqListHz.end());
    int freqMinHz          = *std::min_element(freqListHz.begin(), freqListHz.end());

    // First make sure all the requested frequencies can fit within the available bandwidth
    if (freqMaxHz - freqMinHz > _sampleRateHz) {
        qCritical() << "Requested frequencies are too far apart to fit within the available bandwidth";
        return false;
    }

    QList<int> testCentersHz;

    // Generate the list of center channels to test against. Start with min to max
    if (freqMinHz == freqMaxHz) {
        testCentersHz.push_back(freqMaxHz);
    } else {
        int stepSizeHz     = 1000;  // FIX_ME should be 100
        int nextPossibleHz = freqMinHz;
        while (nextPossibleHz <= freqMaxHz) {
            testCentersHz.push_back(nextPossibleHz);
            nextPossibleHz += stepSizeHz;
        }
    }
    qDebug() << "testCentersHz-raw:" << testCentersHz;

#if 0
    // FIXME
    // Now filter out centers which would place requested frequencies outside the full bandwidth
    auto removeIf = std::remove_if(testCentersHz.begin(), testCentersHz.end(), 
                        [halfBwHz](uint32_t testCenterHz) {
                                return testCenterHz - halfBwHz < -halfBwHz || testCenterHz + halfBwHz > halfBwHz;
                                });
    testCentersHz.erase(removeIf, testCentersHz.end());
    qDebug() << "testCentersHz:-filtered" << testCentersHz;
#endif

    QVector <int>           channelUsageCountsForTestCenter (_nChannels);
    QList   <QList<int>>    channelBinsArray;
    QVector <QList<int>>    channelUsageCountsArray(_nChannels);
    QList   <int>           distanceFromCenterAverages;
    QList   <QList<bool>>   inShoulderArray;

    for (auto testCenterHz : testCentersHz) {
        std::for_each(channelUsageCountsForTestCenter.begin(), channelUsageCountsForTestCenter.end(), [](int& n) { n = 0; });

        QList<int>  distanceFromCenters;
        QList<bool> inShoulders;
        QList<int>  channelBins;

        int firstChannelFreqHz = _firstChannelFreqHz(testCenterHz);

        for (auto requestedFreqHz : freqListHz) {
            // Adjust the requested frequency to first channel frequency start
            int adjustedFreqHz = requestedFreqHz - firstChannelFreqHz;

            int channelBucket       = adjustedFreqHz  / (int)_channelBwHz;
            int distanceFromCenter  = (adjustedFreqHz % (int)_channelBwHz) - _halfChannelBwHz;

            double  shoulderPercent = 0.925;
            auto    halfCenterBwHz  = _halfChannelBwHz * shoulderPercent;
            bool    inShoulder      = abs(distanceFromCenter) > halfCenterBwHz;

#if 1
            qDebug() << "---- single freq build begin ---";
            qDebug() << "requestedFreqHz:" << requestedFreqHz/1000 <<
                        "testCenterHz:" << testCenterHz/1000 <<
                        "adjustedFreqHz:" << adjustedFreqHz/1000 <<
                        "distanceFromCenter:" << distanceFromCenter <<
                        "channelBucket" << channelBucket <<
                        "inShoulder:" << inShoulder;
            _printChannelMap(testCenterHz, QVector<int>({requestedFreqHz}));
            qDebug() << "---- single freq build end ---";
#endif

            distanceFromCenters.push_back(abs(distanceFromCenter));
            inShoulders.push_back(inShoulder);
            channelBins.push_back(channelBucket + 1);

            channelUsageCountsForTestCenter[channelBucket]++;
        }

        channelBinsArray.push_back(channelBins);
        inShoulderArray.push_back(inShoulders);

        // Find the average of the distances from the center of each channel
        auto averageDistanceFromCenter = std::accumulate(distanceFromCenters.begin(), distanceFromCenters.end(), 0) / distanceFromCenters.size();
        distanceFromCenterAverages.push_back(averageDistanceFromCenter);

        qDebug() << "---- test center build begin ---";
        _printChannelMap(testCenterHz, freqListHz);
        qDebug() << "averageDistanceFromCenter:" << averageDistanceFromCenter;
        qDebug() << "---- test center build end ---";

    }

    // Find the best center frequency

    int        smallestDistanceFromCenterAverage   = _channelBwHz;
    int        bestTestCenterHz                    = 0;
    QList<int> channelBins;

    for (int i = 0; i < testCentersHz.size(); i++) {
        auto testCenterHz = testCentersHz[i];

        // We can't use a center frequency if it has more than one frequency in the same channel
        if (std::any_of(channelUsageCountsForTestCenter.begin(), channelUsageCountsForTestCenter.end(), [](uint32_t n) { return n > 1; })) {
            continue;
        }

        // We can use a center frequency if any of the frequencies are in the shoulder
        if (std::any_of(inShoulderArray[i].begin(), inShoulderArray[i].end(), [](bool inShoulder) { return inShoulder; })) {
            continue;
        }

        // Keep track of the smallest average distance from center
        if (distanceFromCenterAverages[i] < smallestDistanceFromCenterAverage) {
            smallestDistanceFromCenterAverage = distanceFromCenterAverages[i];
            bestTestCenterHz = testCenterHz;
            channelBins = channelBinsArray[i];
        }
    }

    qDebug() << "bestTestCenterHz:" << bestTestCenterHz << "channelBins:" << channelBins;
    if (bestTestCenterHz != 0) {
        _printChannelMap(bestTestCenterHz, freqListHz);
        for (int i=0; i<size(); i++) {
            (*this)[i].tagInfo.channelizer_bin = channelBins[i] + 1;
        }
        _radioCenterHz = bestTestCenterHz;
        return true;
    } else {
        _radioCenterHz = 0;
        return false;
    }
}

void TagInfoList::_printChannelMap(const int centerFreq, const QVector<int>& requestedFreqsHz)
{
        qDebug() << "Frequencies:" << requestedFreqsHz;

        QString channelHeader("------    :    ------");
        int firstChannelFreqHz = _firstChannelFreqHz(centerFreq);

        QString channelBinsHeaderStr("|");
        QString channelBinsValuesStr("|");
        int nextChannelStartFreqHz = firstChannelFreqHz;
        for (int i=0; i<_nChannels; i++) {
            int channelStart   = nextChannelStartFreqHz                    / 1000;
            int channelEnd     = (nextChannelStartFreqHz + _channelBwHz)    / 1000;
            int channelCenter  = channelStart + (_halfChannelBwHz / 1000);

            channelBinsHeaderStr    += channelHeader;
            channelBinsHeaderStr    += "|";
            channelBinsValuesStr    += QStringLiteral("%1 %2:%3 %4|").arg(channelStart).arg(channelCenter/1000,3,10,QChar('0')).arg(channelCenter%1000,3,10,QChar('0')).arg(channelEnd);
            nextChannelStartFreqHz  += _channelBwHz;
        }

        QString freqPositionStr = channelBinsHeaderStr;
        
        for (auto requestedFreqHz : requestedFreqsHz) {
            double pctInBandwith = (double)(requestedFreqHz - firstChannelFreqHz) / (double)(_channelBwHz * _nChannels);

            int adjustedFreqHz  = requestedFreqHz - firstChannelFreqHz;
            int channelBucket   = adjustedFreqHz  / (int)_channelBwHz;

            // Characted count without dividers
            double cCharsInChannelPrintDisplay = channelBinsHeaderStr.length() - (_nChannels + 1);

            // Make sure to take abck into account dividers
            int    freqPositinInChannelDisplay = round(pctInBandwith * cCharsInChannelPrintDisplay) + channelBucket;

            freqPositionStr[freqPositinInChannelDisplay] = 'v';
        }

        qDebug() << channelBinsHeaderStr;
        qDebug() << channelBinsValuesStr;
        qDebug() << freqPositionStr;
        qDebug() << QString(channelBinsHeaderStr.length(), '-');
}

int TagInfoList::_firstChannelFreqHz(const int centerFreqHz)
{
    return centerFreqHz - _halfBwHz - _halfChannelBwHz;
}
