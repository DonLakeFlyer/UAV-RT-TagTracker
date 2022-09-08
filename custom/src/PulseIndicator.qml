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

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0
import MAVLink                              1.0

//-------------------------------------------------------------------------
//-- VHF Pulse Indicator


Item {
    id:             _root
    anchors.top:    parent.top
    anchors.bottom: parent.bottom
    width:          pulseIndicatorRow.width

    property bool showIndicator: true

    property var    _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property var    _corePlugin:    QGroundControl.corePlugin
    property real   _pulseStrength: 0

    Connections {
        target: _corePlugin
        onBeepStrengthChanged: {
            _pulseStrength = beepStrength
        }
    }

    RowLayout {
        id:             pulseIndicatorRow
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        width:          ScreenTools.defaultFontPixelWidth * 30

        Rectangle {
            Layout.fillWidth:   true
            height:             ScreenTools.defaultFontPixelHeight * 2
            color:              "white"
            border.color:       "green"

            Rectangle {
                id:                     indicatorBar
                anchors.margins:        1
                anchors.rightMargin:    _rightMargin
                anchors.fill:           parent
                color:                  "green"

                property real   _maximumPulse:      100
                property real   _indicatorStrength: _pulseStrength
                property real   _value:             _indicatorStrength
                property real   _rightMargin:       (parent.width - 2) - ((parent.width - 2) * (Math.min(_indicatorStrength, _maximumPulse) / _maximumPulse))

                Connections {
                    target: _corePlugin
                    onBeepStrengthChanged: {
                        pulseResetTimer.restart()
                        //noDronePulsesTimer.restart()
                        indicatorBar._indicatorStrength = _pulseStrength
                        //noDronePulses.visible = false
                    }
                }

                Timer {
                    id:             pulseResetTimer
                    interval:       750
                    repeat:         false
                    onTriggered:    indicatorBar._indicatorStrength = 0
                }
            }
        }

        QGCLabel {
            width:                      (ScreenTools.defaultFontPixelWidth * ScreenTools.largeFontPointRatio) * 4
            text:                       _corePlugin.beepStrength.toFixed(0)
            color:                      "black"
            font.pointSize:             ScreenTools.largeFontPointSize
        }
    }
}
