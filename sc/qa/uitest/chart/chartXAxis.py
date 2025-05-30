# -*- tab-width: 4; indent-tabs-mode: nil; py-indent-offset: 4 -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
from uitest.framework import UITestCase
from uitest.uihelper.common import change_measurement_unit
from uitest.uihelper.common import get_state_as_dict, get_url_for_data_file
from uitest.uihelper.common import select_by_text, select_pos

from libreoffice.uno.propertyvalue import mkPropertyValues


# Chart -  X Axis
class chartXAxis(UITestCase):
   def test_chart_x_axis_dialog(self):
    with self.ui_test.load_file(get_url_for_data_file("tdf122398.ods")):

        with change_measurement_unit(self, "Centimeter"):
            xCalcDoc = self.xUITest.getTopFocusWindow()
            gridwin = xCalcDoc.getChild("grid_window")

            gridwin.executeAction("SELECT", mkPropertyValues({"OBJECT": "Object 1"}))
            gridwin.executeAction("ACTIVATE", tuple())
            xChartMainTop = self.xUITest.getTopFocusWindow()
            xChartMain = xChartMainTop.getChild("chart_window")
            xSeriesObj =  xChartMain.getChild("CID/D=0:CS=0:CT=0:Series=0")
            with self.ui_test.execute_dialog_through_action(xSeriesObj, "COMMAND", mkPropertyValues({"COMMAND": "DiagramAxisX"})) as xDialog:
                #Click on tab "Scale".
                tabcontrol = xDialog.getChild("tabcontrol")
                select_pos(tabcontrol, "0")

                reverseDirection = xDialog.getChild("CBX_REVERSE")
                logarithmicScale = xDialog.getChild("CBX_LOGARITHM")
                autoMinimum = xDialog.getChild("CBX_AUTO_MIN")
                autoMaximum = xDialog.getChild("CBX_AUTO_MAX")
                majorInterval = xDialog.getChild("CBX_AUTO_STEP_MAIN")
                minorInterval = xDialog.getChild("CBX_AUTO_STEP_HELP")
                minimum = xDialog.getChild("EDT_MIN")
                maximum = xDialog.getChild("EDT_MAX")
                major = xDialog.getChild("EDT_STEP_MAIN")
                minor = xDialog.getChild("MT_STEPHELP")

                reverseDirection.executeAction("CLICK", tuple())
                logarithmicScale.executeAction("CLICK", tuple())
                autoMinimum.executeAction("CLICK", tuple())
                autoMaximum.executeAction("CLICK", tuple())
                majorInterval.executeAction("CLICK", tuple())
                minorInterval.executeAction("CLICK", tuple())

                minimum.executeAction("DOWN", tuple()) #10.12.2017
                maximum.executeAction("DOWN", tuple()) #29.04.2018
                major.executeAction("DOWN", tuple())   #19
                minor.executeAction("UP", tuple())  #3

            #reopen and verify
            gridwin.executeAction("SELECT", mkPropertyValues({"OBJECT": "Object 1"}))
            gridwin.executeAction("ACTIVATE", tuple())
            xChartMainTop = self.xUITest.getTopFocusWindow()
            xChartMain = xChartMainTop.getChild("chart_window")
            xSeriesObj =  xChartMain.getChild("CID/D=0:CS=0:CT=0:Series=0")
            with self.ui_test.execute_dialog_through_action(xSeriesObj, "COMMAND", mkPropertyValues({"COMMAND": "DiagramAxisX"})) as xDialog:
                #Click on tab "Scale".
                tabcontrol = xDialog.getChild("tabcontrol")
                select_pos(tabcontrol, "0")

                reverseDirection = xDialog.getChild("CBX_REVERSE")
                logarithmicScale = xDialog.getChild("CBX_LOGARITHM")
                autoMinimum = xDialog.getChild("CBX_AUTO_MIN")
                autoMaximum = xDialog.getChild("CBX_AUTO_MAX")
                majorInterval = xDialog.getChild("CBX_AUTO_STEP_MAIN")
                minorInterval = xDialog.getChild("CBX_AUTO_STEP_HELP")
                minimum = xDialog.getChild("EDT_MIN")
                maximum = xDialog.getChild("EDT_MAX")
                major = xDialog.getChild("EDT_STEP_MAIN")
                minor = xDialog.getChild("MT_STEPHELP")

                self.assertEqual(get_state_as_dict(reverseDirection)["Selected"], "true")
                self.assertEqual(get_state_as_dict(logarithmicScale)["Selected"], "true")
                self.assertEqual(get_state_as_dict(autoMinimum)["Selected"], "false")
                self.assertEqual(get_state_as_dict(autoMaximum)["Selected"], "false")
                self.assertEqual(get_state_as_dict(majorInterval)["Selected"], "false")
                self.assertEqual(get_state_as_dict(minorInterval)["Selected"], "false")
                self.assertEqual(get_state_as_dict(minimum)["Text"], "10.12.2017")
                self.assertEqual(get_state_as_dict(maximum)["Text"], "29.04.2018")
                self.assertEqual(get_state_as_dict(major)["Text"], "19")
                self.assertEqual(get_state_as_dict(minor)["Text"], "3")

                #Click on tab "positioning".
                tabcontrol = xDialog.getChild("tabcontrol")
                select_pos(tabcontrol, "1")

                crossAxis = xDialog.getChild("LB_CROSSES_OTHER_AXIS_AT")
                crossAxisValue = xDialog.getChild("EDT_CROSSES_OTHER_AXIS_AT") #only available when crossAxis = Value
                placeLabels = xDialog.getChild("LB_PLACE_LABELS")
                innerMajorTick = xDialog.getChild("CB_TICKS_INNER")
                outerMajorTick = xDialog.getChild("CB_TICKS_OUTER")
                innerMinorTick = xDialog.getChild("CB_MINOR_INNER")
                outerMinorTick = xDialog.getChild("CB_MINOR_OUTER")
                placeMarks = xDialog.getChild("LB_PLACE_TICKS")

                select_by_text(crossAxis, "Start")
                select_by_text(placeLabels, "Outside end")
                innerMajorTick.executeAction("CLICK", tuple())
                outerMajorTick.executeAction("CLICK", tuple())
                innerMinorTick.executeAction("CLICK", tuple())
                outerMinorTick.executeAction("CLICK", tuple())
                select_by_text(placeMarks, "At axis")


            #reopen and verify tab "positioning".
            gridwin.executeAction("SELECT", mkPropertyValues({"OBJECT": "Object 1"}))
            gridwin.executeAction("ACTIVATE", tuple())
            xChartMainTop = self.xUITest.getTopFocusWindow()
            xChartMain = xChartMainTop.getChild("chart_window")
            xSeriesObj =  xChartMain.getChild("CID/D=0:CS=0:CT=0:Series=0")
            with self.ui_test.execute_dialog_through_action(xSeriesObj, "COMMAND", mkPropertyValues({"COMMAND": "DiagramAxisX"})) as xDialog:

                tabcontrol = xDialog.getChild("tabcontrol")
                select_pos(tabcontrol, "1")

                crossAxis = xDialog.getChild("LB_CROSSES_OTHER_AXIS_AT")
                crossAxisValue = xDialog.getChild("EDT_CROSSES_OTHER_AXIS_AT") #only available when crossAxis = Value
                placeLabels = xDialog.getChild("LB_PLACE_LABELS")
                innerMajorTick = xDialog.getChild("CB_TICKS_INNER")
                outerMajorTick = xDialog.getChild("CB_TICKS_OUTER")
                innerMinorTick = xDialog.getChild("CB_MINOR_INNER")
                outerMinorTick = xDialog.getChild("CB_MINOR_OUTER")
                placeMarks = xDialog.getChild("LB_PLACE_TICKS")

                self.assertEqual(get_state_as_dict(crossAxis)["SelectEntryText"], "Start")
                self.assertEqual(get_state_as_dict(placeLabels)["SelectEntryText"], "Outside end")
                self.assertEqual(get_state_as_dict(innerMajorTick)["Selected"], "true")
                self.assertEqual(get_state_as_dict(outerMajorTick)["Selected"], "false")
                self.assertEqual(get_state_as_dict(innerMinorTick)["Selected"], "true")
                self.assertEqual(get_state_as_dict(outerMinorTick)["Selected"], "true")
                self.assertEqual(get_state_as_dict(placeMarks)["SelectEntryText"], "At axis")
                #change tab "positioning".
                select_by_text(crossAxis, "Value")
                crossAxisValue.executeAction("UP", tuple())  #1


            #reopen and verify tab "positioning".
            gridwin.executeAction("SELECT", mkPropertyValues({"OBJECT": "Object 1"}))
            gridwin.executeAction("ACTIVATE", tuple())
            xChartMainTop = self.xUITest.getTopFocusWindow()
            xChartMain = xChartMainTop.getChild("chart_window")
            xSeriesObj =  xChartMain.getChild("CID/D=0:CS=0:CT=0:Series=0")
            with self.ui_test.execute_dialog_through_action(xSeriesObj, "COMMAND", mkPropertyValues({"COMMAND": "DiagramAxisX"})) as xDialog:

                tabcontrol = xDialog.getChild("tabcontrol")
                select_pos(tabcontrol, "1")

                crossAxis = xDialog.getChild("LB_CROSSES_OTHER_AXIS_AT")
                crossAxisValue = xDialog.getChild("EDT_CROSSES_OTHER_AXIS_AT") #only available when crossAxis = Value
                placeLabels = xDialog.getChild("LB_PLACE_LABELS")
                innerMajorTick = xDialog.getChild("CB_TICKS_INNER")
                outerMajorTick = xDialog.getChild("CB_TICKS_OUTER")
                innerMinorTick = xDialog.getChild("CB_MINOR_INNER")
                outerMinorTick = xDialog.getChild("CB_MINOR_OUTER")
                placeMarks = xDialog.getChild("LB_PLACE_TICKS")

                self.assertEqual(get_state_as_dict(crossAxis)["SelectEntryText"], "Value")
                self.assertEqual(get_state_as_dict(crossAxisValue)["Text"], "1")
                self.assertEqual(get_state_as_dict(placeLabels)["SelectEntryText"], "Outside end")
                self.assertEqual(get_state_as_dict(innerMajorTick)["Selected"], "true")
                self.assertEqual(get_state_as_dict(outerMajorTick)["Selected"], "false")
                self.assertEqual(get_state_as_dict(innerMinorTick)["Selected"], "true")
                self.assertEqual(get_state_as_dict(outerMinorTick)["Selected"], "true")
                self.assertEqual(get_state_as_dict(placeMarks)["SelectEntryText"], "At axis")
                #change tab "Line".
                select_pos(tabcontrol, "2")

                xWidth = xDialog.getChild("MTR_FLD_LINE_WIDTH")
                transparency = xDialog.getChild("MTR_LINE_TRANSPARENT")

                xWidth.executeAction("UP", tuple())
                transparency.executeAction("UP", tuple())


            #reopen and verify tab "Line".
            gridwin.executeAction("SELECT", mkPropertyValues({"OBJECT": "Object 1"}))
            gridwin.executeAction("ACTIVATE", tuple())
            xChartMainTop = self.xUITest.getTopFocusWindow()
            xChartMain = xChartMainTop.getChild("chart_window")
            xSeriesObj =  xChartMain.getChild("CID/D=0:CS=0:CT=0:Series=0")
            with self.ui_test.execute_dialog_through_action(xSeriesObj, "COMMAND", mkPropertyValues({"COMMAND": "DiagramAxisX"})) as xDialog:

                tabcontrol = xDialog.getChild("tabcontrol")
                select_pos(tabcontrol, "2")

                xWidth = xDialog.getChild("MTR_FLD_LINE_WIDTH")
                transparency = xDialog.getChild("MTR_LINE_TRANSPARENT")

                self.assertEqual(get_state_as_dict(xWidth)["Text"], "0.10 cm")
                self.assertEqual(get_state_as_dict(transparency)["Text"], "5%")

                #change tab "Label"
                tabcontrol = xDialog.getChild("tabcontrol")
                select_pos(tabcontrol, "3")

                tile = xDialog.getChild("tile")
                overlapCB = xDialog.getChild("overlapCB")
                breakCB = xDialog.getChild("breakCB")
                stackedCB = xDialog.getChild("stackedCB")
                textdirLB = xDialog.getChild("textdirLB")

                tile.executeAction("CLICK", tuple())
                overlapCB.executeAction("CLICK", tuple())
                breakCB.executeAction("CLICK", tuple())
                stackedCB.executeAction("CLICK", tuple())
                select_by_text(textdirLB, "Right-to-left")


            #reopen and verify tab "Label".
            gridwin.executeAction("SELECT", mkPropertyValues({"OBJECT": "Object 1"}))
            gridwin.executeAction("ACTIVATE", tuple())
            xChartMainTop = self.xUITest.getTopFocusWindow()
            xChartMain = xChartMainTop.getChild("chart_window")
            xSeriesObj =  xChartMain.getChild("CID/D=0:CS=0:CT=0:Series=0")
            with self.ui_test.execute_dialog_through_action(xSeriesObj, "COMMAND", mkPropertyValues({"COMMAND": "DiagramAxisX"})) as xDialog:

                tabcontrol = xDialog.getChild("tabcontrol")
                select_pos(tabcontrol, "3")

                tile = xDialog.getChild("tile")
                overlapCB = xDialog.getChild("overlapCB")
                breakCB = xDialog.getChild("breakCB")
                stackedCB = xDialog.getChild("stackedCB")
                textdirLB = xDialog.getChild("textdirLB")

                self.assertEqual(get_state_as_dict(tile)["Checked"], "true")
                self.assertEqual(get_state_as_dict(overlapCB)["Selected"], "true")
                self.assertEqual(get_state_as_dict(breakCB)["Selected"], "true")
                self.assertEqual(get_state_as_dict(stackedCB)["Selected"], "true")
                self.assertEqual(get_state_as_dict(textdirLB)["SelectEntryText"], "Right-to-left")


# vim: set shiftwidth=4 softtabstop=4 expandtab:
