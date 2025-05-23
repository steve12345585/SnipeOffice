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

class tdf104378(UITestCase):

    def test_tdf104378(self):
        with self.ui_test.create_doc_in_start_center("calc") as document:
            xCalcDoc = self.xUITest.getTopFocusWindow()
            gridwin = xCalcDoc.getChild("grid_window")

            enter_text_to_cell(gridwin, "A1", "遠程MySQL連接遠程MySQL連接遠程MySQL連接")

            with self.ui_test.execute_dialog_through_command(".uno:ChineseConversion"):
                pass


            # Without the fix in place, this test would have failed with
            # - 远程MySQL連接遠程MySQL連接遠程MySQL連接
            # + 远程MySQL连接远程MySQL连接远程MySQL连接
            self.assertEqual(get_cell_by_position(
                document, 0, 0, 0).getString(), "远程MySQL连接远程MySQL连接远程MySQL连接")


# vim: set shiftwidth=4 softtabstop=4 expandtab:
