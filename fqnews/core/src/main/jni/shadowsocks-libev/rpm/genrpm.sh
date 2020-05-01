#!/usr/bin/env bash
set -e

NAME=shadowsocks-libev

SELF=$(readlink -f -- "$0")
HERE=$(dirname -- "$SELF")

SOURCES="${HERE}"/SOURCES
SPEC_TEMPLATE="${HERE}"/SPECS/${NAME}.spec.in
SPEC_FILE="${SPEC_TEMPLATE%%.in}"

GIT_VERSION=$("${HERE}"/../scripts/git_version.sh)

OPT_OUTDIR="${HERE}/SRPMS"
OPT_USE_SYSTEM_LIB=0
OUT_BUILD_RPM=0

version=$(echo ${GIT_VERSION} | cut -d' ' -f1)
release=$(echo ${GIT_VERSION} | cut -d' ' -f2)

name_version=${NAME}-${version}-${release}
source_name=${name_version}.tar.gz

archive()
{
    "${HERE}"/../scripts/git_archive.sh -o "${SOURCES}" -n ${name_version}
}

build_src_rpm()
{
    rpmbuild -bs "${SPEC_FILE}" \
       --undefine "dist" \
       --define "%_topdir ${HERE}" \
       --define "%_srcrpmdir ${OPT_OUTDIR}"
}

build_rpm()
{
    rpmbuild --rebuild "${OPT_OUTDIR}"/${name_version}.src.rpm \
       --define "%_topdir ${HERE}" \
       --define "%use_system_lib ${OPT_USE_SYSTEM_LIB}"
}

create_spec()
{
    sed -e "s/@NAME@/${NAME}/g" \
        -e "s/@VERSION@/${version}/g" \
        -e "s/@RELEASE@/${release}/g" \
        -e "s/@SOURCE@/${source_name}/g" \
        -e "s/@NAME_VERSION@/${name_version}/g" \
        "${SPEC_TEMPLATE}" > "${SPEC_FILE}"
}

show_help()
{
    echo -e "$(basename $0) [OPTION...]"
    echo -e "Create and build shadowsocks-libev SRPM"
    echo
    echo -e "Options:"
    echo -e "  -h    show this help."
    echo -e "  -b    use rpmbuld to build resulting SRPM"
    echo -e "  -s    use system shared libraries (RPM only)"
    echo -e "  -o    output directory"
}

while getopts "hbso:" opt
do
    case ${opt} in
        h)
            show_help
            exit 0
            ;;
        b)
            OPT_BUILD_RPM=1
            ;;
        s)
            OPT_USE_SYSTEM_LIB=1
            ;;
        o)
            OPT_OUTDIR=$(readlink -f -- $OPTARG)
            ;;
        *)
            show_help
            exit 1
            ;;
    esac
done

create_spec
archive
build_src_rpm
if [ "${OPT_BUILD_RPM}" = "1" ] ; then
    build_rpm
fi
