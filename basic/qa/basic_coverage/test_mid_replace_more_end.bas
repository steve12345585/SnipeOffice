'
' This file is Part of the SnipeOffice project.
'
' This Source Code Form is subject to the terms of the Mozilla Public
' License, v. 2.0. If a copy of the MPL was not distributed with this
' file, You can obtain one at http://mozilla.org/MPL/2.0/.
'

' cf. examples at <https://docs.microsoft.com/en-us/dotnet/visual-basic/language-reference/
' statements/mid-statement>:
Function doUnitTest as String
    s = "The fox jumps"
    Mid(s, 5, 100, "cow jumped over")
    If (s = "The cow jumpe") Then
        doUnitTest = "OK"
    Else
        doUnitTest = "FAIL"
    End If
End Function
