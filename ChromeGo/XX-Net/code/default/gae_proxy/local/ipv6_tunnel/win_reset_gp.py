# coding:utf-8

import os
import sys
import platform
import locale
import ctypes
import win32runas


def win32_notify( msg='msg', title='Title'):
    res = ctypes.windll.user32.MessageBoxW(None, msg, title, 1)
    # Yes:1 No:2
    return res == 1

def reset_teredo():
    gp_split = b'[\x00'
    gp_teredo = 'v6Transition\x00;Teredo'
    gp_teredo = '\x00'.join(b for b in gp_teredo).encode()

    with open(gp_regpol_file, 'rb') as f:
        gp_regpol_old = f.read().split(gp_split)

    gp_regpol_new = [gp for gp in gp_regpol_old if gp_teredo not in gp]

    if len(gp_regpol_new) != len(gp_regpol_old) and \
            win32_notify(u'发现组策略 Teredo 设置，是否重置？', u'提醒'):
        with open(gp_regpol_file, 'wb') as f:
            f.write(gp_split.join(gp_regpol_new))
        os.system(sysnative + '\\gpupdate /target:computer /force')

if '__main__' == __name__:
    if os.name != 'nt':
        sys.exit(0)

    sysver = platform.version()
    if sysver < '6':
        # Teredo item was added to Group Policy starting with Windows Vista
        sys.exit(0)

    windir = os.environ.get('windir')
    if not windir:
        sys.exit(-1)

    sys64 = os.path.exists(windir + '\\SysWOW64')
    pe32 = platform.architecture()[0] == '32bit'
    sysalias = 'Sysnative' if sys64 and pe32 else 'System32'
    sysnative = '%s\\%s' % (windir, sysalias)
    gp_regpol_file = sysnative + '\\GroupPolicy\\Machine\\Registry.pol'

    if os.path.exists(gp_regpol_file):
        if not win32runas.is_admin():
            win32runas.runas()
            sys.exit(0)
        reset_teredo()
