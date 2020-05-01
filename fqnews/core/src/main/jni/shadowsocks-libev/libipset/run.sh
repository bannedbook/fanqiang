#!/bin/sh
# Usage: run.sh [debug|release] program arguments
#
# Runs a program from one of the build directories, with
# LD_LIBRARY_PATH set correctly so that it can find all of the shared
# libraries before they're installed.


# Check that there are enough command-line parameters.

if [ $# -lt 1 ]; then
    echo "Usage: run.sh [debug|release] program arguments"
    exit 1
fi


# Verify that the user chose a valid build type.

BUILD="$1"
shift

case "$BUILD" in
    debug)
        ;;
    release)
        ;;
    *)
        echo "Unknown build type $BUILD"
        exit 1
        ;;
esac


# Find all of the src subdirectories in the build directory, and use
# those as the LD_LIBRARY_PATH.

SRC_DIRS=$(find build/$BUILD -name src)
JOINED=$(echo $SRC_DIRS | perl -ne 'print join(":", split)')


# Run the desired program, and pass on any command-line arguments
# as-is.

LD_LIBRARY_PATH="$JOINED:$LD_LIBRARY_PATH" \
DYLD_LIBRARY_PATH="$JOINED:$DYLD_LIBRARY_PATH" \
  "$@"
