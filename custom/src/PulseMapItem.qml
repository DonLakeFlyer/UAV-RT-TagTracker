import QtQuick
import QtQuick.Controls
import QtLocation

import QGroundControl
import QGroundControl.ScreenTools
import QGroundControl.Palette
import QGroundControl.Controls

MapQuickItem {
    coordinate:     customMapObject.coordinate
    anchorPoint.x:  (_labelMargin * 2) + (tagIdRect.width / 2)
    anchorPoint.y:  labelControl.height / 2
    z:              QGroundControl.zOrderWidgets - 1

    property var customMapObject

    property real   _indicatorRadius:   Math.ceil(ScreenTools.defaultFontPixelHeight / 2)
    property real   _labelMargin:       2
    property real   _labelRadius:       _indicatorRadius + _labelMargin

    sourceItem: Rectangle {
        id:                     labelControl
        width:                  snrLabel.x + snrLabel.contentWidth + (_labelMargin * 2)
        height:                 tagIdRect.height + (_labelMargin * 2)
        radius:                 height / 2
        color:                  "white"
        border.color:           "black"

        Rectangle {
            id:                     tagIdRect
            anchors.left:           parent.left
            anchors.leftMargin:     _labelMargin
            anchors.verticalCenter: parent.verticalCenter
            width:                  Math.max(tagIdLabel.contentWidth + (_labelMargin * 2), height)
            height:                 tagIdLabel.contentHeight + (_labelMargin * 2)
            radius:                 height / 2
            color:                  "white"
            border.color:           "black"

            QGCLabel {
                id:                     tagIdLabel
                anchors.centerIn:       parent
                horizontalAlignment:    Text.AlignHCenter
                verticalAlignment:      Text.AlignVCenter
                text:                   customMapObject.tagId
                color:                  "black"
            }
        }

        QGCLabel {
            id:                     snrLabel
            anchors.left:           tagIdRect.right
            anchors.leftMargin:     _labelMargin
            anchors.verticalCenter: parent.verticalCenter
            verticalAlignment:      Text.AlignVCenter
            text:                   customMapObject.snr.toFixed(1)
            color:                  "black"
        }
    }
}
