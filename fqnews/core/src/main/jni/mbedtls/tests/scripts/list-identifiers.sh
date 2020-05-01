#!/bin/sh

set -eu

if [ -d include/mbedtls ]; then :; else
    echo "$0: must be run from root" >&2
    exit 1
fi

HEADERS=$( ls include/mbedtls/*.h | egrep -v 'compat-1\.3\.h|bn_mul' )

rm -f identifiers

grep '^[^ /#{]' $HEADERS | \
    sed -e 's/^[^:]*://' | \
    egrep -v '^(extern "C"|(typedef )?(struct|enum)( {)?$|};?$)' \
    > _decls

if true; then
sed -n -e 's/.* \**\([a-zA-Z_][a-zA-Z0-9_]*\)(.*/\1/p' \
       -e 's/.*(\*\(.*\))(.*/\1/p' _decls
grep -v '(' _decls | sed -e 's/\([a-zA-Z0-9_]*\)[;[].*/\1/' -e 's/.* \**//'
fi > _identifiers

if [ $( wc -l < _identifiers ) -eq $( wc -l < _decls ) ]; then
    rm _decls
    egrep -v '^(u?int(16|32|64)_t)$' _identifiers | sort > identifiers
    rm _identifiers
else
    echo "$0: oops, lost some identifiers" 2>&1
    exit 1
fi

wc -l identifiers
