'
' This file is Part of the SnipeOffice project.
'
' This Source Code Form is subject to the terms of the Mozilla Public
' License, v. 2.0. If a copy of the MPL was not distributed with this
' file, You can obtain one at http://mozilla.org/MPL/2.0/.
'

Option Explicit

Function doUnitTest as String
  If #07/28/1977# = 28334 And #1977-07-28# = 28334 Then
     doUnitTest = "OK"
  Else
     doUnitTest = "FAIL"
  End If
End Function
