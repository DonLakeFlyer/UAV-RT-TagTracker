#pragma once

#include "FirmwarePlugin.h"

class PX4FirmwarePlugin;
class ArduCopterFirmwarePlugin;

class CustomFirmwarePluginFactory : public FirmwarePluginFactory
{
    Q_OBJECT
public:
    CustomFirmwarePluginFactory(void);

    // Overrides from FirmwarePluginFactory
    FirmwarePlugin*                     firmwarePluginForAutopilot  (MAV_AUTOPILOT autopilotType, MAV_TYPE vehicleType) final;
    QList<QGCMAVLink::FirmwareClass_t>  supportedFirmwareClasses(void) const final;
    QList<QGCMAVLink::VehicleClass_t>   supportedVehicleClasses(void) const final;

private:
    PX4FirmwarePlugin*          _px4PluginInstance          = NULL;
    ArduCopterFirmwarePlugin*   _arduCopterPluginInstance   = NULL;
};
