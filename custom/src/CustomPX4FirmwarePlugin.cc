#include "CustomPX4FirmwarePlugin.h"

CustomPX4FirmwarePlugin::CustomPX4FirmwarePlugin(void)
{

}

const QVariantList& CustomPX4FirmwarePlugin::toolIndicators(const Vehicle*)
{
    //-- Default list of indicators for all vehicles.
    if(_toolIndicatorList.size() == 0) {
        _toolIndicatorList = QVariantList({
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/MessageIndicator.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/GPSIndicator.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/TelemetryRSSIIndicator.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/BatteryIndicator.qml")),
            QVariant::fromValue(QUrl::fromUserInput("qrc:/qml/PulseLogIndicator.qml")),
        });
    }
    return _toolIndicatorList;
}

