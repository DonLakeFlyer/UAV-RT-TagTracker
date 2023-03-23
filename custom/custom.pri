message("Adding Custom Plugin")

#-- Version control
#   Major and minor versions are defined here (manually)

CUSTOM_QGC_VER_MAJOR = 0
CUSTOM_QGC_VER_MINOR = 0
CUSTOM_QGC_VER_FIRST_BUILD = 0

linux {
    QMAKE_CXXFLAGS_WARN_ON += -Wno-strict-aliasing
}

# Build number is automatic
# Uses the current branch. This way it works on any branch including build-server's PR branches
CUSTOM_QGC_VER_BUILD = $$system(git --git-dir ../.git rev-list $$GIT_BRANCH --first-parent --count)
win32 {
    CUSTOM_QGC_VER_BUILD = $$system("set /a $$CUSTOM_QGC_VER_BUILD - $$CUSTOM_QGC_VER_FIRST_BUILD")
} else {
    CUSTOM_QGC_VER_BUILD = $$system("echo $(($$CUSTOM_QGC_VER_BUILD - $$CUSTOM_QGC_VER_FIRST_BUILD))")
}
CUSTOM_QGC_VERSION = $${CUSTOM_QGC_VER_MAJOR}.$${CUSTOM_QGC_VER_MINOR}.$${CUSTOM_QGC_VER_BUILD}

DEFINES -= APP_VERSION_STR=\"\\\"$$APP_VERSION_STR\\\"\"
DEFINES += APP_VERSION_STR=\"\\\"$$CUSTOM_QGC_VERSION\\\"\"

message(Custom QGC Version: $${CUSTOM_QGC_VERSION})

# Build a single flight stack by disabling APM support
# common is broken in upstream master so we can't do this
#MAVLINK_CONF = common
CONFIG  += QGC_DISABLE_APM_MAVLINK
CONFIG  += QGC_DISABLE_APM_PLUGIN QGC_DISABLE_APM_PLUGIN_FACTORY

# We implement our own PX4 plugin factory
CONFIG  += QGC_DISABLE_PX4_PLUGIN_FACTORY

# Disable things we don't need
CONFIG += DISABLE_AIRMAP DISABLE_VIDEOSTREAMING

# Branding

DEFINES += CUSTOMHEADER=\"\\\"CustomPlugin.h\\\"\"
DEFINES += CUSTOMCLASS=CustomPlugin

TARGET   = TagTracker
DEFINES += QGC_APPLICATION_NAME='"\\\"Tag Tracker\\\""'

DEFINES += QGC_ORG_NAME=\"\\\"qgroundcontrol.org\\\"\"
DEFINES += QGC_ORG_DOMAIN=\"\\\"org.qgroundcontrol\\\"\"

QGC_APP_NAME        = "Tag Tracker"
QGC_BINARY_NAME     = "TagTracker"
QGC_ORG_NAME        = "Custom"
QGC_ORG_DOMAIN      = "org.custom"
QGC_ANDROID_PACKAGE = "org.custom.qgroundcontrol"
QGC_APP_DESCRIPTION = "Tag Telemetry"
QGC_APP_COPYRIGHT   = "Copyright (C) 2020 QGroundControl Development Team. All rights reserved."

# Our own, custom resources
RESOURCES += \
    $$PWD/custom.qrc

QML_IMPORT_PATH += \
   $$PWD/src

# Our own, custom sources
SOURCES += \
    $$PWD/src/CustomFirmwarePlugin.cc \
    $$PWD/src/CustomFirmwarePluginFactory.cc \
    $$PWD/src/CustomOptions.cc \
    $$PWD/src/CustomPlugin.cc \
    $$PWD/src/CustomSettings.cc \
    $$PWD/src/TagInfoList.cc \
    $$PWD/src/PulseInfo.cc \
    $$PWD/src/PulseInfoList.cc \

HEADERS += \
    $$PWD/src/CustomFirmwarePlugin.h \
    $$PWD/src/CustomFirmwarePluginFactory.h \
    $$PWD/src/CustomOptions.h \
    $$PWD/src/CustomPlugin.h \
    $$PWD/src/CustomSettings.h \
    $$PWD/src/TagInfoList.h \
    $$PWD/src/PulseInfo.h \
    $$PWD/src/PulseInfoList.h \

INCLUDEPATH += \
    $$PWD/src \

# Shared header files for Tunnel Protocol
INCLUDEPATH += \
    $$PWD/uavrt_interfaces/include/uavrt_interfaces \

# Setup to build uavrt_thresholds

SOURCES += \
    $$files($$PWD/uavrt_thresholds/codegen/lib/thresholdGeneratorPre/*.cpp, false) \
    $$files($$PWD/uavrt_thresholds/codegen/lib/thresholdGeneratorPre/*.c, false) \
    $$files($$PWD/uavrt_thresholds/codegen/lib/thresholdGeneratorPost/*.cpp, false) \
    $$files($$PWD/uavrt_thresholds/codegen/lib/thresholdGeneratorPost/*.c, false) \
    $$files($$PWD/uavrt_thresholds/codegen/lib/thresholdGeneratorSingleTrial/*.cpp, false) \
    $$files($$PWD/uavrt_thresholds/codegen/lib/thresholdGeneratorSingleTrial/*.c, false) \

INCLUDEPATH += \
    $$PWD/uavrt_thresholds/codegen/lib/thresholdGeneratorPre \
    $$PWD/uavrt_thresholds/codegen/lib/thresholdGeneratorPost \
    $$PWD/uavrt_thresholds/matlab-coder-utils/coder-headers \
    $$PWD/uavrt_thresholds/codegen/lib/thresholdGeneratorSingleTrial \

MacBuild {
    QMAKE_CXXFLAGS += -fopenmp=libomp
    QMAKE_LFLAGS += -fopenmp=libomp -z muldefs
} else {
    QMAKE_CXXFLAGS += -fopenmp
    QMAKE_LFLAGS += -fopenmp -z muldefs
}
