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
DECLARE_SETTINGSFACT(CustomSettings, tagId)
DECLARE_SETTINGSFACT(CustomSettings, frequency)
DECLARE_SETTINGSFACT(CustomSettings, frequencyDelta)
DECLARE_SETTINGSFACT(CustomSettings, pulseDuration)
DECLARE_SETTINGSFACT(CustomSettings, intraPulse1)
DECLARE_SETTINGSFACT(CustomSettings, intraPulse2)
DECLARE_SETTINGSFACT(CustomSettings, intraPulseUncertainty)
DECLARE_SETTINGSFACT(CustomSettings, intraPulseJitter)
DECLARE_SETTINGSFACT(CustomSettings, maxPulse)
DECLARE_SETTINGSFACT(CustomSettings, gain)
