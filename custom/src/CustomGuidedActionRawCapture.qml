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

GuidedToolStripAction {
    text:       _guidedController.rawCaptureTitle
    iconSource: "/res/action.svg"
    visible:    true
    enabled:    QGroundControl.multiVehicleManager.activeVehicle && QGroundControl.corePlugin.controllerStatus == CustomPlugin.ControllerStatusHasTags
    actionID:   _guidedController.actionRawCapture
}
