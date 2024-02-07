/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QGroundControl
import QGroundControl.FlightDisplay

// The action handles:
//  Send Tags
//  Start/Stop Detection
GuidedToolStripAction {
    text:       _guidedController.startDetectionTitle
    iconSource: "/res/action.svg"
    visible:    true
    enabled:    QGroundControl.multiVehicleManager.activeVehicle && actionEnabled
    actionID:   _guidedController.actionStartDetection

    property bool actionEnabled: false
    property var controllerStatus: QGroundControl.corePlugin.controllerStatus

    onControllerStatusChanged: _update(controllerStatus)

    function _update(controllerStatus) {
        switch (controllerStatus) {
        case CustomPlugin.ControllerStatusIdle:
        case CustomPlugin.ControllerStatusReceivingTags:
            text            = _guidedController.startDetectionTitle
            actionEnabled   = false
            break
        case CustomPlugin.ControllerStatusHasTags:
            text            = _guidedController.startDetectionTitle
            actionEnabled   = true
            actionID        = _guidedController.actionStartDetection
            break
        case CustomPlugin.ControllerStatusDetecting:
            text            = _guidedController.stopDetectionTitle
            actionEnabled   = true
            actionID        = _guidedController.actionStopDetection
            break
        case CustomPlugin.ControllerStatusCapture:
            text            = _guidedController.startDetectionTitle
            actionEnabled   = false
            break
        }
    }
}
