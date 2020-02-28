On Error Resume Next

Sub includeFile (fSpec)
    dim fileSys, file, fileData
    set fileSys = createObject ("Scripting.FileSystemObject")
    set file = fileSys.openTextFile (fSpec)
    fileData = file.readAll ()
    file.close
    executeGlobal fileData
    set file = nothing
    set fileSys = nothing
End Sub


Set fso=CreateObject("Scripting.FileSystemObject")

strCurrentPath = CurrentPath()
strVersion = CurrentVersion()
Dim oShell : Set oShell = CreateObject("WScript.Shell")
oShell.CurrentDirectory = strCurrentPath

pythonDir = "python27\2.0\"
If Not DirIsExist(pythonDir) then
    includeFile strCurrentPath & "\code\" & strVersion & "\launcher\download.vbs"
    includeFile strCurrentPath & "\code\" & strVersion & "\launcher\unzip.vbs"
End If

Sub ChangeCurrentPathToRoot()
    strCurrentPath = CurrentPath()

End Sub

Function PreparePython(download_id)
    ' Check if python have installed.
    pythonDir = "python27\2.0\"
    If DirIsExist(pythonDir) then
        PreparePython = True
        Exit Function
    End If

    CreateDir("data")
    CreateDir("data\download")

    url = "https://raw.githubusercontent.com/XX-net/XX-Net-dev/master/download/py27.zip"
    fp = "data\download\py27.zip"
    fsize = 6594715
    If download_id = 1 then
        call DownloadFile1(url, fp)
    Elseif download_id = 2 then
        call DownloadFile2(url, fp)
    End If

    fs = GetFileSize(fp)
    if not fs = fsize then
        PreparePython = False
        Exit Function
    end if

    call UnzipFiles("data\download\py27.zip", "data\download\py27")

    CreateDir("python27")
    call MoveDir("data\download\py27\py27", "python27\2.0")

    call RemoveDir("data\download")
    PreparePython = True
End Function

Function DirIsExist(strPath)
    Set fso = CreateObject("Scripting.FileSystemObject")
    Set shl = CreateObject("WScript.Shell")

    exists = fso.FolderExists(strPath)

    if (exists) then
        DirIsExist = True
    Else
        DirIsExist = False
    end if
End Function

Function CreateDir(strPath)
    set filesys=CreateObject("Scripting.FileSystemObject")
    If Not filesys.FolderExists(strPath) Then
        Set newfolder = filesys.CreateFolder(strPath)
    End If
End Function

Function GetFileSize(strPath)
    set filesys=CreateObject("Scripting.FileSystemObject")
    GetFileSize = filesys.GetFile(strPath).Size
End Function

Function GetAbslutePath(path)
    Dim fso
    Set fso = CreateObject("Scripting.FileSystemObject")
    GetAbslutePath = fso.GetAbsolutePathName(path)
End Function

Sub UnzipFiles(zipFile, targetDir)
    'WScript.Echo "unzip " & zipFile
    call Unzip(zipFile, targetDir)
End Sub

Sub MoveDir(srcDir, dstDir)
    srcDir = GetAbslutePath(srcDir)
    dstDir = GetAbslutePath(dstDir)
    set fs = CreateObject("Scripting.FileSystemObject")
    set folder = fs.GetFolder(srcDir)
    folder.Move dstDir
End Sub

Sub RemoveDir(strFolderPath)
    Dim objFSO, objFolder
    Set objFSO = CreateObject ("Scripting.FileSystemObject")
    If objFSO.FolderExists(strFolderPath) Then
        objFSO.DeleteFolder strFolderPath, True
    End If
End Sub


python_is_ready = PreparePython(1)
If not python_is_ready then
    python_is_ready = PreparePython(2)
End If

If not python_is_ready then
    WScript.Echo "XX-Net Download Python Environment fail!"
    Wscript.Quit
End if


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


Dim strArgs
quo = """"

If isConsole() Then
    python_cmd = "python.exe"
Else
    python_cmd = "pythonw.exe"
End If

strExecutable = quo & strCurrentPath & "\python27\2.0\" & python_cmd & quo
strArgs = strExecutable & " " & quo & strCurrentPath & "\code\" & strVersion & "\launcher\start.py" & quo
RunningLockFn = strCurrentPath & "\data\launcher\Running.Lck"
'WScript.Echo strArgs

Set fso = CreateObject("Scripting.FileSystemObject")
Set oShell = CreateObject ("Wscript.Shell")
oShell.Environment("Process")("PYTHONPATH")=""
oShell.Environment("Process")("PYTHONHOME")=""
Do
    oShell.Run strArgs, isConsole(), true
Loop Until not fso.FileExists(RunningLockFn)
