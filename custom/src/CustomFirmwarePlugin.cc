#include "CustomFirmwarePlugin.h"

CustomFirmwarePlugin::CustomFirmwarePlugin(void)
{

}

const QVariantList& CustomFirmwarePlugin::toolIndicators(const Vehicle* vehicle)
{
    //-- Default list of indicators for all vehicles.
    if(_toolIndicatorList.size() == 0) {
        _toolIndicatorList = FirmwarePlugin::toolIndicators(vehicle);

        _toolIndicatorList += QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/PulseIndicator.qml"));
    }
    return _toolIndicatorList;
}
