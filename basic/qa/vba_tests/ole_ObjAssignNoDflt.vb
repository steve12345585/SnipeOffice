'
' This file is Part of the SnipeOffice project.
'
' This Source Code Form is subject to the terms of the Mozilla Public
' License, v. 2.0. If a copy of the MPL was not distributed with this
' file, You can obtain one at http://mozilla.org/MPL/2.0/.
'

Option VBASupport 1
Function doUnitTest(TestData as String, Driver as String) as String
Rem Ensure object assignment is by reference
Rem when object member is used ( as lhs )
Dim origTimeout As Long
Dim modifiedTimeout As Long
Set cn = New ADODB.Connection
origTimeout = cn.CommandTimeout
modifiedTimeout = origTimeout * 2
cn.CommandTimeout = modifiedTimeout
Dim conStr As String
conStr = "Provider=MSDASQL;Driver={" & Driver & "};DBQ="
conStr = conStr & TestData & "; ReadOnly=False;"
cn.Open conStr
Set objCmd = New ADODB.Command
objCmd.ActiveConnection = cn
If objCmd.ActiveConnection.CommandTimeout <> modifiedTimeout Then
    Rem if we copied the object by reference then we should have the
    Rem modified timeout ( because we should be just pointing as cn )
    doUnitTest = "FAIL expected modified timeout " & modifiedTimeout & " but got " &  objCmd.ActiveConnection.CommandTimeout
    Exit Function
End If
cn.CommandTimeout = origTimeout ' restore timeout
Rem Double check objCmd.ActiveConnection is pointing to objCmd.ActiveConnection
If objCmd.ActiveConnection.CommandTimeout <> origTimeout Then
    doUnitTest = "FAIL expected original timeout " & origTimeout & " but got " &  objCmd.ActiveConnection.CommandTimeout
    Exit Function
End If
doUnitTest = "OK" ' no error
End Function
