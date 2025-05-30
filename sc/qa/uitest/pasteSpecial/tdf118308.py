# -*- tab-width: 4; indent-tabs-mode: nil; py-indent-offset: 4 -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
from uitest.framework import UITestCase
from libreoffice.uno.propertyvalue import mkPropertyValues
from uitest.uihelper.calc import enter_text_to_cell
from libreoffice.calc.document import get_cell_by_position
from libreoffice.calc.paste_special import reset_default_values

class tdf118308(UITestCase):

    def test_tdf118308(self):
        with self.ui_test.create_doc_in_start_center("calc"):
            xCalcDoc = self.xUITest.getTopFocusWindow()
            gridwin = xCalcDoc.getChild("grid_window")

            enter_text_to_cell(gridwin, "A1", "A")
            gridwin.executeAction("SELECT", mkPropertyValues({"CELL": "A1"}))
            self.xUITest.executeCommand(".uno:Copy")

        with self.ui_test.load_empty_file("calc") as calc_document:

            xCalcDoc = self.xUITest.getTopFocusWindow()
            gridwin = xCalcDoc.getChild("grid_window")

            gridwin.executeAction("SELECT", mkPropertyValues({"CELL": "A1"}))

            with self.ui_test.execute_dialog_through_command(".uno:PasteSpecial") as xDialog:

                # Without the fix in place, this test would have failed here
                # since a different dialog would have been opened and the children
                # wouldn't have been found
                reset_default_values(self, xDialog)

            self.assertEqual("A", get_cell_by_position(calc_document, 0, 0, 0).getString())

# vim: set shiftwidth=4 softtabstop=4 expandtab:
