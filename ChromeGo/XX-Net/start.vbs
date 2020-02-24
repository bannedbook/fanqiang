


Function CurrentPath()
    strPath = Wscript.ScriptFullName
    Set objFSO = CreateObject("Scripting.FileSystemObject")
    Set objFile = objFSO.GetFile(strPath)
    CurrentPath = objFSO.GetParentFolderName(objFile)
End Function

Function CurrentVersion()
    strCurrentPath = CurrentPath()
    strVersionFile = strCurrentPath & "/code/version.txt"

    Set fso = CreateObject("Scripting.FileSystemObject")
    If (fso.FileExists(strVersionFile)) Then

        Set objFileToRead = CreateObject("Scripting.FileSystemObject").OpenTextFile(strVersionFile,1)
        CurrentVersion = objFileToRead.ReadLine()

        version_path = strCurrentPath & "/code/" & CurrentVersion & "/launcher/start.py"
        If( Not fso.FileExists(version_path) ) Then
            CurrentVersion = "default"
        End If

        objFileToRead.Close
        Set objFileToRead = Nothing
    Else
       CurrentVersion = "default"
    End If

End Function


Function isConsole()
    Set objArgs = Wscript.Arguments
    'WScript.Echo objArgs.Count
    'WScript.Echo objArgs(0)
    isConsole = 0
    If objArgs.Count > 0 Then
        if objArgs(0) = "console" Then
            isConsole = 1
        End If
    End If
End Function


strCurrentPath = CurrentPath()
strVersion = CurrentVersion()
Dim strArgs
quo = """"

If isConsole() Then
    python_cmd = "python.exe"
Else
    python_cmd = "pythonw.exe"
End If

strExecutable = quo & strCurrentPath & "\code\" & strVersion & "\python27\1.0\" & python_cmd & quo
strArgs = strExecutable & " " & quo & strCurrentPath & "\code\" & strVersion & "\launcher\start.py" & quo
'WScript.Echo strArgs

Set oShell = CreateObject ("Wscript.Shell")
oShell.Environment("Process")("PYTHONPATH")=""
oShell.Environment("Process")("PYTHONHOME")=""
oShell.Run strArgs, isConsole(), false
