Dim objWShell
Set objWShell = WScript.CreateObject("WScript.Shell")
appData = objWShell.expandEnvironmentStrings("%APPDATA%")
settingFile=appData+"\Lantern\settings.yaml"
'Wscript.echo appData

Set fso = Wscript.CreateObject("Scripting.FileSystemObject")

If fso.FileExists(settingFile) Then

set f=fso.opentextfile(settingFile)
s=replace(f.readall,"systemProxy: false","systemProxy: true")
s=replace(s,"autoLaunch: true","autoLaunch: false")
f.close
set r=fso.opentextfile(settingFile,2,true)
r.write s
WScript.sleep 5000
Wscript.quit

Else
Wscript.quit
End If



