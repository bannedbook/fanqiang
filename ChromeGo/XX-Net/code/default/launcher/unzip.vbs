
Sub Unzip(ZipFile, ExtractTo)
    Dim objShell: set objShell = CreateObject("Shell.Application")
    Dim FilesInZip
    dim ns
    ZipFile = GetAbslutePath(ZipFile)
    ExtractTo = GetAbslutePath(ExtractTo)
    x = CreateDir(ExtractTo)
    set ns = objShell.NameSpace(ZipFile)
    set FilesInZip = ns.items()
    objShell.NameSpace(ExtractTo).CopyHere(FilesInZip)
End Sub