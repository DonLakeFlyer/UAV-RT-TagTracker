#pragma once

#include "FirmwarePlugin.h"
#include "PX4FirmwarePlugin.h"

class CustomPX4FirmwarePlugin : public PX4FirmwarePlugin
{
    Q_OBJECT

public:
    CustomPX4FirmwarePlugin(void);

    // FirmwarePlugin overrides
    const QVariantList& toolIndicators(const Vehicle* vehicle) final;
    const QVariantList& modeIndicators(const Vehicle* vehicle) final;

private:
    QVariantList _toolIndicatorList;
    QVariantList _modeIndicatorList;
};
