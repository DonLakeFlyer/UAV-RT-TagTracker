/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.MultiVehicleManager
import QGroundControl.ScreenTools
import QGroundControl.Palette
import QGroundControl.FactSystem
import QGroundControl.FactControls
import MAVLink

Item {
    id:             control
    anchors.top:    parent.top
    anchors.bottom: parent.bottom
    width:          indicatorRow.width

    property bool       showIndicator:      true
    property bool       waitForParameters:  true   // UI won't show until parameters are ready
    property Component  expandedPageComponent

    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle

    Row {
        id:             indicatorRow
        anchors.top:    parent.top
        anchors.bottom: parent.bottom

        QGCLabel {
            font.pointSize: ScreenTools.largeFontPointSize
            text:           qsTr("Logs")
        }
    }

    MouseArea {
        anchors.fill:   parent
        onClicked: {
            mainWindow.showIndicatorDrawer(indicatorPopup, control)
        }
    }

    Component {
        id: indicatorPopup

        ToolIndicatorPage {
            showExpand:         false
            waitForParameters:  control.waitForParameters
            contentComponent:   indicatorContentComponent
        }
    }

    Component {
        id: indicatorContentComponent

        ColumnLayout {
            spacing: ScreenTools.defaultFontPixelHeight / 2

            Connections {
                target: QGroundControl.corePlugin

                onLogDirListDownloaded: (dirList, errorMsg) => {
                    console.log(dirList, errorMsg)
                    logDirRepeater.model = dirList
                }

                onDownloadLogDirFilesComplete: (errorMsg) => {
                    if (errorMsg === "") {
                        mainWindow.showMessageDialog(qsTr("Logs Download"), qsTr("Succeeded"))
                    } else {
                        mainWindow.showMessageDialog(qsTr("Logs Download"), qsTr("Failed: %1").arg(errorMsg))
                    }
                }
            }

            Component.onCompleted: {
                QGroundControl.corePlugin.downloadLogDirList()
            }

            Repeater {
                id: logDirRepeater

                RowLayout {
                    QGCLabel { text: modelData }

                    QGCButton {
                        text:       qsTr("Download")
                        onClicked:  QGroundControl.corePlugin.downloadLogDirFiles(modelData)
                    }
                }
            }
        }
    }
}
