#pragma once

#include "QGCOptions.h"

class HerelinkCorePlugin;

class HerelinkOptions : public QGCOptions
{
public:
    HerelinkOptions(HerelinkCorePlugin* plugin, QObject* parent = NULL);

    // QGCOptions overrides
    bool wifiReliableForCalibration () const final { return true; }
    bool showFirmwareUpgrade        () const final { return false; }
    bool multiVehicleEnabled        () const final { return false; }
};
