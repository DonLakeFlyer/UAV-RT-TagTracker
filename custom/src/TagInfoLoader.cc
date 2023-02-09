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
    tagList.clear();
    QList<TagInfo_t> newTagList;

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
    while (tagStream.readLineInto(&tagLine)) {
        lineCount++;
        if (tagLine.startsWith("#")) {
            continue;
        }

        int         valueCount = 8;
        QStringList tagValues = tagLine.split(",");
        if (tagValues.count() != valueCount) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoLoader: Line #%1 Does not contain %2 values").arg(lineCount).arg(valueCount));
            return false;
        }

        bool        ok;
        TagInfo_t   tagInfo;
        QString     tagValueString;
        int         tagValuePosition = 0;

        tagInfo.header.command = COMMAND_ID_TAG;

        // id, freq_hz, ip_msecs, pulse_width_msecs, ip_uncertainty_msecs, ip_jitter_msecs, k, false_alarm

        tagValueString  = tagValues[tagValuePosition++];
        tagInfo.id      = tagValueString.toUInt(&ok);
        if (!ok) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoLoader: Line #%1 Value:'%2'. Unable to convert id to uint.").arg(lineCount).arg(tagValueString));
            return false;
        }
        if (tagInfo.id <= 1) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoLoader: Line #%1 Value:'%2'. Tag ids must be greater than 1").arg(lineCount).arg(tagValueString));
            return false;
        }

        tagValueString          = tagValues[tagValuePosition++];
        tagInfo.frequency_hz    = tagValueString.toUInt(&ok);
        if (!ok) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoLoader: Line #%1 Value:'%2'. Unable to convert freq_hz to uint.").arg(lineCount).arg(tagValueString));
            return false;
        }

        tagValueString              = tagValues[tagValuePosition++];
        tagInfo.intra_pulse1_msecs  = tagValueString.toUInt(&ok);
        if (!ok) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoLoader: Line #%1 Value:'%2'. Unable to convert ip_msecs to uint.").arg(lineCount).arg(tagValueString));
            return false;
        }
        if (tagInfo.intra_pulse1_msecs == 0) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoLoader: Line #%1 Value:'%2'. ip_msecs value cannot be 0").arg(lineCount).arg(tagValueString));
            return false;
        }

        tagValueString              = tagValues[tagValuePosition++];
        tagInfo.intra_pulse1_msecs  = tagValueString.toUInt(&ok);
        if (!ok) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoLoader: Line #%1 Value:'%2'. Unable to convert pulse_width_msecs to uint.").arg(lineCount).arg(tagValueString));
            return false;
        }
        if (tagInfo.intra_pulse1_msecs == 0) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoLoader: Line #%1 Value:'%2'. ip_msecs value cannot be 0").arg(lineCount).arg(tagValueString));
            return false;
        }

        tagValueString                          = tagValues[tagValuePosition++];
        tagInfo.intra_pulse_uncertainty_msecs   = tagValueString.toUInt(&ok);
        if (!ok) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoLoader: Line #%1 Value:'%2'. Unable to convert ip_uncertainty_msecs to uint.").arg(lineCount).arg(tagValueString));
            return false;
        }
        if (tagInfo.intra_pulse_uncertainty_msecs == 0) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoLoader: Line #%1 Value:'%2'. ip_uncertainty_msecs value cannot be 0").arg(lineCount).arg(tagValueString));
            return false;
        }

        tagValueString                      = tagValues[tagValuePosition++];
        tagInfo.intra_pulse_jitter_msecs    = tagValueString.toUInt(&ok);
        if (!ok) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoLoader: Line #%1 Value:'%2'. Unable to convert ip_jitter_msecs to uint.").arg(lineCount).arg(tagValueString));
            return false;
        }
        if (tagInfo.intra_pulse_jitter_msecs == 0) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoLoader: Line #%1 Value:'%2'. ip_jitter_msecs value cannot be 0").arg(lineCount).arg(tagValueString));
            return false;
        }

        tagValueString  = tagValues[tagValuePosition++];
        tagInfo.k       = tagValueString.toUInt(&ok);
        if (!ok) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoLoader: Line #%1 Value:'%2'. Unable to convert k to uint.").arg(lineCount).arg(tagValueString));
            return false;
        }
        if (tagInfo.k == 0) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoLoader: Line #%1 Value:'%2'. k value cannot be 0").arg(lineCount).arg(tagValueString));
            return false;
        }

        tagValueString                  = tagValues[tagValuePosition++];
        tagInfo.false_alarm_probability = tagValueString.toDouble(&ok);
        if (!ok) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoLoader: Line #%1 Value:'%2'. Unable to convert false_alarm to uint.").arg(lineCount).arg(tagValueString));
            return false;
        }
        if (tagInfo.false_alarm_probability == 0) {
            qgcApp()->showAppMessage(QStringLiteral("TagInfoLoader: Line #%1 Value:'%2'. false_alarm value cannot be 0").arg(lineCount).arg(tagValueString));
            return false;
        }
    }

    tagList = newTagList;

    return true;
}
