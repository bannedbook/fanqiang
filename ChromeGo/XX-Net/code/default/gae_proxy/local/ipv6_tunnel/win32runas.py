# coding:utf-8
# run as admin in Windows

import os
import sys
import ctypes
import subprocess
from ctypes import c_ulong, c_char_p, c_int, c_void_p
from ctypes.wintypes import HANDLE, DWORD, HWND, HINSTANCE, HKEY


def is_admin():
    try:
        return ctypes.windll.shell32.IsUserAnAdmin()
    except:
        return False

def encode_for_locale(s):
    if s is None:
        return
    return s.encode('mbcs')

if os.name == 'nt':
    class ShellExecuteInfo(ctypes.Structure):
        _fields_ = [('cbSize', DWORD),
                    ('fMask', c_ulong),
                    ('hwnd', HWND),
                    ('lpVerb', c_char_p),
                    ('lpFile', c_char_p),
                    ('lpParameters', c_char_p),
                    ('lpDirectory', c_char_p),
                    ('nShow', c_int),
                    ('hInstApp', HINSTANCE),
                    ('lpIDList', c_void_p),
                    ('lpClass', c_char_p),
                    ('hKeyClass', HKEY),
                    ('dwHotKey', DWORD),
                    ('hIcon', HANDLE),
                    ('hProcess', HANDLE)]

    SEE_MASK_NOCLOSEPROCESS = 0x00000040
    ShellExecuteEx = ctypes.windll.Shell32.ShellExecuteEx
    ShellExecuteEx.argtypes = ctypes.POINTER(ShellExecuteInfo),
    WaitForSingleObject = ctypes.windll.kernel32.WaitForSingleObject
    SE_ERR_CODES = {
        0: 'Out of memory or resources',
        2: 'File not found',
        3: 'Path not found',
        5: 'Access denied',
        8: 'Out of memory',
        26: 'Cannot share an open file',
        27: 'File association information not complete',
        28: 'DDE operation timed out',
        29: 'DDE operation failed',
        30: 'DDE operation is busy',
        31: 'File association not available',
        32: 'Dynamic-link library not found',
    }
    sys.argv[0] = os.path.abspath(sys.argv[0])

def runas(args=sys.argv, executable=sys.executable, cwd=None,
          nShow=1, waitClose=True, waitTimeout=-1):
    if not 0 <= nShow <= 10:
        nShow = 1
    err = None
    try:
        if args is not None and not isinstance(args, str):
            args = subprocess.list2cmdline(args)
        pExecInfo = ShellExecuteInfo()
        pExecInfo.cbSize = ctypes.sizeof(pExecInfo)
        pExecInfo.fMask |= SEE_MASK_NOCLOSEPROCESS
        pExecInfo.lpVerb = b'open' if is_admin() else b'runas'
        pExecInfo.lpFile = encode_for_locale(executable)
        pExecInfo.lpParameters = encode_for_locale(args)
        pExecInfo.lpDirectory = encode_for_locale(cwd)
        pExecInfo.nShow = nShow
        if ShellExecuteEx(pExecInfo):
            if waitClose:
                WaitForSingleObject(pExecInfo.hProcess, waitTimeout)
                return True
            else:
                return pExecInfo.hProcess
        else:
            err = SE_ERR_CODES.get(pExecInfo.hInstApp, 'unknown')
    except Exception as e:
        err = e
    if err:
        print('runas failed! error: %r' % err)

if __name__ == '__main__':
    if len(sys.argv) > 1:
       runas(sys.argv[2:], sys.argv[1])
