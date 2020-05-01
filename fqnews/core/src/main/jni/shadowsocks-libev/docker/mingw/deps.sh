#!/bin/bash
#
# Functions for building MinGW port in Docker
#
# This file is part of the shadowsocks-libev.
#
# shadowsocks-libev is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# shadowsocks-libev is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with shadowsocks-libev; see the file COPYING. If not, see
# <http://www.gnu.org/licenses/>.
#

# Exit on error
set -e

. /prepare.sh

build_deps() {
    arch=$1
    host=$arch-w64-mingw32
    prefix=${PREFIX}/$arch
    args="--host=${host} --prefix=${prefix} --disable-shared --enable-static"
    cpu="$(nproc --all)"

    # libev
    cd "$SRC/$LIBEV_SRC"
    ./configure $args
    make clean
    make -j$cpu install

    # mbedtls
    cd "$SRC/$MBEDTLS_SRC"
    make clean
    make -j$cpu lib WINDOWS=1 CC="${host}-gcc" AR="${host}-ar"
    ## "make install" command from mbedtls
    DESTDIR="${prefix}"
    mkdir -p "${DESTDIR}"/include/mbedtls
    cp -r include/mbedtls "${DESTDIR}"/include
    mkdir -p "${DESTDIR}"/lib
    cp -RP library/libmbedtls.*    "${DESTDIR}"/lib
    cp -RP library/libmbedx509.*   "${DESTDIR}"/lib
    cp -RP library/libmbedcrypto.* "${DESTDIR}"/lib
    unset DESTDIR

    # sodium
    cd "$SRC/$SODIUM_SRC"
    ./autogen.sh
    ./configure $args
    make clean
    make -j$cpu install

    # pcre
    cd "$SRC/$PCRE_SRC"
    ./configure $args --disable-cpp \
      --enable-unicode-properties
    make clean
    make -j$cpu install

    # c-ares
    cd "$SRC/$CARES_SRC"
    ./configure $args
    make clean
    make -j$cpu install
}

dk_deps() {
    for arch in i686 x86_64; do
        build_deps $arch
    done
}
