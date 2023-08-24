#pragma once

#include "FirmwarePlugin.h"
#include "APM/ArduCopterFirmwarePlugin.h"

class CustomArduCopterFirmwarePlugin : public ArduCopterFirmwarePlugin
{
    Q_OBJECT

public:
    CustomArduCopterFirmwarePlugin(void);

    // FirmwarePlugin overrides
    const QVariantList& toolIndicators(const Vehicle* vehicle) final;
    const QVariantList& modeIndicators(const Vehicle* vehicle) final;

private:
    QVariantList _toolIndicatorList;
    QVariantList _modeIndicatorList;
};
