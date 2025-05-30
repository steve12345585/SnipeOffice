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

class Comments(UITestCase):
    def test_comment(self):
        with self.ui_test.create_doc_in_start_center("calc"):
            xCalcDoc = self.xUITest.getTopFocusWindow()
            gridwin = xCalcDoc.getChild("grid_window")

            # Select Cell D8
            gridwin.executeAction("SELECT", mkPropertyValues({"TABLE": "0"}))
            gridwin.executeAction("SELECT", mkPropertyValues({"CELL": "D8"}))

            # Create comment and open it's window
            gridwin.executeAction("COMMENT", mkPropertyValues({"OPEN": ""}))

            # Write text in the Comment Window
            gridwin.executeAction("TYPE", mkPropertyValues({"TEXT": "First Comment"}))

            # Close Comment Window
            gridwin.executeAction("COMMENT", mkPropertyValues({"CLOSE":""}))

            # Check on the comment text
            self.assertEqual(get_state_as_dict(gridwin)["CurrentCellCommentText"], "First Comment")

            # Check on comment in another cell
            gridwin.executeAction("SELECT", mkPropertyValues({"CELL": "A1"}))
            gridwin.executeAction("COMMENT", mkPropertyValues({"OPEN": ""}))
            gridwin.executeAction("TYPE", mkPropertyValues({"TEXT": "Second Comment"}))
            gridwin.executeAction("COMMENT", mkPropertyValues({"CLOSE":""}))
            self.assertEqual(get_state_as_dict(gridwin)["CurrentCellCommentText"], "Second Comment")

            # Write Comment without opening Comment window
            gridwin.executeAction("SELECT", mkPropertyValues({"CELL": "B5"}))
            gridwin.executeAction("COMMENT", mkPropertyValues({"SETTEXT": "Third Comment"}))
            self.assertEqual(get_state_as_dict(gridwin)["CurrentCellCommentText"], "Third Comment")


# vim: set shiftwidth=4 softtabstop=4 expandtab:
