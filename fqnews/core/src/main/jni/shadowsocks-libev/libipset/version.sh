#!/bin/sh
# ----------------------------------------------------------------------
# Copyright © 2011-2013, RedJack, LLC.
# All rights reserved.
#
# Please see the LICENSE.txt file in this distribution for license
# details.
# ----------------------------------------------------------------------

# Calculates the current version number.  If possible, this is the
# output of “git describe”.  If “git describe” returns an error (most
# likely because we're in an unpacked copy of a release tarball, rather
# than in a git working copy), then we fall back on reading the contents
# of the RELEASE-VERSION file.
#
# This will automatically update the RELEASE-VERSION file, if necessary.
# Note that the RELEASE-VERSION file should *not* be checked into git;
# please add it to your top-level .gitignore file.

version=$(git describe)
if [ -n ${version} ]; then
    # If we got something from git-describe, write the version to the
    # output file.
    echo ${version} > RELEASE-VERSION
else
    version=$(cat RELEASE-VERSION)
    if [ -z ${version} ]; then
        echo "Cannot find the version number!" >&2
        exit 1
    fi
fi

echo ${version}
