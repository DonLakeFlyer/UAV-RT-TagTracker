#pragma once

#include "QGCOptions.h"

class HerelinkCorePlugin;

class HerelinkOptions : public QGCOptions
{
public:
    HerelinkOptions(HerelinkCorePlugin* plugin, QObject* parent = NULL);
};
