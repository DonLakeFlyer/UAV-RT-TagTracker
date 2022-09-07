#include "CustomOptions.h"
#include "CustomOptions.h"

CustomOptions::CustomOptions(CustomPlugin* plugin, QObject* parent)
    : QGCOptions    (parent)
    , _vhfQGCPlugin (plugin)
{

}
