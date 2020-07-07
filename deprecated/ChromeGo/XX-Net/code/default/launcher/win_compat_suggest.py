# coding:utf-8

import os
import ctypes
import collections
import locale

def get_process_list():
    process_list = []
    if os.name != "nt":
        return process_list

    Process = collections.namedtuple("Process", "filename name")
    PROCESS_QUERY_INFORMATION = 0x0400
    PROCESS_VM_READ = 0x0010
    lpidProcess= (ctypes.c_ulong * 1024)()
    cbNeeded = ctypes.c_ulong()

    ctypes.windll.psapi.EnumProcesses(ctypes.byref(lpidProcess), ctypes.sizeof(lpidProcess), ctypes.byref(cbNeeded))
    nReturned = cbNeeded.value // ctypes.sizeof(cbNeeded)
    pidProcess = [i for i in lpidProcess][:nReturned]
    has_queryimage = hasattr(ctypes.windll.kernel32, "QueryFullProcessImageNameA")

    for pid in pidProcess:
        hProcess = ctypes.windll.kernel32.OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, pid)
        if hProcess:
            modname = ctypes.create_string_buffer(2048)
            count = ctypes.c_ulong(ctypes.sizeof(modname))
            if has_queryimage:
                ctypes.windll.kernel32.QueryFullProcessImageNameA(hProcess, 0, ctypes.byref(modname), ctypes.byref(count))
            else:
                ctypes.windll.psapi.GetModuleFileNameExA(hProcess, 0, ctypes.byref(modname), ctypes.byref(count))

            path = modname.value.decode("mbcs")
            filename = os.path.basename(path)
            name, ext = os.path.splitext(filename)
            process_list.append(Process(filename=filename, name=name))
            ctypes.windll.kernel32.CloseHandle(hProcess)

    return process_list

_blacklist = {
    #(u"测试", "Test"): [
    #    u"汉字测试",
    #    "explorer",
    #    "Python",
    #],
    (u"渣雷", "Xhunder"): [
        "ThunderPlatform",
        "ThunderFW",
        "ThunderLiveUD",
        "ThunderService",
        "ThunderSmartLimiter",
        "ThunderWelcome",
        "DownloadSDKServer",
        "LimitingDriver",
        "LiteUD",
        "LiteViewBundleInst",
        "XLNXService ",
        "XLServicePlatform",
        "XLGameBoot",
        "XMPBoot",
    ],
    (u"百毒", "Baidu"): [
        "BaiduSdSvc",
        "BaiduSdTray",
        "BaiduSd",
        "BaiduAn",
        "bddownloader",
        "baiduansvx",
    ],
    (u"流氓 360", u"360"): [
        "360sd",
        "360tray",
        "360Safe",
        "safeboxTray",
        "360safebox",
        "360se",
    ],
    (u"疼讯复制机", "Tencent"): [
        "QQPCRTP",
        "QQPCTray",
        "QQProtect",
    ],
    (u"金山", "Kingsoft"): [
        "kismain",
        "ksafe",
        "KSafeSvc",
        "KSafeTray",
        "KAVStart",
        "KWatch",
        "KMailMon",
    ],
    (u"瑞星", "Rising"): [
        "rstray",
        "ravmond",
        "rsmain",
    ],
    (u"江民", "Jiangmin"): [
        "UIHost",
        "KVMonXP",
        "kvsrvxp",
        "kvxp",
    ],
    (u"2345 不安全", "2345"): [
        "2345MPCSafe",
    ],
    (u"天网防火墙", "SkyNet"): [
        "PFW",
    ],
}

_title = u"XX-Net 兼容性建议", u"XX-Net compatibility suggest"
_notice = (
u"某些软件可能和 XX-Net 存在冲突，导致 CPU 占用过高或者无法正常使用。"
u"如有此现象建议暂时退出以下软件来保证 XX-Net 正常运行：\n",
u"Some software may conflict with XX-Net, "
u"causing the CPU to be overused or not working properly."
u"If this is the case, it is recommended to temporarily quit the following "
u"software to ensure XX-Net runnig:\n",
u"\n你可以在配置页面关闭此建议。",
u"\nYou can close this suggestion on the configuration page.",
)


def main():
    if os.name != "nt":
        return

    lang = 0 if locale.getdefaultlocale()[0] == "zh_CN" else 1
    blacklist = {}
    for k, v in _blacklist.items():
        for name in v:
            blacklist[name] = k[lang]

    processlist = dict((process.name.lower(), process) for process in get_process_list())
    softwares = [name for name in blacklist if name.lower() in processlist]

    if softwares:
        displaylist = {}
        for software in softwares:
            company = blacklist[software]
            if company not in displaylist:
                displaylist[company] = []
            displaylist[company].append(software)

        displaystr = [_notice[lang],]
        for company, softwares in displaylist.items():
            displaystr.append(u"    %s: \n\t%s" % (company, 
                u"\n\t".join(processlist[name.lower()].filename for name in softwares)))

        title = _title[lang]
        displaystr.append(_notice[lang + 2])
        error = u"\n".join(displaystr)

        ctypes.windll.user32.MessageBoxW(None, error, title, 48)


if "__main__" == __name__:
    if os.name != "nt":
        sys.exit(0)
    main()
