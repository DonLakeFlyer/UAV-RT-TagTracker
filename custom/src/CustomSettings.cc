/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "CustomSettings.h"

#include <QQmlEngine>
#include <QtQml>

DECLARE_SETTINGGROUP(Custom, "Custom")
{
    qmlRegisterUncreatableType<CustomSettings>("QGroundControl.SettingsManager", 1, 0, "CustomSettings", "Reference only");
}

DECLARE_SETTINGSFACT(CustomSettings, altitude)
DECLARE_SETTINGSFACT(CustomSettings, divisions)
DECLARE_SETTINGSFACT(CustomSettings, maxPulse)
DECLARE_SETTINGSFACT(CustomSettings, k)
DECLARE_SETTINGSFACT(CustomSettings, falseAlarmProbability)
