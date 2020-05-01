#!/usr/bin/env bash
set -e

archive() {
    export TARBALL_NAME=$1
    export TARBALL_OUTDIR=$2

    # archive this repo
    cd "$(git rev-parse --show-toplevel)"
    git archive HEAD --format=tar --prefix="${TARBALL_NAME}/" \
        -o "${TARBALL_OUTDIR}/${TARBALL_NAME}.tar"
    # archive submodules
    git submodule update --init
    git submodule foreach --quiet 'git archive HEAD --format=tar \
            --prefix="${TARBALL_NAME}/${path}/" \
            -o "${TARBALL_OUTDIR}/${TARBALL_NAME}-submodule-${path}-${sha1}.tar"
        tar -n --concatenate --file="${TARBALL_OUTDIR}/${TARBALL_NAME}.tar" \
            "${TARBALL_OUTDIR}/${TARBALL_NAME}-submodule-${path}-${sha1}.tar"'
    gzip -c "${TARBALL_OUTDIR}/${TARBALL_NAME}.tar" > "${TARBALL_OUTDIR}/${TARBALL_NAME}.tar.gz"

    # clean-up
    git submodule foreach --quiet 'rm ${TARBALL_OUTDIR}/${TARBALL_NAME}-submodule-${path}-${sha1}.tar'
    rm "${TARBALL_OUTDIR}/${TARBALL_NAME}.tar"
}

TARGET_TARBALL_NAME=shadowsocks-libev
TARGET_TARBALL_DIR=$(git rev-parse --show-toplevel)

while getopts "n:o:" opt
do
    case ${opt} in
        o)
            TARGET_TARBALL_DIR=$(readlink -f -- $OPTARG)
            ;;
        n)
            TARGET_TARBALL_NAME=$OPTARG
            ;;
        \?)
            exit 1
            ;;
    esac
done

archive "${TARGET_TARBALL_NAME}" "${TARGET_TARBALL_DIR}"
