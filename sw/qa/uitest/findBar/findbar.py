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

#test Find Bar
class FindBar(UITestCase):

    def test_find_bar(self):

        with self.ui_test.create_doc_in_start_center("writer"):
            xWriterDoc = self.xUITest.getTopFocusWindow()
            xWriterEdit = xWriterDoc.getChild("writer_edit")

            # Type some lines to search for words on them
            xWriterEdit.executeAction("TYPE", mkPropertyValues({"TEXT": "LibreOffice"}))
            xWriterEdit.executeAction("TYPE", mkPropertyValues({"KEYCODE": "RETURN"}))
            xWriterEdit.executeAction("TYPE", mkPropertyValues({"TEXT": "LibreOffice Writer"}))
            xWriterEdit.executeAction("TYPE", mkPropertyValues({"KEYCODE": "RETURN"}))
            xWriterEdit.executeAction("TYPE", mkPropertyValues({"TEXT": "LibreOffice Calc"}))
            xWriterEdit.executeAction("TYPE", mkPropertyValues({"KEYCODE": "RETURN"}))
            xWriterEdit.executeAction("TYPE", mkPropertyValues({"TEXT": "The Document Foundation"}))
            xWriterEdit.executeAction("TYPE", mkPropertyValues({"KEYCODE": "RETURN"}))
            xWriterEdit.executeAction("TYPE", mkPropertyValues({"TEXT": "LibréOffice Math"}))
            xWriterEdit.executeAction("TYPE", mkPropertyValues({"KEYCODE": "RETURN"}))
            xWriterEdit.executeAction("TYPE", mkPropertyValues({"TEXT": "libreOffice Calc"}))

            # open the Find Bar
            xWriterEdit.executeAction("TYPE", mkPropertyValues({"KEYCODE": "CTRL+f"}))

            # Type the Word that we want to search for it
            xfind = xWriterDoc.getChild("find")
            xfind.executeAction("TYPE", mkPropertyValues({"TEXT": "Libre"}))

            # Select the Find Bar
            xfind_bar = xWriterDoc.getChild("FindBar")
            self.assertEqual(get_state_as_dict(xfind_bar)["ItemCount"], "15")

            # Press on FindAll in the Find Bar
            xfind_bar.executeAction("CLICK", mkPropertyValues({"POS": "4"}))
            self.assertEqual(get_state_as_dict(xfind_bar)["CurrSelectedItemID"], "5") # 5 is FindAll id for Pos 4
            self.assertEqual(get_state_as_dict(xfind_bar)["CurrSelectedItemText"], "Find All")
            self.assertEqual(get_state_as_dict(xfind_bar)["CurrSelectedItemCommand"], ".uno:FindAll")
            self.assertEqual(get_state_as_dict(xWriterEdit)["SelectedText"], "LibreLibreLibrélibreLibre")

            # Press on Find Next in the Find Bar
            xfind_bar.executeAction("CLICK", mkPropertyValues({"POS": "3"}))  # 3 is Find Next pos
            self.assertEqual(get_state_as_dict(xfind_bar)["CurrSelectedItemID"], "4")
            self.assertEqual(get_state_as_dict(xfind_bar)["CurrSelectedItemText"], "Find Next")
            self.assertEqual(get_state_as_dict(xfind_bar)["CurrSelectedItemCommand"], ".uno:DownSearch")
            self.assertEqual(get_state_as_dict(xWriterEdit)["SelectedText"], "Libre")

            # Press on Find Previous in the Find Bar
            xfind_bar.executeAction("CLICK", mkPropertyValues({"POS": "2"}))  # 2 is Find Previous pos
            self.assertEqual(get_state_as_dict(xfind_bar)["CurrSelectedItemID"], "3")
            self.assertEqual(get_state_as_dict(xfind_bar)["CurrSelectedItemText"], "Find Previous")
            self.assertEqual(get_state_as_dict(xfind_bar)["CurrSelectedItemCommand"], ".uno:UpSearch")
            self.assertEqual(get_state_as_dict(xWriterEdit)["SelectedText"], "libre")

            # Press on Match Case in the Find Bar
            xfind_bar.executeAction("CLICK", mkPropertyValues({"POS": "5"}))  # 5 is Match Case pos
            self.assertEqual(get_state_as_dict(xfind_bar)["CurrSelectedItemID"], "6")
            self.assertEqual(get_state_as_dict(xfind_bar)["CurrSelectedItemText"], "Match Case")
            self.assertEqual(get_state_as_dict(xfind_bar)["CurrSelectedItemCommand"], ".uno:MatchCase")
            xfind_bar.executeAction("CLICK", mkPropertyValues({"POS": "4"})) # Press on Find All to see the effect of Match Case
            self.assertEqual(get_state_as_dict(xWriterEdit)["SelectedText"], "LibreLibreLibréLibre")

            # Press on Match Diacritics in the Find Bar
            xfind_bar.executeAction("CLICK", mkPropertyValues({"POS": "6"}))  # 6 is Match Diacritics pos
            self.assertEqual(get_state_as_dict(xfind_bar)["CurrSelectedItemID"], "7")
            self.assertEqual(get_state_as_dict(xfind_bar)["CurrSelectedItemText"], "Match Diacritics")
            self.assertEqual(get_state_as_dict(xfind_bar)["CurrSelectedItemCommand"], ".uno:MatchDiacritics")
            xfind_bar.executeAction("CLICK", mkPropertyValues({"POS": "4"})) # Press on Find All to see the effect of Match Diacritics (with Match Case still active)
            self.assertEqual(get_state_as_dict(xWriterEdit)["SelectedText"], "LibreLibreLibre")

            # Additional test with word containing diacritic and uppercase, match case and diacritics filters on
            xfind.executeAction ("CLEAR", tuple())
            xfind.executeAction("TYPE", mkPropertyValues({"TEXT": "Libré"}))
            xfind_bar.executeAction("CLICK", mkPropertyValues({"POS": "4"}))
            self.assertEqual(get_state_as_dict(xWriterEdit)["SelectedText"], "Libré")

            # Close the Find Bar
            xfind_bar.executeAction("CLICK", mkPropertyValues({"POS": "0"}))  # 0 is pos for close

# vim: set shiftwidth=4 softtabstop=4 expandtab:
