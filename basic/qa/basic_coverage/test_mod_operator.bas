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

    Dim a As Double, b as Double
    a = 16.4
    b = 5.9

    ' tdf#141201 - MOD operands are rounded to Integer values before the operation is performed
    if (a MOD b <> 4) Then Exit Function
    if (16.4 MOD 5.9 <> 4) Then Exit Function
    if (15.9 MOD 6.4 <> 4) Then Exit Function
    if (2147483647.4 MOD 4 <> 3) Then Exit Function

    doUnitTest = "OK"

End Function
