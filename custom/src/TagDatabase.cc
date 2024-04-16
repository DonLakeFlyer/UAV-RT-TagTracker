#include "TagDatabase.h"
#include "FactMetaData.h"
#include "Fact.h"
#include "QmlObjectListModel.h"
#include "QGCApplication.h"
#include "AppSettings.h"
#include "SettingsManager.h"

#include <QFile>

//#define DEBUG_TUNER

TagInfo::TagInfo(TagDatabase* parent)
{
    _initCommon(parent);

    uint32_t manufacturerId = 1;
    if (_parent->_tagManufacturerListModel->count() > 0) {
        TagManufacturer* tagManufacturer = _parent->_tagManufacturerListModel->value<TagManufacturer*>(0);
        manufacturerId = tagManufacturer->_idFact->rawValue().toUInt();
    } else {
        qWarning() << "TagInfo::TagInfo(default) - Internal error: no TagManufacturer objects found.";
    }
    _init(
        false,                      // selected
        _parent->_nextTagInfoId(),
        _nextTagName(),
        manufacturerId,
        _tagInfoMetaData["freq_hz"]->rawDefaultValue().toUInt());
}

TagInfo::TagInfo(bool selected, uint32_t id, QString& name, uint32_t manufacturerId, uint32_t frequencyHz, TagDatabase* parent)
{
    _initCommon(parent);
    _init(selected, id, name, manufacturerId, frequencyHz);
}

QString TagInfo::_nextTagName()
{
    QString nextNameFmt("Tag %1");

    int nextIndex = 1;
    QString nextName = nextNameFmt.arg(nextIndex);
    while (true) {
        bool duplicate = false;
        for (int i=0; i<_parent->_tagInfoListModel->count(); i++) {
            TagInfo* tagInfo = _parent->_tagInfoListModel->value<TagInfo*>(i);
            if (tagInfo->_nameFact->rawValue().toString() == nextName) {
                duplicate = true;
                break;
            }
        }
        if (duplicate) {
            nextIndex++;
            nextName = nextNameFmt.arg(nextIndex);
        } else {
            return nextName;
        }
    }
}

void TagInfo::_initCommon(TagDatabase* parent)
{
    _parent = parent;

    if (_tagInfoMetaData.isEmpty()) {
        _tagInfoMetaData = FactMetaData::createMapFromJsonFile(":/json/TagInfo.json", this);
    }
}

void TagInfo::_init(bool selected, uint32_t id, const QString& name, uint32_t manufacturerId, uint32_t frequencyHz)
{
    FactMetaData* selectedMetaData          = _tagInfoMetaData["selected"];
    FactMetaData* idMetaData                = _tagInfoMetaData["id"];
    FactMetaData* nameMetaData              = _tagInfoMetaData["name"];
    FactMetaData* manufacturerIdMetaData    = _tagInfoMetaData["manufacturerId"];
    FactMetaData* frequencyHzMetaData       = _tagInfoMetaData["freq_hz"];

    _selectedFact       = new Fact(-1, selectedMetaData->name(),        selectedMetaData->type(),       this);
    _idFact             = new Fact(-1, idMetaData->name(),              idMetaData->type(),             this);
    _nameFact           = new Fact(-1, nameMetaData->name(),            nameMetaData->type(),           this);
    _manufacturerIdFact = new Fact(-1, manufacturerIdMetaData->name(),  manufacturerIdMetaData->type(), this);
    _frequencyHzFact    = new Fact(-1, frequencyHzMetaData->name(),     frequencyHzMetaData->type(),    this);

    _selectedFact->setMetaData(selectedMetaData, false /* setDefaultFromMetaData */);
    _idFact->setMetaData(idMetaData, false /* setDefaultFromMetaData */);
    _nameFact->setMetaData(nameMetaData, false /* setDefaultFromMetaData */);
    _manufacturerIdFact->setMetaData(manufacturerIdMetaData, false /* setDefaultFromMetaData */);
    _frequencyHzFact->setMetaData(frequencyHzMetaData, false /* setDefaultFromMetaData */);

    _selectedFact->setRawValue(selected);
    _idFact->setRawValue(id);
    _nameFact->setRawValue(name);
    _manufacturerIdFact->setRawValue(manufacturerId);
    _frequencyHzFact->setRawValue(frequencyHz);
}

