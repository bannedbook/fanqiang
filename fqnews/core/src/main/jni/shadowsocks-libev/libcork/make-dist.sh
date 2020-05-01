#!/bin/sh

PROJECT=libcork

COMMIT="$1"
if [ -z "$COMMIT" ]; then
    COMMIT="HEAD"
fi

VERSION=$(git describe ${COMMIT})
git archive --prefix=${PROJECT}-${VERSION}/ --format=tar ${COMMIT} | \
    bzip2 -c > ${PROJECT}-${VERSION}.tar.bz2
