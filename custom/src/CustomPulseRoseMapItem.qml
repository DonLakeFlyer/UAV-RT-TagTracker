/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtPositioning
import QtLocation

import QGroundControl
import QGroundControl.Palette
import QGroundControl.ScreenTools
import QGroundControl.Controls

MapQuickItem {
    coordinate:     customMapObject.rotationCenter
    anchorPoint.x:  mapRect.width / 2
    anchorPoint.y:  mapRect.height / 2

    property var customMapObject

    property var    _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property var    _flightMap:     parent
    property var    _corePlugin:    QGroundControl.corePlugin
    property var    _vhfSettings:   _corePlugin.customSettings
    property var    _divisions:     _vhfSettings.divisions.rawValue
    property real   _sliceSize:     360 / _divisions
    property int    _rotationIndex: customMapObject.rotationIndex
    property real   _ratio:         _corePlugin.angleRatios.length - 1 == _rotationIndex ? _largeRatio : _smallRatio

    readonly property real _largeRatio: 0.5
    readonly property real _smallRatio: 0.3

    sourceItem: Rectangle {
        id:             mapRect
        width:          _flightMap.height * _ratio
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
                property real strengthRatio:    _corePlugin.angleRatios[_rotationIndex][index]

                Connections {
                    target:                 _corePlugin
                    onAngleRatiosChanged:   arcCanvas.requestPaint()
                }
            }
        }

        Rectangle {
            id:             calcedBearingIndicator
            width:          radius * 2
            height:         width
            radius:         _radius
            x:              (parent.width / 2) - radius
            y:              -radius
            color:          "white"
            visible:        !isNaN(_calcedBearing)

            transform: Rotation {
                id:         calcedBearingIndicatorRotation
                origin.x:   calcedBearingIndicator.radius
                origin.y:   (mapRect.height / 2) + calcedBearingIndicator.radius
                angle:      isNaN(calcedBearingIndicator._calcedBearing) ? 0 : calcedBearingIndicator._calcedBearing
            }

            property real _calcedBearing: _corePlugin.calcedBearings[_rotationIndex]
            property real _radius:        ScreenTools.defaultFontPixelWidth * 4

            QGCLabel {
                id:                         calcedBearingLabel
                anchors.horizontalCenter:   parent.horizontalCenter
                anchors.verticalCenter:     parent.verticalCenter
                text:                       calcedBearingIndicator._calcedBearing.toFixed(0)
                font.pointSize:             ScreenTools.largeFontPointSize
                color:                      "black"

                transform: Rotation {
                    origin.x:   calcedBearingLabel.width / 2
                    origin.y:   calcedBearingLabel.height / 2
                    angle:      -calcedBearingIndicatorRotation.angle
                }
            }
        }
    }

    
    QGCMapPalette { id: mapPal; lightColors: _flightMap.isSatelliteMap }
}
