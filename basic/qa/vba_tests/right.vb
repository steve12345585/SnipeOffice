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
    verify_testRight
    doUnitTest = TestUtil.GetResult()
End Function

Sub verify_testRight()
    On Error GoTo errorHandler

    TestUtil.AssertEqual(Right("sometext", 4),  "text",     "Right(""sometext"", 4)")
    TestUtil.AssertEqual(Right("sometext", 48), "sometext", "Right(""sometext"", 48)")
    TestUtil.AssertEqual(Right("", 4),          "",         "Right("""", 4)")

    ' tdf#141474 keyword names need to match that of VBA
    TestUtil.AssertEqual(Right(Length:=4, String:="sometext"), "text", "Right(Length:=4, String:=""sometext"")")

    Exit Sub
errorHandler:
    TestUtil.ReportErrorHandler("verify_testRight", Err, Error$, Erl)
End Sub
