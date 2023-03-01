#include "PulseInfo.h"

PulseInfo::PulseInfo(TunnelProtocol::PulseInfo_t pulseInfo, QString& name, QString& rateChar, QObject* parent)
    : QObject   (parent)
    , _pulseInfo(pulseInfo)
    , _name     (name)
    , _rateChar (rateChar)
{

}

PulseInfo::~PulseInfo()
{

}
