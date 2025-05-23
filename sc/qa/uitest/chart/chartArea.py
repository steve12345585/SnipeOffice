# -*- tab-width: 4; indent-tabs-mode: nil; py-indent-offset: 4 -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
from uitest.framework import UITestCase
from uitest.uihelper.common import change_measurement_unit, select_pos
from uitest.uihelper.common import get_state_as_dict, get_url_for_data_file

from libreoffice.uno.propertyvalue import mkPropertyValues


# Chart Area dialog
class chartArea(UITestCase):
   def test_chart_area_dialog(self):
    with self.ui_test.load_file(get_url_for_data_file("chartArea.ods")) as calc_doc:

        with change_measurement_unit(self, "Centimeter"):
            xCalcDoc = self.xUITest.getTopFocusWindow()
            gridwin = xCalcDoc.getChild("grid_window")

            xArea = calc_doc.Sheets[0].Charts[0].getEmbeddedObject().Area
            self.assertEqual(xArea.LineWidth, 0)
            self.assertEqual(xArea.LineTransparence, 0)
            self.assertEqual(hex(xArea.FillColor), '0xffffff')
            self.assertEqual(xArea.FillTransparence, 0)

            gridwin.executeAction("SELECT", mkPropertyValues({"OBJECT": "Object 1"}))
            gridwin.executeAction("ACTIVATE", tuple())
            xChartMainTop = self.xUITest.getTopFocusWindow()
            xChartMain = xChartMainTop.getChild("chart_window")
            xSeriesObj =  xChartMain.getChild("CID/D=0:CS=0:CT=0:Series=0")
            with self.ui_test.execute_dialog_through_action(xSeriesObj, "COMMAND", mkPropertyValues({"COMMAND": "DiagramArea"})) as xDialog:
                #Click on tab "Borders".
                tabcontrol = xDialog.getChild("tabcontrol")
                select_pos(tabcontrol, "0")

                xWidth = xDialog.getChild("MTR_FLD_LINE_WIDTH")
                transparency = xDialog.getChild("MTR_LINE_TRANSPARENT")

                xWidth.executeAction("UP", tuple())
                transparency.executeAction("UP", tuple())


            self.assertEqual(xArea.LineWidth, 100)
            self.assertEqual(xArea.LineTransparence, 5)
            self.assertEqual(hex(xArea.FillColor), '0xffffff')
            self.assertEqual(xArea.FillTransparence, 0)

            #reopen and verify tab "Borders".
            gridwin.executeAction("SELECT", mkPropertyValues({"OBJECT": "Object 1"}))
            gridwin.executeAction("ACTIVATE", tuple())
            xChartMainTop = self.xUITest.getTopFocusWindow()
            xChartMain = xChartMainTop.getChild("chart_window")
            xSeriesObj =  xChartMain.getChild("CID/D=0:CS=0:CT=0:Series=0")
            with self.ui_test.execute_dialog_through_action(xSeriesObj, "COMMAND", mkPropertyValues({"COMMAND": "DiagramArea"})) as xDialog:

                tabcontrol = xDialog.getChild("tabcontrol")
                select_pos(tabcontrol, "0")
                xWidth = xDialog.getChild("MTR_FLD_LINE_WIDTH")
                transparency = xDialog.getChild("MTR_LINE_TRANSPARENT")

                self.assertEqual(get_state_as_dict(xWidth)["Text"], "0.10 cm")
                self.assertEqual(get_state_as_dict(transparency)["Text"], "5%")

                #Click on tab "Area"
                tabcontrol = xDialog.getChild("tabcontrol")
                select_pos(tabcontrol, "1")

                btncolor = xDialog.getChild("btncolor")
                btncolor.executeAction("CLICK", tuple())

                rCustom = xDialog.getChild("R_custom")
                gCustom = xDialog.getChild("G_custom")
                bCustom = xDialog.getChild("B_custom")

                rCustom.executeAction("CLEAR", tuple())
                rCustom.executeAction("TYPE", mkPropertyValues({"TEXT":"35"}))
                rCustom.executeAction("UP", tuple())
                rCustom.executeAction("DOWN", tuple())  #without this save data doesn't works
                self.assertEqual(get_state_as_dict(rCustom)["Text"], "35")
                gCustom.executeAction("CLEAR", tuple())
                gCustom.executeAction("TYPE", mkPropertyValues({"TEXT":"169"}))
                gCustom.executeAction("UP", tuple())
                gCustom.executeAction("DOWN", tuple())  #without this save data doesn't works
                bCustom.executeAction("CLEAR", tuple())
                bCustom.executeAction("TYPE", mkPropertyValues({"TEXT":"211"}))
                bCustom.executeAction("UP", tuple())
                bCustom.executeAction("DOWN", tuple())  #without this save data doesn't works

            self.assertEqual(xArea.LineWidth, 100)
            self.assertEqual(xArea.LineTransparence, 5)
            self.assertEqual(hex(xArea.FillColor), '0x23a9d3')
            self.assertEqual(xArea.FillTransparence, 0)

            #reopen and verify tab "Area".
            gridwin.executeAction("SELECT", mkPropertyValues({"OBJECT": "Object 1"}))
            gridwin.executeAction("ACTIVATE", tuple())
            xChartMainTop = self.xUITest.getTopFocusWindow()
            xChartMain = xChartMainTop.getChild("chart_window")
            xSeriesObj =  xChartMain.getChild("CID/D=0:CS=0:CT=0:Series=0")
            with self.ui_test.execute_dialog_through_action(xSeriesObj, "COMMAND", mkPropertyValues({"COMMAND": "DiagramArea"})) as xDialog:

                tabcontrol = xDialog.getChild("tabcontrol")
                select_pos(tabcontrol, "1")

                rCustom = xDialog.getChild("R_custom")
                gCustom = xDialog.getChild("G_custom")
                bCustom = xDialog.getChild("B_custom")

                self.assertEqual(get_state_as_dict(rCustom)["Text"], "35")
                self.assertEqual(get_state_as_dict(gCustom)["Text"], "169")
                self.assertEqual(get_state_as_dict(bCustom)["Text"], "211")

                #change tab "Transparency"
                select_pos(tabcontrol, "2")
                transparency = xDialog.getChild("RBT_TRANS_LINEAR")
                transparencyPercent = xDialog.getChild("MTR_TRANSPARENT")  #51%

                transparency.executeAction("CLICK", tuple())
                transparencyPercent.executeAction("UP", tuple())


            self.assertEqual(xArea.LineWidth, 100)
            self.assertEqual(xArea.LineTransparence, 5)
            self.assertEqual(hex(xArea.FillColor), '0x23a9d3')
            self.assertEqual(xArea.FillTransparence, 51)

            #reopen and verify tab "Transparency"
            gridwin.executeAction("SELECT", mkPropertyValues({"OBJECT": "Object 1"}))
            gridwin.executeAction("ACTIVATE", tuple())
            xChartMainTop = self.xUITest.getTopFocusWindow()
            xChartMain = xChartMainTop.getChild("chart_window")
            xSeriesObj =  xChartMain.getChild("CID/D=0:CS=0:CT=0:Series=0")
            with self.ui_test.execute_dialog_through_action(xSeriesObj, "COMMAND", mkPropertyValues({"COMMAND": "DiagramArea"})) as xDialog:

                tabcontrol = xDialog.getChild("tabcontrol")
                select_pos(tabcontrol, "2")

                transparency = xDialog.getChild("RBT_TRANS_LINEAR")
                transparencyPercent = xDialog.getChild("MTR_TRANSPARENT")  #51%

                self.assertEqual(get_state_as_dict(transparency)["Checked"], "true")
                self.assertEqual(get_state_as_dict(transparencyPercent)["Text"], "51%")


            self.assertEqual(xArea.LineWidth, 100)
            self.assertEqual(xArea.LineTransparence, 5)
            self.assertEqual(hex(xArea.FillColor), '0x23a9d3')
            self.assertEqual(xArea.FillTransparence, 51)

# vim: set shiftwidth=4 softtabstop=4 expandtab:
