#! /usr/bin/env sh

# output_env.sh
#
# This file is part of mbed TLS (https://tls.mbed.org)
#
# Copyright (c) 2016, ARM Limited, All Rights Reserved
#
# Purpose
#
# To print out all the relevant information about the development environment.
#
# This includes:
#   - architecture of the system
#   - type and version of the operating system
#   - version of armcc, clang, gcc-arm and gcc compilers
#   - version of libc, clang, asan and valgrind if installed
#   - version of gnuTLS and OpenSSL

print_version()
{
    BIN="$1"
    shift
    ARGS="$1"
    shift
    FAIL_MSG="$1"
    shift

    if ! `type "$BIN" > /dev/null 2>&1`; then
        echo "* $FAIL_MSG"
        return 0
    fi

    BIN=`which "$BIN"`
    VERSION_STR=`$BIN $ARGS 2>&1`

    # Apply all filters
    while [ $# -gt 0 ]; do
        FILTER="$1"
        shift
        VERSION_STR=`echo "$VERSION_STR" | $FILTER`
    done

    echo "* ${BIN##*/}: $BIN: $VERSION_STR"
}

print_version "uname" "-a" ""
echo

if [ "${RUN_ARMCC:-1}" -ne 0 ]; then
    : "${ARMC5_CC:=armcc}"
    print_version "$ARMC5_CC" "--vsn" "armcc not found!" "head -n 2"
    echo

    : "${ARMC6_CC:=armclang}"
    print_version "$ARMC6_CC" "--vsn" "armclang not found!" "head -n 2"
    echo
fi

print_version "arm-none-eabi-gcc" "--version" "gcc-arm not found!" "head -n 1"
echo

print_version "gcc" "--version" "gcc not found!" "head -n 1"
echo

print_version "clang" "--version" "clang not found" "head -n 2"
echo

print_version "ldd" "--version"                     \
    "No ldd present: can't determine libc version!" \
    "head -n 1"
echo

print_version "valgrind" "--version" "valgrind not found!"
echo

: ${OPENSSL:=openssl}
print_version "$OPENSSL" "version" "openssl not found!"
echo

if [ -n "${OPENSSL_LEGACY+set}" ]; then
    print_version "$OPENSSL_LEGACY" "version" "openssl legacy version not found!"
    echo
fi

if [ -n "${OPENSSL_NEXT+set}" ]; then
    print_version "$OPENSSL_NEXT" "version" "openssl next version not found!"
    echo
fi

: ${GNUTLS_CLI:=gnutls-cli}
print_version "$GNUTLS_CLI" "--version" "gnuTLS client not found!" "head -n 1"
echo

: ${GNUTLS_SERV:=gnutls-serv}
print_version "$GNUTLS_SERV" "--version" "gnuTLS server not found!" "head -n 1"
echo

if [ -n "${GNUTLS_LEGACY_CLI+set}" ]; then
    print_version "$GNUTLS_LEGACY_CLI" "--version" \
        "gnuTLS client legacy version not found!"  \
        "head -n 1"
    echo
fi

if [ -n "${GNUTLS_LEGACY_SERV+set}" ]; then
    print_version "$GNUTLS_LEGACY_SERV" "--version" \
        "gnuTLS server legacy version not found!"   \
        "head -n 1"
    echo
fi

if `hash dpkg > /dev/null 2>&1`; then
    echo "* asan:"
    dpkg -s libasan2 2> /dev/null | grep -i version
    dpkg -s libasan1 2> /dev/null | grep -i version
    dpkg -s libasan0 2> /dev/null | grep -i version
else
    echo "* No dpkg present: can't determine asan version!"
fi
echo
