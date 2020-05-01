# shadowsocks-libev

[![Build Status](https://travis-ci.com/shadowsocks/shadowsocks-libev.svg?branch=master)](https://travis-ci.com/shadowsocks/shadowsocks-libev) [![Snap Status](https://build.snapcraft.io/badge/shadowsocks/shadowsocks-libev.svg)](https://build.snapcraft.io/user/shadowsocks/shadowsocks-libev)

## Intro

[Shadowsocks-libev](https://shadowsocks.org) is a lightweight secured SOCKS5
proxy for embedded devices and low-end boxes.

It is a port of [Shadowsocks](https://github.com/shadowsocks/shadowsocks)
created by [@clowwindy](https://github.com/clowwindy), and maintained by
[@madeye](https://github.com/madeye) and [@linusyang](https://github.com/linusyang).

Current version: 3.3.4 | [Changelog](debian/changelog)

## Features

Shadowsocks-libev is written in pure C and depends on [libev](http://software.schmorp.de/pkg/libev.html). It's designed
to be a lightweight implementation of shadowsocks protocol, in order to keep the resource usage as low as possible.

For a full list of feature comparison between different versions of shadowsocks,
refer to the [Wiki page](https://github.com/shadowsocks/shadowsocks/wiki/Feature-Comparison-across-Different-Versions).

## Quick Start

Snap is the recommended way to install the latest binaries.

### Install snap core

https://snapcraft.io/core

### Install from snapcraft.io

Stable channel:

```bash
sudo snap install shadowsocks-libev
```

Edge channel:

```bash
sudo snap install shadowsocks-libev --edge
```

## Installation

### Distribution-specific guide

- [Debian & Ubuntu](#debian--ubuntu)
    + [Install from repository](#install-from-repository)
    + [Build deb package from source](#build-deb-package-from-source)
    + [Configure and start the service](#configure-and-start-the-service)
- [Fedora & RHEL](#fedora--rhel)
    + [Build from source with centos](#build-from-source-with-centos)
    + [Install from repository](#install-from-repository-1)
- [Archlinux & Manjaro](#archlinux--manjaro)
- [NixOS](#nixos)
- [Nix](#nix)
- [Directly build and install on UNIX-like system](#linux)
- [FreeBSD](#freebsd)
    + [Install](#install)
    + [Configuration](#configuration)
    + [Run](#run)
    + [Run as client](#run-as-client)
- [OpenWRT](#openwrt)
- [OS X](#os-x)
- [Windows (MinGW)](#windows-mingw)
- [Docker](#docker)

* * *

### Pre-build configure guide

For a complete list of available configure-time option,
try `configure --help`.

### Debian & Ubuntu

#### Install from repository

Shadowsocks-libev is available in the official repository for following distributions:

* Debian 8 or higher, including oldoldstable (jessie), old stable (stretch), stable (buster), testing (bullseye) and unstable (sid)
* Ubuntu 16.10 or higher

```bash
sudo apt update
sudo apt install shadowsocks-libev
```

For **Debian 9 (Stretch)** users, please install it from `stretch-backports`:
We strongly encourage you to install shadowsocks-libev from `stretch-backports`.
For more info about backports, you can refer [Debian Backports](https://backports.debian.org).

```bash
sudo sh -c 'printf "deb http://deb.debian.org/debian stretch-backports main" > /etc/apt/sources.list.d/stretch-backports.list'
sudo apt update
sudo apt -t stretch-backports install shadowsocks-libev
```

#### Build deb package from source

Supported distributions:

* Debian 8, 9 or higher
* Ubuntu 14.04 LTS, 16.04 LTS, 16.10 or higher

You can build shadowsocks-libev and all its dependencies by script:

```bash
mkdir -p ~/build-area/
cp ./scripts/build_deb.sh ~/build-area/
cd ~/build-area
./build_deb.sh
```

For older systems, building `.deb` packages is not supported.
Please try to build and install directly from source. See the [Linux](#linux) section below.

**Note for Debian 8 (Jessie) users to build their own deb packages**:

We strongly encourage you to install shadowsocks-libev from `jessie-backports-sloppy`. If you insist on building from source, you will need to manually install libsodium from `jessie-backports-sloppy`, **NOT** libsodium in main repository.

For more info about backports, you can refer [Debian Backports](https://backports.debian.org).

``` bash
cd shadowsocks-libev
sudo sh -c 'printf "deb http://deb.debian.org/debian jessie-backports main" > /etc/apt/sources.list.d/jessie-backports.list'
sudo sh -c 'printf "deb http://deb.debian.org/debian jessie-backports-sloppy main" >> /etc/apt/sources.list.d/jessie-backports.list'
sudo apt-get install --no-install-recommends devscripts equivs
mk-build-deps --root-cmd sudo --install --tool "apt-get -o Debug::pkgProblemResolver=yes --no-install-recommends -y"
./autogen.sh && dpkg-buildpackage -b -us -uc
cd ..
sudo dpkg -i shadowsocks-libev*.deb
```

**Note for Debian 9 (Stretch) users to build their own deb packages**:

We strongly encourage you to install shadowsocks-libev from `stretch-backports`. If you insist on building from source, you will need to manually install libsodium from `stretch-backports`, **NOT** libsodium in main repository.

For more info about backports, you can refer [Debian Backports](https://backports.debian.org).

``` bash
cd shadowsocks-libev
sudo sh -c 'printf "deb http://deb.debian.org/debian stretch-backports main" > /etc/apt/sources.list.d/stretch-backports.list'
sudo apt-get install --no-install-recommends devscripts equivs
mk-build-deps --root-cmd sudo --install --tool "apt-get -o Debug::pkgProblemResolver=yes --no-install-recommends -y"
./autogen.sh && dpkg-buildpackage -b -us -uc
cd ..
sudo dpkg -i shadowsocks-libev*.deb
```

#### Configure and start the service

```
# Edit the configuration file
sudo vim /etc/shadowsocks-libev/config.json

# Edit the default configuration for debian
sudo vim /etc/default/shadowsocks-libev

# Start the service
sudo /etc/init.d/shadowsocks-libev start    # for sysvinit, or
sudo systemctl start shadowsocks-libev      # for systemd
```

### Fedora & RHEL

Supported distributions:

* Recent Fedora versions (until EOL)
* RHEL 6, 7 and derivatives (including CentOS, Scientific Linux)

#### Build from source with centos

If you are using CentOS 7, you need to install these prequirement to build from source code:

```bash
yum install epel-release -y
yum install gcc gettext autoconf libtool automake make pcre-devel asciidoc xmlto c-ares-devel libev-devel libsodium-devel mbedtls-devel -y
```

#### Install from repository

Enable repo via `dnf`:

```
su -c 'dnf copr enable librehat/shadowsocks'
```

Or download yum repo on [Fedora Copr](https://copr.fedoraproject.org/coprs/librehat/shadowsocks/) and put it inside `/etc/yum.repos.d/`. The release `Epel` is for RHEL and its derivatives.

Then, install `shadowsocks-libev` via `dnf`:

```bash
su -c 'dnf update'
su -c 'dnf install shadowsocks-libev'
```

or `yum`:

```bash
su -c 'yum update'
su -c 'yum install shadowsocks-libev'
```
The repository is maintained by [@librehat](https://github.com/librehat), any issues, please report [here](https://github.com/librehat/shadowsocks-libev/issues)

### Archlinux & Manjaro

```bash
sudo pacman -S shadowsocks-libev
```

Please refer to downstream [PKGBUILD](https://projects.archlinux.org/svntogit/community.git/tree/trunk?h=packages/shadowsocks-libev)
script for extra modifications and distribution-specific bugs.

### NixOS

```bash
nix-env -iA nixos.shadowsocks-libev
```

### Nix

```bash
nix-env -iA nixpkgs.shadowsocks-libev
```

### Linux

In general, you need the following build dependencies:

* autotools (autoconf, automake, libtool)
* gettext
* pkg-config
* libmbedtls
* libsodium
* libpcre3 (old pcre library)
* libev
* libc-ares
* asciidoc (for documentation only)
* xmlto (for documentation only)

Notes: Fedora 26  libsodium version >= 1.0.12, so you can install via dnf install libsodium instead build from source.

If your system is too old to provide libmbedtls and libsodium (later than **v1.0.8**), you will need to either install those libraries manually or upgrade your system.

If your system provides with those libraries, you **should not** install them from source. You should jump this section and install them from distribution repository instead.

For some of the distributions, you might install build dependencies like this:

```bash
# Installation of basic build dependencies
## Debian / Ubuntu
sudo apt-get install --no-install-recommends gettext build-essential autoconf libtool libpcre3-dev asciidoc xmlto libev-dev libc-ares-dev automake libmbedtls-dev libsodium-dev
## CentOS / Fedora / RHEL
sudo yum install gettext gcc autoconf libtool automake make asciidoc xmlto c-ares-devel libev-devel
## Arch
sudo pacman -S gettext gcc autoconf libtool automake make asciidoc xmlto c-ares libev

# Installation of libsodium
export LIBSODIUM_VER=1.0.16
wget https://download.libsodium.org/libsodium/releases/libsodium-$LIBSODIUM_VER.tar.gz
tar xvf libsodium-$LIBSODIUM_VER.tar.gz
pushd libsodium-$LIBSODIUM_VER
./configure --prefix=/usr && make
sudo make install
popd
sudo ldconfig

# Installation of MbedTLS
export MBEDTLS_VER=2.6.0
wget https://tls.mbed.org/download/mbedtls-$MBEDTLS_VER-gpl.tgz
tar xvf mbedtls-$MBEDTLS_VER-gpl.tgz
pushd mbedtls-$MBEDTLS_VER
make SHARED=1 CFLAGS="-O2 -fPIC"
sudo make DESTDIR=/usr install
popd
sudo ldconfig

# Start building
./autogen.sh && ./configure && make
sudo make install
```

You may need to manually install missing softwares.

### FreeBSD
#### Install
Shadowsocks-libev is available in FreeBSD Ports Collection. You can install it in either way, `pkg` or `ports`.

**pkg (recommended)**

```bash
pkg install shadowsocks-libev
```

**ports**

```bash
cd /usr/ports/net/shadowsocks-libev
make install
```

#### Configuration
Edit your `config.json` file. By default, it's located in `/usr/local/etc/shadowsocks-libev`.

To enable shadowsocks-libev, add the following rc variable to your `/etc/rc.conf` file:

```
shadowsocks_libev_enable="YES"
```

#### Run

Start the Shadowsocks server:

```bash
service shadowsocks_libev start
```

#### Run as client
By default, shadowsocks-libev is running as a server in FreeBSD. If you would like to start shadowsocks-libev in client mode, you can modify the rc script (`/usr/local/etc/rc.d/shadowsocks_libev`) manually.

```
# modify the following line from "ss-server" to "ss-local"
command="/usr/local/bin/ss-local"
```

Note that is simply a workaround, each time you upgrade the port your changes will be overwritten by the new version.

### OpenWRT

The OpenWRT project is maintained here:
[openwrt-shadowsocks](https://github.com/shadowsocks/openwrt-shadowsocks).

### OS X
For OS X, use [Homebrew](http://brew.sh) to install or build.

Install Homebrew:

```bash
ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
```
Install shadowsocks-libev:

```bash
brew install shadowsocks-libev
```

### Windows (MinGW)
To build Windows native binaries, the recommended method is to use Docker:

* On Windows: double-click `make.bat` in `docker\mingw`
* On Unix-like system:

        cd shadowsocks-libev/docker/mingw
        make

A tarball with 32-bit and 64-bit binaries will be generated in the same directory.

You could also manually use MinGW-w64 compilers to build in Unix-like shell (MSYS2/Cygwin), or cross-compile on Unix-like systems (Linux/MacOS). Please refer to build scripts in `docker/mingw`.

Currently you need to use a patched libev library for MinGW:

* https://github.com/shadowsocks/libev/archive/mingw.zip

Notice that TCP Fast Open (TFO) is only available on **Windows 10**, **1607** or later version (precisely, build >= 14393). If you are using **1709** (build 16299) or later version, you also need to run the following command in PowerShell/Command Prompt **as Administrator** and **reboot** to use TFO properly:

        netsh int tcp set global fastopenfallback=disabled

### Docker

As you expect, simply pull the image and run.
```
docker pull shadowsocks/shadowsocks-libev
docker run -e PASSWORD=<password> -p<server-port>:8388 -p<server-port>:8388/udp -d shadowsocks/shadowsocks-libev
```

More information about the image can be found [here](docker/alpine/README.md).

## Usage

For a detailed and complete list of all supported arguments,
you may refer to the man pages of the applications, respectively.

    ss-[local|redir|server|tunnel|manager]

       -s <server_host>           Host name or IP address of your remote server.

       -p <server_port>           Port number of your remote server.

       -l <local_port>            Port number of your local server.

       -k <password>              Password of your remote server.

       -m <encrypt_method>        Encrypt method: rc4-md5,
                                  aes-128-gcm, aes-192-gcm, aes-256-gcm,
                                  aes-128-cfb, aes-192-cfb, aes-256-cfb,
                                  aes-128-ctr, aes-192-ctr, aes-256-ctr,
                                  camellia-128-cfb, camellia-192-cfb,
                                  camellia-256-cfb, bf-cfb,
                                  chacha20-ietf-poly1305,
                                  xchacha20-ietf-poly1305,
                                  salsa20, chacha20 and chacha20-ietf.
                                  The default cipher is chacha20-ietf-poly1305.

       [-a <user>]                Run as another user.

       [-f <pid_file>]            The file path to store pid.

       [-t <timeout>]             Socket timeout in seconds.

       [-c <config_file>]         The path to config file.

       [-n <number>]              Max number of open files.

       [-i <interface>]           Network interface to bind.
                                  (not available in redir mode)

       [-b <local_address>]       Local address to bind.
                                  For servers: Specify the local address to use 
                                  while this server is making outbound 
                                  connections to remote servers on behalf of the
                                  clients.
                                  For clients: Specify the local address to use 
                                  while this client is making outbound 
                                  connections to the server.

       [-u]                       Enable UDP relay.
                                  (TPROXY is required in redir mode)

       [-U]                       Enable UDP relay and disable TCP relay.
                                  (not available in local mode)

       [-L <addr>:<port>]         Destination server address and port
                                  for local port forwarding.
                                  (only available in tunnel mode)

       [-6]                       Resolve hostname to IPv6 address first.

       [-d <addr>]                Name servers for internal DNS resolver.
                                  (only available in server mode)

       [--reuse-port]             Enable port reuse.

       [--fast-open]              Enable TCP fast open.
                                  with Linux kernel > 3.7.0.
                                  (only available in local and server mode)

       [--acl <acl_file>]         Path to ACL (Access Control List).
                                  (only available in local and server mode)

       [--manager-address <addr>] UNIX domain socket address.
                                  (only available in server and manager mode)

       [--mtu <MTU>]              MTU of your network interface.

       [--mptcp]                  Enable Multipath TCP on MPTCP Kernel.

       [--no-delay]               Enable TCP_NODELAY.

       [--executable <path>]      Path to the executable of ss-server.
                                  (only available in manager mode)

       [-D <path>]                Path to the working directory of ss-manager.
                                  (only available in manager mode)

       [--key <key_in_base64>]    Key of your remote server.

       [--plugin <name>]          Enable SIP003 plugin. (Experimental)

       [--plugin-opts <options>]  Set SIP003 plugin options. (Experimental)

       [-v]                       Verbose mode.

## Transparent proxy

The latest shadowsocks-libev has provided a *redir* mode. You can configure your Linux-based box or router to proxy all TCP traffic transparently, which is handy if you use an OpenWRT-powered router.

    # Create new chain
    iptables -t nat -N SHADOWSOCKS
    iptables -t mangle -N SHADOWSOCKS

    # Ignore your shadowsocks server's addresses
    # It's very IMPORTANT, just be careful.
    iptables -t nat -A SHADOWSOCKS -d 123.123.123.123 -j RETURN

    # Ignore LANs and any other addresses you'd like to bypass the proxy
    # See Wikipedia and RFC5735 for full list of reserved networks.
    # See ashi009/bestroutetb for a highly optimized CHN route list.
    iptables -t nat -A SHADOWSOCKS -d 0.0.0.0/8 -j RETURN
    iptables -t nat -A SHADOWSOCKS -d 10.0.0.0/8 -j RETURN
    iptables -t nat -A SHADOWSOCKS -d 127.0.0.0/8 -j RETURN
    iptables -t nat -A SHADOWSOCKS -d 169.254.0.0/16 -j RETURN
    iptables -t nat -A SHADOWSOCKS -d 172.16.0.0/12 -j RETURN
    iptables -t nat -A SHADOWSOCKS -d 192.168.0.0/16 -j RETURN
    iptables -t nat -A SHADOWSOCKS -d 224.0.0.0/4 -j RETURN
    iptables -t nat -A SHADOWSOCKS -d 240.0.0.0/4 -j RETURN

    # Anything else should be redirected to shadowsocks's local port
    iptables -t nat -A SHADOWSOCKS -p tcp -j REDIRECT --to-ports 12345

    # Add any UDP rules
    ip route add local default dev lo table 100
    ip rule add fwmark 1 lookup 100
    iptables -t mangle -A SHADOWSOCKS -p udp --dport 53 -j TPROXY --on-port 12345 --tproxy-mark 0x01/0x01

    # Apply the rules
    iptables -t nat -A PREROUTING -p tcp -j SHADOWSOCKS
    iptables -t mangle -A PREROUTING -j SHADOWSOCKS

    # Start the shadowsocks-redir
    ss-redir -u -c /etc/config/shadowsocks.json -f /var/run/shadowsocks.pid


## Security Tips

For any public server, to avoid users accessing localhost of your server, please add `--acl acl/server_block_local.acl` to the command line.

Although shadowsocks-libev can handle thousands of concurrent connections nicely, we still recommend
setting up your server's firewall rules to limit connections from each user:

    # Up to 32 connections are enough for normal usage
    iptables -A INPUT -p tcp --syn --dport ${SHADOWSOCKS_PORT} -m connlimit --connlimit-above 32 -j REJECT --reject-with tcp-reset

## License

```
Copyright: 2013-2015, Clow Windy <clowwindy42@gmail.com>
           2013-2018, Max Lv <max.c.lv@gmail.com>
           2014, Linus Yang <linusyang@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
```
