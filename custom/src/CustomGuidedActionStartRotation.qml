/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QGroundControl               1.0
import QGroundControl.FlightDisplay 1.0

GuidedToolStripAction {
    text:       _guidedController.startRotationTitle
    iconSource: "/res/action.svg"
    visible:    true
    enabled:    _activeVehicle && _activeVehicle.flying 
    actionID:   _guidedController.actionStartRotation

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
}
