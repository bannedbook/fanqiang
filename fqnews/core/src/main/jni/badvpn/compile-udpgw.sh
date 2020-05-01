#!/usr/bin/env bash
#
# Compiles udpgw for Linux.
# Intended as a convenience if you don't want to deal with CMake.

# Input environment vars:
#   SRCDIR - BadVPN source code
#   CC - compiler
#   CFLAGS - compiler compile flags
#   LDFLAGS - compiler link flags
#   ENDIAN - "little" or "big"
#   KERNEL - "2.6" or "2.4", default "2.6"
#
# Puts object files and the executable in the working directory.
#

if [[ -z $SRCDIR ]] || [[ ! -e $SRCDIR/CMakeLists.txt ]]; then
    echo "SRCDIR is wrong"
    exit 1
fi

if ! "${CC}" --version &>/dev/null; then
    echo "CC is wrong"
    exit 1
fi

if [[ $ENDIAN != "little" ]] && [[ $ENDIAN != "big" ]]; then
    echo "ENDIAN is wrong"
    exit 1
fi

if [[ -z $KERNEL ]]; then
    KERNEL="2.6"
elif [[ $KERNEL != "2.6" ]] && [[ $KERNEL != "2.4" ]]; then
    echo "KERNEL is wrong"
    exit 1
fi

CFLAGS="${CFLAGS} -std=gnu99"
INCLUDES=( "-I${SRCDIR}" )
DEFS=( -DBADVPN_THREAD_SAFE=0 -DBADVPN_LINUX -DBADVPN_BREACTOR_BADVPN -D_GNU_SOURCE )

[[ $KERNEL = "2.4" ]] && DEFS=( "${DEFS[@]}" -DBADVPN_USE_SELFPIPE -DBADVPN_USE_POLL ) || DEFS=( "${DEFS[@]}" -DBADVPN_USE_SIGNALFD -DBADVPN_USE_EPOLL )

[[ $ENDIAN = "little" ]] && DEFS=( "${DEFS[@]}" -DBADVPN_LITTLE_ENDIAN ) || DEFS=( "${DEFS[@]}" -DBADVPN_BIG_ENDIAN )
    
SOURCES="
base/BLog_syslog.c
system/BReactor_badvpn.c
system/BSignal.c
system/BConnection_unix.c
system/BConnection_common.c
system/BDatagram_unix.c
system/BTime.c
system/BUnixSignal.c
system/BNetwork.c
flow/StreamRecvInterface.c
flow/PacketRecvInterface.c
flow/PacketPassInterface.c
flow/StreamPassInterface.c
flow/SinglePacketBuffer.c
flow/BufferWriter.c
flow/PacketBuffer.c
flow/PacketStreamSender.c
flow/PacketProtoFlow.c
flow/PacketPassFairQueue.c
flow/PacketProtoEncoder.c
flow/PacketProtoDecoder.c
base/DebugObject.c
base/BLog.c
base/BPending.c
udpgw/udpgw.c
"

set -e
set -x

OBJS=()
for f in $SOURCES; do
    obj=$(basename "${f}").o
    "${CC}" -c ${CFLAGS} "${INCLUDES[@]}" "${DEFS[@]}" "${SRCDIR}/${f}" -o "${obj}"
    OBJS=( "${OBJS[@]}" "${obj}" )
done

"${CC}" ${LDFLAGS} "${OBJS[@]}" -o udpgw -lrt -lpthread
