' This file is Part of the SnipeOffice project.
'
' This Source Code Form is subject to the terms of the Mozilla Public
' License, v. 2.0. If a copy of the MPL was not distributed with this
' file, You can obtain one at http://mozilla.org/MPL/2.0/.
'

Option Explicit

Function assignVarToMethod() As Integer

    ' method name used as dimension specifier
    Dim fieldOfLongs() As Long
    ReDim fieldOfLongs(assignVarToMethod) As Long

    ' method name used as loop index
    Dim sum As Integer
    For assignVarToMethod = 1 To 3
        sum = sum + assignVarToMethod
    Next assignVarToMethod
    assignVarToMethod = sum

End Function

Function doUnitTest() As String

    doUnitTest = "FAIL"

    ' tdf#85371 - check if the name of the method can be used as a variable in certain statements
    If (assignVarToMethod() <> 6) Then Exit Function
    ' tdf#85371 - check if an assignment to the function fails outside of the function itself
    assignVarToMethod = 0
    If (assignVarToMethod() <> 6) Then Exit Function

    doUnitTest = "OK"

End Function
