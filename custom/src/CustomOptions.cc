#include "CustomOptions.h"
#include "CustomPlugin.h"

CustomOptions::CustomOptions(CustomPlugin* plugin, QObject* parent)
#ifdef HERELINK_BUILD
    : HerelinkOptions(qobject_cast<HerelinkCorePlugin*>(plugin), parent)
#else
    : QGCOptions(parent)
#endif
{

}
