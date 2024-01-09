#include "CustomPX4FirmwarePlugin.h"

CustomPX4FirmwarePlugin::CustomPX4FirmwarePlugin(void)
{

}

const QVariantList& CustomPX4FirmwarePlugin::toolIndicators(const Vehicle*)
{
    //-- Default list of indicators for all vehicles.
    if(_toolIndicatorList.size() == 0) {
        _toolIndicatorList = QVariantList({
            QVariant::fromValue(QUrl::fromUserInput("qrc:/PX4/Indicators/PX4FlightModeIndicator.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/MessageIndicator.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/GPSIndicator.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/TelemetryRSSIIndicator.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/RCRSSIIndicator.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/PX4/Indicators/PX4BatteryIndicator.qml")),
        });
    }
    return _toolIndicatorList;
}

const QVariantList& CustomPX4FirmwarePlugin::modeIndicators(const Vehicle*)
{
    //-- Default list of indicators for all vehicles.
    if(_modeIndicatorList.size() == 0) {
        _modeIndicatorList = QVariantList({
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/LinkIndicator.qml")),
        });
    }
    return _modeIndicatorList;
}
