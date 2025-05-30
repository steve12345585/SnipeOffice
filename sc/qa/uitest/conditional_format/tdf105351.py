# -*- tab-width: 4; indent-tabs-mode: nil; py-indent-offset: 4 -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
from uitest.framework import UITestCase
from uitest.uihelper.calc import enter_text_to_cell
from uitest.uihelper.common import get_state_as_dict, select_by_text

from libreoffice.uno.propertyvalue import mkPropertyValues


# Bug 105351 - FORMATTING: Unable to change Data Bar conditional formatting
class tdf105351(UITestCase):
    def test_tdf105351_cond_format_data_bar(self):
        with self.ui_test.create_doc_in_start_center("calc"):
            xCalcDoc = self.xUITest.getTopFocusWindow()
            gridwin = xCalcDoc.getChild("grid_window")
            #2. Set the value of cell A1 to 5
            enter_text_to_cell(gridwin, "A1", "5")
            #3. Select cell A1, then choose from the menus Format -> Conditional Formatting -> Data Bar
            gridwin.executeAction("SELECT", mkPropertyValues({"CELL": "A1"}))
            with self.ui_test.execute_modeless_dialog_through_command(".uno:DataBarFormatDialog") as xDialog:
                xcolscalemin = xDialog.getChild("colscalemin")
                xedcolscalemin = xDialog.getChild("edcolscalemin")
                xcolscalemax = xDialog.getChild("colscalemax")
                xedcolscalemax = xDialog.getChild("edcolscalemax")
                #4. In the conditional formatting window, select the dropdown on the left that says "Automatic"
                #and change it to "Value". In the text field below it enter 0.
                #5. Select the dropdown on the right that says "Automatic" and change it to "Value". In the text field below it enter 10
                select_by_text(xcolscalemin, "Value")
                xedcolscalemin.executeAction("TYPE", mkPropertyValues({"TEXT":"0"}))

                select_by_text(xcolscalemax, "Value")
                xedcolscalemax.executeAction("TYPE", mkPropertyValues({"TEXT":"10"}))

            #Verify : The conditions set for cell A1 should be shown and editable.
            with self.ui_test.execute_modeless_dialog_through_command(".uno:DataBarFormatDialog") as xDialog:
                xcolscalemin = xDialog.getChild("colscalemin")
                xedcolscalemin = xDialog.getChild("edcolscalemin")
                xcolscalemax = xDialog.getChild("colscalemax")
                xedcolscalemax = xDialog.getChild("edcolscalemax")

                self.assertEqual(get_state_as_dict(xcolscalemin)["SelectEntryText"], "Value")
                self.assertEqual(get_state_as_dict(xedcolscalemin)["Text"], "0")
                self.assertEqual(get_state_as_dict(xcolscalemax)["SelectEntryText"], "Value")
                self.assertEqual(get_state_as_dict(xedcolscalemax)["Text"], "10")
                #editable - change value and then verify
                xedcolscalemax.executeAction("TYPE", mkPropertyValues({"TEXT":"1"}))
                self.assertEqual(get_state_as_dict(xedcolscalemax)["Text"], "110")



# vim: set shiftwidth=4 softtabstop=4 expandtab:
