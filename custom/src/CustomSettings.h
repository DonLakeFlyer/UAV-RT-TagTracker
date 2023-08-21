/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "SettingsGroup.h"

class CustomSettings : public SettingsGroup
{
    Q_OBJECT
    
public:
    CustomSettings(QObject* parent = nullptr);

    DEFINE_SETTING_NAME_GROUP()

    DEFINE_SETTINGFACT(altitude)
    DEFINE_SETTINGFACT(divisions)
    DEFINE_SETTINGFACT(maxPulse)
    DEFINE_SETTINGFACT(k)
    DEFINE_SETTINGFACT(falseAlarmProbability)
    DEFINE_SETTINGFACT(sdrType)
    DEFINE_SETTINGFACT(showPulseOnMap)
    DEFINE_SETTINGFACT(antennaOffset)
    DEFINE_SETTINGFACT(rotationKWaitCount)
};
