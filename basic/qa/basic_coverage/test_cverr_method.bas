'
' This file is Part of the SnipeOffice project.
'
' This Source Code Form is subject to the terms of the Mozilla Public
' License, v. 2.0. If a copy of the MPL was not distributed with this
' file, You can obtain one at http://mozilla.org/MPL/2.0/.
'

Option Explicit

Function doUnitTest as String

    doUnitTest = "FAIL"

    ' CVERR
    If (CVerr(100) <> 100) Then Exit Function
    ' tdf#79426 - passing an error object to a function
    If (TestCVErr(CVErr(2)) <> 2) Then Exit Function
    ' tdf#79426 - test with Error-Code 448 ( ERRCODE_BASIC_NAMED_NOT_FOUND )
    If (TestCVErr(CVErr(448)) <> 448) Then Exit Function

    doUnitTest = "OK"

End Function

Function TestCVErr(vErr As Variant)
    Dim nValue As Integer
    nValue = vErr
    TestCVErr = nValue
End Function