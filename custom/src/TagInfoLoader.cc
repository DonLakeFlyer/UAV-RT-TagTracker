#include "TagInfoLoader.h"
#include "QGCApplication.h"
#include "SettingsManager.h"
#include "AppSettings.h"

#include <QFile>
#include <QTextStream>

using namespace TunnelProtocol;

TagInfoLoader::TagInfoLoader(QObject* parent)
    : QObject(parent)
{

}

TagInfoLoader::~TagInfoLoader()
{

}

bool TagInfoLoader::loadTags(void)
{
    _tagInfoMap.clear();

    TagInfoMap_t newTagInfoMap;

    QString tagFilename = QString::asprintf("%s/TagInfo.txt",
                                            qgcApp()->toolbox()->settingsManager()->appSettings()->parameterSavePath().toStdString().c_str());
    QFile   tagFile(tagFilename);

    if (!tagFile.open(QIODevice::ReadOnly)) {
        qgcApp()->showAppMessage(QStringLiteral("TagInfoLoader: Unable to open tag file: %1").arg(tagFilename));
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
            qgcApp()->showAppMessage(QStringLiteral("TagInfoLoader: Line #%1 Does not contain %2 values").arg(lineCount).arg(valueCount));
            return false;
        }

        bool                ok;
        ExtendedTagInfo_t   extTagInfo;
        QString             tagValueString;
        int                 tagValuePosition = 0;

        extTagInfo.tagInfo.header.command = COMMAND_ID_TAG;

        // id, freq_hz, ip_msecs, pulse_width_msecs, ip_uncertainty_msecs, ip_jitter_msecs, k, false_alarm

        tagValueString          = tagValues[tagValuePosition++];
        extTagInfo.tagInfo.id   = tagValueString.toUInt(&ok);
        if (!ok) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoLoader: Line #%1 Value:'%2'. Unable to convert id to uint.").arg(lineCount).arg(tagValueString));
            return false;
        }
        if (extTagInfo.tagInfo.id <= 1) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoLoader: Line #%1 Value:'%2'. Tag ids must be greater than 1").arg(lineCount).arg(tagValueString));
            return false;
        }
        if (extTagInfo.tagInfo.id % 2) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoLoader: Line #%1 Value:'%2'. Tag ids must be even numbers").arg(lineCount).arg(tagValueString));
            return false;
        }

        extTagInfo.name = tagValues[tagValuePosition++];
        if (extTagInfo.name.length() == 0) {
            extTagInfo.name.setNum(extTagInfo.tagInfo.id);
        }

        tagValueString          = tagValues[tagValuePosition++];
        extTagInfo.tagInfo.frequency_hz    = tagValueString.toUInt(&ok);
        if (!ok) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoLoader: Line #%1 Value:'%2'. Unable to convert freq_hz to uint.").arg(lineCount).arg(tagValueString));
            return false;
        }

        tagValueString              = tagValues[tagValuePosition++];
        extTagInfo.tagInfo.intra_pulse1_msecs  = tagValueString.toUInt(&ok);
        if (!ok) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoLoader: Line #%1 Value:'%2'. Unable to convert ip_msecs_1 to uint.").arg(lineCount).arg(tagValueString));
            return false;
        }
        if (extTagInfo.tagInfo.intra_pulse1_msecs == 0) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoLoader: Line #%1 Value:'%2'. ip_msecs_1 value cannot be 0").arg(lineCount).arg(tagValueString));
            return false;
        }

        extTagInfo.ip_msecs_1_id = tagValues[tagValuePosition++];
        if (extTagInfo.ip_msecs_1_id.length() == 0) {
            extTagInfo.ip_msecs_1_id = "-";
        }

        tagValueString              = tagValues[tagValuePosition++];
        extTagInfo.tagInfo.intra_pulse2_msecs  = tagValueString.toUInt(&ok);
        if (!ok) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoLoader: Line #%1 Value:'%2'. Unable to convert ip_msecs_2 to uint.").arg(lineCount).arg(tagValueString));
            return false;
        }

        extTagInfo.ip_msecs_2_id = tagValues[tagValuePosition++];
        if (extTagInfo.ip_msecs_2_id.length() == 0) {
            extTagInfo.ip_msecs_2_id = "-";
        }

        tagValueString              = tagValues[tagValuePosition++];
        extTagInfo.tagInfo.pulse_width_msecs   = tagValueString.toUInt(&ok);
        if (!ok) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoLoader: Line #%1 Value:'%2'. Unable to convert pulse_width_msecs to uint.").arg(lineCount).arg(tagValueString));
            return false;
        }
        if (extTagInfo.tagInfo.pulse_width_msecs == 0) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoLoader: Line #%1 Value:'%2'. pulse_width_msecs value cannot be 0").arg(lineCount).arg(tagValueString));
            return false;
        }

        tagValueString                          = tagValues[tagValuePosition++];
        extTagInfo.tagInfo.intra_pulse_uncertainty_msecs   = tagValueString.toUInt(&ok);
        if (!ok) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoLoader: Line #%1 Value:'%2'. Unable to convert ip_uncertainty_msecs to uint.").arg(lineCount).arg(tagValueString));
            return false;
        }
        if (extTagInfo.tagInfo.intra_pulse_uncertainty_msecs == 0) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoLoader: Line #%1 Value:'%2'. ip_uncertainty_msecs value cannot be 0").arg(lineCount).arg(tagValueString));
            return false;
        }

        tagValueString                      = tagValues[tagValuePosition++];
        extTagInfo.tagInfo.intra_pulse_jitter_msecs    = tagValueString.toUInt(&ok);
        if (!ok) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoLoader: Line #%1 Value:'%2'. Unable to convert ip_jitter_msecs to uint.").arg(lineCount).arg(tagValueString));
            return false;
        }
        if (extTagInfo.tagInfo.intra_pulse_jitter_msecs == 0) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoLoader: Line #%1 Value:'%2'. ip_jitter_msecs value cannot be 0").arg(lineCount).arg(tagValueString));
            return false;
        }

        tagValueString  = tagValues[tagValuePosition++];
        extTagInfo.tagInfo.k       = tagValueString.toUInt(&ok);
        if (!ok) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoLoader: Line #%1 Value:'%2'. Unable to convert k to uint.").arg(lineCount).arg(tagValueString));
            return false;
        }
        if (extTagInfo.tagInfo.k == 0) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoLoader: Line #%1 Value:'%2'. k value cannot be 0").arg(lineCount).arg(tagValueString));
            return false;
        }

        tagValueString                  = tagValues[tagValuePosition++];
        extTagInfo.tagInfo.false_alarm_probability = tagValueString.toDouble(&ok);
        if (!ok) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoLoader: Line #%1 Value:'%2'. Unable to convert false_alarm to uint.").arg(lineCount).arg(tagValueString));
            return false;
        }
        if (extTagInfo.tagInfo.false_alarm_probability == 0) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoLoader: Line #%1 Value:'%2'. false_alarm value cannot be 0").arg(lineCount).arg(tagValueString));
            return false;
        }

        newTagInfoMap[extTagInfo.tagInfo.id] = extTagInfo;
    }

    _tagInfoMap = newTagInfoMap;

    return true;
}

TagInfoLoader::TagList_t TagInfoLoader::getTagList(void)
{
    TagList_t tagList;

    TagInfoMap_t::const_iterator iter = _tagInfoMap.constBegin();
    while (iter != _tagInfoMap.constEnd()) {
        tagList.append(iter.value());
        ++iter;
    }

    return tagList;
}
