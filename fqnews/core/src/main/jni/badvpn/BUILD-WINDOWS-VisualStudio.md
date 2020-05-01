# Windows build using Visual Studio

This document describes how to build on Windows using Visual Studio. Note that only
32-bit build has been tested and is described here.

## Prerequisites

### Visual Studio

You need Visual Studio 2017.

### CMake

You need CMake, it is best to get the latest version.

### OpenSSL

You don't need this if you only need tun2socks or udpgw (but only for the VPN software).

Install ActivePerl if not already.

Download and extract the OpenSSL source code.

Open a Visual Studio x86 native tools command prompt (found under Programs -> Visual
Studio 2017) and enter the OpenSSL source code directory. In this terminal, run the
following commands:

```
perl Configure VC-WIN32 no-asm --prefix=%cd%\install-dir
ms\do_ms
nmake -f ms\ntdll.mak install
```

### NSS

You don't need this if you only need tun2socks or udpgw (but only for the VPN software).

Install MozillaBuild (https://wiki.mozilla.org/MozillaBuild).

Download and extract the NSS source code that includes NSPR
(`nss-VERSION-with-nspr-VERSION.tar.gz`).

Copy the file `C:\mozilla-build\start-shell.bat` to
`C:\mozilla-build\start-shell-fixed.bat`, and in the latter file REMOVE the following
lines near the beginning:

```
SET INCLUDE=
SET LIB=
IF NOT DEFINED MOZ_NO_RESET_PATH (
  SET PATH=%SystemRoot%\System32;%SystemRoot%;%SystemRoot%\System32\Wbem
)
```

Open a Visual Studio x86 native tools command prompt. In this terminal, first run the
following command to enter the mozilla-build bash shell:

```
C:\mozilla-build\start-shell-fixed.bat
```

Enter the NSS source code directory and run the following commands:

```
make -C nss nss_build_all OS_TARGET=WINNT BUILD_OPT=1
cp -r dist/private/. dist/public/. dist/WINNT*.OBJ/include/
```

## Building BadVPN

Open a Visual Studio x86 native tools command prompt (found under Programs -> Visual
Studio 2017) and enter the BadVPN source code directory.

If you needed to build OpenSSL and NSS, then specify the paths to the builds of these
libraries by setting the `CMAKE_PREFIX_PATH` environment variable as shown below;
replace `<openssl-source-dir>` and `<nss-source-dir>` with the correct paths. For NSS,
check if the `.OBJ` directory name is correct, if not then adjust that as well.

```
set CMAKE_PREFIX_PATH=<openssl-source-dir>\install-dir;<nss-source-dir>\dist\WINNT6.2_OPT.OBJ
```

Run the commands shown below. If you only need tun2socks and udpgw then also add
the following parameters to first `cmake` command:
`-DBUILD_NOTHING_BY_DEFAULT=1 -DBUILD_TUN2SOCKS=1 -DBUILD_UDPGW=1`.

```
mkdir build
cd build
cmake .. -G "Visual Studio 15 2017" -DCMAKE_INSTALL_PREFIX=%cd%\..\install-dir
cmake --build . --config Release --target install
```

If you did need OpenSSL and NSS, then copy the needed DLL so that the programs will
be able to find them. You can use the following commands to do this (while still in
the `build` directory):

```
copy <openssl-source-dir>\install-dir\bin\libeay32.dll ..\install-dir\bin\
copy <nss-source-dir>\dist\WINNT6.2_OPT.OBJ\lib\*.dll ..\install-dir\bin\
```

The build is now complete and is located in `<badvpn-source-dir>\install-dir`.
