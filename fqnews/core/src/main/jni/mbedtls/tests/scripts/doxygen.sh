#!/bin/sh

# Make sure the doxygen documentation builds without warnings

# Abort on errors (and uninitiliased variables)
set -eu

if [ -d library -a -d include -a -d tests ]; then :; else
    echo "Must be run from mbed TLS root" >&2
    exit 1
fi

if scripts/apidoc_full.sh > doc.out 2>doc.err; then :; else
    cat doc.err
    echo "FAIL" >&2
    exit 1;
fi

cat doc.out doc.err | \
    grep -v "warning: ignoring unsupported tag" \
    > doc.filtered

if egrep "(warning|error):" doc.filtered; then
    echo "FAIL" >&2
    exit 1;
fi

make apidoc_clean
rm -f doc.out doc.err doc.filtered
