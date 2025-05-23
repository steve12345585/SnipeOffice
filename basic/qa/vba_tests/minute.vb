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
    verify_testMinute
    doUnitTest = TestUtil.GetResult()
End Function

Sub verify_testMinute()
    On Error GoTo errorHandler

    TestUtil.AssertEqual(Minute("09:34:20"), 34, "Minute(""09:34:20"")")

    Exit Sub
errorHandler:
    TestUtil.ReportErrorHandler("verify_testMinute", Err, Error$, Erl)
End Sub
