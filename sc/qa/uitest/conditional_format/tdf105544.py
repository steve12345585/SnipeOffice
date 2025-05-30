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

from libreoffice.uno.propertyvalue import mkPropertyValues


# Bug 105544 - Manage Conditional Formatting is not able to edit a condition
class tdf105544(UITestCase):
    def test_tdf105544_Manage_Conditional_Formatting_edit_condition(self):

        with self.ui_test.load_file(get_url_for_data_file("tdf105544.ods")):
            xCalcDoc = self.xUITest.getTopFocusWindow()
            gridwin = xCalcDoc.getChild("grid_window")
            #2. select B3. Format> conditional formatting> manage
            gridwin.executeAction("SELECT", mkPropertyValues({"CELL": "B3"}))
            with self.ui_test.execute_dialog_through_command(".uno:ConditionalFormatManagerDialog", close_button="") as xCondFormatMgr:

                # check that we have exactly four conditional formats in the beginning
                xList = xCondFormatMgr.getChild("CONTAINER")
                list_state = get_state_as_dict(xList)
                self.assertEqual(list_state['Children'], '4')

                #select B3:B37 range and click edit, then click yes
                xList.executeAction("TYPE", mkPropertyValues({"KEYCODE": "DOWN"}))  #2nd position in the list
                xEditBtn = xCondFormatMgr.getChild("edit")
                with self.ui_test.execute_dialog_through_action(xEditBtn, "CLICK", event_name = "ModelessDialogVisible"):
                    pass

                # we need to get a pointer again as the old window has been deleted
                xCondFormatMgr = self.xUITest.getTopFocusWindow()

                # check again that we still have 4 entry in the list
                xList = xCondFormatMgr.getChild("CONTAINER")
                list_state = get_state_as_dict(xList)
                self.assertEqual(list_state['Children'], '4')

                # close the conditional format manager
                xOKBtn = xCondFormatMgr.getChild("ok")
                self.ui_test.close_dialog_through_button(xOKBtn)

# vim: set shiftwidth=4 softtabstop=4 expandtab:
