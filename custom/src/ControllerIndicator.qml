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
import QtQuick.Controls

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls
import QGroundControl.MultiVehicleManager
import QGroundControl.ScreenTools
import QGroundControl.Palette
import MAVLink

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
                    property real filteredSNR: Math.max(0, Math.min(object.lastPulseSNR, maxSNR))

                    Rectangle {
                        id:                     pulseRect
                        Layout.fillHeight:      true
                        Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 20
                        color:                  object.heartbeatLost ? 
                                                    "red" :
                                                    object.lastPulseStale ? "yellow" : "transparent" 

                        Rectangle {
                            anchors.topMargin:      2
                            anchors.bottomMargin:   2
                            anchors.leftMargin:     2
                            anchors.rightMargin:    ((maxSNR - filteredSNR) / maxSNR) *  (parent.width - 4)
                            anchors.fill:           parent
                            color:                  object.lastPulseStale ? "yellow" : "green"
                            visible:                !object.heartbeatLost

                        }

                        QGCLabel {
                            anchors.fill:           parent
                            text:                   filteredSNR.toFixed(1)
                            font.bold:              true
                            color:                  "black"
                            horizontalAlignment:    Text.AlignHCenter
                            verticalAlignment:      Text.AlignVCenter
                        }
                    }

                    QGCLabel {
                        Layout.preferredHeight: pulseRect.height
                        text:                   object.tagId + object.tagLabel
                        fontSizeMode:           Text.VerticalFit
                        verticalAlignment:      Text.AlignVCenter
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
        id: settingsDialogComponent

        QGCPopupDialog {
            id:         settingsDialog
            title:      qsTr("Settings")
            buttons:    StandardButton.Close

            property var _customSettings: QGroundControl.corePlugin.customSettings

            Column {
                spacing: ScreenTools.defaultFontPixelHeight

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

    Component {
        id: manufacturerDialogComponent

        QGCPopupDialog {
            id:         manufacturerDialog
            title:      qsTr("Tag Manufacturer")
            buttons:    StandardButton.Close

            property var tagManufacturer

            onClosed: QGroundControl.corePlugin.tagDatabase.save()

            Column {
                spacing: ScreenTools.defaultFontPixelHeight

                FactTextFieldGrid {
                    factList: [
                        tagManufacturer.name,
                        tagManufacturer.ip_msecs_1, 
                        tagManufacturer.ip_msecs_2, 
                        tagManufacturer.ip_msecs_1_id,
                        tagManufacturer.ip_msecs_2_id,
                        tagManufacturer.pulse_width_msecs, 
                        tagManufacturer.ip_uncertainty_msecs,
                        tagManufacturer.ip_jitter_msecs,
                    ]
                }
            }
        }
    }

    Component {
        id: manufacturersDialogComponent

        QGCPopupDialog {
            id:         manufacturersDialog
            title:      qsTr("Manufacturers")
            buttons:    StandardButton.Close

            property var tagDatabase: QGroundControl.corePlugin.tagDatabase

            ColumnLayout {
                id:         mainLayout
                spacing:    ScreenTools.defaultFontPixelHeight

                QGCButton {
                    text:       qsTr("New Manufacturer")
                    onClicked:  manufacturerDialogComponent.createObject(mainWindow, { tagManufacturer: tagDatabase.newTagManufacturer() }).open()
                }

                GridLayout {
                    rows:       tagDatabase.tagManufacturerList.count + 1
                    columns:    10
                    flow:       GridLayout.TopToBottom

                    QGCLabel { text: qsTr("Id") }
                    Repeater {
                        model: tagDatabase.tagManufacturerList

                        QGCLabel { text: object.id.valueString }
                    }

                    QGCLabel { text: qsTr("Name") }
                    Repeater {
                        model: tagDatabase.tagManufacturerList

                        QGCLabel { text: object.name.valueString }
                    }

                    QGCLabel { text: qsTr("Rate 1") }
                    Repeater {
                        model: tagDatabase.tagManufacturerList

                        QGCLabel { text: object.ip_msecs_1.valueString }
                    }

                    QGCLabel { text: qsTr("Rate 2") }
                    Repeater {
                        model: tagDatabase.tagManufacturerList

                        QGCLabel { text: object.ip_msecs_2.valueString }
                    }

                    QGCLabel { text: qsTr("Id 1") }
                    Repeater {
                        model: tagDatabase.tagManufacturerList

                        QGCLabel { text: object.ip_msecs_1_id.valueString }
                    }

                    QGCLabel { text: qsTr("Id 2") }
                    Repeater {
                        model: tagDatabase.tagManufacturerList

                        QGCLabel { text: object.ip_msecs_2_id.valueString }
                    }

                    QGCLabel { text: qsTr("Width") }
                    Repeater {
                        model: tagDatabase.tagManufacturerList

                        QGCLabel { text: object.pulse_width_msecs.valueString }
                    }

                    QGCLabel { text: qsTr("Uncertainty") }
                    Repeater {
                        model: tagDatabase.tagManufacturerList

                        QGCLabel { text: object.ip_uncertainty_msecs.valueString }
                    }

                    QGCLabel { text: qsTr("Jitter") }
                    Repeater {
                        model: tagDatabase.tagManufacturerList

                        QGCLabel { text: object.ip_jitter_msecs.valueString }
                    }

                    QGCLabel { }
                    Repeater {
                        model: tagDatabase.tagManufacturerList

                        QGCButton {
                            text:       qsTr("Edit")
                            onClicked:  manufacturerDialogComponent.createObject(mainWindow, { tagManufacturer: object }).open()
                        }
                    }

                    QGCLabel { }
                    Repeater {
                        model: tagDatabase.tagManufacturerList

                        QGCButton {
                            text:       qsTr("Del")
                            onClicked: {
                                if (!tagDatabase.deleteTagManufacturerListItem(object)) {
                                    mainWindow.showMessageDialog(qsTr("Delete Manufacturer"), qsTr("Unable to delete manufacturer. Still being referenced by tag(s)."))
                                } else {
                                    QGroundControl.corePlugin.tagDatabase.save()
                                }
                            }
                        }                
                    }
                }
            }
        }
    }

    Component {
        id: tagInfoDialogComponent

        QGCPopupDialog {
            id:         tagInfoDialog
            title:      qsTr("Tag Info")
            buttons:    Dialog.Close

            property var tagInfo
            property var _manufacturerList: QGroundControl.corePlugin.tagDatabase.tagManufacturerList

            onClosed: QGroundControl.corePlugin.tagDatabase.save()
            
            Column {
                spacing: ScreenTools.defaultFontPixelHeight

                QGCComboBox {
                    id:             manufacturerCombo
                    sizeToContents: true

                    onActivated: tagInfo.manufacturerId.rawValue = _manufacturerList.get(index).id.rawValue

                    Component.onCompleted: {
                        var selectedIndex = -1
                        var listModel = []
                        for (var i=0; i<_manufacturerList.count; i++) {
                            var manufacturer = _manufacturerList.get(i)
                            var manufacturerId = manufacturer.id.rawValue
                            listModel.push(manufacturer.name.valueString)
                            if (manufacturerId === tagInfo.manufacturerId.rawValue) {
                                selectedIndex = i
                            }
                        }
                        manufacturerCombo.model = listModel
                        manufacturerCombo._calcPopupWidth()

                        if (selectedIndex == -1) {
                            console.warning("tagInfoDialogComponent: manufacturer id not found in list", manufacturerId)
                        } else {
                            manufacturerCombo.currentIndex = selectedIndex
                        }
                    }

                }

                FactTextFieldGrid {
                    factList: [
                        tagInfo.name,
                        tagInfo.frequencyHz,
                    ]
                }
            }
        }
    }

    Component {
        id: indicatorPopup

        Rectangle {
            id:     popupRect
            width:  mainWindow.width * 0.75
            height: mainWindow.height - ScreenTools.toolbarHeight - (ScreenTools.defaultFontPixelWidth * 0.75 * 2)
            color:  "white"

            property var tagDatabase: QGroundControl.corePlugin.tagDatabase

            QGCFlickable {
                anchors.margins:    ScreenTools.defaultFontPixelHeight
                anchors.fill:       parent
                contentHeight:      mainLayout.height
                clip:               false
                
                ColumnLayout {
                    id:         mainLayout
                    spacing:    ScreenTools.defaultFontPixelHeight

                    RowLayout {
                        spacing: ScreenTools.defaultFontPixelWidth

                        QGCButton {
                            text: qsTr("Settings")
                            onClicked: {
                                mainWindow.hideIndicatorPopup()
                                settingsDialogComponent.createObject(mainWindow).open()
                            }
                        }

                        QGCButton {
                            text:       qsTr("Manufacturers")
                            onClicked:  manufacturersDialogComponent.createObject(mainWindow).open()
                        }

                        QGCButton {
                            text:       qsTr("New Tag")
                            onClicked: { 
                                if (tagDatabase.tagManufacturerList.count == 0) {
                                    mainWindow.showMessageDialog(qsTr("New Tag"), qsTr("You must add a Manufacturer first."))
                                } else {
                                    tagInfoDialogComponent.createObject(mainWindow, { tagInfo: tagDatabase.newTagInfo() }).open()
                                }
                            }
                        }
                    }

                    GridLayout {
                        rows:       tagDatabase.tagInfoList.count + 1
                        columns:    5
                        flow:       GridLayout.TopToBottom

                        QGCLabel { }
                        Repeater {
                            model: tagDatabase.tagInfoList

                            FactCheckBox { 
                                fact:       object.selected 
                                onClicked:  QGroundControl.corePlugin.tagDatabase.save()
                            }
                        }

                        QGCLabel { text: qsTr("Id") }
                        Repeater {
                            model: tagDatabase.tagInfoList

                            QGCLabel { text: object.id.valueString }
                        }

                        QGCLabel { text: qsTr("Name") }
                        Repeater {
                            model: tagDatabase.tagInfoList

                            QGCLabel { text: object.name.valueString }
                        }

                        QGCLabel { text: qsTr("Frequency") }
                        Repeater {
                            model: tagDatabase.tagInfoList

                            QGCLabel { text: object.frequencyHz.valueString }
                        }

                        QGCLabel { }
                        Repeater {
                            model: tagDatabase.tagInfoList

                            QGCButton {
                                text:       qsTr("Edit")
                                onClicked:  tagInfoDialogComponent.createObject(mainWindow, { tagInfo: object }).open()
                            }
                        }

                        QGCLabel { }
                        Repeater {
                            model: tagDatabase.tagInfoList

                            QGCButton {
                                text:       qsTr("Del")
                                onClicked: {
                                    tagDatabase.deleteTagInfoListItem(object)
                                    QGroundControl.corePlugin.tagDatabase.save()
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
