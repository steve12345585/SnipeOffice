'
' This file is Part of the SnipeOffice project.
'
' This Source Code Form is subject to the terms of the Mozilla Public
' License, v. 2.0. If a copy of the MPL was not distributed with this
' file, You can obtain one at http://mozilla.org/MPL/2.0/.
'

Option Explicit

Function doUnitTest() as String
    Dim A As String
    Dim B As Double
    Dim Expected As String
    A = "222.222"
    ' in da-DK locale ',' is the decimal separator
    Expected = "222222"
    B = Cdbl(A)
    If B <> Expected Then
        doUnitTest = "FAIL"
    Else
        doUnitTest = "OK"
    End If
End Function
