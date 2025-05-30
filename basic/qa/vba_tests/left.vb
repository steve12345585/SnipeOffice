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
    verify_testLeft
    doUnitTest = TestUtil.GetResult()
End Function

Sub verify_testLeft()
    On Error GoTo errorHandler

    TestUtil.AssertEqual(Left("sometext", 4),  "some",     "Left(""sometext"", 4)")
    TestUtil.AssertEqual(Left("sometext", 48), "sometext", "Left(""sometext"", 48)")
    TestUtil.AssertEqual(Left("", 4),          "",         "Left("""", 4)")

    ' tdf#141474 keyword names need to match that of VBA
    TestUtil.AssertEqual(Left(Length:=4, String:="sometext"), "some", "Left(Length:=4, String:=""sometext"")")

    Exit Sub
errorHandler:
    TestUtil.ReportErrorHandler("verify_testLeft", Err, Error$, Erl)
End Sub
