/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQml.Models

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightDisplay

ToolStripActionList {
    id: _root

    signal displayPreFlightChecklist

    model: [
        GuidedActionTakeoff { },
        GuidedActionLand { },
        GuidedActionRTL { },
        CustomGuidedActionSendTags { },
        CustomGuidedActionStartStopDetection { },
        CustomGuidedActionStartRotation { },
        CustomGuidedActionRawCapture { }
    ]
}
