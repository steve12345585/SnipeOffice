# -*- tab-width: 4; indent-tabs-mode: nil; py-indent-offset: 4 -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
from uitest.framework import UITestCase
from uitest.uihelper.common import get_state_as_dict, get_url_for_data_file
from uitest.uihelper.common import select_pos

from libreoffice.uno.propertyvalue import mkPropertyValues


# Bug 122398 - UI: Cannot specify min/max in axis scale or axis position. Limited between 0 and 100
class tdf122398(UITestCase):
   def test_tdf122398_chart_min_max_x_axis(self):
    with self.ui_test.load_file(get_url_for_data_file("tdf122398.ods")):
        xCalcDoc = self.xUITest.getTopFocusWindow()
        gridwin = xCalcDoc.getChild("grid_window")

        #Open attached file. Set chart into edit mode. Select x-axis and then Format Selection.
        #Disable the Automatic for min and max. You cannot change the values at all, neither with direct
        #input nor with up-down arrow buttons.
        gridwin.executeAction("SELECT", mkPropertyValues({"OBJECT": "Object 1"}))
        gridwin.executeAction("ACTIVATE", tuple())
        xChartMainTop = self.xUITest.getTopFocusWindow()
        xChartMain = xChartMainTop.getChild("chart_window")
        xSeriesObj =  xChartMain.getChild("CID/D=0:CS=0:CT=0:Series=0")
        with self.ui_test.execute_dialog_through_action(xSeriesObj, "COMMAND", mkPropertyValues({"COMMAND": "DiagramAxisX"})) as xDialog:
            #Click on tab "Scale".
            tabcontrol = xDialog.getChild("tabcontrol")
            select_pos(tabcontrol, "0")

            autoMinimum = xDialog.getChild("CBX_AUTO_MIN")
            autoMaximum = xDialog.getChild("CBX_AUTO_MAX")
            majorInterval = xDialog.getChild("CBX_AUTO_STEP_MAIN")
            minorInterval = xDialog.getChild("CBX_AUTO_STEP_HELP")
            minimum = xDialog.getChild("EDT_MIN")
            maximum = xDialog.getChild("EDT_MAX")
            major = xDialog.getChild("EDT_STEP_MAIN")
            minor = xDialog.getChild("MT_STEPHELP")

            autoMinimum.executeAction("CLICK", tuple())
            autoMaximum.executeAction("CLICK", tuple())
            majorInterval.executeAction("CLICK", tuple())
            minorInterval.executeAction("CLICK", tuple())
            #In a chart that contains an axis with a date datatype, the UI does not allow specifying
            #a minimum or maximum value greater than 09/04/1900 (i.e., April 9, 1900)
            minimum.executeAction("CLEAR", tuple())
            minimum.executeAction("TYPE", mkPropertyValues({"TEXT":"01.01.2018"}))
            maximum.executeAction("DOWN", tuple()) #29.04.2018
            major.executeAction("UP", tuple())   #21
            minor.executeAction("DOWN", tuple())  #1

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

            autoMinimum = xDialog.getChild("CBX_AUTO_MIN")
            autoMaximum = xDialog.getChild("CBX_AUTO_MAX")
            majorInterval = xDialog.getChild("CBX_AUTO_STEP_MAIN")
            minorInterval = xDialog.getChild("CBX_AUTO_STEP_HELP")
            minimum = xDialog.getChild("EDT_MIN")
            maximum = xDialog.getChild("EDT_MAX")
            major = xDialog.getChild("EDT_STEP_MAIN")
            minor = xDialog.getChild("MT_STEPHELP")

            self.assertEqual(get_state_as_dict(autoMinimum)["Selected"], "false")
            self.assertEqual(get_state_as_dict(autoMaximum)["Selected"], "false")
            self.assertEqual(get_state_as_dict(majorInterval)["Selected"], "false")
            self.assertEqual(get_state_as_dict(minorInterval)["Selected"], "false")
            self.assertEqual(get_state_as_dict(minimum)["Text"], "01.01.2018")
            self.assertEqual(get_state_as_dict(maximum)["Text"], "29.04.2018")
            self.assertEqual(get_state_as_dict(major)["Text"], "21")
            self.assertEqual(get_state_as_dict(minor)["Text"], "1")


# vim: set shiftwidth=4 softtabstop=4 expandtab:
