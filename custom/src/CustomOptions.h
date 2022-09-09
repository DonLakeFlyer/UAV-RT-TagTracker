#pragma once

#include "QGCOptions.h"

class CustomPlugin;

class CustomOptions : public QGCOptions
{
public:
    CustomOptions(CustomPlugin* plugin, QObject* parent = NULL);

    // QGCOptions overrides
    virtual bool guidedBarShowOrbit () const final { return false; }
    virtual bool guidedBarShowROI   () const final { return false; }
};
