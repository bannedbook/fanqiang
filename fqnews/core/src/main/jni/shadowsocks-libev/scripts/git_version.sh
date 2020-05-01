#!/usr/bin/env bash
set -e

# determine version and release number
GIT_DESCRIBE=$(git describe --tags --match 'v*' --long)
# GIT_DESCRIBE is like v3.0.3-11-g1e3f35c-dirty
if [[ ! "$GIT_DESCRIBE" =~ ^v([^-]+)-([0-9]+)-g([0-9a-f]+)$ ]]; then
    >&2 echo 'ERROR - unrecognized `git describe` output: '"$GIT_DESCRIBE"
    exit 1
fi

version=${BASH_REMATCH[1]}
commits=${BASH_REMATCH[2]}
short_hash=${BASH_REMATCH[3]}

release=1
if [ "${commits}" -gt 0 ] ; then
    release+=.${commits}.git${short_hash}
fi

echo "${version} ${release}"
