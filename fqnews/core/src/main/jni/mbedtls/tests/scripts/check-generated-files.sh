#! /usr/bin/env sh

# This file is part of mbed TLS (https://tls.mbed.org)
#
# Copyright (c) 2018, ARM Limited, All Rights Reserved
#
# Purpose
#
# Check if generated files are up-to-date.

set -eu

if [ -d library -a -d include -a -d tests ]; then :; else
    echo "Must be run from mbed TLS root" >&2
    exit 1
fi

check()
{
    SCRIPT=$1
    TO_CHECK=$2
    PATTERN=""
    FILES=""

    if [ -d $TO_CHECK ]; then
        for FILE in $TO_CHECK/*; do
            FILES="$FILE $FILES"
        done
    else
        FILES=$TO_CHECK
    fi

    for FILE in $FILES; do
        cp $FILE $FILE.bak
    done

    $SCRIPT

    # Compare the script output to the old files and remove backups
    for FILE in $FILES; do
        if ! diff $FILE $FILE.bak >/dev/null 2>&1; then
            echo "'$FILE' was either modified or deleted by '$SCRIPT'"
            exit 1
        fi
        mv $FILE.bak $FILE

        if [ -d $TO_CHECK ]; then
            # Create a grep regular expression that we can check against the
            # directory contents to test whether new files have been created
            if [ -z $PATTERN ]; then
                PATTERN="$(basename $FILE)"
            else
                PATTERN="$PATTERN\|$(basename $FILE)"
            fi
        fi
    done

    if [ -d $TO_CHECK ]; then
        # Check if there are any new files
        if ls -1 $TO_CHECK | grep -v "$PATTERN" >/dev/null 2>&1; then
            echo "Files were created by '$SCRIPT'"
            exit 1
        fi
    fi
}

check scripts/generate_errors.pl library/error.c
check scripts/generate_query_config.pl programs/ssl/query_config.c
check scripts/generate_features.pl library/version_features.c
check scripts/generate_visualc_files.pl visualc/VS2010
