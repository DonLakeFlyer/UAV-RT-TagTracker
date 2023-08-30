/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.11
import QtQuick.Layouts  1.11
import QtQuick.Controls 2.15

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.FactControls          1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0
import MAVLink                              1.0

Item {
    id:                 _root
    anchors.margins:    -ScreenTools.defaultFontPixelHeight / 2
    anchors.top:        parent.top
    anchors.bottom:     parent.bottom
    width:              rowLayout.width

    property bool showIndicator: true

    property var    activeVehicle:  QGroundControl.multiVehicleManager.activeVehicle
    property real   maxSNR:         QGroundControl.corePlugin.customSettings.maxPulse.rawValue

    Row {
        id:             rowLayout
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        spacing:        ScreenTools.defaultFontPixelWidth / 2

        Rectangle {
            height: parent.height
            width:  ScreenTools.defaultFontPixelWidth * 3
            color:  QGroundControl.corePlugin.controllerLostHeartbeat ? "red" : "green" 
        }

        ColumnLayout {
            height:     parent.height
            spacing:    2

            Repeater {
                model: QGroundControl.corePlugin.detectorInfoList

                RowLayout {
                    Rectangle {
                        id:                     pulseRect
                        Layout.fillHeight:      true
                        Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 20
                        color:                  object.heartbeatLost ? "red" : "transparent" 
                        border.color:           object.heartbeatLost ? "red" : "green" 

                        Rectangle {
                            anchors.topMargin:      2
                            anchors.bottomMargin:   2
                            anchors.leftMargin:     2
                            anchors.rightMargin:    ((maxSNR - filteredSNR) / maxSNR) *  (parent.width - 4)
                            anchors.fill:           parent
                            color:                  object.lastPulseStale ? "transparent" : "green"
                            border.color:           "green"
                            visible:                !object.heartbeatLost

                            property real filteredSNR: Math.max(0, Math.min(object.lastPulseSNR, maxSNR))
                        }
                    }

                    QGCLabel {
                        Layout.preferredHeight: pulseRect.height
                        text:                   object.tagId + object.tagLabel
                        fontSizeMode:           Text.VerticalFit
                        verticalAlignment:      Text.AlignVCenter

                        Component.onCompleted: console.log("Label", object.tagId, object.tagLabel)
                    }
                }
            }
        }
    }

    MouseArea {
        anchors.fill:   rowLayout
        onClicked:      mainWindow.showIndicatorPopup(_root, indicatorPopup)
    }

    Component {
        id: indicatorPopup

        Rectangle {
            id:     popupRect
            width:  mainWindow.width * 0.75
            height: mainWindow.height
            color:  "white"

            property var _activeVehicle:    QGroundControl.multiVehicleManager.activeVehicle
            property var _customSettings:   QGroundControl.corePlugin.customSettings

            QGCFlickable {
                anchors.margins:    ScreenTools.defaultFontPixelHeight
                anchors.fill:       parent
                contentHeight:      column.height
                clip:               false
                
                Column {
                    id:         column
                    spacing:    ScreenTools.defaultFontPixelHeight

                    FactCheckBox {
                        text:   qsTr("Show pulse strength on map")
                        fact:   _customSettings.showPulseOnMap
                    }

                    FactTextFieldGrid {
                        id: grid

                        factList: [
                            _customSettings.altitude,
                            _customSettings.divisions,
                            _customSettings.k,
                            _customSettings.falseAlarmProbability,
                            _customSettings.maxPulse,
                            _customSettings.antennaOffset,
                        ]
                    }

                    FactComboBox {
                        fact:           QGroundControl.corePlugin.customSettings.sdrType
                        indexModel:     false
                        sizeToContents: true
                    }
                }
            }
        }
    }
}
