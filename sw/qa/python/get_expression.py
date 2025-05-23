#! /usr/bin/env python
# -*- tab-width: 4; indent-tabs-mode: nil; py-indent-offset: 4 -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
import unittest
from org.libreoffice.unotest import UnoInProcess


class TestGetExpression(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls._uno = UnoInProcess()
        cls._uno.setUp()
        cls._xDoc = cls._uno.openEmptyWriterDoc()

    @classmethod
    def tearDownClass(cls):
        cls._uno.tearDown()
        # HACK in case cls._xDoc holds a UNO proxy to an SwXTextDocument (whose dtor calls
        # Application::GetSolarMutex via sw::UnoImplPtrDeleter), which would potentially only be
        # garbage-collected after VCL has already been deinitialized:
        cls._xDoc = None

    def test_get_expression(self):
        self.__class__._uno.checkProperties(
            self.__class__._xDoc.createInstance("com.sun.star.text.textfield.GetExpression"),
            {"Content": "foo",
             "CurrentPresentation": "bar",
             "NumberFormat": 0,
             "IsShowFormula": False,
             "SubType": 0,
             "VariableSubtype": 1,
             "IsFixedLanguage": False,
             },
            self
            )

    # property 'Value' is read only?
    @unittest.expectedFailure
    def test_get_expression_veto_read_only(self):
        self.__class__._uno.checkProperties(
            self.__class__._xDoc.createInstance("com.sun.star.text.textfield.GetExpression"),
            {"Value": 0.0},
            self
            )

if __name__ == '__main__':
    unittest.main()
