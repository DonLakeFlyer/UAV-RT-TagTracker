/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                  2.12
import QtQuick.Controls         2.4
import QtQuick.Dialogs          1.3
import QtQuick.Layouts          1.12
import QtCharts                 2.15

import QtLocation               5.3
import QtPositioning            5.3
import QtQuick.Window           2.2
import QtQml.Models             2.1

import QGroundControl               1.0
import QGroundControl.Airspace      1.0
import QGroundControl.Airmap        1.0
import QGroundControl.Controllers   1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FlightDisplay 1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0

// To implement a custom overlay copy this code to your own control in your custom code source. Then override the
// FlyViewCustomLayer.qml resource with your own qml. See the custom example and documentation for details.
Item {
    id: _root

    property var parentToolInsets               // These insets tell you what screen real estate is available for positioning the controls in your overlay
    property var totalToolInsets:   _toolInsets // These are the insets for your custom overlay additions
    property var mapControl

    property real   _toolsMargin:   ScreenTools.defaultFontPixelWidth * 0.75
    property var    _corePlugin:    QGroundControl.corePlugin

    QGCToolInsets {
        id:                         _toolInsets
        leftEdgeCenterInset:    0
        leftEdgeTopInset:           0
        leftEdgeBottomInset:        0
        rightEdgeCenterInset:   0
        rightEdgeTopInset:          0
        rightEdgeBottomInset:       0
        topEdgeCenterInset:       0
        topEdgeLeftInset:           0
        topEdgeRightInset:          0
        bottomEdgeCenterInset:    0
        bottomEdgeLeftInset:        0
        bottomEdgeRightInset:       0
    }

    Rectangle {
        width:                  parentToolInsets.rightEdgeTopInset
        anchors.topMargin:      parentToolInsets.topEdgeRightInset
        anchors.bottom:         parent.bottom
        anchors.top:            parent.top
        anchors.right:          parent.right
        color:                  "white"

        property real _margins: ScreenTools.defaultFontPixelWidth / 2

        ChartView {
            id:                 chart
            anchors.fill:       parent
            antialiasing:       true
            margins.top:            0
            margins.bottom:            0
            margins.left:            0
            margins.right:            0
            legend.visible: false

            ValueAxis {
                id:             axisX
                min:            0
                max:            100
                tickCount:      5
                labelsVisible:  false
            }

            ValueAxis {
                id:         axisY
                min:        0
                max:        12
                tickCount:  12
                reverse:    true
            }

            ScatterSeries {
                id: pulseSeries
                axisX: axisX
                axisY: axisY
            }

            Timer {
                interval: 100
                repeat: true
                running: true
                onTriggered: {
                    axisY.min = axisY.min + 0.1
                    axisY.max = axisY.max + 0.1
                }
            }

            Connections {
                target: _corePlugin

                property bool firstPulse:       true
                property real timeAdjustment:   0

                function onPulseReceived() {
                    if (firstPulse) {
                        firstPulse = false
                        timeAdjustment = _corePlugin.pulseTimeSeconds + axisY.min
                    }
                    console.log("pulse received", _corePlugin.pulseTimeSeconds, timeAdjustment)
                    pulseSeries.append(_corePlugin.pulseSNR, _corePlugin.pulseTimeSeconds - timeAdjustment);
                }
            }
        }

    }
}