TagManufacturer::TagManufacturer(TagDatabase* parent)
{
    _initCommon(parent);
    _init(
        _parent->_nextTagManufacturerId(),
        _nextManufacturerName(),
        _tagManufacturerMetaData["ip_msecs_1"]->rawDefaultValue().toUInt(),
        _tagManufacturerMetaData["ip_msecs_2"]->rawDefaultValue().toUInt(),
        _tagManufacturerMetaData["ip_msecs_1_id"]->rawDefaultValue().toString(),
        _tagManufacturerMetaData["ip_msecs_2_id"]->rawDefaultValue().toString(),
        _tagManufacturerMetaData["pulse_width_msecs"]->rawDefaultValue().toUInt(),
        _tagManufacturerMetaData["ip_uncertainty_msecs"]->rawDefaultValue().toUInt(),
        _tagManufacturerMetaData["ip_jitter_msecs"]->rawDefaultValue().toUInt());
}

TagManufacturer::TagManufacturer(
                    uint32_t        id,
                    const QString&  name,
                    uint32_t        ip_msecs_1,
                    uint32_t        ip_msecs_2,
                    const QString&  ip_msecs_1_id,
                    const QString&  ip_msecs_2_id,
                    uint32_t        pulse_width_msecs,
                    uint32_t        ip_uncertainty_msecs,
                    uint32_t        ip_jitter_msecs,
                    TagDatabase*    parent)
{
    _initCommon(parent);
    _init(id, name, ip_msecs_1, ip_msecs_2, ip_msecs_1_id, ip_msecs_2_id, pulse_width_msecs, ip_uncertainty_msecs, ip_jitter_msecs);
}

QString TagManufacturer::_nextManufacturerName()
{
    QString nextNameFmt("Manufacturer %1");

    int nextIndex = 1;
    QString nextName = nextNameFmt.arg(nextIndex);
    while (true) {
        bool duplicate = false;
        for (int i=0; i<_parent->_tagManufacturerListModel->count(); i++) {
            TagManufacturer* tagManufacturer = _parent->_tagManufacturerListModel->value<TagManufacturer*>(i);
            if (tagManufacturer->_nameFact->rawValue().toString() == nextName) {
                duplicate = true;
                break;
            }
        }
        if (duplicate) {
            nextIndex++;
            nextName = nextNameFmt.arg(nextIndex);
        } else {
            return nextName;
        }
    }
}

void TagManufacturer::_initCommon(TagDatabase* parent)
{
    _parent = parent;

    if (_tagManufacturerMetaData.isEmpty()) {
        _tagManufacturerMetaData = FactMetaData::createMapFromJsonFile(":/json/TagManufacturer.json", this);
    }
}

void TagManufacturer::_init(
                uint32_t        id,
                const QString&  name,
                uint32_t        ip_msecs_1,
                uint32_t        ip_msecs_2,
                const QString&  ip_msecs_1_id,
                const QString&  ip_msecs_2_id,
                uint32_t        pulse_width_msecs,
                uint32_t        ip_uncertainty_msecs,
                uint32_t        ip_jitter_msecs)
{
    FactMetaData* idMetaData                    = _tagManufacturerMetaData["id"];
    FactMetaData* nameMetaData                  = _tagManufacturerMetaData["name"];
    FactMetaData* ip_msecs_1MetaData            = _tagManufacturerMetaData["ip_msecs_1"];
    FactMetaData* ip_msecs_2MetaData            = _tagManufacturerMetaData["ip_msecs_2"];
    FactMetaData* ip_msecs_1_idMetaData         = _tagManufacturerMetaData["ip_msecs_1_id"];
    FactMetaData* ip_msecs_2_idMetaData         = _tagManufacturerMetaData["ip_msecs_2_id"];
    FactMetaData* pulse_width_msecsMetaData     = _tagManufacturerMetaData["pulse_width_msecs"];
    FactMetaData* ip_uncertainty_msecsMetaData  = _tagManufacturerMetaData["ip_uncertainty_msecs"];
    FactMetaData* ip_jitter_msecsMetaData       = _tagManufacturerMetaData["ip_jitter_msecs"];

    _idFact                     = new Fact(-1, idMetaData->name(),                      idMetaData->type(),                     this);
    _nameFact                   = new Fact(-1, nameMetaData->name(),                    nameMetaData->type(),                   this);
    _ip_msecs_1Fact             = new Fact(-1, ip_msecs_1MetaData->name(),              ip_msecs_1MetaData->type(),             this);
    _ip_msecs_2Fact             = new Fact(-1, ip_msecs_2MetaData->name(),              ip_msecs_2MetaData->type(),             this);
    _ip_msecs_1_idFact          = new Fact(-1, ip_msecs_1_idMetaData->name(),           ip_msecs_1_idMetaData->type(),          this);
    _ip_msecs_2_idFact          = new Fact(-1, ip_msecs_2_idMetaData->name(),           ip_msecs_2_idMetaData->type(),          this);
    _pulse_width_msecsFact      = new Fact(-1, pulse_width_msecsMetaData->name(),       pulse_width_msecsMetaData->type(),      this);
    _ip_uncertainty_msecsFact   = new Fact(-1, ip_uncertainty_msecsMetaData->name(),    ip_uncertainty_msecsMetaData->type(),   this);
    _ip_jitter_msecsFact        = new Fact(-1, ip_jitter_msecsMetaData->name(),         ip_jitter_msecsMetaData->type(),        this);

    _idFact->setMetaData(idMetaData, false /* setDefaultFromMetaData */);
    _nameFact->setMetaData(nameMetaData, false /* setDefaultFromMetaData */);
    _ip_msecs_1Fact->setMetaData(ip_msecs_1MetaData, false /* setDefaultFromMetaData */);
    _ip_msecs_2Fact->setMetaData(ip_msecs_2MetaData, false /* setDefaultFromMetaData */);
    _ip_msecs_1_idFact->setMetaData(ip_msecs_1_idMetaData, false /* setDefaultFromMetaData */);
    _ip_msecs_2_idFact->setMetaData(ip_msecs_2_idMetaData, false /* setDefaultFromMetaData */);
    _pulse_width_msecsFact->setMetaData(pulse_width_msecsMetaData, false /* setDefaultFromMetaData */);
    _ip_uncertainty_msecsFact->setMetaData(ip_uncertainty_msecsMetaData, false /* setDefaultFromMetaData */);
    _ip_jitter_msecsFact->setMetaData(ip_jitter_msecsMetaData, false /* setDefaultFromMetaData */);

    _idFact->setRawValue(id);
    _nameFact->setRawValue(name);
    _ip_msecs_1Fact->setRawValue(ip_msecs_1);
    _ip_msecs_2Fact->setRawValue(ip_msecs_2);
    _ip_msecs_1_idFact->setRawValue(ip_msecs_1_id);
    _ip_msecs_2_idFact->setRawValue(ip_msecs_2_id);
    _pulse_width_msecsFact->setRawValue(pulse_width_msecs);
    _ip_uncertainty_msecsFact->setRawValue(ip_uncertainty_msecs);
    _ip_jitter_msecsFact->setRawValue(ip_jitter_msecs);
}

TagDatabase::TagDatabase(QObject* parent)
    : QObject                   (parent)
    , _tagInfoListModel         (new QmlObjectListModel(this))
    , _tagManufacturerListModel (new QmlObjectListModel(this))
{
    _load();
}

TagDatabase::~TagDatabase()
{
    _tagInfoListModel->clearAndDeleteContents();
    _tagManufacturerListModel->clearAndDeleteContents();
}

QObject* TagDatabase::newTagInfo()
{
    TagInfo* tagInfo = new TagInfo(this);
    _tagInfoListModel->append(tagInfo);
    return tagInfo;
}

QObject* TagDatabase::newTagManufacturer()
{
    TagManufacturer* tagManufacturer = new TagManufacturer(this);
    _tagManufacturerListModel->append(tagManufacturer);
    return tagManufacturer;
}

void TagDatabase::deleteTagInfoListItem(QObject* tagInfoListItem)
{
    delete _tagInfoListModel->removeOne(tagInfoListItem);
}

bool TagDatabase::deleteTagManufacturerListItem(QObject* tagManufacturerListItem)
{
    // Check that the manufacturer is not in use by any tag info
    for (int i=0; i<_tagInfoListModel->count(); i++) {
        TagInfo* tagInfo = _tagInfoListModel->value<TagInfo*>(i);
        if (tagInfo->_manufacturerIdFact->rawValue().toInt() == qobject_cast<TagManufacturer*>(tagManufacturerListItem)->_idFact->rawValue().toInt()) {
            return false;
        }
    }

    delete _tagManufacturerListModel->removeOne(tagManufacturerListItem);
    
    return true;
}

uint32_t TagDatabase::_nextTagInfoId()
{
    uint32_t returnedTagId = _nextTagId;
    _nextTagId += 2;
    return returnedTagId;
}

uint32_t TagDatabase::_nextTagManufacturerId()
{
    uint32_t returnedManufacturerId = _nextManufacturerId;
    _nextManufacturerId++;
    return returnedManufacturerId;
}

QString TagDatabase::_tagInfoFilePath()
{
    return QString::asprintf(
                        "%s/TagInfo.db",
                        qgcApp()->toolbox()->settingsManager()->appSettings()->parameterSavePath().toStdString().c_str());
}
   
QString TagDatabase::_tagManufacturerFilePath()
{
    return QString::asprintf(
                        "%s/TagManufacturer.db",
                        qgcApp()->toolbox()->settingsManager()->appSettings()->parameterSavePath().toStdString().c_str());
}

bool TagDatabase::_saveTagInfo()
{
    QString filename = _tagInfoFilePath();
    QFile   file(filename);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qgcApp()->showAppMessage(QString("Open for write '%1' failed: %2").arg(filename).arg(file.errorString()));
        return false;
    }

    for (int i=0; i<_tagInfoListModel->count(); i++) {
        TagInfo* tagInfo = _tagInfoListModel->value<TagInfo*>(i);
        file.write(QStringLiteral("%1,%2,%3,%4,%5\n")
            .arg(tagInfo->_selectedFact->rawValue().toInt())
            .arg(tagInfo->_idFact->rawValue().toInt())
            .arg(tagInfo->_nameFact->rawValue().toString())
            .arg(tagInfo->_manufacturerIdFact->rawValue().toInt())
            .arg(tagInfo->_frequencyHzFact->rawValue().toInt())
            .toUtf8());
    }

    return true;
}

bool TagDatabase::_saveTagManufacturer()
{
    QString filename = _tagManufacturerFilePath();
    QFile   file(filename);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qgcApp()->showAppMessage(QStringLiteral("Open for write '%1' failed: %2").arg(filename).arg(file.errorString()));
        return false;
    }

    for (int i=0; i<_tagManufacturerListModel->count(); i++) {
        TagManufacturer* tagManufacturer = _tagManufacturerListModel->value<TagManufacturer*>(i);
        file.write(QStringLiteral("%1,%2,%3,%4,%5,%6,%7,%8,%9\n")
            .arg(tagManufacturer->_idFact->rawValue().toInt())
            .arg(tagManufacturer->_nameFact->rawValue().toString())
            .arg(tagManufacturer->_ip_msecs_1Fact->rawValue().toInt())
            .arg(tagManufacturer->_ip_msecs_2Fact->rawValue().toInt())
            .arg(tagManufacturer->_ip_msecs_1_idFact->rawValue().toString())
            .arg(tagManufacturer->_ip_msecs_2_idFact->rawValue().toString())
            .arg(tagManufacturer->_pulse_width_msecsFact->rawValue().toInt())
            .arg(tagManufacturer->_ip_uncertainty_msecsFact->rawValue().toInt())
            .arg(tagManufacturer->_ip_jitter_msecsFact->rawValue().toInt())
            .toUtf8());
    }

    return true;
}

void TagDatabase::save()
{
    _saveTagInfo();
    _saveTagManufacturer();
}

TagInfo* TagDatabase::findTagInfo(uint32_t id)
{
    for (int i=0; i<_tagInfoListModel->count(); i++) {
        TagInfo* tagInfo = _tagInfoListModel->value<TagInfo*>(i);
        if (tagInfo->_idFact->rawValue().toUInt() == id) {
            return tagInfo;
        }
    }
    return nullptr;
}

TagManufacturer* TagDatabase::findTagManufacturer(uint32_t id)
{
    for (int i=0; i<_tagManufacturerListModel->count(); i++) {
        TagManufacturer* tagManufacturer = _tagManufacturerListModel->value<TagManufacturer*>(i);
        if (tagManufacturer->_idFact->rawValue().toUInt() == id) {
            return tagManufacturer;
        }
    }
    return nullptr;
}

void TagDatabase::_updateNextIds(void)
{
    uint32_t maxId = 0;
    for (int i=0; i<_tagInfoListModel->count(); i++) {
        TagInfo* tagInfo = _tagInfoListModel->value<TagInfo*>(i);
        maxId = std::max(tagInfo->id()->rawValue().toUInt(), maxId);
    }
    _nextTagId = maxId + 2;

    maxId = 0;
    for (int i=0; i<_tagManufacturerListModel->count(); i++) {
        TagManufacturer* tagManufacturer = _tagManufacturerListModel->value<TagManufacturer*>(i);
        maxId = std::max(tagManufacturer->id()->rawValue().toUInt(), maxId);
    }
    _nextManufacturerId = maxId + 1;
}

bool TagDatabase::_loadTagInfo(void)
{
    QString filename = _tagInfoFilePath();
    QFile   file(filename);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qgcApp()->showAppMessage(QStringLiteral("Unable to open `%1` for saving: %2").arg(filename).arg(file.errorString()));
        return false;
    }

    QString     strLine;
    QTextStream stream(&file);
    int         lineCount           = 0;
    int         cExpectedValueCount = 5;   

    while (stream.readLineInto(&strLine)) {
        lineCount++;
        if (strLine.startsWith("#")) {
            continue;
        }
        if (strLine.length() == 0) {
            continue;
        }

        QStringList values = strLine.split(",");
        if (values.count() != cExpectedValueCount) {
            qgcApp()->showAppMessage(QStringLiteral("Load Tag Info: Line #%1 Does not contain %2 values").arg(lineCount).arg(cExpectedValueCount));
            return false;
        }

        bool                ok;
        QString             valueString;
        int                 valuePosition = 0;
        bool                selected;
        uint32_t            id;
        QString             name;
        uint32_t            manufacturerId;
        uint32_t            frequencyHz;

        valueString = values[valuePosition++];
        selected    = valueString.toUInt(&ok);
        if (!ok) {
            qgcApp()->showAppMessage(QStringLiteral("Load Tag Info: Line #%1 Value:'%2'. Unable to convert selected to boolean.").arg(lineCount).arg(valueString));
            return false;
        }

        valueString = values[valuePosition++];
        id          = valueString.toUInt(&ok);
        if (!ok) {
            qgcApp()->showAppMessage(QStringLiteral("Load Tag Info: Line #%1 Value:'%2'. Unable to convert id to uint.").arg(lineCount).arg(valueString));
            return false;
        }
        if (id <= 1) {
            qgcApp()->showAppMessage(QStringLiteral("Load Tag Info: Line #%1 Value:'%2'. Tag ids must be greater than 1").arg(lineCount).arg(valueString));
            return false;
        }
        if (id % 2) {
            qgcApp()->showAppMessage(QStringLiteral("Load Tag Info: Line #%1 Value:'%2'. Tag ids must be even numbers").arg(lineCount).arg(valueString));
            return false;
        }
        if (findTagInfo(id)) {
            qgcApp()->showAppMessage(QStringLiteral("Load Tag Info: Line #%1 Value:'%2'. Duplicate tag id.").arg(lineCount).arg(valueString));
            return false;
        }

        name = values[valuePosition++];
        if (name.length() == 0) {
            name.setNum(id);
        }

        valueString     = values[valuePosition++];
        manufacturerId  = valueString.toUInt(&ok);
        if (!ok) {
            qgcApp()->showAppMessage(QStringLiteral("Load Tag Info: Line #%1 Value:'%2'. Unable to convert manufacturer id to uint.").arg(lineCount).arg(valueString));
            return false;
        }
        if (!findTagManufacturer(manufacturerId)) {
            qgcApp()->showAppMessage(QStringLiteral("Load Tag Info: Line #%1 Value:'%2'. Manufacturer id not found in manufacturer list.").arg(lineCount).arg(valueString));
            return false;
        }

        valueString = values[valuePosition++];
        frequencyHz = valueString.toUInt(&ok);
        if (!ok) {
            qgcApp()->showAppMessage(QStringLiteral("Load Tag Info: Line #%1 Value:'%2'. Unable to convert frequency to uint.").arg(lineCount).arg(valueString));
            return false;
        }

        _tagInfoListModel->append(new TagInfo(selected, id, name, manufacturerId, frequencyHz, this));
    }

    _updateNextIds();

    return true;
}

bool TagDatabase::_loadTagManufacturer(void)
{
    QString filename = _tagManufacturerFilePath();
    QFile   file(filename);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qgcApp()->showAppMessage(QStringLiteral("Unable to open `%1` for reading: %2").arg(filename).arg(file.errorString()));
        return false;
    }

    QString     strLine;
    QTextStream stream(&file);
    int         lineCount           = 0;
    int         cExpectedValueCount = 9;   

    while (stream.readLineInto(&strLine)) {
        lineCount++;
        if (strLine.startsWith("#")) {
            continue;
        }
        if (strLine.length() == 0) {
            continue;
        }

        QStringList values = strLine.split(",");
        if (values.count() != cExpectedValueCount) {
            qgcApp()->showAppMessage(QStringLiteral("Load Manufacturer Info: Line #%1 Does not contain %2 values").arg(lineCount).arg(cExpectedValueCount));
            return false;
        }

        bool                ok;
        QString             valueString;
        int                 valuePosition = 0;
        uint32_t            id;
        QString             name;
        uint32_t            ipMsecs1;
        uint32_t            ipMsecs2;
        QString             ipMsecs1Id;
        QString             ipMsecs2Id;
        uint32_t            pulseWidthMsecs;
        uint32_t            ipUncertaintyMsecs;
        uint32_t            ipJitterMsecs;

        valueString = values[valuePosition++];
        id          = valueString.toUInt(&ok);
        if (!ok) {
            qgcApp()->showAppMessage(QStringLiteral("Load Manufacturer Info: Line #%1 Value:'%2'. Unable to convert id to uint.").arg(lineCount).arg(valueString));
            return false;
        }
        if (id < 1) {
            qgcApp()->showAppMessage(QStringLiteral("Load Manufacturer Info: Line #%1 Value:'%2'. Manufacturer ids must be greater than 0").arg(lineCount).arg(valueString));
            return false;
        }
        if (findTagManufacturer(id)) {
            qgcApp()->showAppMessage(QStringLiteral("Load Manufacturer Info: Line #%1 Value:'%2'. Duplicate manufacturer id.").arg(lineCount).arg(valueString));
            return false;
        }

        name = values[valuePosition++];
        if (name.length() == 0) {
            name.setNum(id);
        }

        valueString = values[valuePosition++];
        ipMsecs1    = valueString.toUInt(&ok);
        if (!ok) {
            qgcApp()->showAppMessage(QStringLiteral("Load Manufacturer Info: Line #%1 Value:'%2'. Unable to convert ip msecs 1 to uint.").arg(lineCount).arg(valueString));
            return false;
        }

        valueString = values[valuePosition++];
        ipMsecs2    = valueString.toUInt(&ok);
        if (!ok) {
            qgcApp()->showAppMessage(QStringLiteral("Load Manufacturer Info: Line #%1 Value:'%2'. Unable to convert ip msecs 2 to uint.").arg(lineCount).arg(valueString));
            return false;
        }

        ipMsecs1Id = values[valuePosition++];
        if (ipMsecs1Id.length() == 0) {
            ipMsecs1Id.setNum(1);
        }

        ipMsecs2Id = values[valuePosition++];
        if (ipMsecs2Id.length() == 0) {
            ipMsecs2Id.setNum(2);
        }

        valueString     = values[valuePosition++];
        pulseWidthMsecs = valueString.toUInt(&ok);
        if (!ok) {
            qgcApp()->showAppMessage(QStringLiteral("Load Manufacturer Info: Line #%1 Value:'%2'. Unable to convert pulse width msecs 2 to uint.").arg(lineCount).arg(valueString));
            return false;
        }

        valueString         = values[valuePosition++];
        ipUncertaintyMsecs  = valueString.toUInt(&ok);
        if (!ok) {
            qgcApp()->showAppMessage(QStringLiteral("Load Manufacturer Info: Line #%1 Value:'%2'. Unable to convert uncertainty msecs 2 to uint.").arg(lineCount).arg(valueString));
            return false;
        }

        valueString     = values[valuePosition++];
        ipJitterMsecs   = valueString.toUInt(&ok);
        if (!ok) {
            qgcApp()->showAppMessage(QStringLiteral("Load Manufacturer Info: Line #%1 Value:'%2'. Unable to convert jitter msecs 2 to uint.").arg(lineCount).arg(valueString));
            return false;
        }

        _tagManufacturerListModel->append(new TagManufacturer(id, name, ipMsecs1, ipMsecs2, ipMsecs1Id, ipMsecs2Id, pulseWidthMsecs, ipUncertaintyMsecs, ipJitterMsecs, this));
    }

    return true;
}

void TagDatabase::_load()
{
    _tagManufacturerListModel->clearAndDeleteContents();
    _tagInfoListModel->clearAndDeleteContents();

    if (!QFileInfo(_tagInfoFilePath()).exists() || !QFileInfo(_tagManufacturerFilePath()).exists()) {
        qDebug() << "Not loading tag info since files are missing";
        return;
    }

    if (!_loadTagManufacturer()) {
        goto Error;
    }
    if (!_loadTagInfo()) {
        goto Error;
    }

    return;

Error:
    _tagManufacturerListModel->clearAndDeleteContents();
    _tagInfoListModel->clearAndDeleteContents();
}

void TagDatabase::_setupTunerVars()
{
    //_sampleRateHz       = sdrType == SDR_TYPE_AIRSPY_MINI ? 375000 : 192000;
    _sampleRateHz       = 375000;
    _fullBwHz           = _sampleRateHz;
    _halfBwHz           = _fullBwHz / 2;
    _channelBwHz        = _sampleRateHz / _nChannels;
    _halfChannelBwHz    = _channelBwHz / 2;

#ifdef DEBUG_TUNER
    qDebug() <<
        "_sampleRateHz:" << _sampleRateHz <<
        "_fullBwHz:" << _fullBwHz <<
        "_halfBwHz:" << _halfBwHz <<
        "_channelBwHz:" << _channelBwHz <<
        "_halfChannelBwHz:" << _halfChannelBwHz;
#endif
}

// Find the best center frequency for the requested frequencies such that the there is only one frequency per channel
// bin. And the requested frequested is as close to center within the channel bin as possible. Frequencies are not
// allowed to be within a shoulder area of the channel bin. Once the best center frequency is found, the channel bin
// values are updated for each tag.
//
// The channel bin structure follows the functionality of Matlab dsp.channelizer. The channel bins are 1-based to follow
// Matlab's 1-based vector/array indexing. Channel 1 is the centered at the radio center frequency Then remainder of the
// channels follow and wrap around back to the beginning of the bandwidth.
bool TagDatabase::channelizerTuner()
{
    _setupTunerVars();

    // Build the list of requested frequencies
    QVector<uint32_t> freqListHz;
    for (int i=0; i<_tagInfoListModel->count(); i++) {
        TagInfo* tagInfo = _tagInfoListModel->value<TagInfo*>(i);

        if (tagInfo->selected()->rawValue().toUInt()) {
            freqListHz.push_back(tagInfo->_frequencyHzFact->rawValue().toUInt());
        }
    }

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
#ifdef DEBUG_TUNER
    qDebug() << "testCentersHz-raw:" << testCentersHz;
#endif

    // Remove any test centers for which a requested frequency would fall outside the full bandwidth
    for (auto testCenterHzIter = testCentersHz.begin(); testCenterHzIter != testCentersHz.end(); ) {
        if (freqMinHz < *testCenterHzIter - _halfBwHz || freqMaxHz > *testCenterHzIter + _halfBwHz) {
            testCenterHzIter = testCentersHz.erase(testCenterHzIter);
        } else {
            ++testCenterHzIter;
        }
    }
#ifdef DEBUG_TUNER
    qDebug() << "testCentersHz-filtered:" << testCentersHz;
#endif

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
            [this, testCenterHz](uint32_t freqHz) {
                if (freqHz >= testCenterHz - _halfChannelBwHz) {
                    return freqHz;
                } else {
                    return freqHz + _sampleRateHz;
                }
            });

#ifdef DEBUG_TUNER
        qDebug() << "freqListHz:" << freqListHz << "wrappedRequestedFreqsHz:" << wrappedRequestedFreqsHz;
#endif

        for (auto wrappedFreqHz : wrappedRequestedFreqsHz) {
            uint32_t tagFreqZeroBasedBandwidthHz        = wrappedFreqHz - ((int)testCenterHz - (int)_halfChannelBwHz);      // Adjust to zero-based within total bandwidth
            uint32_t zeroBasedChannelBucket             = tagFreqZeroBasedBandwidthHz  / _channelBwHz;
            uint32_t channelStartZeroBasedHz            = zeroBasedChannelBucket * _channelBwHz;                            // Zero based Hz of channel starting point
            uint32_t tagFreqZeroBasedWithinChannelHz    = tagFreqZeroBasedBandwidthHz - channelStartZeroBasedHz;            // Tag freq adjusted to zero based from start of containing channel
            uint32_t distanceFromChannelCenter          = abs((int)tagFreqZeroBasedWithinChannelHz - (int)_halfChannelBwHz);

            double  shoulderPercent = 0.85; // Shoulder is 15% from channel edge
            bool    inShoulder      = distanceFromChannelCenter > _halfChannelBwHz * shoulderPercent;

#ifdef DEBUG_TUNER
            qDebug() << "---- single freq build begin ---";
            qDebug() <<
                "testCenterHz:" << testCenterHz <<
                "wrappedFreqHz:" << wrappedFreqHz <<
                "tagFreqZeroBasedBandwidthHz:" << tagFreqZeroBasedBandwidthHz <<
                "zeroBasedChannelBucket:" << zeroBasedChannelBucket <<
                "channelStartZeroBasedHz:" << channelStartZeroBasedHz <<
                "tagFreqZeroBasedWithinChannelHz:" << tagFreqZeroBasedWithinChannelHz <<
                "distanceFromChannelCenter:" << distanceFromChannelCenter <<
                "inShoulder:" << inShoulder;
            //_printChannelMap(testCenterHz, QVector<uint32_t>({wrappedFreqHz}));
            qDebug() << "---- single freq build end ---";
#endif

            distanceFromCenters.push_back(distanceFromChannelCenter);
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

#ifdef DEBUG_TUNER
        qDebug() << "---- test center build begin ---";
        qDebug() << "testCenterHz" << testCenterHz;
        //_printChannelMap(testCenterHz, wrappedRequestedFreqsHz);
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
        int j = 0;
        for (int i=0; i<_tagInfoListModel->count(); i++) {
            auto tagInfo = _tagInfoListModel->value<TagInfo*>(i);

            if (tagInfo->selected()->rawValue().toUInt()) {
                tagInfo->channelizer_channel_number               = oneBasedChannelBuckets[j];
                tagInfo->channelizer_channel_center_frequency_hz  = channelCentersHz[oneBasedChannelBuckets[j] - 1];
                qDebug() << i << j
                        << tagInfo->channelizer_channel_number
                        << tagInfo->channelizer_channel_center_frequency_hz;
                j++;
            }
        }


        _radioCenterHz = bestTestCenterHz;
        return true;
    } else {
        _radioCenterHz = 0;
        return false;
    }
}

void TagDatabase::_printChannelMap(const uint32_t centerFreqHz, const QVector<uint32_t>& wrappedRequestedFreqsHz)
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

int TagDatabase::_firstChannelFreqHz(const int centerFreqHz)
{
    return centerFreqHz - _halfBwHz - _halfChannelBwHz;
}

uint32_t TagDatabase::maxIntraPulseMsecs()
{
    uint32_t maxIntraPulseMsecs  = 0;

    for (int i=0; i<_tagInfoListModel->count(); i++) {
        TagInfo*            tagInfo         = _tagInfoListModel->value<TagInfo*>(i);
        TagManufacturer*    tagManufacturer = findTagManufacturer(tagInfo->manufacturerId()->rawValue().toUInt());

        maxIntraPulseMsecs = std::max(maxIntraPulseMsecs, tagManufacturer->ip_msecs_1()->rawValue().toUInt());

        if (tagManufacturer->ip_msecs_2()->rawValue().toUInt() != 0) {
            maxIntraPulseMsecs = std::max(maxIntraPulseMsecs, tagManufacturer->ip_msecs_2()->rawValue().toUInt());
        }
    }

    return maxIntraPulseMsecs;
}
