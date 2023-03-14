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
    id:             _root
    anchors.top:    parent.top
    anchors.bottom: parent.bottom
    width:          indicator.width

    property bool showIndicator: true

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

    Rectangle {
        id:             indicator
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        width:          ScreenTools.defaultFontPixelHeight * 5
        color:          "green"  

    }
    MouseArea {
        anchors.fill:   parent
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
                                height: popupRect.height / 2
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
