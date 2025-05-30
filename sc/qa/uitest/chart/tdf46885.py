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

from libreoffice.calc.document import get_cell_by_position
from libreoffice.uno.propertyvalue import mkPropertyValues


# Bug 46885 - LibO crash when creating chart with no cells selected
class tdf46885(UITestCase):
    def test_tdf46885_crash_chart_no_cell_selected_nextButton(self):
        with self.ui_test.create_doc_in_start_center("calc") as document:
            xCalcDoc = self.xUITest.getTopFocusWindow()
            gridwin = xCalcDoc.getChild("grid_window")
            enter_text_to_cell(gridwin, "A10", "col1")
            #When you start a new chart and have one empty cell selected LibO will crash when you select the Next>> button.
            with self.ui_test.execute_dialog_through_command(".uno:InsertObjectChart", close_button="finish") as xChartDlg:
                xNextBtn = xChartDlg.getChild("next")
                xNextBtn.executeAction("CLICK", tuple())

            #verify, we didn't crash
            self.assertEqual(get_cell_by_position(document, 0, 0, 9).getString(), "col1")


    def test_tdf46885_crash_chart_multiple_empty_cells_selected(self):
        with self.ui_test.create_doc_in_start_center("calc") as document:
            xCalcDoc = self.xUITest.getTopFocusWindow()
            gridwin = xCalcDoc.getChild("grid_window")
            enter_text_to_cell(gridwin, "A10", "col1")
            #If you select multiple empty cells and then start a new chart LibO will crash immediately.
            gridwin.executeAction("SELECT", mkPropertyValues({"RANGE": "A1:C4"}))
            with self.ui_test.execute_dialog_through_command(".uno:InsertObjectChart", close_button="finish") as xChartDlg:
                xNextBtn = xChartDlg.getChild("next")
                xNextBtn.executeAction("CLICK", tuple())

            #verify, we didn't crash
            self.assertEqual(get_cell_by_position(document, 0, 0, 9).getString(), "col1")

# vim: set shiftwidth=4 softtabstop=4 expandtab:
