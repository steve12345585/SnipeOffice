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
    verify_testday
    doUnitTest = TestUtil.GetResult()
End Function

Sub verify_testday()
    On Error GoTo errorHandler

    TestUtil.AssertEqual(Day("1969-02-12"), 12, "Day(""1969-02-12"")")

    Exit Sub
errorHandler:
    TestUtil.ReportErrorHandler("verify_testday", Err, Error$, Erl)
End Sub
