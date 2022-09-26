/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts  1.2

import QGroundControl                   1.0
import QGroundControl.Controls          1.0
import QGroundControl.FactControls      1.0
import QGroundControl.ScreenTools       1.0
import QGroundControl.Palette           1.0
import QGroundControl.SettingsManager   1.0

Rectangle {
    id:                 _root
    color:              qgcPal.window
    anchors.fill:       parent
    anchors.margins:    ScreenTools.defaultFontPixelWidth

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

    QGCFlickable {
        clip:               true
        anchors.fill:       parent
        contentHeight:      column.height
        contentWidth:       column.width

        Column {
            id:         column
            spacing:    ScreenTools.defaultFontPixelHeight

            FactTextFieldGrid {
                id: grid

                factList: [
                    QGroundControl.corePlugin.customSettings.altitude,
                    QGroundControl.corePlugin.customSettings.divisions,
                    QGroundControl.corePlugin.customSettings.tagId,
                    QGroundControl.corePlugin.customSettings.frequency,
                    QGroundControl.corePlugin.customSettings.frequencyDelta,
                    QGroundControl.corePlugin.customSettings.pulseDuration,
                    QGroundControl.corePlugin.customSettings.intraPulse1,
                    QGroundControl.corePlugin.customSettings.intraPulse2,
                    QGroundControl.corePlugin.customSettings.intraPulseUncertainty,
                    QGroundControl.corePlugin.customSettings.intraPulseJitter
                ]
            }

            Row {
                spacing: ScreenTools.defaultFontPixelWidth * 5

                QGCButton {
                    text:       "Send Tag"
                    enabled:    _activeVehicle
                    onClicked:  QGroundControl.corePlugin.sendTag()
                }

                QGCButton {
                    text:       "Start Detection"
                    enabled:    _activeVehicle
                    onClicked:  QGroundControl.corePlugin.startDetection()
                }
            }
        }
    }
}
