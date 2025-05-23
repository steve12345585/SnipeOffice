'
' This file is Part of the SnipeOffice project.
'
' This Source Code Form is subject to the terms of the Mozilla Public
' License, v. 2.0. If a copy of the MPL was not distributed with this
' file, You can obtain one at http://mozilla.org/MPL/2.0/.
'

Option VBASupport 1
Option Explicit

Function doUnitTest() As String
    TestUtil.TestInit
    verify_testIsNumeric
    doUnitTest = TestUtil.GetResult()
End Function

Sub verify_testIsNumeric()
    On Error GoTo errorHandler

    TestUtil.Assert(IsNumeric(123),     "IsNumeric(123)")
    TestUtil.Assert(IsNumeric(-123),    "IsNumeric(-123)")
    TestUtil.Assert(IsNumeric(123.8),   "IsNumeric(123.8)")
    TestUtil.Assert(Not IsNumeric("a"), "Not IsNumeric(""a"")")
    TestUtil.Assert(IsNumeric(True), "IsNumeric(True)")
    TestUtil.Assert(IsNumeric("123"),   "IsNumeric(""123"")")
    TestUtil.Assert(IsNumeric("+123"),  "IsNumeric(""+123"")")

    TestUtil.Assert(Not IsNumeric(""), "Not IsNumeric("""")")
    TestUtil.Assert(Not IsNumeric(" "), "Not IsNumeric("" "")")
    TestUtil.Assert(Not IsNumeric(" + "), "Not IsNumeric("" + "")")
    TestUtil.Assert(Not IsNumeric(" - "), "Not IsNumeric("" - "")")

    TestUtil.Assert(IsNumeric(" + 0 "), "IsNumeric("" + 0 "")")
    TestUtil.Assert(IsNumeric(" - 0 "), "IsNumeric("" - 0 "")")

    Exit Sub
errorHandler:
    TestUtil.ReportErrorHandler("verify_testIsNumeric", Err, Error$, Erl)
End Sub
