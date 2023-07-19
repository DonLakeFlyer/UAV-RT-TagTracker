#pragma once

#include "HerelinkOptions.h"

#include "QGCCorePlugin.h"
#include "QGCLoggingCategory.h"

#include <QObject>

Q_DECLARE_LOGGING_CATEGORY(HerelinkCorePluginLog)

class HerelinkCorePlugin : public QGCCorePlugin
{
    Q_OBJECT

public:
    HerelinkCorePlugin(QGCApplication* app, QGCToolbox* toolbox);

    // Overrides from QGCCorePlugin
    QGCOptions* options (void) final { return qobject_cast<QGCOptions*>(_herelinkOptions); }

    // Overrides from QGCTool
    void setToolbox(QGCToolbox* toolbox) final;

private:
    HerelinkOptions* _herelinkOptions = nullptr;
};
