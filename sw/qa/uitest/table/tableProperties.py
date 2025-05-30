# -*- tab-width: 4; indent-tabs-mode: nil; py-indent-offset: 4 -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
from uitest.framework import UITestCase
from uitest.uihelper.common import select_pos
from uitest.uihelper.common import select_by_text
from libreoffice.uno.propertyvalue import mkPropertyValues
from uitest.uihelper.common import get_state_as_dict, get_url_for_data_file
from uitest.uihelper.common import change_measurement_unit

#Writer Table Properties

class tableProperties(UITestCase):
    def test_table_properties(self):
        with self.ui_test.load_file(get_url_for_data_file("tableToText.odt")):

            with change_measurement_unit(self, "Centimeter"):

                #dialog Table Properties - Table
                with self.ui_test.execute_dialog_through_command(".uno:TableDialog") as xDialog:
                    tabcontrol = xDialog.getChild("tabcontrol")
                    select_pos(tabcontrol, "0")

                    name = xDialog.getChild("name")
                    free = xDialog.getChild("free")
                    widthmf = xDialog.getChild("widthmf")
                    leftmf = xDialog.getChild("leftmf")
                    rightmf = xDialog.getChild("rightmf")
                    abovemf = xDialog.getChild("abovemf")
                    belowmf = xDialog.getChild("belowmf")
                    textdirection = xDialog.getChild("textdirection")

                    name.executeAction("TYPE", mkPropertyValues({"KEYCODE":"CTRL+A"}))
                    name.executeAction("TYPE", mkPropertyValues({"KEYCODE":"BACKSPACE"}))
                    name.executeAction("TYPE", mkPropertyValues({"TEXT":"NewName"}))
                    free.executeAction("CLICK", tuple())
                    widthmf.executeAction("TYPE", mkPropertyValues({"KEYCODE":"CTRL+A"}))
                    widthmf.executeAction("TYPE", mkPropertyValues({"KEYCODE":"BACKSPACE"}))
                    widthmf.executeAction("TYPE", mkPropertyValues({"TEXT":"15"}))
                    leftmf.executeAction("TYPE", mkPropertyValues({"KEYCODE":"CTRL+A"}))
                    leftmf.executeAction("TYPE", mkPropertyValues({"KEYCODE":"BACKSPACE"}))
                    leftmf.executeAction("TYPE", mkPropertyValues({"TEXT":"1"}))
                    rightmf.executeAction("TYPE", mkPropertyValues({"KEYCODE":"CTRL+A"}))
                    rightmf.executeAction("TYPE", mkPropertyValues({"KEYCODE":"BACKSPACE"}))
                    rightmf.executeAction("TYPE", mkPropertyValues({"TEXT":"1"}))
                    abovemf.executeAction("TYPE", mkPropertyValues({"KEYCODE":"CTRL+A"}))
                    abovemf.executeAction("TYPE", mkPropertyValues({"KEYCODE":"BACKSPACE"}))
                    abovemf.executeAction("TYPE", mkPropertyValues({"TEXT":"1"}))
                    belowmf.executeAction("TYPE", mkPropertyValues({"KEYCODE":"CTRL+A"}))
                    belowmf.executeAction("TYPE", mkPropertyValues({"KEYCODE":"BACKSPACE"}))
                    belowmf.executeAction("TYPE", mkPropertyValues({"TEXT":"1"}))
                    select_by_text(textdirection, "Left-to-right (LTR)")
                #verify
                with self.ui_test.execute_dialog_through_command(".uno:TableDialog") as xDialog:
                    tabcontrol = xDialog.getChild("tabcontrol")
                    select_pos(tabcontrol, "0")

                    name = xDialog.getChild("name")
                    free = xDialog.getChild("free")
                    widthmf = xDialog.getChild("widthmf")
                    leftmf = xDialog.getChild("leftmf")
                    rightmf = xDialog.getChild("rightmf")
                    abovemf = xDialog.getChild("abovemf")
                    belowmf = xDialog.getChild("belowmf")
                    textdirection = xDialog.getChild("textdirection")

                    self.assertEqual(get_state_as_dict(name)["Text"], "NewName")
                    self.assertEqual(get_state_as_dict(free)["Checked"], "true")
                    self.assertEqual(get_state_as_dict(widthmf)["Text"], "15.00 cm")
                    self.assertEqual(get_state_as_dict(leftmf)["Text"], "1.00 cm")
                    self.assertEqual(get_state_as_dict(rightmf)["Text"], "1.00 cm")
                    self.assertEqual(get_state_as_dict(abovemf)["Text"], "1.00 cm")
                    self.assertEqual(get_state_as_dict(belowmf)["Text"], "1.00 cm")

                #dialog Table Properties - Text flow
                with self.ui_test.execute_dialog_through_command(".uno:TableDialog") as xDialog:
                    tabcontrol = xDialog.getChild("tabcontrol")
                    select_pos(tabcontrol, "1")

                    xbreak = xDialog.getChild("break")
                    xbreak.executeAction("CLICK", tuple())
                    column = xDialog.getChild("column")
                    column.executeAction("CLICK", tuple())
                    after = xDialog.getChild("after")
                    after.executeAction("CLICK", tuple())
                    keep = xDialog.getChild("keep")
                    keep.executeAction("CLICK", tuple())
                    headline = xDialog.getChild("headline")
                    headline.executeAction("CLICK", tuple())
                    textdirection = xDialog.getChild("textorientation")
                    select_by_text(textdirection, "Vertical (bottom to top)")
                    vertorient = xDialog.getChild("vertorient")
                    select_by_text(vertorient, "Bottom")
                #verify
                with self.ui_test.execute_dialog_through_command(".uno:TableDialog") as xDialog:
                    tabcontrol = xDialog.getChild("tabcontrol")
                    select_pos(tabcontrol, "1")

                    xbreak = xDialog.getChild("break")
                    self.assertEqual(get_state_as_dict(xbreak)["Selected"], "true")
                    column = xDialog.getChild("column")
                    self.assertEqual(get_state_as_dict(column)["Checked"], "true")
                    after = xDialog.getChild("column")
                    self.assertEqual(get_state_as_dict(after)["Checked"], "true")
                    keep = xDialog.getChild("keep")
                    self.assertEqual(get_state_as_dict(keep)["Selected"], "true")
                    headline = xDialog.getChild("headline")
                    self.assertEqual(get_state_as_dict(headline)["Selected"], "true")
                    textdirection = xDialog.getChild("textorientation")
                    self.assertEqual(get_state_as_dict(textdirection)["SelectEntryText"], "Vertical (bottom to top)")
                    vertorient = xDialog.getChild("vertorient")
                    self.assertEqual(get_state_as_dict(vertorient)["SelectEntryText"], "Bottom")

                #dialog Table Properties - Columns
                with self.ui_test.execute_dialog_through_command(".uno:TableDialog") as xDialog:
                    tabcontrol = xDialog.getChild("tabcontrol")
                    select_pos(tabcontrol, "2")

                    adaptwidth = xDialog.getChild("adaptwidth")
                    adaptwidth.executeAction("CLICK", tuple())

                #verify
                #doesn't work / probably Bug 100537 - Width and relative checkboxes disabled in Table
                #dialog by default with automatic alignment
        #        self.ui_test.execute_dialog_through_command(".uno:TableDialog")
        #        xDialog = self.xUITest.getTopFocusWindow()
        #        tabcontrol = xDialog.getChild("tabcontrol")
        #        select_pos(tabcontrol, "2")
        #        adaptwidth = xDialog.getChild("adaptwidth")
        #        self.assertEqual(get_state_as_dict(adaptwidth)["Selected"], "true")
        #        xOKBtn = xDialog.getChild("ok")
        #        self.ui_test.close_dialog_through_button(xOKBtn)

                #dialog Table Properties - Borders
                with self.ui_test.execute_dialog_through_command(".uno:TableDialog") as xDialog:
                    tabcontrol = xDialog.getChild("tabcontrol")
                    select_pos(tabcontrol, "3")

                    sync = xDialog.getChild("sync")
                    mergeadjacent = xDialog.getChild("mergeadjacent")
                    sync.executeAction("CLICK", tuple())
                    mergeadjacent.executeAction("CLICK", tuple())

                #verify
                with self.ui_test.execute_dialog_through_command(".uno:TableDialog") as xDialog:
                    tabcontrol = xDialog.getChild("tabcontrol")
                    select_pos(tabcontrol, "3")
                    sync = xDialog.getChild("sync")
                    mergeadjacent = xDialog.getChild("mergeadjacent")
            #        self.assertEqual(get_state_as_dict(sync)["Selected"], "false") #need change spacing, but ui names are not unique
                    self.assertEqual(get_state_as_dict(mergeadjacent)["Selected"], "false")

                #dialog Table Properties - Background
                with self.ui_test.execute_dialog_through_command(".uno:TableDialog") as xDialog:
                    tabcontrol = xDialog.getChild("tabcontrol")
                    select_pos(tabcontrol, "4")

                    btncolor = xDialog.getChild("btncolor")
                    btncolor.executeAction("CLICK", tuple())
                    R_custom = xDialog.getChild("R_custom")
                    G_custom = xDialog.getChild("G_custom")
                    B_custom = xDialog.getChild("B_custom")
                    R_custom.executeAction("TYPE", mkPropertyValues({"KEYCODE":"CTRL+A"}))
                    R_custom.executeAction("TYPE", mkPropertyValues({"KEYCODE":"BACKSPACE"}))
                    R_custom.executeAction("TYPE", mkPropertyValues({"TEXT":"100"}))
                    G_custom.executeAction("TYPE", mkPropertyValues({"KEYCODE":"CTRL+A"}))
                    G_custom.executeAction("TYPE", mkPropertyValues({"KEYCODE":"BACKSPACE"}))
                    G_custom.executeAction("TYPE", mkPropertyValues({"TEXT":"100"}))
                    B_custom.executeAction("TYPE", mkPropertyValues({"KEYCODE":"CTRL+A"}))
                    B_custom.executeAction("TYPE", mkPropertyValues({"KEYCODE":"BACKSPACE"}))
                    B_custom.executeAction("TYPE", mkPropertyValues({"TEXT":"100"}))
                    B_custom.executeAction("UP", tuple())
                    B_custom.executeAction("DOWN", tuple())  #need to refresh HEX value...

                #verify
                with self.ui_test.execute_dialog_through_command(".uno:TableDialog") as xDialog:
                    tabcontrol = xDialog.getChild("tabcontrol")
                    select_pos(tabcontrol, "4")
                    btncolor = xDialog.getChild("btncolor")
                    btncolor.executeAction("CLICK", tuple())
                    R_custom = xDialog.getChild("R_custom")
                    G_custom = xDialog.getChild("G_custom")
                    B_custom = xDialog.getChild("B_custom")

                    self.assertEqual(get_state_as_dict(R_custom)["Text"], "100")
                    self.assertEqual(get_state_as_dict(B_custom)["Text"], "100")
                    self.assertEqual(get_state_as_dict(G_custom)["Text"], "100")

# vim: set shiftwidth=4 softtabstop=4 expandtab:
