#!/bin/sh

# This script splits the data test files containing the test cases into
# individual files (one test case per file) suitable for use with afl
# (American Fuzzy Lop). http://lcamtuf.coredump.cx/afl/
#
# Usage: generate-afl-tests.sh <test data file path>
#  <test data file path> - should be the path to one of the test suite files
#                          such as 'test_suite_mpi.data'

# Abort on errors
set -e

if [ -z $1 ]
then
    echo " [!] No test file specified" >&2
    echo "Usage: $0 <test data file>" >&2
    exit 1
fi

SRC_FILEPATH=$(dirname $1)/$(basename $1)
TESTSUITE=$(basename $1 .data)

THIS_DIR=$(basename $PWD)

if [ -d ../library -a -d ../include -a -d ../tests -a $THIS_DIR == "tests" ];
then :;
else
    echo " [!] Must be run from mbed TLS tests directory" >&2
    exit 1
fi

DEST_TESTCASE_DIR=$TESTSUITE-afl-tests
DEST_OUTPUT_DIR=$TESTSUITE-afl-out

echo " [+] Creating output directories" >&2

if [ -e $DEST_OUTPUT_DIR/* ];
then :
    echo " [!] Test output files already exist." >&2
    exit 1
else
    mkdir -p $DEST_OUTPUT_DIR
fi

if [ -e $DEST_TESTCASE_DIR/* ];
then :
    echo " [!] Test output files already exist." >&2
else
    mkdir -p $DEST_TESTCASE_DIR
fi

echo " [+] Creating test cases" >&2
cd $DEST_TESTCASE_DIR

split -p '^\s*$' ../$SRC_FILEPATH

for f in *;
do
    # Strip out any blank lines (no trim on OS X)
    sed '/^\s*$/d' $f >testcase_$f
    rm $f
done

cd ..

echo " [+] Test cases in $DEST_TESTCASE_DIR" >&2

