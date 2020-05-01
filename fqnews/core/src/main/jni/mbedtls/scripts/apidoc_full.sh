#!/bin/sh

# Generate doxygen documentation with a full config.h (this ensures that every
# available flag is documented, and avoids warnings about documentation
# without a corresponding #define).
#
# /!\ This must not be a Makefile target, as it would create a race condition
# when multiple targets are invoked in the same parallel build.

set -eu

CONFIG_H='include/mbedtls/config.h'

if [ -r $CONFIG_H ]; then :; else
    echo "$CONFIG_H not found" >&2
    exit 1
fi

CONFIG_BAK=${CONFIG_H}.bak
cp -p $CONFIG_H $CONFIG_BAK

scripts/config.pl realfull
make apidoc

mv $CONFIG_BAK $CONFIG_H
