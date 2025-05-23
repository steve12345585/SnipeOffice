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
from libreoffice.uno.propertyvalue import mkPropertyValues
#uitest sc / AutoFormat Styles

class autoFormat(UITestCase):
    def test_autoformat_styles(self):
        with self.ui_test.create_doc_in_start_center("calc"):
            xCalcDoc = self.xUITest.getTopFocusWindow()
            gridwin = xCalcDoc.getChild("grid_window")
            #select A1:C5
            gridwin.executeAction("SELECT", mkPropertyValues({"RANGE": "A1:C5"}))
            #AutoFormat Styles
            with self.ui_test.execute_dialog_through_command(".uno:AutoFormat") as xDialog:

                formatlb = xDialog.getChild("formatlb")
                numformatcb = xDialog.getChild("numformatcb")
                bordercb = xDialog.getChild("bordercb")
                fontcb = xDialog.getChild("fontcb")
                patterncb = xDialog.getChild("patterncb")
                alignmentcb = xDialog.getChild("alignmentcb")
                autofitcb = xDialog.getChild("autofitcb")

                entry = formatlb.getChild("7") #Financial
                entry.executeAction("SELECT", tuple())
                numformatcb.executeAction("CLICK", tuple())
                bordercb.executeAction("CLICK", tuple())
                fontcb.executeAction("CLICK", tuple())
                patterncb.executeAction("CLICK", tuple())
                alignmentcb.executeAction("CLICK", tuple())
                autofitcb.executeAction("CLICK", tuple())


            #verify
            with self.ui_test.execute_dialog_through_command(".uno:AutoFormat") as xDialog:

                formatlb = xDialog.getChild("formatlb")
                numformatcb = xDialog.getChild("numformatcb")
                bordercb = xDialog.getChild("bordercb")
                fontcb = xDialog.getChild("fontcb")
                patterncb = xDialog.getChild("patterncb")
                alignmentcb = xDialog.getChild("alignmentcb")
                autofitcb = xDialog.getChild("autofitcb")

                entry = formatlb.getChild("7") #Financial
                entry.executeAction("SELECT", tuple())
                self.assertEqual(get_state_as_dict(numformatcb)["Selected"], "false")
                self.assertEqual(get_state_as_dict(bordercb)["Selected"], "false")
                self.assertEqual(get_state_as_dict(fontcb)["Selected"], "false")
                self.assertEqual(get_state_as_dict(patterncb)["Selected"], "false")
                self.assertEqual(get_state_as_dict(alignmentcb)["Selected"], "false")
                self.assertEqual(get_state_as_dict(autofitcb)["Selected"], "false")
                numformatcb.executeAction("CLICK", tuple())
                bordercb.executeAction("CLICK", tuple())
                fontcb.executeAction("CLICK", tuple())
                patterncb.executeAction("CLICK", tuple())
                alignmentcb.executeAction("CLICK", tuple())
                autofitcb.executeAction("CLICK", tuple())


# vim: set shiftwidth=4 softtabstop=4 expandtab:
