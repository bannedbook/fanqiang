#!/bin/sh

# travis-log-failure.sh
#
# This file is part of mbed TLS (https://tls.mbed.org)
#
# Copyright (c) 2016, ARM Limited, All Rights Reserved
#
# Purpose
#
# List the server and client logs on failed ssl-opt.sh and compat.sh tests.
# This script is used to make the logs show up in the Travis test results.
#
# Some of the logs can be very long: this means usually a couple of megabytes
# but it can be much more. For example, the client log of test 273 in ssl-opt.sh
# is more than 630 Megabytes long.

if [ -d include/mbedtls ]; then :; else
    echo "$0: must be run from root" >&2
    exit 1
fi

FILES="o-srv-*.log o-cli-*.log c-srv-*.log c-cli-*.log o-pxy-*.log"
MAX_LOG_SIZE=1048576

for PATTERN in $FILES; do
    for LOG in $( ls tests/$PATTERN 2>/dev/null ); do
        echo
        echo "****** BEGIN file: $LOG ******"
        echo
        tail -c $MAX_LOG_SIZE $LOG
        echo "****** END file: $LOG ******"
        echo
        rm $LOG
    done
done
