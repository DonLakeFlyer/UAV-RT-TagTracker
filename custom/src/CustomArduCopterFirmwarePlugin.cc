#include "CustomArduCopterFirmwarePlugin.h"

CustomArduCopterFirmwarePlugin::CustomArduCopterFirmwarePlugin(void)
{

}

const QVariantList& CustomArduCopterFirmwarePlugin::toolIndicators(const Vehicle*)
{
    //-- Default list of indicators for all vehicles.
    if(_toolIndicatorList.size() == 0) {
        _toolIndicatorList = QVariantList({
            QVariant::fromValue(QUrl::fromUserInput("qrc:/qml/QGroundControl/Controls/FlightModeIndicator.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/MessageIndicator.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/GPSIndicator.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/TelemetryRSSIIndicator.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/RCRSSIIndicator.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/qml/QGroundControl/Controls/BatteryIndicator.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/RemoteIDIndicator.qml")),
        });
    }
    return _toolIndicatorList;
}

const QVariantList& CustomArduCopterFirmwarePlugin::modeIndicators(const Vehicle*)
{
    //-- Default list of indicators for all vehicles.
    if(_modeIndicatorList.size() == 0) {
        _modeIndicatorList = QVariantList({
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/LinkIndicator.qml")),
        });
    }
    return _modeIndicatorList;
}
