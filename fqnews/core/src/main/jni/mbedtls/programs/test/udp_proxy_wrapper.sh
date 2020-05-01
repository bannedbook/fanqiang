#!/bin/sh
# -*-sh-basic-offset: 4-*-
# Usage: udp_proxy_wrapper.sh [PROXY_PARAM...] -- [SERVER_PARAM...]

set -u

MBEDTLS_BASE="$(dirname -- "$0")/../.."
TPXY_BIN="$MBEDTLS_BASE/programs/test/udp_proxy"
SRV_BIN="$MBEDTLS_BASE/programs/ssl/ssl_server2"

: ${VERBOSE:=0}

stop_proxy() {
    if [ -n "${tpxy_pid:-}" ]; then
        echo
        echo "  * Killing proxy (pid $tpxy_pid) ..."
        kill $tpxy_pid
    fi
}

stop_server() {
    if [ -n "${srv_pid:-}" ]; then
        echo
        echo "  * Killing server (pid $srv_pid) ..."
        kill $srv_pid >/dev/null 2>/dev/null
    fi
}

cleanup() {
    stop_server
    stop_proxy
    exit 129
}

trap cleanup INT TERM HUP

# Extract the proxy parameters
tpxy_cmd_snippet='"$TPXY_BIN"'
while [ $# -ne 0 ] && [ "$1" != "--" ]; do
    tail="$1" quoted=""
    while [ -n "$tail" ]; do
        case "$tail" in
            *\'*) quoted="${quoted}${tail%%\'*}'\\''" tail="${tail#*\'}";;
            *) quoted="${quoted}${tail}"; tail=; false;;
        esac
    done
    tpxy_cmd_snippet="$tpxy_cmd_snippet '$quoted'"
    shift
done
unset tail quoted
if [ $# -eq 0 ]; then
    echo "  * No server arguments (must be preceded by \" -- \") - exit"
    exit 3
fi
shift

dtls_enabled=
ipv6_in_use=
server_port_orig=
server_addr_orig=
for param; do
    case "$param" in
        server_port=*) server_port_orig="${param#*=}";;
        server_addr=*:*) server_addr_orig="${param#*=}"; ipv6_in_use=1;;
        server_addr=*) server_addr_orig="${param#*=}";;
        dtls=[!0]*) dtls_enabled=1;;
    esac
done

if [ -z "$dtls_enabled" ] || [ -n "$ipv6_in_use" ]; then
    echo >&2 "$0: Couldn't find DTLS enabling, or IPv6 is in use - immediate fallback to server application..."
    if [ $VERBOSE -gt 0 ]; then
        echo "[ $SRV_BIN $* ]"
    fi
    exec "$SRV_BIN" "$@"
fi

if [ -z "$server_port_orig" ]; then
    server_port_orig=4433
fi
echo "  * Server port:       $server_port_orig"
tpxy_cmd_snippet="$tpxy_cmd_snippet \"listen_port=\$server_port_orig\""
tpxy_cmd_snippet="$tpxy_cmd_snippet \"server_port=\$server_port\""

if [ -n "$server_addr_orig" ]; then
    echo "  * Server address:    $server_addr_orig"
    tpxy_cmd_snippet="$tpxy_cmd_snippet \"server_addr=\$server_addr_orig\""
    tpxy_cmd_snippet="$tpxy_cmd_snippet \"listen_addr=\$server_addr_orig\""
fi

server_port=$(( server_port_orig + 1 ))
set -- "$@" "server_port=$server_port"
echo "  * Intermediate port: $server_port"

echo "  * Start proxy in background ..."
if [ $VERBOSE -gt 0 ]; then
    echo "[ $tpxy_cmd_snippet ]"
fi
eval exec "$tpxy_cmd_snippet" >/dev/null 2>&1 &
tpxy_pid=$!

if [ $VERBOSE -gt 0 ]; then
    echo "  * Proxy ID:          $TPXY_PID"
fi

echo "  * Starting server ..."
if [ $VERBOSE -gt 0 ]; then
    echo "[ $SRV_BIN $* ]"
fi

exec "$SRV_BIN" "$@" >&2 &
srv_pid=$!

wait $srv_pid

stop_proxy
return 0
