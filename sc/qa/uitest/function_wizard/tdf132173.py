# -*- tab-width: 4; indent-tabs-mode: nil; py-indent-offset: 4 -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
from uitest.framework import UITestCase
from uitest.uihelper.common import get_state_as_dict
from libreoffice.calc.document import get_cell_by_position
from libreoffice.uno.propertyvalue import mkPropertyValues

class tdf132173(UITestCase):
    def test_tdf132173(self):
        with self.ui_test.create_doc_in_start_center("calc") as document:

            with self.ui_test.execute_modeless_dialog_through_command(".uno:FunctionDialog") as xDialog:
                xFormula = xDialog.getChild("ed_formula")
                xFormula.executeAction("TYPE", mkPropertyValues({"TEXT": 'FIND({"A";"B";"C"},"SAMPLE TEXT")'}))

                self.assertEqual(get_state_as_dict(xFormula)["Text"], '=FIND({"A";"B";"C"},"SAMPLE TEXT")')



            #Without the fix in place, cell's value would have been #NAME?
            self.assertEqual(get_cell_by_position(document, 0, 0, 0).getString(), "2")


# vim: set shiftwidth=4 softtabstop=4 expandtab:
