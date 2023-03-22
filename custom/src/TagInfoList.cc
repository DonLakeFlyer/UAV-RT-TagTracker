#include "TagInfoList.h"
#include "QGCApplication.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "CustomPlugin.h"
#include "CustomSettings.h"

#include "thresholdGeneratorPre.h"
#include "thresholdGeneratorPost.h"
#include "thresholdGeneratorSingleTrial.h"

#include <QFile>
#include <QTextStream>

using namespace TunnelProtocol;

TagInfoList::TagInfoList()
    : _channelBucketCenters(_nChannels)
{

}

void TagInfoList::checkForTagFile (void)
{
    QString tagFilename = _tagInfoFilePath();
    QFile   tagFile(tagFilename);

    if (!tagFile.open(QIODevice::ReadOnly)) {
        qgcApp()->showAppMessage(QStringLiteral("TagInfo.txt does not exist. Creating default file at: %1").arg(tagFilename));
        if (!QFile::copy(":/res/TagInfo.txt", tagFilename)) {
            qgcApp()->showAppMessage(QStringLiteral("Unable to create default TagIndo.txt file"));
        }
    }
}

bool TagInfoList::loadTags(void)
{
    clear();

    TagInfoList newTagInfoList;

    QString tagFilename = QString::asprintf("%s/TagInfo.txt",
                                            qgcApp()->toolbox()->settingsManager()->appSettings()->parameterSavePath().toStdString().c_str());
    QFile   tagFile(tagFilename);

    if (!tagFile.open(QIODevice::ReadOnly)) {
        qgcApp()->showAppMessage(QStringLiteral("Unable to open TagInfo.txt at: %1").arg(tagFilename));
        return false;
    }

    QString         tagLine;
    QTextStream     tagStream(&tagFile);
    uint32_t        lineCount               = 0;
    QString         lineFormat              = "id, name, freq_hz, ip_msecs_1, ip_msecs_1_id, ip_msecs_2, ip_msecs_2_id, pulse_width_msecs, ip_uncertainty_msecs, ip_jitter_msecs";
    CustomSettings* customSettings          = qobject_cast<CustomPlugin*>(qgcApp()->toolbox()->corePlugin())->customSettings();
    uint32_t        k                       = customSettings->k()->rawValue().toUInt();
    double          falseAlarmProbability   = customSettings->falseAlarmProbability()->rawValue().toDouble() / 100.0;

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

        memset(&extTagInfo.tagInfo, 0, sizeof(extTagInfo.tagInfo));

        extTagInfo.tagInfo.header.command = COMMAND_ID_TAG;

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

        extTagInfo.tagInfo.k = k;
        extTagInfo.tagInfo.false_alarm_probability = falseAlarmProbability;

        newTagInfoList.push_back(extTagInfo);
    }

    (*this) = newTagInfoList;

    if (!_channelizerTuner()) {
        qgcApp()->showAppMessage(QStringLiteral("TagInfoList: Unable to tune channelizer"));
        return false;
    }

    //_generateThresholds();

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
    QVector<uint32_t> freqListHz(size());
    std::transform(begin(), end(), freqListHz.begin(), [](const ExtendedTagInfo_t& tag) { return tag.tagInfo.frequency_hz; });

    uint32_t freqMaxHz = *std::max_element(freqListHz.begin(), freqListHz.end());
    uint32_t freqMinHz = *std::min_element(freqListHz.begin(), freqListHz.end());

    // First make sure all the requested frequencies can fit within the available bandwidth
    if (freqMaxHz - freqMinHz > _sampleRateHz) {
        qCritical() << "Requested frequencies are too far apart to fit within the available bandwidth";
        return false;
    }

    QList<uint32_t> testCentersHz;

    // Generate the list of center channels to test against. Start with min to max
    if (freqMinHz == freqMaxHz) {
        testCentersHz.push_back(freqMaxHz);
    } else {
        uint32_t stepSizeHz     = 100;
        uint32_t nextPossibleHz = freqMinHz;
        while (nextPossibleHz <= freqMaxHz) {
            testCentersHz.push_back(nextPossibleHz);
            nextPossibleHz += stepSizeHz;
        }
    }
    //qDebug() << "testCentersHz-raw:" << testCentersHz;

    // Remove any test centers for which a requested frequency would fall outside the full bandwidth
    for (auto testCenterHzIter = testCentersHz.begin(); testCenterHzIter != testCentersHz.end(); ) {
        if (freqMinHz < *testCenterHzIter - _halfBwHz || freqMaxHz > *testCenterHzIter + _halfBwHz) {
            testCenterHzIter = testCentersHz.erase(testCenterHzIter);
        } else {
            ++testCenterHzIter;
        }
    }
    //qDebug() << "testCentersHz-filtered:" << testCentersHz;

    QVector <uint>          channelBucketUsageCountsForTestCenter (_nChannels);
    QList   <QList<uint>>   oneBasedChannelBucketsArray;
    QList   <QVector<uint>> channelBucketUsageCountsArray;
    QList   <uint32_t>      distanceFromCenterAverages;
    QList   <QList<bool>>   inShoulderArray;

    for (auto testCenterHz : testCentersHz) {
        channelBucketUsageCountsForTestCenter.fill(0);

        QList<uint32_t>  distanceFromCenters;
        QList<bool>     inShoulders;
        QList<uint>     oneBasedChannelBuckets;

        // If the requested frequency is below the center frequency bucket, we add the sample rate to it.
        // The simplifies channel bucket calculations.
        auto wrappedRequestedFreqsHz = freqListHz;
        std::transform(freqListHz.begin(), freqListHz.end(), wrappedRequestedFreqsHz.begin(),
            [testCenterHz](uint32_t freqHz) {
                if (freqHz >= testCenterHz - _halfChannelBwHz) {
                    return freqHz;
                } else {
                    return freqHz + _sampleRateHz;
                }
            });

#if 0
        qDebug() << "freqListHz:" << freqListHz << "wrappedRequestedFreqsHz:" << wrappedRequestedFreqsHz;
#endif

        for (auto wrappedFreqHz : wrappedRequestedFreqsHz) {
            uint32_t adjustedFreqHz         = wrappedFreqHz - ((int)testCenterHz - (int)_halfChannelBwHz); // Adjust to zero-based within total bandwidth
            uint32_t zeroBasedChannelBucket = adjustedFreqHz  / _channelBwHz;
            uint32_t distanceFromCenter     = abs((int)(adjustedFreqHz % _channelBwHz) - (int)_halfChannelBwHz);

            double  shoulderPercent = 0.85; // Shoulder is 15% from channel edge
            bool    inShoulder      = distanceFromCenter > _halfChannelBwHz * shoulderPercent;

#if 0
            qDebug() << "---- single freq build begin ---";
            qDebug() << "wrappedFreqHz:" << wrappedFreqHz <<
                        "testCenterHz:" << testCenterHz <<
                        "adjustedFreqHz:" << adjustedFreqHz <<
                        "distanceFromCenter:" << distanceFromCenter <<
                        "zeroBasedChannelBucket" << zeroBasedChannelBucket <<
                        "inShoulder:" << inShoulder;
            _printChannelMap(testCenterHz, QVector<uint32_t>({wrappedFreqHz}));
            qDebug() << "---- single freq build end ---";
#endif

            distanceFromCenters.push_back(distanceFromCenter);
            inShoulders.push_back(inShoulder);
            oneBasedChannelBuckets.push_back(zeroBasedChannelBucket + 1);

            channelBucketUsageCountsForTestCenter[zeroBasedChannelBucket]++;
        }

        oneBasedChannelBucketsArray.push_back(oneBasedChannelBuckets);
        channelBucketUsageCountsArray.push_back(channelBucketUsageCountsForTestCenter);
        inShoulderArray.push_back(inShoulders);

        // Find the average of the distances from the center of each channel
        auto averageDistanceFromCenter = std::accumulate(distanceFromCenters.begin(), distanceFromCenters.end(), 0) / distanceFromCenters.size();
        distanceFromCenterAverages.push_back(averageDistanceFromCenter);

#if 0
        qDebug() << "---- test center build begin ---";
        qDebug() << "testCenterHz" << testCenterHz;
        _printChannelMap(testCenterHz, wrappedRequestedFreqsHz);
        qDebug() << "averageDistanceFromCenter:" << averageDistanceFromCenter;
        qDebug() << "---- test center build end ---";
#endif
    }

    // Find the best center frequency

    uint32_t    smallestDistanceFromCenterAverage   = _channelBwHz;
    uint32_t    bestTestCenterHz                    = 0;
    QList<uint> oneBasedChannelBuckets;

    for (int i = 0; i < testCentersHz.size(); i++) {
        auto testCenterHz = testCentersHz[i];

        // We can't use a center frequency if it has more than one frequency in the same channel
        if (std::any_of(channelBucketUsageCountsForTestCenter.begin(), channelBucketUsageCountsForTestCenter.end(), [](uint32_t n) { return n > 1; })) {
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
            oneBasedChannelBuckets = oneBasedChannelBucketsArray[i];
        }
    }

    qDebug() << "bestTestCenterHz:" << bestTestCenterHz
             << "smallestDistanceFromCenterAverage" << smallestDistanceFromCenterAverage
             << "oneBasedChannelBuckets:" << oneBasedChannelBuckets;
    if (bestTestCenterHz != 0) {
        //_printChannelMap(bestTestCenterHz, freqListHz);

        // Generate the channel centers

        QList<uint32_t> channelCentersHz;
        uint32_t        nextCenterHz = bestTestCenterHz;
        for (uint i=0; i<_nChannels; i++) {
            if (i > _nChannels / 2) {
                // Deal with odd matlab channel wrap-around
                channelCentersHz.append(nextCenterHz - _fullBwHz);
            } else {
                channelCentersHz.append(nextCenterHz);
            }
            nextCenterHz += _channelBwHz;
        }
        qDebug() << "channelCentersHz" << channelCentersHz;

        // Add the channel bucket numbers to the tagInfo
        for (int i=0; i<size(); i++) {
            (*this)[i].tagInfo.channelizer_channel_number               = oneBasedChannelBuckets[i];
            (*this)[i].tagInfo.channelizer_channel_center_frequency_hz  = channelCentersHz[oneBasedChannelBuckets[i] - 1];
            qDebug() << i
                    << (*this)[i].tagInfo.channelizer_channel_number
                     << (*this)[i].tagInfo.channelizer_channel_center_frequency_hz;
        }


        _radioCenterHz = bestTestCenterHz;
        return true;
    } else {
        _radioCenterHz = 0;
        return false;
    }
}

