# -*- tab-width: 4; indent-tabs-mode: nil; py-indent-offset: 4 -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

from uitest.framework import UITestCase

#Bug 123378 - Printing always sets "document modified" status

class tdf123378(UITestCase):
   def test_tdf123378_print_sets_modified(self):
        # FIXME unstable test
        return
        with self.ui_test.create_doc_in_start_center("writer") as document:

            self.xUITest.executeCommand(".uno:Print")
            xDialog = self.xUITest.getTopFocusWindow()
            xOK = xDialog.getChild("cancel")
            self.ui_test.close_dialog_through_button(xOK)

            self.assertEqual(document.isModified(), False)

# vim: set shiftwidth=4 softtabstop=4 expandtab:
