'
' This file is Part of the SnipeOffice project.
'
' This Source Code Form is subject to the terms of the Mozilla Public
' License, v. 2.0. If a copy of the MPL was not distributed with this
' file, You can obtain one at http://mozilla.org/MPL/2.0/.
'

' cf. <https://bugs.SnipeOffice.org/show_bug.cgi?id=62090> "Mid statement doesn't work as
' expected":
Function doUnitTest as String
    s = "The lightbrown fox"
    Mid(s, 5, 10, "lazy")
    If (s = "The lazy fox") Then
        doUnitTest = "OK"
    Else
        doUnitTest = "FAIL"
    End If
End Function