void TagInfoList::_printChannelMap(const uint32_t centerFreqHz, const QVector<uint32_t>& wrappedRequestedFreqsHz)
{
        qDebug() << "Target frequencies:" << wrappedRequestedFreqsHz;

        QString channelHeader("------    :    ------");
        uint32_t firstChannelFreqHz = centerFreqHz - _halfChannelBwHz;

        QString zeroBasedChannelBucketsHeaderStr("|");
        QString zeroBasedChannelBucketsValuesStr("|");
        int nextChannelStartFreqHz = firstChannelFreqHz;
        for (uint i=0; i<_nChannels; i++) {
            int channelStart   = nextChannelStartFreqHz                    / 1000;
            int channelEnd     = (nextChannelStartFreqHz + _channelBwHz)    / 1000;
            int channelCenter  = channelStart + (_halfChannelBwHz / 1000);

            zeroBasedChannelBucketsHeaderStr    += channelHeader;
            zeroBasedChannelBucketsHeaderStr    += "|";
            zeroBasedChannelBucketsValuesStr    += QStringLiteral("%1 %2:%3 %4|").arg(channelStart).arg(channelCenter/1000,3,10,QChar('0')).arg(channelCenter%1000,3,10,QChar('0')).arg(channelEnd);
            nextChannelStartFreqHz  += _channelBwHz;
        }

        QString freqPositionStr = zeroBasedChannelBucketsHeaderStr;

        for (auto wrappedRequestedFreqHz : wrappedRequestedFreqsHz) {
            double pctInBandwith = (double)(wrappedRequestedFreqHz - firstChannelFreqHz) / (double)(_channelBwHz * _nChannels);

            uint32_t    adjustedFreqHz  = wrappedRequestedFreqHz - firstChannelFreqHz;
            uint        channelBucket   = adjustedFreqHz  / _channelBwHz;

            // Character count without dividers
            double cCharsInChannelPrintDisplay = zeroBasedChannelBucketsHeaderStr.length() - (_nChannels + 1);

            // Make sure to take back into account dividers
            int    freqPositinInChannelDisplay = round(pctInBandwith * cCharsInChannelPrintDisplay) + channelBucket;

            freqPositionStr[freqPositinInChannelDisplay] = 'v';
        }

        qDebug() << zeroBasedChannelBucketsHeaderStr;
        qDebug() << zeroBasedChannelBucketsValuesStr;
        qDebug() << freqPositionStr;
        qDebug() << QString(zeroBasedChannelBucketsHeaderStr.length(), '-');
}

int TagInfoList::_firstChannelFreqHz(const int centerFreqHz)
{
    return centerFreqHz - _halfBwHz - _halfChannelBwHz;
}

QString TagInfoList::_tagInfoFilePath()
{
    return QString::asprintf("%s/TagInfo.txt",
                             qgcApp()->toolbox()->settingsManager()->appSettings()->parameterSavePath().toStdString().c_str());

}

bool TagInfoList::_generateThresholds()
{
    int                         trials = 100;
    coder::array<creal_T, 2>    W;
    coder::array<creal_T, 3>    Ssynth;
    coder::sparse               Wq;
    boolean_T                   success;
    coder::array<double, 1>     scores;
    double                      mu;
    double                      sigma;

    scores.set_size(trials);

    thresholdGeneratorPre(3750.0, 0.15, 2.0, 0.02, 0.06, 3, W, Ssynth, &Wq, &success);
    for (int i=1; i<trials+1; i++) {
        scores[i] = thresholdGeneratorSingleTrial(W, Ssynth, &Wq, i);
    }
    thresholdGeneratorPost(scores, &mu, &sigma);

    qDebug() << "mu:" << mu << "sigma:" << sigma;

    return true;
}
