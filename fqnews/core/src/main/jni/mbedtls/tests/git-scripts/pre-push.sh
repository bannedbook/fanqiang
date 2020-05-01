#!/bin/sh
# pre-push.sh
#
# This file is part of mbed TLS (https://tls.mbed.org)
#
# Copyright (c) 2017, ARM Limited, All Rights Reserved
#
# Purpose
#
# Called by "git push" after it has checked the remote status, but before anything has been
# pushed.  If this script exits with a non-zero status nothing will be pushed.
# This script can also be used independently, not using git.
#
# This hook is called with the following parameters:
#
# $1 -- Name of the remote to which the push is being done
# $2 -- URL to which the push is being done
#
# If pushing without using a named remote those arguments will be equal.
#
# Information about the commits which are being pushed is supplied as lines to
# the standard input in the form:
#
#   <local ref> <local sha1> <remote ref> <remote sha1>
#

REMOTE="$1"
URL="$2"

echo "REMOTE is $REMOTE"
echo "URL is $URL"

set -eu

run_test()
{
    TEST=$1
    echo "running '$TEST'"
    if ! `$TEST > /dev/null 2>&1`; then
        echo "test '$TEST' failed"
        return 1
    fi
}

run_test ./tests/scripts/check-doxy-blocks.pl
run_test ./tests/scripts/check-names.sh
run_test ./tests/scripts/check-generated-files.sh
run_test ./tests/scripts/check-files.py
run_test ./tests/scripts/doxygen.sh
