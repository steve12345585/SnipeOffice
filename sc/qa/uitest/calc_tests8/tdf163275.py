# -*- tab-width: 4; indent-tabs-mode: nil; py-indent-offset: 4 -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

from uitest.framework import UITestCase

from uitest.uihelper.common import type_text
from libreoffice.calc.document import get_cell_by_position
from libreoffice.uno.propertyvalue import mkPropertyValues

class tdf163275(UITestCase):

    def test_tdf163275(self):

        with self.ui_test.create_doc_in_start_center("calc") as document:

            calcDoc = self.xUITest.getTopFocusWindow()
            gridwin = calcDoc.getChild("grid_window")

            gridwin.executeAction("SELECT", mkPropertyValues({"CELL": "A1"}))
            type_text(gridwin, "-(!1)")

            # Without the fix in place, this test would have crashed
            with self.ui_test.execute_blocking_action(gridwin.executeAction,
                    args=("TYPE", mkPropertyValues({"KEYCODE": "RETURN"})), close_button="no"):
                pass

            self.assertEqual("-(!1)", get_cell_by_position(document, 0, 0, 0).getString())

            self.xUITest.executeCommand(".uno:Undo")

            gridwin.executeAction("SELECT", mkPropertyValues({"CELL": "A1"}))
            type_text(gridwin, "-(!1)")

            with self.ui_test.execute_blocking_action(gridwin.executeAction,
                    args=("TYPE", mkPropertyValues({"KEYCODE": "RETURN"})), close_button="yes"):
                pass

            self.assertEqual("-1", get_cell_by_position(document, 0, 0, 0).getString())

# vim: set shiftwidth=4 softtabstop=4 expandtab:
