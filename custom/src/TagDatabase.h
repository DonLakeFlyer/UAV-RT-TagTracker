#pragma once

#include <QObject>
#include <QMap>

class FactMetaData;
class Fact;
class TagDatabase;
class QmlObjectListModel;
class TagDatabase;

class TagInfo : public QObject
{
    Q_OBJECT

    friend class TagDatabase;

public:
    TagInfo(TagDatabase* parent);
    TagInfo(bool selected, uint32_t id, QString& name, uint32_t manufacturerId, uint32_t frequencyHz, TagDatabase* parent);

    Q_PROPERTY(Fact* selected       MEMBER _selectedFact        CONSTANT)
    Q_PROPERTY(Fact* id             MEMBER _idFact              CONSTANT)
    Q_PROPERTY(Fact* name           MEMBER _nameFact            CONSTANT)
    Q_PROPERTY(Fact* manufacturerId MEMBER _manufacturerIdFact  CONSTANT)
    Q_PROPERTY(Fact* frequencyHz    MEMBER _frequencyHzFact     CONSTANT)

    Fact* selected()       { return _selectedFact; }
    Fact* id()             { return _idFact; }
    Fact* name()           { return _nameFact; }
    Fact* manufacturerId() { return _manufacturerIdFact; }
    Fact* frequencyHz()    { return _frequencyHzFact; }

	// The 1-based channel index from which this channel is output from the channelizer.
	uint32_t channelizer_channel_number;
	// The center frequency of the above channel
	uint32_t channelizer_channel_center_frequency_hz;

private:
    void _initCommon(TagDatabase* parent);
    void _init(bool selected, uint32_t id, const QString& name, uint32_t manufacturedId, uint32_t frequencyHz);
    QString _nextTagName();
    
    TagDatabase*                    _parent             = nullptr;
    QMap<QString, FactMetaData*>    _tagInfoMetaData;
    Fact*                           _selectedFact       = nullptr;
    Fact*                           _idFact             = nullptr;
    Fact*                           _nameFact           = nullptr;
    Fact*                           _manufacturerIdFact = nullptr;
    Fact*                           _frequencyHzFact    = nullptr;
};

class TagManufacturer : public QObject
{
    Q_OBJECT

    friend class TagInfo;
    friend class TagDatabase;

public:
    TagManufacturer(TagDatabase* parent);
    TagManufacturer(uint32_t        id,
                    const QString&  name,
                    uint32_t        ip_msecs_1, 
                    uint32_t        ip_msecs_2, 
                    const QString&  ip_msecs_1_id,
                    const QString&  ip_msecs_2_id,
                    uint32_t        pulse_width_msecs, 
                    uint32_t        ip_uncertainty_msecs,
                    uint32_t        ip_jitter_msecs,
                    TagDatabase*    parent);

    Q_PROPERTY(Fact* id                     MEMBER _idFact                      CONSTANT)
    Q_PROPERTY(Fact* name                   MEMBER _nameFact                    CONSTANT)
    Q_PROPERTY(Fact* ip_msecs_1             MEMBER _ip_msecs_1Fact              CONSTANT)
    Q_PROPERTY(Fact* ip_msecs_2             MEMBER _ip_msecs_2Fact              CONSTANT)
    Q_PROPERTY(Fact* ip_msecs_1_id          MEMBER _ip_msecs_1_idFact           CONSTANT)
    Q_PROPERTY(Fact* ip_msecs_2_id          MEMBER _ip_msecs_2_idFact           CONSTANT)
    Q_PROPERTY(Fact* pulse_width_msecs      MEMBER _pulse_width_msecsFact       CONSTANT)
    Q_PROPERTY(Fact* ip_uncertainty_msecs   MEMBER _ip_uncertainty_msecsFact    CONSTANT)
    Q_PROPERTY(Fact* ip_jitter_msecs        MEMBER _ip_jitter_msecsFact         CONSTANT)

    Fact* id                    () { return _idFact; }
    Fact* name                  () { return _nameFact; }
    Fact* ip_msecs_1            () { return _ip_msecs_1Fact; }
    Fact* ip_msecs_2            () { return _ip_msecs_2Fact; }
    Fact* ip_msecs_1_id         () { return _ip_msecs_1_idFact; }
    Fact* ip_msecs_2_id         () { return _ip_msecs_2_idFact; }
    Fact* pulse_width_msecs     () { return _pulse_width_msecsFact; }
    Fact* ip_uncertainty_msecs  () { return _ip_uncertainty_msecsFact; }
    Fact* ip_jitter_msecs       () { return _ip_jitter_msecsFact; }

private:
    void _initCommon(TagDatabase* parent);
    void _init(
            uint32_t        id,
            const QString&  name,
            uint32_t        ip_msecs_1, 
            uint32_t        ip_msecs_2, 
            const QString&  ip_msecs_1_id,
            const QString&  ip_msecs_2_id,
            uint32_t        pulse_width_msecs, 
            uint32_t        ip_uncertainty_msecs,
            uint32_t        ip_jitter_msecs);
    QString _nextManufacturerName();

    TagDatabase*                    _parent                      = nullptr;
    QMap<QString, FactMetaData*>    _tagManufacturerMetaData;
    Fact*                           _idFact                     = nullptr;
    Fact*                           _nameFact                   = nullptr;
    Fact*                           _ip_msecs_1Fact             = nullptr;
    Fact*                           _ip_msecs_2Fact             = nullptr;
    Fact*                           _ip_msecs_1_idFact          = nullptr;
    Fact*                           _ip_msecs_2_idFact          = nullptr;
    Fact*                           _pulse_width_msecsFact      = nullptr;
    Fact*                           _ip_uncertainty_msecsFact   = nullptr;
    Fact*                           _ip_jitter_msecsFact        = nullptr;
};

class TagDatabase : public QObject
{
    Q_OBJECT

    friend class TagInfo;
    friend class TagManufacturer;

public:
    TagDatabase(QObject* parent = NULL);
    ~TagDatabase();

    Q_PROPERTY(QmlObjectListModel* tagInfoList          MEMBER _tagInfoListModel            CONSTANT)
    Q_PROPERTY(QmlObjectListModel* tagManufacturerList  MEMBER _tagManufacturerListModel    CONSTANT)

    Q_INVOKABLE QObject* newTagInfo();
    Q_INVOKABLE QObject* newTagManufacturer();
    Q_INVOKABLE void deleteTagInfoListItem(QObject* tagInfoListItem);
    Q_INVOKABLE bool deleteTagManufacturerListItem(QObject* tagManufacturerListItem);
    Q_INVOKABLE void save();

    QmlObjectListModel* tagInfoListModel        () { return _tagInfoListModel; }
    QmlObjectListModel* tagManufacturerListModel() { return _tagManufacturerListModel; }

    TagInfo*            findTagInfo         (uint32_t id);
    TagManufacturer*    findTagManufacturer (uint32_t id);

    uint32_t radioCenterHz      () { return _radioCenterHz; }
    uint32_t maxIntraPulseMsecs ();
    bool    channelizerTuner    ();

private:
    uint32_t _nextTagInfoId();
    uint32_t _nextTagManufacturerId();
    QString  _tagInfoFilePath();
    QString  _tagManufacturerFilePath();
    bool     _saveTagInfo();
    bool     _saveTagManufacturer();
    bool     _loadTagInfo();
    bool     _loadTagManufacturer();
    void     _load();
    void    _setupTunerVars     ();
    void    _printChannelMap    (const uint32_t centerFreqHz, const QVector<uint32_t>& wrappedRequestedFreqsHz);
    int     _firstChannelFreqHz (const int centerFreq);
    
private:
    QmlObjectListModel* _tagInfoListModel           = nullptr;
    QmlObjectListModel* _tagManufacturerListModel   = nullptr;
    uint32_t            _nextTagId                  = 2;
    uint32_t            _nextManufacturerId         = 1;

    uint32_t            _radioCenterHz      = 0;
    QVector<uint32_t>   _channelBucketCenters;

    uint32_t   _sampleRateHz;
    uint32_t   _fullBwHz;
    uint32_t   _halfBwHz;
    uint32_t   _channelBwHz;
    uint32_t   _halfChannelBwHz;

    static const uint       _nChannels          = 100;
};
