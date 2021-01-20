Sub DownloadFile1(url, strPath)
    dim xHttp: Set xHttp = createobject("Microsoft.XMLHTTP")
    dim bStrm: Set bStrm = createobject("Adodb.Stream")
    xHttp.Open "GET", url, False
    xHttp.Send

    with bStrm
        .type = 1 '//binary
        .open
        .write xHttp.responseBody
        .savetofile strPath, 2 '//overwrite
    end with

End Sub


Sub DownloadFile2(url, strPath)
    set WinHttpReq =CreateObject("WinHttp.WinHttpRequest.5.1")
    WinHttpReq.Open "GET", url, False
    WinHttpReq.Send

    dim BinStream: Set BinStream = createobject("Adodb.Stream")
    BinStream.Type = 1
    BinStream.Open
    BinStream.Write WinHttpReq.ResponseBody
    BinStream.SaveToFile strPath
End Sub