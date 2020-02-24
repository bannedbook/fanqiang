'''
Copyright (c) 2013 by JustAMan at GitHub
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
This file provides the ability to re-elevate the rights of running Python script to
Administrative ones allowing console of elevated process to remain user-interactive.
You should use elevateAdminRights() at the very possible start point of your scripts because
it will re-launch your script from the very beginning.
'''

import os
import sys
import subprocess

import ctypes
from ctypes.wintypes import HANDLE, BOOL, DWORD, HWND, HINSTANCE, HKEY
from ctypes import c_ulong, c_char_p, c_int, c_void_p
PHANDLE = ctypes.POINTER(HANDLE)
PDWORD = ctypes.POINTER(DWORD)

GetCurrentProcess = ctypes.windll.kernel32.GetCurrentProcess
GetCurrentProcess.argtypes = ()
GetCurrentProcess.restype = HANDLE

OpenProcessToken = ctypes.windll.kernel32.OpenProcessToken
OpenProcessToken.argtypes = (HANDLE, DWORD, PHANDLE)
OpenProcessToken.restype = BOOL

CloseHandle = ctypes.windll.kernel32.CloseHandle
CloseHandle.argtypes = (HANDLE, )
CloseHandle.restype = BOOL

GetTokenInformation = ctypes.windll.Advapi32.GetTokenInformation
GetTokenInformation.argtypes = (HANDLE, ctypes.c_int, ctypes.c_void_p, DWORD, PDWORD)
GetTokenInformation.restype = BOOL

TOKEN_READ = 0x20008
TokenElevation = 0x14

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
    def __init__(self, **kw):
        ctypes.Structure.__init__(self)
        self.cbSize = ctypes.sizeof(self)
        for fieldName, fieldValue in kw.iteritems():
            setattr(self, fieldName, fieldValue)

PShellExecuteInfo = ctypes.POINTER(ShellExecuteInfo)

ShellExecuteEx = ctypes.windll.Shell32.ShellExecuteExA
ShellExecuteEx.argtypes = (PShellExecuteInfo, )
ShellExecuteEx.restype = BOOL

WaitForSingleObject = ctypes.windll.kernel32.WaitForSingleObject
WaitForSingleObject.argtypes = (HANDLE, DWORD)
WaitForSingleObject.restype = DWORD

SW_HIDE = 0
SW_SHOW = 5
SEE_MASK_NOCLOSEPROCESS = 0x00000040
INFINITE = -1

ELEVATE_MARKER = 'win32elevate_marker_parameter'

FreeConsole = ctypes.windll.kernel32.FreeConsole
FreeConsole.argtypes = ()
FreeConsole.restype = BOOL

AttachConsole = ctypes.windll.kernel32.AttachConsole
AttachConsole.argtypes = (DWORD, )
AttachConsole.restype = BOOL

ATTACH_PARENT_PROCESS = -1

sysargv = [os.path.abspath(sys.argv[0])] + sys.argv[1:]

def areAdminRightsElevated():
    '''
    Tells you whether current script already has Administrative rights.
    '''
    pid = GetCurrentProcess()
    processToken = HANDLE()
    if not OpenProcessToken(pid, TOKEN_READ, ctypes.byref(processToken)):
        raise ctypes.WinError()
    try:
        elevated, elevatedSize = DWORD(), DWORD()
        if not GetTokenInformation(processToken, TokenElevation, ctypes.byref(elevated),
                                   ctypes.sizeof(elevated), ctypes.byref(elevatedSize)):
            raise ctypes.WinError()
        return bool(elevated)
    finally:
        CloseHandle(processToken)

def waitAndCloseHandle(processHandle):
    '''
    Waits till spawned process finishes and closes the handle for it
    '''
    WaitForSingleObject(processHandle, INFINITE)
    CloseHandle(processHandle)

def elevateAdminRights(waitAndClose=True, reattachConsole=True):
    '''
    This will re-run current Python script requesting to elevate administrative rights.
    If waitAndClose is True the process that called elevateAdminRights() will wait till elevated
    process exits and then will quit.
    If waitAndClose is False this function returns None for elevated process and process handle
    for parent process (like POSIX os.fork).
    If reattachConsole is False console of elevated process won't be attached to parent process
    so you won't see any output of it.
    '''
    if not areAdminRightsElevated():
        # this is host process that doesn't have administrative rights
        params = subprocess.list2cmdline(sysargv + [ELEVATE_MARKER])
        executeInfo = ShellExecuteInfo(fMask=SEE_MASK_NOCLOSEPROCESS, hwnd=None, lpVerb='runas',
                                       lpFile=sys.executable, lpParameters=params,
                                       lpDirectory=None,
                                       nShow=SW_HIDE if reattachConsole else SW_SHOW)
        if reattachConsole and not all(stream.isatty() for stream in (sys.stdin, sys.stdout,
                                                                      sys.stderr)):
            # TODO: some streams were redirected, we need to manually work them
            # currently just raise an exception
            raise NotImplementedError("win32elevate doesn't support elevating scripts with "
                                      "redirected input or output")

        if not ShellExecuteEx(ctypes.byref(executeInfo)):
            raise ctypes.WinError()
        if waitAndClose:
            waitAndCloseHandle(executeInfo.hProcess)
            sys.exit(0)
        else:
            return executeInfo.hProcess
    else:
        # This is elevated process, either it is launched by host process or user manually
        # elevated the rights for this script. We check it by examining last parameter
        if sys.argv[-1] == ELEVATE_MARKER:
            # this is script-elevated process, remove the marker
            del sys.argv[-1]
            if reattachConsole:
                # Now attach our elevated console to parent's console.
                # first we free our own console
                if not FreeConsole():
                    raise ctypes.WinError()
                # then we attach to parent process console
                if not AttachConsole(ATTACH_PARENT_PROCESS):
                    raise ctypes.WinError()

        # indicate we're already running with administrative rights, see docstring
        return None

def elevateAdminRun(args=sysargv, executable=sys.executable,
                    waitAndClose=False, reattachConsole=True):
    if (waitAndClose or
        reattachConsole and
        not all(stream.isatty() for stream in (sys.stdin, sys.stdout,sys.stderr))):
        # Don't attached to parent process when waitAndClose is "True"
        # TODO: some streams were redirected, we need to manually work them
        # currently just don't attached to parent process
        reattachConsole = False
    if args is not None and not isinstance(args, str):
        args = subprocess.list2cmdline(args)
    executeInfo = ShellExecuteInfo(fMask=SEE_MASK_NOCLOSEPROCESS, hwnd=None,
                                   lpVerb='' if areAdminRightsElevated() else 'runas',
                                   lpFile=executable, lpParameters=args,
                                   lpDirectory=None,
                                   nShow=SW_HIDE if reattachConsole else SW_SHOW)

    if not ShellExecuteEx(ctypes.byref(executeInfo)):
        raise ctypes.WinError()
    if waitAndClose:
        waitAndCloseHandle(executeInfo.hProcess)
    else:
        return executeInfo.hProcess

if __name__ == '__main__':
    elevateAdminRights(reattachConsole=False)
    sys.exit(1)