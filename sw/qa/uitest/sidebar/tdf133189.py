# -*- tab-width: 4; indent-tabs-mode: nil; py-indent-offset: 4 -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
from uitest.framework import UITestCase
from uitest.uihelper.common import get_state_as_dict, select_pos
from uitest.uihelper.common import change_measurement_unit
from libreoffice.uno.propertyvalue import mkPropertyValues

class tdf133189(UITestCase):
    def test_tdf133189(self):
        with self.ui_test.create_doc_in_start_center("writer"):

            xWriterDoc = self.xUITest.getTopFocusWindow()
            xWriterEdit = xWriterDoc.getChild("writer_edit")

            self.xUITest.executeCommand(".uno:Sidebar")
            xWriterEdit.executeAction("SIDEBAR", mkPropertyValues({"PANEL": "PageStylesPanel"}))

            xPaperSize = xWriterEdit.getChild('papersize')
            xPaperWidth = xWriterEdit.getChild('paperwidth')
            xPaperHeight = xWriterEdit.getChild('paperheight')
            xPaperOrient = xWriterEdit.getChild('paperorientation')
            xPaperMargin = xWriterEdit.getChild('marginLB')

            with self.ui_test.execute_dialog_through_command(".uno:PageDialog") as xDialog:
                tabcontrol = xDialog.getChild("tabcontrol")
                select_pos(tabcontrol, "1")

                xWidth = xDialog.getChild('spinWidth')
                xHeight = xDialog.getChild('spinHeight')

                props = {"VALUE": '8.0'}
                actionProps = mkPropertyValues(props)

                xWidth.executeAction("VALUE", actionProps)
                xHeight.executeAction("VALUE", actionProps)


            self.ui_test.wait_until_property_is_updated(xPaperMargin, "SelectEntryText", "Normal (0.75″)")
            self.assertEqual(get_state_as_dict(xPaperMargin)['SelectEntryText'], "Normal (0.75″)")
            self.ui_test.wait_until_property_is_updated(xPaperSize, "SelectEntryText", "User")
            self.assertEqual(get_state_as_dict(xPaperSize)['SelectEntryText'], "User")
            self.ui_test.wait_until_property_is_updated(xPaperOrient, "SelectEntryText", "Portrait")
            self.assertEqual(get_state_as_dict(xPaperOrient)['SelectEntryText'], "Portrait")
            self.ui_test.wait_until_property_is_updated(xPaperWidth, "Text", "8.00″")
            self.assertEqual(get_state_as_dict(xPaperWidth)['Text'], "8.00″")
            self.ui_test.wait_until_property_is_updated(xPaperHeight, "Text", "8.00″")
            self.assertEqual(get_state_as_dict(xPaperHeight)['Text'], "8.00″")

            with change_measurement_unit(self, 'Centimeter'):

                self.ui_test.wait_until_property_is_updated(xPaperMargin, "SelectEntryText", "Normal (1.90 cm)")
                # tdf#129267
                self.assertEqual(get_state_as_dict(xPaperMargin)['SelectEntryText'], "Normal (1.90 cm)")
                self.ui_test.wait_until_property_is_updated(xPaperSize, "SelectEntryText", "User")
                self.assertEqual(get_state_as_dict(xPaperSize)['SelectEntryText'], "User")
                self.ui_test.wait_until_property_is_updated(xPaperOrient, "SelectEntryText", "Portrait")
                self.assertEqual(get_state_as_dict(xPaperOrient)['SelectEntryText'], "Portrait")
                self.ui_test.wait_until_property_is_updated(xPaperWidth, "Text", "20.32 cm")
                self.assertEqual(get_state_as_dict(xPaperWidth)['Text'], "20.32 cm")
                self.ui_test.wait_until_property_is_updated(xPaperHeight, "Text", "20.32 cm")
                self.assertEqual(get_state_as_dict(xPaperHeight)['Text'], "20.32 cm")

            self.xUITest.executeCommand(".uno:Sidebar")

# vim: set shiftwidth=4 softtabstop=4 expandtab:
