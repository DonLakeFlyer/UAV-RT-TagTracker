message("Adding Custom Plugin")

#-- Version control
#   Major and minor versions are defined here (manually)

CUSTOM_QGC_VER_MAJOR = 0
CUSTOM_QGC_VER_MINOR = 0
CUSTOM_QGC_VER_FIRST_BUILD = 0

linux {
    QMAKE_CXXFLAGS_WARN_ON += -Wno-strict-aliasing
}

!WindowsBuild {
    QMAKE_CXXFLAGS_WARN_ON += -Wno-sometimes-uninitialized
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

# We implement our own plugin factories
CONFIG  +=  \
    QGC_DISABLE_PX4_PLUGIN_FACTORY \
    QGC_DISABLE_APM_PLUGIN_FACTORY

# Tag Tracker is set as an android Home App
CONFIG += AndroidHomeApp

# Branding

DEFINES += CUSTOMHEADER=\"\\\"CustomPlugin.h\\\"\"
DEFINES += CUSTOMCLASS=CustomPlugin

TARGET   = TagTracker
DEFINES += QGC_APPLICATION_NAME='"\\\"Tag Tracker\\\""'

DEFINES += QGC_ORG_NAME=\"\\\"thegagnes.com\\\"\"
DEFINES += QGC_ORG_DOMAIN=\"\\\"com.thegagnes\\\"\"

QGC_APP_NAME        = "Tag Tracker"
QGC_BINARY_NAME     = "TagTracker"
QGC_ORG_NAME        = "thegagnes"
QGC_ORG_DOMAIN      = "org.thegagnes.com"
QGC_ANDROID_PACKAGE = "org.thegagnes.qgroundcontrol"
QGC_APP_DESCRIPTION = "Tag Tracker"
QGC_APP_COPYRIGHT   = "Copyright (C) 2023 Latest Fiasco Dev Team. All rights reserved."

# Remove code which the Herelink doesn't need
DEFINES += \
    QGC_GST_TAISYNC_DISABLED
    NO_SERIAL_LINK
    QGC_DISABLE_BLUETOOTH

# Our own, custom resources
RESOURCES += \
    $$PWD/custom.qrc

QML_IMPORT_PATH += \
   $$PWD/src

Herelink {
    message("Building for Herelink")

    DEFINES += HERELINK_BUILD

# Herelink sources
SOURCES += \
    $$PWD/src/HerelinkCorePlugin.cc \
    $$PWD/src/HerelinkOptions.cc \

HEADERS += \
    $$PWD/src/HerelinkCorePlugin.h \
    $$PWD/src/HerelinkOptions.h \
}

# TagTracker custom sources
SOURCES += \
    $$PWD/src/CustomFirmwarePluginFactory.cc \
    $$PWD/src/CustomOptions.cc \
    $$PWD/src/CustomPlugin.cc \
    $$PWD/src/CustomPX4FirmwarePlugin.cc \
    $$PWD/src/CustomArduCopterFirmwarePlugin.cc \
    $$PWD/src/CustomSettings.cc \
    $$PWD/src/DetectorInfo.cc \
    $$PWD/src/DetectorInfoListModel.cc \
    $$PWD/src/TagDatabase.cc \
    $$PWD/src/PulseInfo.cc \
    $$PWD/src/PulseInfoList.cc \

HEADERS += \
    $$PWD/src/CustomFirmwarePluginFactory.h \
    $$PWD/src/CustomOptions.h \
    $$PWD/src/CustomPlugin.h \
    $$PWD/src/CustomPX4FirmwarePlugin.h \
    $$PWD/src/CustomArduCopterFirmwarePlugin.h \
    $$PWD/src/CustomSettings.h \
    $$PWD/src/DetectorInfo.h \
    $$PWD/src/DetectorInfoListModel.h \
    $$PWD/src/TagDatabase.h \
    $$PWD/src/PulseInfo.h \
    $$PWD/src/PulseInfoList.h \

INCLUDEPATH += \
    $$PWD/src \

# Bearing calc matlab code
    INCLUDEPATH += \
        $$PWD/matlab-coder-utils/coder-headers \
        $$PWD/uavrt_bearing/codegen/exe/bearing \

    SOURCES += \
        $$files($$PWD/uavrt_bearing/codegen/exe/bearing/*.cpp) \
        $$files($$PWD/uavrt_bearing/codegen/exe/bearing/*.c)

# Shared header files for Tunnel Protocol
INCLUDEPATH += \
    $$PWD/uavrt_interfaces/include/uavrt_interfaces \
