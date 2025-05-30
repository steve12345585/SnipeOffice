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
from uitest.uihelper.common import get_state_as_dict, get_url_for_data_file
from libreoffice.calc.document import get_cell_by_position

class xmlSource(UITestCase):

    def test_importXml(self):

        with self.ui_test.create_doc_in_start_center("calc") as calc_doc:

            # Create a new tab
            with self.ui_test.execute_dialog_through_command(".uno:Insert") as xDialog:
                xAfter = xDialog.getChild('after')
                xAfter.executeAction("CLICK", tuple())

            with self.ui_test.execute_modeless_dialog_through_command(".uno:ManageXMLSource") as xDialog:

                xSource = xDialog.getChild("selectsource")

                with self.ui_test.execute_blocking_action(xSource.executeAction, args=('CLICK', ()), close_button="open") as xOpenDialog:
                    xFileName = xOpenDialog.getChild("file_name")
                    xFileName.executeAction("TYPE", mkPropertyValues({"TEXT": get_url_for_data_file("xmlSource.xml")}))

                xTree = xDialog.getChild("tree")
                self.assertEqual('13', get_state_as_dict(xTree)["Children"])

                xTree.getChild('0').executeAction("SELECT", tuple())
                xTree.executeAction("TYPE", mkPropertyValues({"KEYCODE": "DOWN"}))
                xTree.executeAction("TYPE", mkPropertyValues({"KEYCODE": "DOWN"}))

                self.assertEqual("entry", get_state_as_dict(xTree)["SelectEntryText"])
                self.assertEqual("11", get_state_as_dict(xTree)["SelectionCount"])

                xEdit = xDialog.getChild("edit")
                xEdit.executeAction("TYPE", mkPropertyValues({"TEXT": "$A$1"}))

            # tdf#126565: Without the fix in place, this test would have failed with
            # AssertionError: '' != 'name'
            for i in range(8):
                self.assertEqual("", get_cell_by_position(calc_doc, 0, i, 0).getString())

            # Check the headers
            self.assertEqual("name", get_cell_by_position(calc_doc, 1, 0, 0).getString())
            self.assertEqual("translated-address", get_cell_by_position(calc_doc, 1, 1, 0).getString())
            self.assertEqual("to", get_cell_by_position(calc_doc, 1, 2, 0).getString())
            self.assertEqual("from", get_cell_by_position(calc_doc, 1, 3, 0).getString())
            self.assertEqual("source", get_cell_by_position(calc_doc, 1, 4, 0).getString())
            self.assertEqual("destination", get_cell_by_position(calc_doc, 1, 5, 0).getString())
            self.assertEqual("service", get_cell_by_position(calc_doc, 1, 6, 0).getString())
            self.assertEqual("disabled", get_cell_by_position(calc_doc, 1, 7, 0).getString())

    def test_tdf153604(self):

        with self.ui_test.create_doc_in_start_center("calc") as calc_doc:

            with self.ui_test.execute_modeless_dialog_through_command(".uno:ManageXMLSource") as xDialog:

                xSource = xDialog.getChild("selectsource")

                with self.ui_test.execute_blocking_action(xSource.executeAction, args=('CLICK', ()), close_button="open") as xOpenDialog:
                    xFileName = xOpenDialog.getChild("file_name")
                    xFileName.executeAction("TYPE", mkPropertyValues({"TEXT": get_url_for_data_file("tdf153604.xml")}))

                xTree = xDialog.getChild("tree")
                self.assertEqual('2', get_state_as_dict(xTree)["Children"])

                xTree.getChild('0').executeAction("SELECT", tuple())
                xTree.executeAction("TYPE", mkPropertyValues({"KEYCODE": "DOWN"}))

                self.assertEqual("data", get_state_as_dict(xTree)["SelectEntryText"])

                xEdit = xDialog.getChild("edit")
                xEdit.executeAction("TYPE", mkPropertyValues({"TEXT": "$A$1"}))

            # Without the fix in place, this test would have failed with
            # AssertionError: 'data' != ''
            self.assertEqual("data", get_cell_by_position(calc_doc, 0, 0, 0).getString())
            self.assertEqual("v1", get_cell_by_position(calc_doc, 0, 0, 1).getString())
            self.assertEqual("v2", get_cell_by_position(calc_doc, 0, 0, 2).getString())
            self.assertEqual("v3", get_cell_by_position(calc_doc, 0, 0, 3).getString())

    def test_tdf146260(self):

        with self.ui_test.create_doc_in_start_center("calc") as calc_doc:

            with self.ui_test.execute_modeless_dialog_through_command(".uno:ManageXMLSource") as xDialog:

                xSource = xDialog.getChild("selectsource")

                with self.ui_test.execute_blocking_action(xSource.executeAction, args=('CLICK', ()), close_button="open") as xOpenDialog:
                    xFileName = xOpenDialog.getChild("file_name")
                    xFileName.executeAction("TYPE", mkPropertyValues({"TEXT": get_url_for_data_file("tdf146260.xml")}))

                xTree = xDialog.getChild("tree")
                self.assertEqual('43', get_state_as_dict(xTree)["Children"])

                xTree.getChild('0').executeAction("SELECT", tuple())
                for i in range(5):
                    xTree.executeAction("TYPE", mkPropertyValues({"KEYCODE": "DOWN"}))

                self.assertEqual("Fp", get_state_as_dict(xTree)["SelectEntryText"])
                self.assertEqual("38", get_state_as_dict(xTree)["SelectionCount"])

                xEdit = xDialog.getChild("edit")
                xEdit.executeAction("TYPE", mkPropertyValues({"TEXT": "$A$1"}))

            # Without the fix in place, this test would have failed with
            # AssertionError: 'data' != ''
            self.assertEqual("Djh", get_cell_by_position(calc_doc, 0, 0, 0).getString())
            self.assertEqual("Fpzl", get_cell_by_position(calc_doc, 0, 1, 0).getString())

            for i in range(1, 5):
                # Without the fix in place, this test would have failed with
                # AssertionError: '专用发票' != 'ר�÷�Ʊ'
                self.assertEqual("专用发票", get_cell_by_position(calc_doc, 0, 1, i).getString())

# vim: set shiftwidth=4 softtabstop=4 expandtab:
