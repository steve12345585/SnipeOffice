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


class zTest(UITestCase):
    def test_zTest_column(self):
        with self.ui_test.create_doc_in_start_center("calc") as document:
            xCalcDoc = self.xUITest.getTopFocusWindow()
            gridwin = xCalcDoc.getChild("grid_window")
            #fill data
            enter_text_to_cell(gridwin, "A1", "28")
            enter_text_to_cell(gridwin, "A2", "26")
            enter_text_to_cell(gridwin, "A3", "31")
            enter_text_to_cell(gridwin, "A4", "23")
            enter_text_to_cell(gridwin, "A5", "20")
            enter_text_to_cell(gridwin, "A6", "27")
            enter_text_to_cell(gridwin, "A7", "28")
            enter_text_to_cell(gridwin, "A8", "14")
            enter_text_to_cell(gridwin, "A9", "4")
            enter_text_to_cell(gridwin, "A10", "0")
            enter_text_to_cell(gridwin, "A11", "2")
            enter_text_to_cell(gridwin, "A12", "8")
            enter_text_to_cell(gridwin, "A13", "9")

            enter_text_to_cell(gridwin, "B1", "19")
            enter_text_to_cell(gridwin, "B2", "13")
            enter_text_to_cell(gridwin, "B3", "12")
            enter_text_to_cell(gridwin, "B4", "5")
            enter_text_to_cell(gridwin, "B5", "34")
            enter_text_to_cell(gridwin, "B6", "31")
            enter_text_to_cell(gridwin, "B7", "31")
            enter_text_to_cell(gridwin, "B8", "12")
            enter_text_to_cell(gridwin, "B9", "24")
            enter_text_to_cell(gridwin, "B10", "23")
            enter_text_to_cell(gridwin, "B11", "19")
            enter_text_to_cell(gridwin, "B12", "10")
            enter_text_to_cell(gridwin, "B13", "33")

            gridwin.executeAction("SELECT", mkPropertyValues({"RANGE": "A1:B13"}))
            with self.ui_test.execute_modeless_dialog_through_command(".uno:ZTestDialog") as xDialog:
                xvariable1rangeedit = xDialog.getChild("variable1-range-edit")
                xvariable2rangeedit = xDialog.getChild("variable2-range-edit")
                xoutputrangeedit = xDialog.getChild("output-range-edit")
                xgroupedbycolumnsradio = xDialog.getChild("groupedby-columns-radio")

                xvariable1rangeedit.executeAction("TYPE", mkPropertyValues({"KEYCODE":"CTRL+A"}))
                xvariable1rangeedit.executeAction("TYPE", mkPropertyValues({"KEYCODE":"BACKSPACE"}))
                xvariable1rangeedit.executeAction("TYPE", mkPropertyValues({"TEXT":"$Sheet1.$A$1:$A$13"}))
                xvariable2rangeedit.executeAction("TYPE", mkPropertyValues({"KEYCODE":"CTRL+A"}))
                xvariable2rangeedit.executeAction("TYPE", mkPropertyValues({"KEYCODE":"BACKSPACE"}))
                xvariable2rangeedit.executeAction("TYPE", mkPropertyValues({"TEXT":"$Sheet1.$B$1:$B$13"}))
                xoutputrangeedit.executeAction("TYPE", mkPropertyValues({"KEYCODE":"CTRL+A"}))
                xoutputrangeedit.executeAction("TYPE", mkPropertyValues({"KEYCODE":"BACKSPACE"}))
                xoutputrangeedit.executeAction("TYPE", mkPropertyValues({"TEXT":"F1"}))
                xgroupedbycolumnsradio.executeAction("CLICK", tuple())
            #Verify
            self.assertEqual(get_cell_by_position(document, 0, 5, 0).getString(), "z-test")
            self.assertEqual(get_cell_by_position(document, 0, 5, 1).getString(), "Alpha")

            self.assertEqual(get_cell_by_position(document, 0, 5, 2).getString(), "Hypothesized Mean Difference")
            self.assertEqual(get_cell_by_position(document, 0, 5, 4).getString(), "Known Variance")
            self.assertEqual(get_cell_by_position(document, 0, 5, 5).getString(), "Mean")
            self.assertEqual(get_cell_by_position(document, 0, 5, 6).getString(), "Observations")
            self.assertEqual(get_cell_by_position(document, 0, 5, 7).getString(), "Observed Mean Difference")
            self.assertEqual(get_cell_by_position(document, 0, 5, 8).getString(), "z")
            self.assertEqual(get_cell_by_position(document, 0, 5, 9).getString(), "P (Z<=z) one-tail")
            self.assertEqual(get_cell_by_position(document, 0, 5, 10).getString(), "z Critical one-tail")
            self.assertEqual(get_cell_by_position(document, 0, 5, 11).getString(), "P (Z<=z) two-tail")
            self.assertEqual(get_cell_by_position(document, 0, 5, 12).getString(), "z Critical two-tail")

            self.assertEqual(get_cell_by_position(document, 0, 6, 1).getValue(), 0.05)
            self.assertEqual(get_cell_by_position(document, 0, 6, 2).getValue(), 0)
            self.assertEqual(get_cell_by_position(document, 0, 6, 3).getString(), "Variable 1")
            self.assertEqual(get_cell_by_position(document, 0, 6, 4).getValue(), 0)
            self.assertEqual(round(get_cell_by_position(document, 0, 6, 5).getValue(),12), 16.923076923077)
            self.assertEqual(round(get_cell_by_position(document, 0, 6, 10).getValue(),12), 1.644853626951)
            self.assertEqual(round(get_cell_by_position(document, 0, 6, 12).getValue(),12), 1.959963984540)

            self.assertEqual(get_cell_by_position(document, 0, 7, 3).getString(), "Variable 2")
            self.assertEqual(get_cell_by_position(document, 0, 7, 4).getValue(), 0)
            self.assertEqual(round(get_cell_by_position(document, 0, 7, 5).getValue(),12), 20.461538461538)
            self.assertEqual(get_cell_by_position(document, 0, 7, 6).getValue(), 13)
            #undo
            self.xUITest.executeCommand(".uno:Undo")
            self.assertEqual(get_cell_by_position(document, 0, 5, 0).getString(), "")

            # test cancel button
            with self.ui_test.execute_modeless_dialog_through_command(".uno:ZTestDialog", close_button="cancel"):
                pass

# vim: set shiftwidth=4 softtabstop=4 expandtab:
