# -*- tab-width: 4; indent-tabs-mode: nil; py-indent-offset: 4 -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

from uitest.framework import UITestCase

class tdf105411(UITestCase):

    def test_delete_conditional_format(self):

        with self.ui_test.create_doc_in_start_center("calc"):

            with self.ui_test.execute_modeless_dialog_through_command(".uno:ConditionalFormatDialog", close_button="") as xCondFormatDlg:
                xDeleteBtn = xCondFormatDlg.getChild("delete")
                xDeleteBtn.executeAction("CLICK", tuple())

                xCondFormatDlg = self.xUITest.getTopFocusWindow()

                xDeleteBtn = xCondFormatDlg.getChild("add")
                xDeleteBtn.executeAction("CLICK", tuple())

                xCondFormatDlg = self.xUITest.getTopFocusWindow()

                xOkBtn = xCondFormatDlg.getChild("ok")
                self.ui_test.close_dialog_through_button(xOkBtn)

# vim: set shiftwidth=4 softtabstop=4 expandtab:
