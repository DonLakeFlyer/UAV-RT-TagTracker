#pragma once

#include "FirmwarePlugin.h"
#include "PX4FirmwarePlugin.h"

class CustomFirmwarePlugin : public PX4FirmwarePlugin
{
    Q_OBJECT

public:
    CustomFirmwarePlugin(void);

    // FirmwarePlugin overrides
    virtual const QVariantList& toolIndicators(const Vehicle* vehicle) final;

private:
    QVariantList _toolIndicatorList;
};
