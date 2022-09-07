#pragma once

#include "QGCOptions.h"

class CustomPlugin;

class CustomOptions : public QGCOptions
{
public:
    CustomOptions(CustomPlugin* plugin, QObject* parent = NULL);
};
