#include "CustomFirmwarePluginFactory.h"
#include "CustomArduCopterFirmwarePlugin.h"
#include "CustomPX4FirmwarePlugin.h"

CustomFirmwarePluginFactory CustomFirmwarePluginFactoryImp;

CustomFirmwarePluginFactory::CustomFirmwarePluginFactory(void)
{
}

FirmwarePlugin* CustomFirmwarePluginFactory::firmwarePluginForAutopilot(MAV_AUTOPILOT autopilotType, MAV_TYPE vehicleType)
{
    switch (autopilotType) {
        case MAV_AUTOPILOT_PX4:
            if (!_px4PluginInstance) {
                _px4PluginInstance = new CustomPX4FirmwarePlugin;
            }
            return _px4PluginInstance;
        case MAV_AUTOPILOT_ARDUPILOTMEGA:
            switch (vehicleType) {
            case MAV_TYPE_QUADROTOR:
            case MAV_TYPE_HEXAROTOR:
            case MAV_TYPE_OCTOROTOR:
            case MAV_TYPE_TRICOPTER:
            case MAV_TYPE_COAXIAL:
            case MAV_TYPE_HELICOPTER:
                if (!_arduCopterPluginInstance) {
                    _arduCopterPluginInstance = new ArduCopterFirmwarePlugin;
                }
                return _arduCopterPluginInstance;
            default:
                break;
            }
            break;
        default:
            break;
    }

    return NULL;
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
