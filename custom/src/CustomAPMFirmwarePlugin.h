#pragma once

#include "FirmwarePlugin.h"
#include "APMFirmwarePlugin.h"

class CustomAPMFirmwarePlugin : public APMFirmwarePlugin
{
    Q_OBJECT

public:
    CustomAPMFirmwarePlugin(void);

    // FirmwarePlugin overrides
    const QVariantList& toolIndicators(const Vehicle* vehicle) final;

private:
    QVariantList _toolIndicatorList;
};
