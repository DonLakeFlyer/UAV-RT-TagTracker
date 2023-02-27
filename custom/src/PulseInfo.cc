#include "PulseInfo.h"

PulseInfo::PulseInfo(TunnelProtocol::PulseInfo_t pulseInfo, QObject* parent)
    : QObject   (parent)
    , _pulseInfo(pulseInfo)
{

}

PulseInfo::~PulseInfo()
{

}
