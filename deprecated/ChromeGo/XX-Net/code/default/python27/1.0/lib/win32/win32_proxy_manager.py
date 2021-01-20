import _winreg as winreg
from ctypes import *
from ctypes.wintypes import *

LPWSTR = POINTER(WCHAR)
HINTERNET = LPVOID

INTERNET_OPTION_REFRESH = 37
INTERNET_OPTION_SETTINGS_CHANGED = 39
INTERNET_OPTION_PER_CONNECTION_OPTION = 75
INTERNET_PER_CONN_FLAGS = 1
INTERNET_PER_CONN_PROXY_SERVER = 2
INTERNET_PER_CONN_PROXY_BYPASS = 3
INTERNET_PER_CONN_AUTOCONFIG_URL = 4
INTERNET_PER_CONN_AUTODISCOVERY_FLAGS = 5

PROXY_TYPE_DIRECT = 1
PROXY_TYPE_PROXY = 2
PROXY_TYPE_AUTO_PROXY_URL = 4

CONN_PATH = r'Software\Microsoft\Windows\CurrentVersion\Internet Settings\Connections'
CONNECTIONS = winreg.OpenKey(winreg.HKEY_CURRENT_USER, CONN_PATH, 0, winreg.KEY_QUERY_VALUE)

class INTERNET_PER_CONN_OPTION(Structure):
    class Value(Union):
        _fields_ = [
            ('dwValue', DWORD),
            ('pszValue', LPWSTR),
            ('ftValue', FILETIME),
        ]

    _fields_ = [
        ('dwOption', DWORD),
        ('Value', Value),
    ]

class INTERNET_PER_CONN_OPTION_LIST(Structure):
    _fields_ = [
        ('dwSize', DWORD),
        ('pszConnection', LPWSTR),
        ('dwOptionCount', DWORD),
        ('dwOptionError', DWORD),
        ('pOptions', POINTER(INTERNET_PER_CONN_OPTION)),
    ]

InternetSetOption = windll.wininet.InternetSetOptionW
InternetSetOption.argtypes = [HINTERNET, DWORD, LPVOID, DWORD]
InternetSetOption.restype  = BOOL

def disable_proxy():
    _,values_num,_ = winreg.QueryInfoKey(CONNECTIONS)
    for i in range(0, values_num):
        try:
            key, value,_ = winreg.EnumValue(CONNECTIONS, i)
        except:
            break

        List = INTERNET_PER_CONN_OPTION_LIST()
        Option = (INTERNET_PER_CONN_OPTION * 1)()
        nSize = c_ulong(sizeof(INTERNET_PER_CONN_OPTION_LIST))

        Option[0].dwOption = INTERNET_PER_CONN_FLAGS
        Option[0].Value.dwValue = PROXY_TYPE_DIRECT

        List.dwSize = sizeof(INTERNET_PER_CONN_OPTION_LIST)
        List.pszConnection = create_unicode_buffer(key)
        List.dwOptionCount = 1
        List.dwOptionError = 0
        List.pOptions = Option

        InternetSetOption(None, INTERNET_OPTION_PER_CONNECTION_OPTION, byref(List), nSize)

    InternetSetOption(None, INTERNET_OPTION_SETTINGS_CHANGED, None, 0)
    InternetSetOption(None, INTERNET_OPTION_REFRESH, None, 0)

def set_proxy_auto(proxy_addr, conn_name='DefaultConnectionSettings'):
    setting = create_unicode_buffer(proxy_addr)

    List = INTERNET_PER_CONN_OPTION_LIST()
    Option = (INTERNET_PER_CONN_OPTION * 3)()
    nSize = c_ulong(sizeof(INTERNET_PER_CONN_OPTION_LIST))

    Option[0].dwOption = INTERNET_PER_CONN_FLAGS
    Option[0].Value.dwValue = PROXY_TYPE_DIRECT | PROXY_TYPE_AUTO_PROXY_URL
    Option[1].dwOption = INTERNET_PER_CONN_AUTOCONFIG_URL
    Option[1].Value.pszValue = setting
    Option[2].dwOption = INTERNET_PER_CONN_PROXY_BYPASS
    Option[2].Value.pszValue = create_unicode_buffer("localhost;127.*;10.*;172.16.*;172.17.*;172.18.*;172.19.*;172.20.*;172.21.*;172.22.*;172.23.*;172.24.*;172.25.*;172.26.*;172.27.*;172.28.*;172.29.*;172.30.*;172.31.*;172.32.*;192.168.*")

    List.dwSize = sizeof(INTERNET_PER_CONN_OPTION_LIST)
    List.pszConnection = create_unicode_buffer(conn_name)
    List.dwOptionCount = 3
    List.dwOptionError = 0
    List.pOptions = Option

    InternetSetOption(None, INTERNET_OPTION_PER_CONNECTION_OPTION, byref(List), nSize)
    InternetSetOption(None, INTERNET_OPTION_SETTINGS_CHANGED, None, 0)
    InternetSetOption(None, INTERNET_OPTION_REFRESH, None, 0)

def set_proxy_server(proxy_addr, conn_name='DefaultConnectionSettings'):
    setting = create_unicode_buffer(proxy_addr)

    List = INTERNET_PER_CONN_OPTION_LIST()
    Option = (INTERNET_PER_CONN_OPTION * 3)()
    nSize = c_ulong(sizeof(INTERNET_PER_CONN_OPTION_LIST))

    Option[0].dwOption = INTERNET_PER_CONN_FLAGS
    Option[0].Value.dwValue = PROXY_TYPE_DIRECT | PROXY_TYPE_PROXY
    Option[1].dwOption = INTERNET_PER_CONN_PROXY_SERVER
    Option[1].Value.pszValue = setting
    Option[2].dwOption = INTERNET_PER_CONN_PROXY_BYPASS
    Option[2].Value.pszValue = create_unicode_buffer("localhost;127.*;10.*;172.16.*;172.17.*;172.18.*;172.19.*;172.20.*;172.21.*;172.22.*;172.23.*;172.24.*;172.25.*;172.26.*;172.27.*;172.28.*;172.29.*;172.30.*;172.31.*;172.32.*;192.168.*")

    List.dwSize = sizeof(INTERNET_PER_CONN_OPTION_LIST)
    List.pszConnection = create_unicode_buffer(conn_name)
    List.dwOptionCount = 3
    List.dwOptionError = 0
    List.pOptions = Option

    InternetSetOption(None, INTERNET_OPTION_PER_CONNECTION_OPTION, byref(List), nSize)
    InternetSetOption(None, INTERNET_OPTION_SETTINGS_CHANGED, None, 0)
    InternetSetOption(None, INTERNET_OPTION_REFRESH, None, 0)

def set_proxy(proxy_addr):
    _,values_num,_ = winreg.QueryInfoKey(CONNECTIONS)
    if values_num:
        for i in range(0, values_num):
            try:
                key,value,_ = winreg.EnumValue(CONNECTIONS, i)
            except:
                break

            if '://' in proxy_addr:
                set_proxy_auto(proxy_addr, key)
            else:
                set_proxy_server(proxy_addr, key)

if __name__ == "__main__":
    set_proxy("127.0.0.1:8087")
