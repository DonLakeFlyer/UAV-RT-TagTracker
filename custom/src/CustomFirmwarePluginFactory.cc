#include "CustomFirmwarePluginFactory.h"
#include "CustomAPMFirmwarePlugin.h"
#include "CustomPX4FirmwarePlugin.h"

CustomFirmwarePluginFactory CustomFirmwarePluginFactoryImp;

CustomFirmwarePluginFactory::CustomFirmwarePluginFactory(void)
{

}

FirmwarePlugin* CustomFirmwarePluginFactory::firmwarePluginForAutopilot(MAV_AUTOPILOT autopilotType, MAV_TYPE vehicleType)
{
    Q_UNUSED(vehicleType);
    switch (autopilotType) {
        case MAV_AUTOPILOT_PX4:
            if (!_px4PluginInstance) {
                _px4PluginInstance = new CustomPX4FirmwarePlugin;
            }
            return _px4PluginInstance;
        case MAV_AUTOPILOT_ARDUPILOTMEGA:
            if (!_apmPluginInstance) {
                _apmPluginInstance = new CustomAPMFirmwarePlugin;
            }
            return _apmPluginInstance;
        default:
            return NULL;
    }
}

QList<QGCMAVLink::FirmwareClass_t> CustomFirmwarePluginFactory::supportedFirmwareClasses(void) const
{
    QList<QGCMAVLink::FirmwareClass_t> list;
    list.append(QGCMAVLink::FirmwareClassPX4);
    list.append(QGCMAVLink::FirmwareClassArduPilot);
    return list;
}

QList<QGCMAVLink::VehicleClass_t> CustomFirmwarePluginFactory::supportedVehicleClasses(void) const
{
    QList<QGCMAVLink::FirmwareClass_t> mavTypes;
    mavTypes.append(QGCMAVLink::VehicleClassMultiRotor);
    return mavTypes;
}
