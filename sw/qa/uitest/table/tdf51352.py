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

class tdf51352(UITestCase):

    def test_convert_nested_table_to_text_tdf51352(self):

        with self.ui_test.create_doc_in_start_center("writer") as document:
            xWriterDoc = self.xUITest.getTopFocusWindow()
            xWriterEdit = xWriterDoc.getChild("writer_edit")
            #1. New a text document
            #2. Insert a table and then in one cell create another table
            #-insert a table (by default 2x2)
            with self.ui_test.execute_dialog_through_command(".uno:InsertTable"):
                pass
            #-put the cursor inside first cell top left for example
            #-insert an inner table (by default 2x2) inside this cell
            with self.ui_test.execute_dialog_through_command(".uno:InsertTable"):
                pass
            #3. Select the first table
            xWriterEdit.executeAction("TYPE", mkPropertyValues({"KEYCODE": "RIGHT"}))
            xWriterEdit.executeAction("TYPE", mkPropertyValues({"KEYCODE": "RIGHT"}))
            xWriterEdit.executeAction("TYPE", mkPropertyValues({"KEYCODE": "RIGHT"}))
            xWriterEdit.executeAction("TYPE", mkPropertyValues({"KEYCODE": "RIGHT"}))
            xWriterEdit.executeAction("TYPE", mkPropertyValues({"KEYCODE": "RIGHT"}))
            self.assertEqual(len(document.TextTables), 2)
            #4. From menu "Table->Convert->Table to Text
            with self.ui_test.execute_dialog_through_command(".uno:ConvertTableToText"):
                pass
            self.assertEqual(len(document.TextTables), 0)
            #5. Undo/Redo
            self.xUITest.executeCommand(".uno:Undo")
            self.assertEqual(len(document.TextTables), 2)
            self.xUITest.executeCommand(".uno:Redo")
            self.assertEqual(len(document.TextTables), 0)

# vim: set shiftwidth=4 softtabstop=4 expandtab:
