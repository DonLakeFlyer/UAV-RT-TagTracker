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
            id:         popupRect
            width:      mainWindow.width * 0.75
            height:     mainWindow.height
            color:          "white"

            QGCFlickable {
                anchors.fill:   parent
                contentHeight:  flowLayout.height
                
                Flow {
                    id:             flowLayout
                    anchors.fill:   parent
                    
                    Repeater {
                        model: QGroundControl.corePlugin.pulseLog

                        Row {
                            spacing: ScreenTools.defaultFontPixelWidth * 2

                            Column {
                                height: popupRect.height
                                clip:   true

                                QGCLabel { font.pointSize: ScreenTools.largeFontPointSize; text: modelData.tagName + " - " + modelData.tagId }
                                QGCLabel { font.pointSize: ScreenTools.largeFontPointSize; text: modelData.tagFreqHz }

                                Repeater {
                                    model: modelData

                                    QGCLabel { font.pointSize: ScreenTools.largeFontPointSize; text: object.rateChar + ": " + object.snr.toFixed(1) }
                                }
                            }

                            Item {
                                width:  1
                                height: 1
                            }
                        }
                    }
                }
            }
        }
    }
}
