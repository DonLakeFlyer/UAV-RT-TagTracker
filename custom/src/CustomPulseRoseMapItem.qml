/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Layouts  1.15
import QtQuick.Controls 2.15
import QtPositioning    5.15
import QtLocation       5.15

import QGroundControl                   1.0
import QGroundControl.Palette           1.0
import QGroundControl.ScreenTools       1.0

MapQuickItem {
    coordinate:     _activeVehicle ? _activeVehicle.coordinate : QtPositioning.coordinate()
    anchorPoint.x:  mapRect.width / 2
    anchorPoint.y:  mapRect.height / 2

    property var    _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property var    _flightMap:     parent
    property var    _corePlugin:    QGroundControl.corePlugin
    property var    _vhfSettings:   _corePlugin.customSettings
    property var    _divisions:     _vhfSettings.divisions.rawValue
    property real   _sliceSize:     360 / _divisions

    sourceItem: Rectangle {
        id:             mapRect
        width:          _flightMap.height * 0.66
        height:         width
        radius:         width / 2
        color:          "transparent"
        border.color:   mapPal.text
        border.width:   2

        Repeater {
            model: _divisions

            Canvas {
                id:             arcCanvas
                anchors.fill:   parent
                visible:        !isNaN(strengthRatio)

                onPaint: {
                    var ctx = getContext("2d");
                    ctx.reset();

                    ctx.beginPath();
                    ctx.fillStyle = "transparent";
                    ctx.strokeStyle = "red";
                    ctx.lineWidth = 3;
                    ctx.moveTo(centerX, centerY);
                    ctx.arc(centerX, centerY, (width / 2) * arcCanvas.strengthRatio, 0, arcRadians, false);
                    ctx.lineTo(centerX, centerY);
                    ctx.fill();
                    ctx.stroke();
                }

                transform: Rotation {
                    origin.x:   arcCanvas.centerX
                    origin.y:   arcCanvas.centerY
                    angle:      -90 - (360 / _divisions / 2) + ((360 / _divisions) * index)
                }

                property real centerX:          width / 2
                property real centerY:          height / 2
                property real arcRadians:       (Math.PI * 2) / _divisions
                property real strengthRatio:    _corePlugin.angleRatios[index]

                Connections {
                    target:                 _corePlugin
                    onAngleRatiosChanged:   arcCanvas.requestPaint()
                }
            }
        }
    }

    
    QGCMapPalette { id: mapPal; lightColors: _flightMap.isSatelliteMap }
}
