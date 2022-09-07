#pragma once

#include "QGCOptions.h"

class CustomPlugin;

class CustomOptions : public QGCOptions
{
public:
    CustomOptions(CustomPlugin* plugin, QObject* parent = NULL);

    //QUrl flyViewOverlay  (void) const { return QUrl::fromUserInput("qrc:/qml/VHFTrackerFlyViewOverlay.qml"); }
};
