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
from uitest.uihelper.common import get_state_as_dict

class tdf140863(UITestCase):

    def test_tdf140863(self):

        with self.ui_test.create_doc_in_start_center("writer") as document:

            # Insert one section
            with self.ui_test.execute_dialog_through_command(".uno:InsertSection"):
                pass

            xWriterDoc = self.xUITest.getTopFocusWindow()
            xWriterEdit = xWriterDoc.getChild("writer_edit")

            # Insert a page break in the section
            xWriterEdit.executeAction("TYPE", mkPropertyValues({"KEYCODE": "UP"}))
            self.xUITest.executeCommand(".uno:InsertPagebreak")
            self.assertEqual(get_state_as_dict(xWriterEdit)["CurrentPage"], "2")

            self.assertEqual(1, len(document.TextSections))
            self.assertTrue(document.TextSections.Section1.IsVisible)


            with self.ui_test.execute_dialog_through_command(".uno:EditRegion") as xDialog:
                xHide = xDialog.getChild('hide')
                self.assertEqual('false', get_state_as_dict(xHide)['Selected'])

                xHide.executeAction('CLICK', tuple())


            self.assertEqual(1, len(document.TextSections))
            self.assertFalse(document.TextSections.Section1.IsVisible)
            self.assertEqual(get_state_as_dict(xWriterEdit)["CurrentPage"], "1")

            with self.ui_test.execute_dialog_through_command(".uno:EditRegion") as xDialog:
                xHide = xDialog.getChild('hide')
                self.assertEqual('true', get_state_as_dict(xHide)['Selected'])

                xHide.executeAction('CLICK', tuple())

            self.assertEqual(1, len(document.TextSections))
            self.assertTrue(document.TextSections.Section1.IsVisible)

            # Without the fix in place, this test would have failed with
            # AssertionError: '1' != '2'
            self.assertEqual(get_state_as_dict(xWriterEdit)["CurrentPage"], "2")


# vim: set shiftwidth=4 softtabstop=4 expandtab:
