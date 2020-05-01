#!/bin/sh

# ssl-opt.sh
#
# This file is part of mbed TLS (https://tls.mbed.org)
#
# Copyright (c) 2016, ARM Limited, All Rights Reserved
#
# Purpose
#
# Executes tests to prove various TLS/SSL options and extensions.
#
# The goal is not to cover every ciphersuite/version, but instead to cover
# specific options (max fragment length, truncated hmac, etc) or procedures
# (session resumption from cache or ticket, renego, etc).
#
# The tests assume a build with default options, with exceptions expressed
# with a dependency.  The tests focus on functionality and do not consider
# performance.
#

set -u

if cd $( dirname $0 ); then :; else
    echo "cd $( dirname $0 ) failed" >&2
    exit 1
fi

# default values, can be overriden by the environment
: ${P_SRV:=../programs/ssl/ssl_server2}
: ${P_CLI:=../programs/ssl/ssl_client2}
: ${P_PXY:=../programs/test/udp_proxy}
: ${OPENSSL_CMD:=openssl} # OPENSSL would conflict with the build system
: ${GNUTLS_CLI:=gnutls-cli}
: ${GNUTLS_SERV:=gnutls-serv}
: ${PERL:=perl}

O_SRV="$OPENSSL_CMD s_server -www -cert data_files/server5.crt -key data_files/server5.key"
O_CLI="echo 'GET / HTTP/1.0' | $OPENSSL_CMD s_client"
G_SRV="$GNUTLS_SERV --x509certfile data_files/server5.crt --x509keyfile data_files/server5.key"
G_CLI="echo 'GET / HTTP/1.0' | $GNUTLS_CLI --x509cafile data_files/test-ca_cat12.crt"
TCP_CLIENT="$PERL scripts/tcp_client.pl"

# alternative versions of OpenSSL and GnuTLS (no default path)

if [ -n "${OPENSSL_LEGACY:-}" ]; then
    O_LEGACY_SRV="$OPENSSL_LEGACY s_server -www -cert data_files/server5.crt -key data_files/server5.key"
    O_LEGACY_CLI="echo 'GET / HTTP/1.0' | $OPENSSL_LEGACY s_client"
else
    O_LEGACY_SRV=false
    O_LEGACY_CLI=false
fi

if [ -n "${GNUTLS_NEXT_SERV:-}" ]; then
    G_NEXT_SRV="$GNUTLS_NEXT_SERV --x509certfile data_files/server5.crt --x509keyfile data_files/server5.key"
else
    G_NEXT_SRV=false
fi

if [ -n "${GNUTLS_NEXT_CLI:-}" ]; then
    G_NEXT_CLI="echo 'GET / HTTP/1.0' | $GNUTLS_NEXT_CLI --x509cafile data_files/test-ca_cat12.crt"
else
    G_NEXT_CLI=false
fi

TESTS=0
FAILS=0
SKIPS=0

CONFIG_H='../include/mbedtls/config.h'

MEMCHECK=0
FILTER='.*'
EXCLUDE='^$'

SHOW_TEST_NUMBER=0
RUN_TEST_NUMBER=''

PRESERVE_LOGS=0

# Pick a "unique" server port in the range 10000-19999, and a proxy
# port which is this plus 10000. Each port number may be independently
# overridden by a command line option.
SRV_PORT=$(($$ % 10000 + 10000))
PXY_PORT=$((SRV_PORT + 10000))

print_usage() {
    echo "Usage: $0 [options]"
    printf "  -h|--help\tPrint this help.\n"
    printf "  -m|--memcheck\tCheck memory leaks and errors.\n"
    printf "  -f|--filter\tOnly matching tests are executed (BRE; default: '$FILTER')\n"
    printf "  -e|--exclude\tMatching tests are excluded (BRE; default: '$EXCLUDE')\n"
    printf "  -n|--number\tExecute only numbered test (comma-separated, e.g. '245,256')\n"
    printf "  -s|--show-numbers\tShow test numbers in front of test names\n"
    printf "  -p|--preserve-logs\tPreserve logs of successful tests as well\n"
    printf "     --port\tTCP/UDP port (default: randomish 1xxxx)\n"
    printf "     --proxy-port\tTCP/UDP proxy port (default: randomish 2xxxx)\n"
    printf "     --seed\tInteger seed value to use for this test run\n"
}

get_options() {
    while [ $# -gt 0 ]; do
        case "$1" in
            -f|--filter)
                shift; FILTER=$1
                ;;
            -e|--exclude)
                shift; EXCLUDE=$1
                ;;
            -m|--memcheck)
                MEMCHECK=1
                ;;
            -n|--number)
                shift; RUN_TEST_NUMBER=$1
                ;;
            -s|--show-numbers)
                SHOW_TEST_NUMBER=1
                ;;
            -p|--preserve-logs)
                PRESERVE_LOGS=1
                ;;
            --port)
                shift; SRV_PORT=$1
                ;;
            --proxy-port)
                shift; PXY_PORT=$1
                ;;
            --seed)
                shift; SEED="$1"
                ;;
            -h|--help)
                print_usage
                exit 0
                ;;
            *)
                echo "Unknown argument: '$1'"
                print_usage
                exit 1
                ;;
        esac
        shift
    done
}

# Skip next test; use this macro to skip tests which are legitimate
# in theory and expected to be re-introduced at some point, but
# aren't expected to succeed at the moment due to problems outside
# our control (such as bugs in other TLS implementations).
skip_next_test() {
    SKIP_NEXT="YES"
}

# skip next test if the flag is not enabled in config.h
requires_config_enabled() {
    if grep "^#define $1" $CONFIG_H > /dev/null; then :; else
        SKIP_NEXT="YES"
    fi
}

# skip next test if the flag is enabled in config.h
requires_config_disabled() {
    if grep "^#define $1" $CONFIG_H > /dev/null; then
        SKIP_NEXT="YES"
    fi
}

get_config_value_or_default() {
    # This function uses the query_config command line option to query the
    # required Mbed TLS compile time configuration from the ssl_server2
    # program. The command will always return a success value if the
    # configuration is defined and the value will be printed to stdout.
    #
    # Note that if the configuration is not defined or is defined to nothing,
    # the output of this function will be an empty string.
    ${P_SRV} "query_config=${1}"
}

requires_config_value_at_least() {
    VAL="$( get_config_value_or_default "$1" )"
    if [ -z "$VAL" ]; then
        # Should never happen
        echo "Mbed TLS configuration $1 is not defined"
        exit 1
    elif [ "$VAL" -lt "$2" ]; then
       SKIP_NEXT="YES"
    fi
}

requires_config_value_at_most() {
    VAL=$( get_config_value_or_default "$1" )
    if [ -z "$VAL" ]; then
        # Should never happen
        echo "Mbed TLS configuration $1 is not defined"
        exit 1
    elif [ "$VAL" -gt "$2" ]; then
       SKIP_NEXT="YES"
    fi
}

# skip next test if OpenSSL doesn't support FALLBACK_SCSV
requires_openssl_with_fallback_scsv() {
    if [ -z "${OPENSSL_HAS_FBSCSV:-}" ]; then
        if $OPENSSL_CMD s_client -help 2>&1 | grep fallback_scsv >/dev/null
        then
            OPENSSL_HAS_FBSCSV="YES"
        else
            OPENSSL_HAS_FBSCSV="NO"
        fi
    fi
    if [ "$OPENSSL_HAS_FBSCSV" = "NO" ]; then
        SKIP_NEXT="YES"
    fi
}

# skip next test if GnuTLS isn't available
requires_gnutls() {
    if [ -z "${GNUTLS_AVAILABLE:-}" ]; then
        if ( which "$GNUTLS_CLI" && which "$GNUTLS_SERV" ) >/dev/null 2>&1; then
            GNUTLS_AVAILABLE="YES"
        else
            GNUTLS_AVAILABLE="NO"
        fi
    fi
    if [ "$GNUTLS_AVAILABLE" = "NO" ]; then
        SKIP_NEXT="YES"
    fi
}

# skip next test if GnuTLS-next isn't available
requires_gnutls_next() {
    if [ -z "${GNUTLS_NEXT_AVAILABLE:-}" ]; then
        if ( which "${GNUTLS_NEXT_CLI:-}" && which "${GNUTLS_NEXT_SERV:-}" ) >/dev/null 2>&1; then
            GNUTLS_NEXT_AVAILABLE="YES"
        else
            GNUTLS_NEXT_AVAILABLE="NO"
        fi
    fi
    if [ "$GNUTLS_NEXT_AVAILABLE" = "NO" ]; then
        SKIP_NEXT="YES"
    fi
}

# skip next test if OpenSSL-legacy isn't available
requires_openssl_legacy() {
    if [ -z "${OPENSSL_LEGACY_AVAILABLE:-}" ]; then
        if which "${OPENSSL_LEGACY:-}" >/dev/null 2>&1; then
            OPENSSL_LEGACY_AVAILABLE="YES"
        else
            OPENSSL_LEGACY_AVAILABLE="NO"
        fi
    fi
    if [ "$OPENSSL_LEGACY_AVAILABLE" = "NO" ]; then
        SKIP_NEXT="YES"
    fi
}

# skip next test if IPv6 isn't available on this host
requires_ipv6() {
    if [ -z "${HAS_IPV6:-}" ]; then
        $P_SRV server_addr='::1' > $SRV_OUT 2>&1 &
        SRV_PID=$!
        sleep 1
        kill $SRV_PID >/dev/null 2>&1
        if grep "NET - Binding of the socket failed" $SRV_OUT >/dev/null; then
            HAS_IPV6="NO"
        else
            HAS_IPV6="YES"
        fi
        rm -r $SRV_OUT
    fi

    if [ "$HAS_IPV6" = "NO" ]; then
        SKIP_NEXT="YES"
    fi
}

# skip next test if it's i686 or uname is not available
requires_not_i686() {
    if [ -z "${IS_I686:-}" ]; then
        IS_I686="YES"
        if which "uname" >/dev/null 2>&1; then
            if [ -z "$(uname -a | grep i686)" ]; then
                IS_I686="NO"
            fi
        fi
    fi
    if [ "$IS_I686" = "YES" ]; then
        SKIP_NEXT="YES"
    fi
}

# Calculate the input & output maximum content lengths set in the config
MAX_CONTENT_LEN=$( ../scripts/config.pl get MBEDTLS_SSL_MAX_CONTENT_LEN || echo "16384")
MAX_IN_LEN=$( ../scripts/config.pl get MBEDTLS_SSL_IN_CONTENT_LEN || echo "$MAX_CONTENT_LEN")
MAX_OUT_LEN=$( ../scripts/config.pl get MBEDTLS_SSL_OUT_CONTENT_LEN || echo "$MAX_CONTENT_LEN")

if [ "$MAX_IN_LEN" -lt "$MAX_CONTENT_LEN" ]; then
    MAX_CONTENT_LEN="$MAX_IN_LEN"
fi
if [ "$MAX_OUT_LEN" -lt "$MAX_CONTENT_LEN" ]; then
    MAX_CONTENT_LEN="$MAX_OUT_LEN"
fi

# skip the next test if the SSL output buffer is less than 16KB
requires_full_size_output_buffer() {
    if [ "$MAX_OUT_LEN" -ne 16384 ]; then
        SKIP_NEXT="YES"
    fi
}

# skip the next test if valgrind is in use
not_with_valgrind() {
    if [ "$MEMCHECK" -gt 0 ]; then
        SKIP_NEXT="YES"
    fi
}

# skip the next test if valgrind is NOT in use
only_with_valgrind() {
    if [ "$MEMCHECK" -eq 0 ]; then
        SKIP_NEXT="YES"
    fi
}

# multiply the client timeout delay by the given factor for the next test
client_needs_more_time() {
    CLI_DELAY_FACTOR=$1
}

# wait for the given seconds after the client finished in the next test
server_needs_more_time() {
    SRV_DELAY_SECONDS=$1
}

# print_name <name>
print_name() {
    TESTS=$(( $TESTS + 1 ))
    LINE=""

    if [ "$SHOW_TEST_NUMBER" -gt 0 ]; then
        LINE="$TESTS "
    fi

    LINE="$LINE$1"
    printf "$LINE "
    LEN=$(( 72 - `echo "$LINE" | wc -c` ))
    for i in `seq 1 $LEN`; do printf '.'; done
    printf ' '

}

# fail <message>
fail() {
    echo "FAIL"
    echo "  ! $1"

    mv $SRV_OUT o-srv-${TESTS}.log
    mv $CLI_OUT o-cli-${TESTS}.log
    if [ -n "$PXY_CMD" ]; then
        mv $PXY_OUT o-pxy-${TESTS}.log
    fi
    echo "  ! outputs saved to o-XXX-${TESTS}.log"

    if [ "X${USER:-}" = Xbuildbot -o "X${LOGNAME:-}" = Xbuildbot -o "${LOG_FAILURE_ON_STDOUT:-0}" != 0 ]; then
        echo "  ! server output:"
        cat o-srv-${TESTS}.log
        echo "  ! ========================================================"
        echo "  ! client output:"
        cat o-cli-${TESTS}.log
        if [ -n "$PXY_CMD" ]; then
            echo "  ! ========================================================"
            echo "  ! proxy output:"
            cat o-pxy-${TESTS}.log
        fi
        echo ""
    fi

    FAILS=$(( $FAILS + 1 ))
}

# is_polar <cmd_line>
is_polar() {
    echo "$1" | grep 'ssl_server2\|ssl_client2' > /dev/null
}

# openssl s_server doesn't have -www with DTLS
check_osrv_dtls() {
    if echo "$SRV_CMD" | grep 's_server.*-dtls' >/dev/null; then
        NEEDS_INPUT=1
        SRV_CMD="$( echo $SRV_CMD | sed s/-www// )"
    else
        NEEDS_INPUT=0
    fi
}

# provide input to commands that need it
provide_input() {
    if [ $NEEDS_INPUT -eq 0 ]; then
        return
    fi

    while true; do
        echo "HTTP/1.0 200 OK"
        sleep 1
    done
}

# has_mem_err <log_file_name>
has_mem_err() {
    if ( grep -F 'All heap blocks were freed -- no leaks are possible' "$1" &&
         grep -F 'ERROR SUMMARY: 0 errors from 0 contexts' "$1" ) > /dev/null
    then
        return 1 # false: does not have errors
    else
        return 0 # true: has errors
    fi
}

# Wait for process $2 to be listening on port $1
if type lsof >/dev/null 2>/dev/null; then
    wait_server_start() {
        START_TIME=$(date +%s)
        if [ "$DTLS" -eq 1 ]; then
            proto=UDP
        else
            proto=TCP
        fi
        # Make a tight loop, server normally takes less than 1s to start.
        while ! lsof -a -n -b -i "$proto:$1" -p "$2" >/dev/null 2>/dev/null; do
              if [ $(( $(date +%s) - $START_TIME )) -gt $DOG_DELAY ]; then
                  echo "SERVERSTART TIMEOUT"
                  echo "SERVERSTART TIMEOUT" >> $SRV_OUT
                  break
              fi
              # Linux and *BSD support decimal arguments to sleep. On other
              # OSes this may be a tight loop.
              sleep 0.1 2>/dev/null || true
        done
    }
else
    echo "Warning: lsof not available, wait_server_start = sleep"
    wait_server_start() {
        sleep "$START_DELAY"
    }
fi

# Given the client or server debug output, parse the unix timestamp that is
# included in the first 4 bytes of the random bytes and check that it's within
# acceptable bounds
check_server_hello_time() {
    # Extract the time from the debug (lvl 3) output of the client
    SERVER_HELLO_TIME="$(sed -n 's/.*server hello, current time: //p' < "$1")"
    # Get the Unix timestamp for now
    CUR_TIME=$(date +'%s')
    THRESHOLD_IN_SECS=300

    # Check if the ServerHello time was printed
    if [ -z "$SERVER_HELLO_TIME" ]; then
        return 1
    fi

    # Check the time in ServerHello is within acceptable bounds
    if [ $SERVER_HELLO_TIME -lt $(( $CUR_TIME - $THRESHOLD_IN_SECS )) ]; then
        # The time in ServerHello is at least 5 minutes before now
        return 1
    elif [ $SERVER_HELLO_TIME -gt $(( $CUR_TIME + $THRESHOLD_IN_SECS )) ]; then
        # The time in ServerHello is at least 5 minutes later than now
        return 1
    else
        return 0
    fi
}

# wait for client to terminate and set CLI_EXIT
# must be called right after starting the client
wait_client_done() {
    CLI_PID=$!

    CLI_DELAY=$(( $DOG_DELAY * $CLI_DELAY_FACTOR ))
    CLI_DELAY_FACTOR=1

    ( sleep $CLI_DELAY; echo "===CLIENT_TIMEOUT===" >> $CLI_OUT; kill $CLI_PID ) &
    DOG_PID=$!

    wait $CLI_PID
    CLI_EXIT=$?

    kill $DOG_PID >/dev/null 2>&1
    wait $DOG_PID

    echo "EXIT: $CLI_EXIT" >> $CLI_OUT

    sleep $SRV_DELAY_SECONDS
    SRV_DELAY_SECONDS=0
}

# check if the given command uses dtls and sets global variable DTLS
detect_dtls() {
    if echo "$1" | grep 'dtls=1\|-dtls1\|-u' >/dev/null; then
        DTLS=1
    else
        DTLS=0
    fi
}

# Usage: run_test name [-p proxy_cmd] srv_cmd cli_cmd cli_exit [option [...]]
# Options:  -s pattern  pattern that must be present in server output
#           -c pattern  pattern that must be present in client output
#           -u pattern  lines after pattern must be unique in client output
#           -f call shell function on client output
#           -S pattern  pattern that must be absent in server output
#           -C pattern  pattern that must be absent in client output
#           -U pattern  lines after pattern must be unique in server output
#           -F call shell function on server output
run_test() {
    NAME="$1"
    shift 1

    if echo "$NAME" | grep "$FILTER" | grep -v "$EXCLUDE" >/dev/null; then :
    else
        SKIP_NEXT="NO"
        return
    fi

    print_name "$NAME"

    # Do we only run numbered tests?
    if [ "X$RUN_TEST_NUMBER" = "X" ]; then :
    elif echo ",$RUN_TEST_NUMBER," | grep ",$TESTS," >/dev/null; then :
    else
        SKIP_NEXT="YES"
    fi

    # should we skip?
    if [ "X$SKIP_NEXT" = "XYES" ]; then
        SKIP_NEXT="NO"
        echo "SKIP"
        SKIPS=$(( $SKIPS + 1 ))
        return
    fi

    # does this test use a proxy?
    if [ "X$1" = "X-p" ]; then
        PXY_CMD="$2"
        shift 2
    else
        PXY_CMD=""
    fi

    # get commands and client output
    SRV_CMD="$1"
    CLI_CMD="$2"
    CLI_EXPECT="$3"
    shift 3

    # fix client port
    if [ -n "$PXY_CMD" ]; then
        CLI_CMD=$( echo "$CLI_CMD" | sed s/+SRV_PORT/$PXY_PORT/g )
    else
        CLI_CMD=$( echo "$CLI_CMD" | sed s/+SRV_PORT/$SRV_PORT/g )
    fi

    # update DTLS variable
    detect_dtls "$SRV_CMD"

    # prepend valgrind to our commands if active
    if [ "$MEMCHECK" -gt 0 ]; then
        if is_polar "$SRV_CMD"; then
            SRV_CMD="valgrind --leak-check=full $SRV_CMD"
        fi
        if is_polar "$CLI_CMD"; then
            CLI_CMD="valgrind --leak-check=full $CLI_CMD"
        fi
    fi

    TIMES_LEFT=2
    while [ $TIMES_LEFT -gt 0 ]; do
        TIMES_LEFT=$(( $TIMES_LEFT - 1 ))

        # run the commands
        if [ -n "$PXY_CMD" ]; then
            echo "$PXY_CMD" > $PXY_OUT
            $PXY_CMD >> $PXY_OUT 2>&1 &
            PXY_PID=$!
            # assume proxy starts faster than server
        fi

        check_osrv_dtls
        echo "$SRV_CMD" > $SRV_OUT
        provide_input | $SRV_CMD >> $SRV_OUT 2>&1 &
        SRV_PID=$!
        wait_server_start "$SRV_PORT" "$SRV_PID"

        echo "$CLI_CMD" > $CLI_OUT
        eval "$CLI_CMD" >> $CLI_OUT 2>&1 &
        wait_client_done

        sleep 0.05

        # terminate the server (and the proxy)
        kill $SRV_PID
        wait $SRV_PID

        if [ -n "$PXY_CMD" ]; then
            kill $PXY_PID >/dev/null 2>&1
            wait $PXY_PID
        fi

        # retry only on timeouts
        if grep '===CLIENT_TIMEOUT===' $CLI_OUT >/dev/null; then
            printf "RETRY "
        else
            TIMES_LEFT=0
        fi
    done

    # check if the client and server went at least to the handshake stage
    # (useful to avoid tests with only negative assertions and non-zero
    # expected client exit to incorrectly succeed in case of catastrophic
    # failure)
    if is_polar "$SRV_CMD"; then
        if grep "Performing the SSL/TLS handshake" $SRV_OUT >/dev/null; then :;
        else
            fail "server or client failed to reach handshake stage"
            return
        fi
    fi
    if is_polar "$CLI_CMD"; then
        if grep "Performing the SSL/TLS handshake" $CLI_OUT >/dev/null; then :;
        else
            fail "server or client failed to reach handshake stage"
            return
        fi
    fi

    # check server exit code
    if [ $? != 0 ]; then
        fail "server fail"
        return
    fi

    # check client exit code
    if [ \( "$CLI_EXPECT" = 0 -a "$CLI_EXIT" != 0 \) -o \
         \( "$CLI_EXPECT" != 0 -a "$CLI_EXIT" = 0 \) ]
    then
        fail "bad client exit code (expected $CLI_EXPECT, got $CLI_EXIT)"
        return
    fi

    # check other assertions
    # lines beginning with == are added by valgrind, ignore them
    # lines with 'Serious error when reading debug info', are valgrind issues as well
    while [ $# -gt 0 ]
    do
        case $1 in
            "-s")
                if grep -v '^==' $SRV_OUT | grep -v 'Serious error when reading debug info' | grep "$2" >/dev/null; then :; else
                    fail "pattern '$2' MUST be present in the Server output"
                    return
                fi
                ;;

            "-c")
                if grep -v '^==' $CLI_OUT | grep -v 'Serious error when reading debug info' | grep "$2" >/dev/null; then :; else
                    fail "pattern '$2' MUST be present in the Client output"
                    return
                fi
                ;;

            "-S")
                if grep -v '^==' $SRV_OUT | grep -v 'Serious error when reading debug info' | grep "$2" >/dev/null; then
                    fail "pattern '$2' MUST NOT be present in the Server output"
                    return
                fi
                ;;

            "-C")
                if grep -v '^==' $CLI_OUT | grep -v 'Serious error when reading debug info' | grep "$2" >/dev/null; then
                    fail "pattern '$2' MUST NOT be present in the Client output"
                    return
                fi
                ;;

                # The filtering in the following two options (-u and -U) do the following
                #   - ignore valgrind output
                #   - filter out everything but lines right after the pattern occurances
                #   - keep one of each non-unique line
                #   - count how many lines remain
                # A line with '--' will remain in the result from previous outputs, so the number of lines in the result will be 1
                # if there were no duplicates.
            "-U")
                if [ $(grep -v '^==' $SRV_OUT | grep -v 'Serious error when reading debug info' | grep -A1 "$2" | grep -v "$2" | sort | uniq -d | wc -l) -gt 1 ]; then
                    fail "lines following pattern '$2' must be unique in Server output"
                    return
                fi
                ;;

            "-u")
                if [ $(grep -v '^==' $CLI_OUT | grep -v 'Serious error when reading debug info' | grep -A1 "$2" | grep -v "$2" | sort | uniq -d | wc -l) -gt 1 ]; then
                    fail "lines following pattern '$2' must be unique in Client output"
                    return
                fi
                ;;
            "-F")
                if ! $2 "$SRV_OUT"; then
                    fail "function call to '$2' failed on Server output"
                    return
                fi
                ;;
            "-f")
                if ! $2 "$CLI_OUT"; then
                    fail "function call to '$2' failed on Client output"
                    return
                fi
                ;;

            *)
                echo "Unknown test: $1" >&2
                exit 1
        esac
        shift 2
    done

    # check valgrind's results
    if [ "$MEMCHECK" -gt 0 ]; then
        if is_polar "$SRV_CMD" && has_mem_err $SRV_OUT; then
            fail "Server has memory errors"
            return
        fi
        if is_polar "$CLI_CMD" && has_mem_err $CLI_OUT; then
            fail "Client has memory errors"
            return
        fi
    fi

    # if we're here, everything is ok
    echo "PASS"
    if [ "$PRESERVE_LOGS" -gt 0 ]; then
        mv $SRV_OUT o-srv-${TESTS}.log
        mv $CLI_OUT o-cli-${TESTS}.log
        if [ -n "$PXY_CMD" ]; then
            mv $PXY_OUT o-pxy-${TESTS}.log
        fi
    fi

    rm -f $SRV_OUT $CLI_OUT $PXY_OUT
}

cleanup() {
    rm -f $CLI_OUT $SRV_OUT $PXY_OUT $SESSION
    test -n "${SRV_PID:-}" && kill $SRV_PID >/dev/null 2>&1
    test -n "${PXY_PID:-}" && kill $PXY_PID >/dev/null 2>&1
    test -n "${CLI_PID:-}" && kill $CLI_PID >/dev/null 2>&1
    test -n "${DOG_PID:-}" && kill $DOG_PID >/dev/null 2>&1
    exit 1
}

#
# MAIN
#

get_options "$@"

# sanity checks, avoid an avalanche of errors
P_SRV_BIN="${P_SRV%%[  ]*}"
P_CLI_BIN="${P_CLI%%[  ]*}"
P_PXY_BIN="${P_PXY%%[  ]*}"
if [ ! -x "$P_SRV_BIN" ]; then
    echo "Command '$P_SRV_BIN' is not an executable file"
    exit 1
fi
if [ ! -x "$P_CLI_BIN" ]; then
    echo "Command '$P_CLI_BIN' is not an executable file"
    exit 1
fi
if [ ! -x "$P_PXY_BIN" ]; then
    echo "Command '$P_PXY_BIN' is not an executable file"
    exit 1
fi
if [ "$MEMCHECK" -gt 0 ]; then
    if which valgrind >/dev/null 2>&1; then :; else
        echo "Memcheck not possible. Valgrind not found"
        exit 1
    fi
fi
if which $OPENSSL_CMD >/dev/null 2>&1; then :; else
    echo "Command '$OPENSSL_CMD' not found"
    exit 1
fi

# used by watchdog
MAIN_PID="$$"

# We use somewhat arbitrary delays for tests:
# - how long do we wait for the server to start (when lsof not available)?
# - how long do we allow for the client to finish?
#   (not to check performance, just to avoid waiting indefinitely)
# Things are slower with valgrind, so give extra time here.
#
# Note: without lsof, there is a trade-off between the running time of this
# script and the risk of spurious errors because we didn't wait long enough.
# The watchdog delay on the other hand doesn't affect normal running time of
# the script, only the case where a client or server gets stuck.
if [ "$MEMCHECK" -gt 0 ]; then
    START_DELAY=6
    DOG_DELAY=60
else
    START_DELAY=2
    DOG_DELAY=20
fi

# some particular tests need more time:
# - for the client, we multiply the usual watchdog limit by a factor
# - for the server, we sleep for a number of seconds after the client exits
# see client_need_more_time() and server_needs_more_time()
CLI_DELAY_FACTOR=1
SRV_DELAY_SECONDS=0

# fix commands to use this port, force IPv4 while at it
# +SRV_PORT will be replaced by either $SRV_PORT or $PXY_PORT later
P_SRV="$P_SRV server_addr=127.0.0.1 server_port=$SRV_PORT"
P_CLI="$P_CLI server_addr=127.0.0.1 server_port=+SRV_PORT"
P_PXY="$P_PXY server_addr=127.0.0.1 server_port=$SRV_PORT listen_addr=127.0.0.1 listen_port=$PXY_PORT ${SEED:+"seed=$SEED"}"
O_SRV="$O_SRV -accept $SRV_PORT -dhparam data_files/dhparams.pem"
O_CLI="$O_CLI -connect localhost:+SRV_PORT"
G_SRV="$G_SRV -p $SRV_PORT"
G_CLI="$G_CLI -p +SRV_PORT"

if [ -n "${OPENSSL_LEGACY:-}" ]; then
    O_LEGACY_SRV="$O_LEGACY_SRV -accept $SRV_PORT -dhparam data_files/dhparams.pem"
    O_LEGACY_CLI="$O_LEGACY_CLI -connect localhost:+SRV_PORT"
fi

if [ -n "${GNUTLS_NEXT_SERV:-}" ]; then
    G_NEXT_SRV="$G_NEXT_SRV -p $SRV_PORT"
fi

if [ -n "${GNUTLS_NEXT_CLI:-}" ]; then
    G_NEXT_CLI="$G_NEXT_CLI -p +SRV_PORT"
fi

# Allow SHA-1, because many of our test certificates use it
P_SRV="$P_SRV allow_sha1=1"
P_CLI="$P_CLI allow_sha1=1"

# Also pick a unique name for intermediate files
SRV_OUT="srv_out.$$"
CLI_OUT="cli_out.$$"
PXY_OUT="pxy_out.$$"
SESSION="session.$$"

SKIP_NEXT="NO"

trap cleanup INT TERM HUP

# Basic test

# Checks that:
# - things work with all ciphersuites active (used with config-full in all.sh)
# - the expected (highest security) parameters are selected
#   ("signature_algorithm ext: 6" means SHA-512 (highest common hash))
run_test    "Default" \
            "$P_SRV debug_level=3" \
            "$P_CLI" \
            0 \
            -s "Protocol is TLSv1.2" \
            -s "Ciphersuite is TLS-ECDHE-RSA-WITH-CHACHA20-POLY1305-SHA256" \
            -s "client hello v3, signature_algorithm ext: 6" \
            -s "ECDHE curve: secp521r1" \
            -S "error" \
            -C "error"

run_test    "Default, DTLS" \
            "$P_SRV dtls=1" \
            "$P_CLI dtls=1" \
            0 \
            -s "Protocol is DTLSv1.2" \
            -s "Ciphersuite is TLS-ECDHE-RSA-WITH-CHACHA20-POLY1305-SHA256"

# Test current time in ServerHello
requires_config_enabled MBEDTLS_HAVE_TIME
run_test    "ServerHello contains gmt_unix_time" \
            "$P_SRV debug_level=3" \
            "$P_CLI debug_level=3" \
            0 \
            -f "check_server_hello_time" \
            -F "check_server_hello_time"

# Test for uniqueness of IVs in AEAD ciphersuites
run_test    "Unique IV in GCM" \
            "$P_SRV exchanges=20 debug_level=4" \
            "$P_CLI exchanges=20 debug_level=4 force_ciphersuite=TLS-ECDHE-ECDSA-WITH-AES-256-GCM-SHA384" \
            0 \
            -u "IV used" \
            -U "IV used"

# Tests for rc4 option

requires_config_enabled MBEDTLS_REMOVE_ARC4_CIPHERSUITES
run_test    "RC4: server disabled, client enabled" \
            "$P_SRV" \
            "$P_CLI force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            1 \
            -s "SSL - The server has no ciphersuites in common"

requires_config_enabled MBEDTLS_REMOVE_ARC4_CIPHERSUITES
run_test    "RC4: server half, client enabled" \
            "$P_SRV arc4=1" \
            "$P_CLI force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            1 \
            -s "SSL - The server has no ciphersuites in common"

run_test    "RC4: server enabled, client disabled" \
            "$P_SRV force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            "$P_CLI" \
            1 \
            -s "SSL - The server has no ciphersuites in common"

run_test    "RC4: both enabled" \
            "$P_SRV force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            "$P_CLI force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            0 \
            -S "SSL - None of the common ciphersuites is usable" \
            -S "SSL - The server has no ciphersuites in common"

# Test empty CA list in CertificateRequest in TLS 1.1 and earlier

requires_gnutls
requires_config_enabled MBEDTLS_SSL_PROTO_TLS1_1
run_test    "CertificateRequest with empty CA list, TLS 1.1 (GnuTLS server)" \
            "$G_SRV"\
            "$P_CLI force_version=tls1_1" \
            0

requires_gnutls
requires_config_enabled MBEDTLS_SSL_PROTO_TLS1
run_test    "CertificateRequest with empty CA list, TLS 1.0 (GnuTLS server)" \
            "$G_SRV"\
            "$P_CLI force_version=tls1" \
            0

# Tests for SHA-1 support

requires_config_disabled MBEDTLS_TLS_DEFAULT_ALLOW_SHA1_IN_CERTIFICATES
run_test    "SHA-1 forbidden by default in server certificate" \
            "$P_SRV key_file=data_files/server2.key crt_file=data_files/server2.crt" \
            "$P_CLI debug_level=2 allow_sha1=0" \
            1 \
            -c "The certificate is signed with an unacceptable hash"

requires_config_enabled MBEDTLS_TLS_DEFAULT_ALLOW_SHA1_IN_CERTIFICATES
run_test    "SHA-1 forbidden by default in server certificate" \
            "$P_SRV key_file=data_files/server2.key crt_file=data_files/server2.crt" \
            "$P_CLI debug_level=2 allow_sha1=0" \
            0

run_test    "SHA-1 explicitly allowed in server certificate" \
            "$P_SRV key_file=data_files/server2.key crt_file=data_files/server2.crt" \
            "$P_CLI allow_sha1=1" \
            0

run_test    "SHA-256 allowed by default in server certificate" \
            "$P_SRV key_file=data_files/server2.key crt_file=data_files/server2-sha256.crt" \
            "$P_CLI allow_sha1=0" \
            0

requires_config_disabled MBEDTLS_TLS_DEFAULT_ALLOW_SHA1_IN_CERTIFICATES
run_test    "SHA-1 forbidden by default in client certificate" \
            "$P_SRV auth_mode=required allow_sha1=0" \
            "$P_CLI key_file=data_files/cli-rsa.key crt_file=data_files/cli-rsa-sha1.crt" \
            1 \
            -s "The certificate is signed with an unacceptable hash"

requires_config_enabled MBEDTLS_TLS_DEFAULT_ALLOW_SHA1_IN_CERTIFICATES
run_test    "SHA-1 forbidden by default in client certificate" \
            "$P_SRV auth_mode=required allow_sha1=0" \
            "$P_CLI key_file=data_files/cli-rsa.key crt_file=data_files/cli-rsa-sha1.crt" \
            0

run_test    "SHA-1 explicitly allowed in client certificate" \
            "$P_SRV auth_mode=required allow_sha1=1" \
            "$P_CLI key_file=data_files/cli-rsa.key crt_file=data_files/cli-rsa-sha1.crt" \
            0

run_test    "SHA-256 allowed by default in client certificate" \
            "$P_SRV auth_mode=required allow_sha1=0" \
            "$P_CLI key_file=data_files/cli-rsa.key crt_file=data_files/cli-rsa-sha256.crt" \
            0

# Tests for datagram packing
run_test    "DTLS: multiple records in same datagram, client and server" \
            "$P_SRV dtls=1 dgram_packing=1 debug_level=2" \
            "$P_CLI dtls=1 dgram_packing=1 debug_level=2" \
            0 \
            -c "next record in same datagram" \
            -s "next record in same datagram"

run_test    "DTLS: multiple records in same datagram, client only" \
            "$P_SRV dtls=1 dgram_packing=0 debug_level=2" \
            "$P_CLI dtls=1 dgram_packing=1 debug_level=2" \
            0 \
            -s "next record in same datagram" \
            -C "next record in same datagram"

run_test    "DTLS: multiple records in same datagram, server only" \
            "$P_SRV dtls=1 dgram_packing=1 debug_level=2" \
            "$P_CLI dtls=1 dgram_packing=0 debug_level=2" \
            0 \
            -S "next record in same datagram" \
            -c "next record in same datagram"

run_test    "DTLS: multiple records in same datagram, neither client nor server" \
            "$P_SRV dtls=1 dgram_packing=0 debug_level=2" \
            "$P_CLI dtls=1 dgram_packing=0 debug_level=2" \
            0 \
            -S "next record in same datagram" \
            -C "next record in same datagram"

# Tests for Truncated HMAC extension

run_test    "Truncated HMAC: client default, server default" \
            "$P_SRV debug_level=4" \
            "$P_CLI force_ciphersuite=TLS-RSA-WITH-AES-128-CBC-SHA" \
            0 \
            -s "dumping 'expected mac' (20 bytes)" \
            -S "dumping 'expected mac' (10 bytes)"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Truncated HMAC: client disabled, server default" \
            "$P_SRV debug_level=4" \
            "$P_CLI force_ciphersuite=TLS-RSA-WITH-AES-128-CBC-SHA trunc_hmac=0" \
            0 \
            -s "dumping 'expected mac' (20 bytes)" \
            -S "dumping 'expected mac' (10 bytes)"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Truncated HMAC: client enabled, server default" \
            "$P_SRV debug_level=4" \
            "$P_CLI force_ciphersuite=TLS-RSA-WITH-AES-128-CBC-SHA trunc_hmac=1" \
            0 \
            -s "dumping 'expected mac' (20 bytes)" \
            -S "dumping 'expected mac' (10 bytes)"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Truncated HMAC: client enabled, server disabled" \
            "$P_SRV debug_level=4 trunc_hmac=0" \
            "$P_CLI force_ciphersuite=TLS-RSA-WITH-AES-128-CBC-SHA trunc_hmac=1" \
            0 \
            -s "dumping 'expected mac' (20 bytes)" \
            -S "dumping 'expected mac' (10 bytes)"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Truncated HMAC: client disabled, server enabled" \
            "$P_SRV debug_level=4 trunc_hmac=1" \
            "$P_CLI force_ciphersuite=TLS-RSA-WITH-AES-128-CBC-SHA trunc_hmac=0" \
            0 \
            -s "dumping 'expected mac' (20 bytes)" \
            -S "dumping 'expected mac' (10 bytes)"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Truncated HMAC: client enabled, server enabled" \
            "$P_SRV debug_level=4 trunc_hmac=1" \
            "$P_CLI force_ciphersuite=TLS-RSA-WITH-AES-128-CBC-SHA trunc_hmac=1" \
            0 \
            -S "dumping 'expected mac' (20 bytes)" \
            -s "dumping 'expected mac' (10 bytes)"

run_test    "Truncated HMAC, DTLS: client default, server default" \
            "$P_SRV dtls=1 debug_level=4" \
            "$P_CLI dtls=1 force_ciphersuite=TLS-RSA-WITH-AES-128-CBC-SHA" \
            0 \
            -s "dumping 'expected mac' (20 bytes)" \
            -S "dumping 'expected mac' (10 bytes)"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Truncated HMAC, DTLS: client disabled, server default" \
            "$P_SRV dtls=1 debug_level=4" \
            "$P_CLI dtls=1 force_ciphersuite=TLS-RSA-WITH-AES-128-CBC-SHA trunc_hmac=0" \
            0 \
            -s "dumping 'expected mac' (20 bytes)" \
            -S "dumping 'expected mac' (10 bytes)"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Truncated HMAC, DTLS: client enabled, server default" \
            "$P_SRV dtls=1 debug_level=4" \
            "$P_CLI dtls=1 force_ciphersuite=TLS-RSA-WITH-AES-128-CBC-SHA trunc_hmac=1" \
            0 \
            -s "dumping 'expected mac' (20 bytes)" \
            -S "dumping 'expected mac' (10 bytes)"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Truncated HMAC, DTLS: client enabled, server disabled" \
            "$P_SRV dtls=1 debug_level=4 trunc_hmac=0" \
            "$P_CLI dtls=1 force_ciphersuite=TLS-RSA-WITH-AES-128-CBC-SHA trunc_hmac=1" \
            0 \
            -s "dumping 'expected mac' (20 bytes)" \
            -S "dumping 'expected mac' (10 bytes)"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Truncated HMAC, DTLS: client disabled, server enabled" \
            "$P_SRV dtls=1 debug_level=4 trunc_hmac=1" \
            "$P_CLI dtls=1 force_ciphersuite=TLS-RSA-WITH-AES-128-CBC-SHA trunc_hmac=0" \
            0 \
            -s "dumping 'expected mac' (20 bytes)" \
            -S "dumping 'expected mac' (10 bytes)"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Truncated HMAC, DTLS: client enabled, server enabled" \
            "$P_SRV dtls=1 debug_level=4 trunc_hmac=1" \
            "$P_CLI dtls=1 force_ciphersuite=TLS-RSA-WITH-AES-128-CBC-SHA trunc_hmac=1" \
            0 \
            -S "dumping 'expected mac' (20 bytes)" \
            -s "dumping 'expected mac' (10 bytes)"

# Tests for Encrypt-then-MAC extension

run_test    "Encrypt then MAC: default" \
            "$P_SRV debug_level=3 \
             force_ciphersuite=TLS-RSA-WITH-AES-128-CBC-SHA" \
            "$P_CLI debug_level=3" \
            0 \
            -c "client hello, adding encrypt_then_mac extension" \
            -s "found encrypt then mac extension" \
            -s "server hello, adding encrypt then mac extension" \
            -c "found encrypt_then_mac extension" \
            -c "using encrypt then mac" \
            -s "using encrypt then mac"

run_test    "Encrypt then MAC: client enabled, server disabled" \
            "$P_SRV debug_level=3 etm=0 \
             force_ciphersuite=TLS-RSA-WITH-AES-128-CBC-SHA" \
            "$P_CLI debug_level=3 etm=1" \
            0 \
            -c "client hello, adding encrypt_then_mac extension" \
            -s "found encrypt then mac extension" \
            -S "server hello, adding encrypt then mac extension" \
            -C "found encrypt_then_mac extension" \
            -C "using encrypt then mac" \
            -S "using encrypt then mac"

run_test    "Encrypt then MAC: client enabled, aead cipher" \
            "$P_SRV debug_level=3 etm=1 \
             force_ciphersuite=TLS-RSA-WITH-AES-128-GCM-SHA256" \
            "$P_CLI debug_level=3 etm=1" \
            0 \
            -c "client hello, adding encrypt_then_mac extension" \
            -s "found encrypt then mac extension" \
            -S "server hello, adding encrypt then mac extension" \
            -C "found encrypt_then_mac extension" \
            -C "using encrypt then mac" \
            -S "using encrypt then mac"

run_test    "Encrypt then MAC: client enabled, stream cipher" \
            "$P_SRV debug_level=3 etm=1 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            "$P_CLI debug_level=3 etm=1 arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            0 \
            -c "client hello, adding encrypt_then_mac extension" \
            -s "found encrypt then mac extension" \
            -S "server hello, adding encrypt then mac extension" \
            -C "found encrypt_then_mac extension" \
            -C "using encrypt then mac" \
            -S "using encrypt then mac"

run_test    "Encrypt then MAC: client disabled, server enabled" \
            "$P_SRV debug_level=3 etm=1 \
             force_ciphersuite=TLS-RSA-WITH-AES-128-CBC-SHA" \
            "$P_CLI debug_level=3 etm=0" \
            0 \
            -C "client hello, adding encrypt_then_mac extension" \
            -S "found encrypt then mac extension" \
            -S "server hello, adding encrypt then mac extension" \
            -C "found encrypt_then_mac extension" \
            -C "using encrypt then mac" \
            -S "using encrypt then mac"

requires_config_enabled MBEDTLS_SSL_PROTO_SSL3
run_test    "Encrypt then MAC: client SSLv3, server enabled" \
            "$P_SRV debug_level=3 min_version=ssl3 \
             force_ciphersuite=TLS-RSA-WITH-AES-128-CBC-SHA" \
            "$P_CLI debug_level=3 force_version=ssl3" \
            0 \
            -C "client hello, adding encrypt_then_mac extension" \
            -S "found encrypt then mac extension" \
            -S "server hello, adding encrypt then mac extension" \
            -C "found encrypt_then_mac extension" \
            -C "using encrypt then mac" \
            -S "using encrypt then mac"

requires_config_enabled MBEDTLS_SSL_PROTO_SSL3
run_test    "Encrypt then MAC: client enabled, server SSLv3" \
            "$P_SRV debug_level=3 force_version=ssl3 \
             force_ciphersuite=TLS-RSA-WITH-AES-128-CBC-SHA" \
            "$P_CLI debug_level=3 min_version=ssl3" \
            0 \
            -c "client hello, adding encrypt_then_mac extension" \
            -S "found encrypt then mac extension" \
            -S "server hello, adding encrypt then mac extension" \
            -C "found encrypt_then_mac extension" \
            -C "using encrypt then mac" \
            -S "using encrypt then mac"

# Tests for Extended Master Secret extension

run_test    "Extended Master Secret: default" \
            "$P_SRV debug_level=3" \
            "$P_CLI debug_level=3" \
            0 \
            -c "client hello, adding extended_master_secret extension" \
            -s "found extended master secret extension" \
            -s "server hello, adding extended master secret extension" \
            -c "found extended_master_secret extension" \
            -c "using extended master secret" \
            -s "using extended master secret"

run_test    "Extended Master Secret: client enabled, server disabled" \
            "$P_SRV debug_level=3 extended_ms=0" \
            "$P_CLI debug_level=3 extended_ms=1" \
            0 \
            -c "client hello, adding extended_master_secret extension" \
            -s "found extended master secret extension" \
            -S "server hello, adding extended master secret extension" \
            -C "found extended_master_secret extension" \
            -C "using extended master secret" \
            -S "using extended master secret"

run_test    "Extended Master Secret: client disabled, server enabled" \
            "$P_SRV debug_level=3 extended_ms=1" \
            "$P_CLI debug_level=3 extended_ms=0" \
            0 \
            -C "client hello, adding extended_master_secret extension" \
            -S "found extended master secret extension" \
            -S "server hello, adding extended master secret extension" \
            -C "found extended_master_secret extension" \
            -C "using extended master secret" \
            -S "using extended master secret"

requires_config_enabled MBEDTLS_SSL_PROTO_SSL3
run_test    "Extended Master Secret: client SSLv3, server enabled" \
            "$P_SRV debug_level=3 min_version=ssl3" \
            "$P_CLI debug_level=3 force_version=ssl3" \
            0 \
            -C "client hello, adding extended_master_secret extension" \
            -S "found extended master secret extension" \
            -S "server hello, adding extended master secret extension" \
            -C "found extended_master_secret extension" \
            -C "using extended master secret" \
            -S "using extended master secret"

requires_config_enabled MBEDTLS_SSL_PROTO_SSL3
run_test    "Extended Master Secret: client enabled, server SSLv3" \
            "$P_SRV debug_level=3 force_version=ssl3" \
            "$P_CLI debug_level=3 min_version=ssl3" \
            0 \
            -c "client hello, adding extended_master_secret extension" \
            -S "found extended master secret extension" \
            -S "server hello, adding extended master secret extension" \
            -C "found extended_master_secret extension" \
            -C "using extended master secret" \
            -S "using extended master secret"

# Tests for FALLBACK_SCSV

run_test    "Fallback SCSV: default" \
            "$P_SRV debug_level=2" \
            "$P_CLI debug_level=3 force_version=tls1_1" \
            0 \
            -C "adding FALLBACK_SCSV" \
            -S "received FALLBACK_SCSV" \
            -S "inapropriate fallback" \
            -C "is a fatal alert message (msg 86)"

run_test    "Fallback SCSV: explicitly disabled" \
            "$P_SRV debug_level=2" \
            "$P_CLI debug_level=3 force_version=tls1_1 fallback=0" \
            0 \
            -C "adding FALLBACK_SCSV" \
            -S "received FALLBACK_SCSV" \
            -S "inapropriate fallback" \
            -C "is a fatal alert message (msg 86)"

run_test    "Fallback SCSV: enabled" \
            "$P_SRV debug_level=2" \
            "$P_CLI debug_level=3 force_version=tls1_1 fallback=1" \
            1 \
            -c "adding FALLBACK_SCSV" \
            -s "received FALLBACK_SCSV" \
            -s "inapropriate fallback" \
            -c "is a fatal alert message (msg 86)"

run_test    "Fallback SCSV: enabled, max version" \
            "$P_SRV debug_level=2" \
            "$P_CLI debug_level=3 fallback=1" \
            0 \
            -c "adding FALLBACK_SCSV" \
            -s "received FALLBACK_SCSV" \
            -S "inapropriate fallback" \
            -C "is a fatal alert message (msg 86)"

requires_openssl_with_fallback_scsv
run_test    "Fallback SCSV: default, openssl server" \
            "$O_SRV" \
            "$P_CLI debug_level=3 force_version=tls1_1 fallback=0" \
            0 \
            -C "adding FALLBACK_SCSV" \
            -C "is a fatal alert message (msg 86)"

requires_openssl_with_fallback_scsv
run_test    "Fallback SCSV: enabled, openssl server" \
            "$O_SRV" \
            "$P_CLI debug_level=3 force_version=tls1_1 fallback=1" \
            1 \
            -c "adding FALLBACK_SCSV" \
            -c "is a fatal alert message (msg 86)"

requires_openssl_with_fallback_scsv
run_test    "Fallback SCSV: disabled, openssl client" \
            "$P_SRV debug_level=2" \
            "$O_CLI -tls1_1" \
            0 \
            -S "received FALLBACK_SCSV" \
            -S "inapropriate fallback"

requires_openssl_with_fallback_scsv
run_test    "Fallback SCSV: enabled, openssl client" \
            "$P_SRV debug_level=2" \
            "$O_CLI -tls1_1 -fallback_scsv" \
            1 \
            -s "received FALLBACK_SCSV" \
            -s "inapropriate fallback"

requires_openssl_with_fallback_scsv
run_test    "Fallback SCSV: enabled, max version, openssl client" \
            "$P_SRV debug_level=2" \
            "$O_CLI -fallback_scsv" \
            0 \
            -s "received FALLBACK_SCSV" \
            -S "inapropriate fallback"

# Test sending and receiving empty application data records

run_test    "Encrypt then MAC: empty application data record" \
            "$P_SRV auth_mode=none debug_level=4 etm=1" \
            "$P_CLI auth_mode=none etm=1 request_size=0 force_ciphersuite=TLS-ECDHE-RSA-WITH-AES-256-CBC-SHA" \
            0 \
            -S "0000:  0f 0f 0f 0f 0f 0f 0f 0f 0f 0f 0f 0f 0f 0f 0f 0f" \
            -s "dumping 'input payload after decrypt' (0 bytes)" \
            -c "0 bytes written in 1 fragments"

run_test    "Default, no Encrypt then MAC: empty application data record" \
            "$P_SRV auth_mode=none debug_level=4 etm=0" \
            "$P_CLI auth_mode=none etm=0 request_size=0" \
            0 \
            -s "dumping 'input payload after decrypt' (0 bytes)" \
            -c "0 bytes written in 1 fragments"

run_test    "Encrypt then MAC, DTLS: empty application data record" \
            "$P_SRV auth_mode=none debug_level=4 etm=1 dtls=1" \
            "$P_CLI auth_mode=none etm=1 request_size=0 force_ciphersuite=TLS-ECDHE-RSA-WITH-AES-256-CBC-SHA dtls=1" \
            0 \
            -S "0000:  0f 0f 0f 0f 0f 0f 0f 0f 0f 0f 0f 0f 0f 0f 0f 0f" \
            -s "dumping 'input payload after decrypt' (0 bytes)" \
            -c "0 bytes written in 1 fragments"

run_test    "Default, no Encrypt then MAC, DTLS: empty application data record" \
            "$P_SRV auth_mode=none debug_level=4 etm=0 dtls=1" \
            "$P_CLI auth_mode=none etm=0 request_size=0 dtls=1" \
            0 \
            -s "dumping 'input payload after decrypt' (0 bytes)" \
            -c "0 bytes written in 1 fragments"

## ClientHello generated with
## "openssl s_client -CAfile tests/data_files/test-ca.crt -tls1_1 -connect localhost:4433 -cipher ..."
## then manually twiddling the ciphersuite list.
## The ClientHello content is spelled out below as a hex string as
## "prefix ciphersuite1 ciphersuite2 ciphersuite3 ciphersuite4 suffix".
## The expected response is an inappropriate_fallback alert.
requires_openssl_with_fallback_scsv
run_test    "Fallback SCSV: beginning of list" \
            "$P_SRV debug_level=2" \
            "$TCP_CLIENT localhost $SRV_PORT '160301003e0100003a03022aafb94308dc22ca1086c65acc00e414384d76b61ecab37df1633b1ae1034dbe000008 5600 0031 0032 0033 0100000900230000000f000101' '15030200020256'" \
            0 \
            -s "received FALLBACK_SCSV" \
            -s "inapropriate fallback"

requires_openssl_with_fallback_scsv
run_test    "Fallback SCSV: end of list" \
            "$P_SRV debug_level=2" \
            "$TCP_CLIENT localhost $SRV_PORT '160301003e0100003a03022aafb94308dc22ca1086c65acc00e414384d76b61ecab37df1633b1ae1034dbe000008 0031 0032 0033 5600 0100000900230000000f000101' '15030200020256'" \
            0 \
            -s "received FALLBACK_SCSV" \
            -s "inapropriate fallback"

## Here the expected response is a valid ServerHello prefix, up to the random.
requires_openssl_with_fallback_scsv
run_test    "Fallback SCSV: not in list" \
            "$P_SRV debug_level=2" \
            "$TCP_CLIENT localhost $SRV_PORT '160301003e0100003a03022aafb94308dc22ca1086c65acc00e414384d76b61ecab37df1633b1ae1034dbe000008 0056 0031 0032 0033 0100000900230000000f000101' '16030200300200002c0302'" \
            0 \
            -S "received FALLBACK_SCSV" \
            -S "inapropriate fallback"

# Tests for CBC 1/n-1 record splitting

run_test    "CBC Record splitting: TLS 1.2, no splitting" \
            "$P_SRV" \
            "$P_CLI force_ciphersuite=TLS-RSA-WITH-AES-128-CBC-SHA \
             request_size=123 force_version=tls1_2" \
            0 \
            -s "Read from client: 123 bytes read" \
            -S "Read from client: 1 bytes read" \
            -S "122 bytes read"

run_test    "CBC Record splitting: TLS 1.1, no splitting" \
            "$P_SRV" \
            "$P_CLI force_ciphersuite=TLS-RSA-WITH-AES-128-CBC-SHA \
             request_size=123 force_version=tls1_1" \
            0 \
            -s "Read from client: 123 bytes read" \
            -S "Read from client: 1 bytes read" \
            -S "122 bytes read"

run_test    "CBC Record splitting: TLS 1.0, splitting" \
            "$P_SRV" \
            "$P_CLI force_ciphersuite=TLS-RSA-WITH-AES-128-CBC-SHA \
             request_size=123 force_version=tls1" \
            0 \
            -S "Read from client: 123 bytes read" \
            -s "Read from client: 1 bytes read" \
            -s "122 bytes read"

requires_config_enabled MBEDTLS_SSL_PROTO_SSL3
run_test    "CBC Record splitting: SSLv3, splitting" \
            "$P_SRV min_version=ssl3" \
            "$P_CLI force_ciphersuite=TLS-RSA-WITH-AES-128-CBC-SHA \
             request_size=123 force_version=ssl3" \
            0 \
            -S "Read from client: 123 bytes read" \
            -s "Read from client: 1 bytes read" \
            -s "122 bytes read"

run_test    "CBC Record splitting: TLS 1.0 RC4, no splitting" \
            "$P_SRV arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            "$P_CLI force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA \
             request_size=123 force_version=tls1" \
            0 \
            -s "Read from client: 123 bytes read" \
            -S "Read from client: 1 bytes read" \
            -S "122 bytes read"

run_test    "CBC Record splitting: TLS 1.0, splitting disabled" \
            "$P_SRV" \
            "$P_CLI force_ciphersuite=TLS-RSA-WITH-AES-128-CBC-SHA \
             request_size=123 force_version=tls1 recsplit=0" \
            0 \
            -s "Read from client: 123 bytes read" \
            -S "Read from client: 1 bytes read" \
            -S "122 bytes read"

run_test    "CBC Record splitting: TLS 1.0, splitting, nbio" \
            "$P_SRV nbio=2" \
            "$P_CLI nbio=2 force_ciphersuite=TLS-RSA-WITH-AES-128-CBC-SHA \
             request_size=123 force_version=tls1" \
            0 \
            -S "Read from client: 123 bytes read" \
            -s "Read from client: 1 bytes read" \
            -s "122 bytes read"

# Tests for Session Tickets

run_test    "Session resume using tickets: basic" \
            "$P_SRV debug_level=3 tickets=1" \
            "$P_CLI debug_level=3 tickets=1 reconnect=1" \
            0 \
            -c "client hello, adding session ticket extension" \
            -s "found session ticket extension" \
            -s "server hello, adding session ticket extension" \
            -c "found session_ticket extension" \
            -c "parse new session ticket" \
            -S "session successfully restored from cache" \
            -s "session successfully restored from ticket" \
            -s "a session has been resumed" \
            -c "a session has been resumed"

run_test    "Session resume using tickets: cache disabled" \
            "$P_SRV debug_level=3 tickets=1 cache_max=0" \
            "$P_CLI debug_level=3 tickets=1 reconnect=1" \
            0 \
            -c "client hello, adding session ticket extension" \
            -s "found session ticket extension" \
            -s "server hello, adding session ticket extension" \
            -c "found session_ticket extension" \
            -c "parse new session ticket" \
            -S "session successfully restored from cache" \
            -s "session successfully restored from ticket" \
            -s "a session has been resumed" \
            -c "a session has been resumed"

run_test    "Session resume using tickets: timeout" \
            "$P_SRV debug_level=3 tickets=1 cache_max=0 ticket_timeout=1" \
            "$P_CLI debug_level=3 tickets=1 reconnect=1 reco_delay=2" \
            0 \
            -c "client hello, adding session ticket extension" \
            -s "found session ticket extension" \
            -s "server hello, adding session ticket extension" \
            -c "found session_ticket extension" \
            -c "parse new session ticket" \
            -S "session successfully restored from cache" \
            -S "session successfully restored from ticket" \
            -S "a session has been resumed" \
            -C "a session has been resumed"

run_test    "Session resume using tickets: openssl server" \
            "$O_SRV" \
            "$P_CLI debug_level=3 tickets=1 reconnect=1" \
            0 \
            -c "client hello, adding session ticket extension" \
            -c "found session_ticket extension" \
            -c "parse new session ticket" \
            -c "a session has been resumed"

run_test    "Session resume using tickets: openssl client" \
            "$P_SRV debug_level=3 tickets=1" \
            "( $O_CLI -sess_out $SESSION; \
               $O_CLI -sess_in $SESSION; \
               rm -f $SESSION )" \
            0 \
            -s "found session ticket extension" \
            -s "server hello, adding session ticket extension" \
            -S "session successfully restored from cache" \
            -s "session successfully restored from ticket" \
            -s "a session has been resumed"

# Tests for Session Tickets with DTLS

run_test    "Session resume using tickets, DTLS: basic" \
            "$P_SRV debug_level=3 dtls=1 tickets=1" \
            "$P_CLI debug_level=3 dtls=1 tickets=1 reconnect=1" \
            0 \
            -c "client hello, adding session ticket extension" \
            -s "found session ticket extension" \
            -s "server hello, adding session ticket extension" \
            -c "found session_ticket extension" \
            -c "parse new session ticket" \
            -S "session successfully restored from cache" \
            -s "session successfully restored from ticket" \
            -s "a session has been resumed" \
            -c "a session has been resumed"

run_test    "Session resume using tickets, DTLS: cache disabled" \
            "$P_SRV debug_level=3 dtls=1 tickets=1 cache_max=0" \
            "$P_CLI debug_level=3 dtls=1 tickets=1 reconnect=1" \
            0 \
            -c "client hello, adding session ticket extension" \
            -s "found session ticket extension" \
            -s "server hello, adding session ticket extension" \
            -c "found session_ticket extension" \
            -c "parse new session ticket" \
            -S "session successfully restored from cache" \
            -s "session successfully restored from ticket" \
            -s "a session has been resumed" \
            -c "a session has been resumed"

run_test    "Session resume using tickets, DTLS: timeout" \
            "$P_SRV debug_level=3 dtls=1 tickets=1 cache_max=0 ticket_timeout=1" \
            "$P_CLI debug_level=3 dtls=1 tickets=1 reconnect=1 reco_delay=2" \
            0 \
            -c "client hello, adding session ticket extension" \
            -s "found session ticket extension" \
            -s "server hello, adding session ticket extension" \
            -c "found session_ticket extension" \
            -c "parse new session ticket" \
            -S "session successfully restored from cache" \
            -S "session successfully restored from ticket" \
            -S "a session has been resumed" \
            -C "a session has been resumed"

run_test    "Session resume using tickets, DTLS: openssl server" \
            "$O_SRV -dtls1" \
            "$P_CLI dtls=1 debug_level=3 tickets=1 reconnect=1" \
            0 \
            -c "client hello, adding session ticket extension" \
            -c "found session_ticket extension" \
            -c "parse new session ticket" \
            -c "a session has been resumed"

run_test    "Session resume using tickets, DTLS: openssl client" \
            "$P_SRV dtls=1 debug_level=3 tickets=1" \
            "( $O_CLI -dtls1 -sess_out $SESSION; \
               $O_CLI -dtls1 -sess_in $SESSION; \
               rm -f $SESSION )" \
            0 \
            -s "found session ticket extension" \
            -s "server hello, adding session ticket extension" \
            -S "session successfully restored from cache" \
            -s "session successfully restored from ticket" \
            -s "a session has been resumed"

# Tests for Session Resume based on session-ID and cache

run_test    "Session resume using cache: tickets enabled on client" \
            "$P_SRV debug_level=3 tickets=0" \
            "$P_CLI debug_level=3 tickets=1 reconnect=1" \
            0 \
            -c "client hello, adding session ticket extension" \
            -s "found session ticket extension" \
            -S "server hello, adding session ticket extension" \
            -C "found session_ticket extension" \
            -C "parse new session ticket" \
            -s "session successfully restored from cache" \
            -S "session successfully restored from ticket" \
            -s "a session has been resumed" \
            -c "a session has been resumed"

run_test    "Session resume using cache: tickets enabled on server" \
            "$P_SRV debug_level=3 tickets=1" \
            "$P_CLI debug_level=3 tickets=0 reconnect=1" \
            0 \
            -C "client hello, adding session ticket extension" \
            -S "found session ticket extension" \
            -S "server hello, adding session ticket extension" \
            -C "found session_ticket extension" \
            -C "parse new session ticket" \
            -s "session successfully restored from cache" \
            -S "session successfully restored from ticket" \
            -s "a session has been resumed" \
            -c "a session has been resumed"

run_test    "Session resume using cache: cache_max=0" \
            "$P_SRV debug_level=3 tickets=0 cache_max=0" \
            "$P_CLI debug_level=3 tickets=0 reconnect=1" \
            0 \
            -S "session successfully restored from cache" \
            -S "session successfully restored from ticket" \
            -S "a session has been resumed" \
            -C "a session has been resumed"

run_test    "Session resume using cache: cache_max=1" \
            "$P_SRV debug_level=3 tickets=0 cache_max=1" \
            "$P_CLI debug_level=3 tickets=0 reconnect=1" \
            0 \
            -s "session successfully restored from cache" \
            -S "session successfully restored from ticket" \
            -s "a session has been resumed" \
            -c "a session has been resumed"

run_test    "Session resume using cache: timeout > delay" \
            "$P_SRV debug_level=3 tickets=0" \
            "$P_CLI debug_level=3 tickets=0 reconnect=1 reco_delay=0" \
            0 \
            -s "session successfully restored from cache" \
            -S "session successfully restored from ticket" \
            -s "a session has been resumed" \
            -c "a session has been resumed"

run_test    "Session resume using cache: timeout < delay" \
            "$P_SRV debug_level=3 tickets=0 cache_timeout=1" \
            "$P_CLI debug_level=3 tickets=0 reconnect=1 reco_delay=2" \
            0 \
            -S "session successfully restored from cache" \
            -S "session successfully restored from ticket" \
            -S "a session has been resumed" \
            -C "a session has been resumed"

run_test    "Session resume using cache: no timeout" \
            "$P_SRV debug_level=3 tickets=0 cache_timeout=0" \
            "$P_CLI debug_level=3 tickets=0 reconnect=1 reco_delay=2" \
            0 \
            -s "session successfully restored from cache" \
            -S "session successfully restored from ticket" \
            -s "a session has been resumed" \
            -c "a session has been resumed"

run_test    "Session resume using cache: openssl client" \
            "$P_SRV debug_level=3 tickets=0" \
            "( $O_CLI -sess_out $SESSION; \
               $O_CLI -sess_in $SESSION; \
               rm -f $SESSION )" \
            0 \
            -s "found session ticket extension" \
            -S "server hello, adding session ticket extension" \
            -s "session successfully restored from cache" \
            -S "session successfully restored from ticket" \
            -s "a session has been resumed"

run_test    "Session resume using cache: openssl server" \
            "$O_SRV" \
            "$P_CLI debug_level=3 tickets=0 reconnect=1" \
            0 \
            -C "found session_ticket extension" \
            -C "parse new session ticket" \
            -c "a session has been resumed"

# Tests for Session Resume based on session-ID and cache, DTLS

run_test    "Session resume using cache, DTLS: tickets enabled on client" \
            "$P_SRV dtls=1 debug_level=3 tickets=0" \
            "$P_CLI dtls=1 debug_level=3 tickets=1 reconnect=1" \
            0 \
            -c "client hello, adding session ticket extension" \
            -s "found session ticket extension" \
            -S "server hello, adding session ticket extension" \
            -C "found session_ticket extension" \
            -C "parse new session ticket" \
            -s "session successfully restored from cache" \
            -S "session successfully restored from ticket" \
            -s "a session has been resumed" \
            -c "a session has been resumed"

run_test    "Session resume using cache, DTLS: tickets enabled on server" \
            "$P_SRV dtls=1 debug_level=3 tickets=1" \
            "$P_CLI dtls=1 debug_level=3 tickets=0 reconnect=1" \
            0 \
            -C "client hello, adding session ticket extension" \
            -S "found session ticket extension" \
            -S "server hello, adding session ticket extension" \
            -C "found session_ticket extension" \
            -C "parse new session ticket" \
            -s "session successfully restored from cache" \
            -S "session successfully restored from ticket" \
            -s "a session has been resumed" \
            -c "a session has been resumed"

run_test    "Session resume using cache, DTLS: cache_max=0" \
            "$P_SRV dtls=1 debug_level=3 tickets=0 cache_max=0" \
            "$P_CLI dtls=1 debug_level=3 tickets=0 reconnect=1" \
            0 \
            -S "session successfully restored from cache" \
            -S "session successfully restored from ticket" \
            -S "a session has been resumed" \
            -C "a session has been resumed"

run_test    "Session resume using cache, DTLS: cache_max=1" \
            "$P_SRV dtls=1 debug_level=3 tickets=0 cache_max=1" \
            "$P_CLI dtls=1 debug_level=3 tickets=0 reconnect=1" \
            0 \
            -s "session successfully restored from cache" \
            -S "session successfully restored from ticket" \
            -s "a session has been resumed" \
            -c "a session has been resumed"

run_test    "Session resume using cache, DTLS: timeout > delay" \
            "$P_SRV dtls=1 debug_level=3 tickets=0" \
            "$P_CLI dtls=1 debug_level=3 tickets=0 reconnect=1 reco_delay=0" \
            0 \
            -s "session successfully restored from cache" \
            -S "session successfully restored from ticket" \
            -s "a session has been resumed" \
            -c "a session has been resumed"

run_test    "Session resume using cache, DTLS: timeout < delay" \
            "$P_SRV dtls=1 debug_level=3 tickets=0 cache_timeout=1" \
            "$P_CLI dtls=1 debug_level=3 tickets=0 reconnect=1 reco_delay=2" \
            0 \
            -S "session successfully restored from cache" \
            -S "session successfully restored from ticket" \
            -S "a session has been resumed" \
            -C "a session has been resumed"

run_test    "Session resume using cache, DTLS: no timeout" \
            "$P_SRV dtls=1 debug_level=3 tickets=0 cache_timeout=0" \
            "$P_CLI dtls=1 debug_level=3 tickets=0 reconnect=1 reco_delay=2" \
            0 \
            -s "session successfully restored from cache" \
            -S "session successfully restored from ticket" \
            -s "a session has been resumed" \
            -c "a session has been resumed"

run_test    "Session resume using cache, DTLS: openssl client" \
            "$P_SRV dtls=1 debug_level=3 tickets=0" \
            "( $O_CLI -dtls1 -sess_out $SESSION; \
               $O_CLI -dtls1 -sess_in $SESSION; \
               rm -f $SESSION )" \
            0 \
            -s "found session ticket extension" \
            -S "server hello, adding session ticket extension" \
            -s "session successfully restored from cache" \
            -S "session successfully restored from ticket" \
            -s "a session has been resumed"

run_test    "Session resume using cache, DTLS: openssl server" \
            "$O_SRV -dtls1" \
            "$P_CLI dtls=1 debug_level=3 tickets=0 reconnect=1" \
            0 \
            -C "found session_ticket extension" \
            -C "parse new session ticket" \
            -c "a session has been resumed"

# Tests for Max Fragment Length extension

if [ "$MAX_CONTENT_LEN" -lt "4096" ]; then
    printf "${CONFIG_H} defines MBEDTLS_SSL_MAX_CONTENT_LEN to be less than 4096. Fragment length tests will fail.\n"
    exit 1
fi

if [ $MAX_CONTENT_LEN -ne 16384 ]; then
    printf "Using non-default maximum content length $MAX_CONTENT_LEN\n"
fi

requires_config_enabled MBEDTLS_SSL_MAX_FRAGMENT_LENGTH
run_test    "Max fragment length: enabled, default" \
            "$P_SRV debug_level=3" \
            "$P_CLI debug_level=3" \
            0 \
            -c "Maximum fragment length is $MAX_CONTENT_LEN" \
            -s "Maximum fragment length is $MAX_CONTENT_LEN" \
            -C "client hello, adding max_fragment_length extension" \
            -S "found max fragment length extension" \
            -S "server hello, max_fragment_length extension" \
            -C "found max_fragment_length extension"

requires_config_enabled MBEDTLS_SSL_MAX_FRAGMENT_LENGTH
run_test    "Max fragment length: enabled, default, larger message" \
            "$P_SRV debug_level=3" \
            "$P_CLI debug_level=3 request_size=$(( $MAX_CONTENT_LEN + 1))" \
            0 \
            -c "Maximum fragment length is $MAX_CONTENT_LEN" \
            -s "Maximum fragment length is $MAX_CONTENT_LEN" \
            -C "client hello, adding max_fragment_length extension" \
            -S "found max fragment length extension" \
            -S "server hello, max_fragment_length extension" \
            -C "found max_fragment_length extension" \
            -c "$(( $MAX_CONTENT_LEN + 1)) bytes written in 2 fragments" \
            -s "$MAX_CONTENT_LEN bytes read" \
            -s "1 bytes read"

requires_config_enabled MBEDTLS_SSL_MAX_FRAGMENT_LENGTH
run_test    "Max fragment length, DTLS: enabled, default, larger message" \
            "$P_SRV debug_level=3 dtls=1" \
            "$P_CLI debug_level=3 dtls=1 request_size=$(( $MAX_CONTENT_LEN + 1))" \
            1 \
            -c "Maximum fragment length is $MAX_CONTENT_LEN" \
            -s "Maximum fragment length is $MAX_CONTENT_LEN" \
            -C "client hello, adding max_fragment_length extension" \
            -S "found max fragment length extension" \
            -S "server hello, max_fragment_length extension" \
            -C "found max_fragment_length extension" \
            -c "fragment larger than.*maximum "

# Run some tests with MBEDTLS_SSL_MAX_FRAGMENT_LENGTH disabled
# (session fragment length will be 16384 regardless of mbedtls
# content length configuration.)

requires_config_disabled MBEDTLS_SSL_MAX_FRAGMENT_LENGTH
run_test    "Max fragment length: disabled, larger message" \
            "$P_SRV debug_level=3" \
            "$P_CLI debug_level=3 request_size=$(( $MAX_CONTENT_LEN + 1))" \
            0 \
            -C "Maximum fragment length is 16384" \
            -S "Maximum fragment length is 16384" \
            -c "$(( $MAX_CONTENT_LEN + 1)) bytes written in 2 fragments" \
            -s "$MAX_CONTENT_LEN bytes read" \
            -s "1 bytes read"

requires_config_disabled MBEDTLS_SSL_MAX_FRAGMENT_LENGTH
run_test    "Max fragment length DTLS: disabled, larger message" \
            "$P_SRV debug_level=3 dtls=1" \
            "$P_CLI debug_level=3 dtls=1 request_size=$(( $MAX_CONTENT_LEN + 1))" \
            1 \
            -C "Maximum fragment length is 16384" \
            -S "Maximum fragment length is 16384" \
            -c "fragment larger than.*maximum "

requires_config_enabled MBEDTLS_SSL_MAX_FRAGMENT_LENGTH
run_test    "Max fragment length: used by client" \
            "$P_SRV debug_level=3" \
            "$P_CLI debug_level=3 max_frag_len=4096" \
            0 \
            -c "Maximum fragment length is 4096" \
            -s "Maximum fragment length is 4096" \
            -c "client hello, adding max_fragment_length extension" \
            -s "found max fragment length extension" \
            -s "server hello, max_fragment_length extension" \
            -c "found max_fragment_length extension"

requires_config_enabled MBEDTLS_SSL_MAX_FRAGMENT_LENGTH
run_test    "Max fragment length: used by server" \
            "$P_SRV debug_level=3 max_frag_len=4096" \
            "$P_CLI debug_level=3" \
            0 \
            -c "Maximum fragment length is $MAX_CONTENT_LEN" \
            -s "Maximum fragment length is 4096" \
            -C "client hello, adding max_fragment_length extension" \
            -S "found max fragment length extension" \
            -S "server hello, max_fragment_length extension" \
            -C "found max_fragment_length extension"

requires_config_enabled MBEDTLS_SSL_MAX_FRAGMENT_LENGTH
requires_gnutls
run_test    "Max fragment length: gnutls server" \
            "$G_SRV" \
            "$P_CLI debug_level=3 max_frag_len=4096" \
            0 \
            -c "Maximum fragment length is 4096" \
            -c "client hello, adding max_fragment_length extension" \
            -c "found max_fragment_length extension"

requires_config_enabled MBEDTLS_SSL_MAX_FRAGMENT_LENGTH
run_test    "Max fragment length: client, message just fits" \
            "$P_SRV debug_level=3" \
            "$P_CLI debug_level=3 max_frag_len=2048 request_size=2048" \
            0 \
            -c "Maximum fragment length is 2048" \
            -s "Maximum fragment length is 2048" \
            -c "client hello, adding max_fragment_length extension" \
            -s "found max fragment length extension" \
            -s "server hello, max_fragment_length extension" \
            -c "found max_fragment_length extension" \
            -c "2048 bytes written in 1 fragments" \
            -s "2048 bytes read"

requires_config_enabled MBEDTLS_SSL_MAX_FRAGMENT_LENGTH
run_test    "Max fragment length: client, larger message" \
            "$P_SRV debug_level=3" \
            "$P_CLI debug_level=3 max_frag_len=2048 request_size=2345" \
            0 \
            -c "Maximum fragment length is 2048" \
            -s "Maximum fragment length is 2048" \
            -c "client hello, adding max_fragment_length extension" \
            -s "found max fragment length extension" \
            -s "server hello, max_fragment_length extension" \
            -c "found max_fragment_length extension" \
            -c "2345 bytes written in 2 fragments" \
            -s "2048 bytes read" \
            -s "297 bytes read"

requires_config_enabled MBEDTLS_SSL_MAX_FRAGMENT_LENGTH
run_test    "Max fragment length: DTLS client, larger message" \
            "$P_SRV debug_level=3 dtls=1" \
            "$P_CLI debug_level=3 dtls=1 max_frag_len=2048 request_size=2345" \
            1 \
            -c "Maximum fragment length is 2048" \
            -s "Maximum fragment length is 2048" \
            -c "client hello, adding max_fragment_length extension" \
            -s "found max fragment length extension" \
            -s "server hello, max_fragment_length extension" \
            -c "found max_fragment_length extension" \
            -c "fragment larger than.*maximum"

# Tests for renegotiation

# Renegotiation SCSV always added, regardless of SSL_RENEGOTIATION
run_test    "Renegotiation: none, for reference" \
            "$P_SRV debug_level=3 exchanges=2 auth_mode=optional" \
            "$P_CLI debug_level=3 exchanges=2" \
            0 \
            -C "client hello, adding renegotiation extension" \
            -s "received TLS_EMPTY_RENEGOTIATION_INFO" \
            -S "found renegotiation extension" \
            -s "server hello, secure renegotiation extension" \
            -c "found renegotiation extension" \
            -C "=> renegotiate" \
            -S "=> renegotiate" \
            -S "write hello request"

requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
run_test    "Renegotiation: client-initiated" \
            "$P_SRV debug_level=3 exchanges=2 renegotiation=1 auth_mode=optional" \
            "$P_CLI debug_level=3 exchanges=2 renegotiation=1 renegotiate=1" \
            0 \
            -c "client hello, adding renegotiation extension" \
            -s "received TLS_EMPTY_RENEGOTIATION_INFO" \
            -s "found renegotiation extension" \
            -s "server hello, secure renegotiation extension" \
            -c "found renegotiation extension" \
            -c "=> renegotiate" \
            -s "=> renegotiate" \
            -S "write hello request"

requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
run_test    "Renegotiation: server-initiated" \
            "$P_SRV debug_level=3 exchanges=2 renegotiation=1 auth_mode=optional renegotiate=1" \
            "$P_CLI debug_level=3 exchanges=2 renegotiation=1" \
            0 \
            -c "client hello, adding renegotiation extension" \
            -s "received TLS_EMPTY_RENEGOTIATION_INFO" \
            -s "found renegotiation extension" \
            -s "server hello, secure renegotiation extension" \
            -c "found renegotiation extension" \
            -c "=> renegotiate" \
            -s "=> renegotiate" \
            -s "write hello request"

# Checks that no Signature Algorithm with SHA-1 gets negotiated. Negotiating SHA-1 would mean that
# the server did not parse the Signature Algorithm extension. This test is valid only if an MD
# algorithm stronger than SHA-1 is enabled in config.h
requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
run_test    "Renegotiation: Signature Algorithms parsing, client-initiated" \
            "$P_SRV debug_level=3 exchanges=2 renegotiation=1 auth_mode=optional" \
            "$P_CLI debug_level=3 exchanges=2 renegotiation=1 renegotiate=1" \
            0 \
            -c "client hello, adding renegotiation extension" \
            -s "received TLS_EMPTY_RENEGOTIATION_INFO" \
            -s "found renegotiation extension" \
            -s "server hello, secure renegotiation extension" \
            -c "found renegotiation extension" \
            -c "=> renegotiate" \
            -s "=> renegotiate" \
            -S "write hello request" \
            -S "client hello v3, signature_algorithm ext: 2" # Is SHA-1 negotiated?

# Checks that no Signature Algorithm with SHA-1 gets negotiated. Negotiating SHA-1 would mean that
# the server did not parse the Signature Algorithm extension. This test is valid only if an MD
# algorithm stronger than SHA-1 is enabled in config.h
requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
run_test    "Renegotiation: Signature Algorithms parsing, server-initiated" \
            "$P_SRV debug_level=3 exchanges=2 renegotiation=1 auth_mode=optional renegotiate=1" \
            "$P_CLI debug_level=3 exchanges=2 renegotiation=1" \
            0 \
            -c "client hello, adding renegotiation extension" \
            -s "received TLS_EMPTY_RENEGOTIATION_INFO" \
            -s "found renegotiation extension" \
            -s "server hello, secure renegotiation extension" \
            -c "found renegotiation extension" \
            -c "=> renegotiate" \
            -s "=> renegotiate" \
            -s "write hello request" \
            -S "client hello v3, signature_algorithm ext: 2" # Is SHA-1 negotiated?

requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
run_test    "Renegotiation: double" \
            "$P_SRV debug_level=3 exchanges=2 renegotiation=1 auth_mode=optional renegotiate=1" \
            "$P_CLI debug_level=3 exchanges=2 renegotiation=1 renegotiate=1" \
            0 \
            -c "client hello, adding renegotiation extension" \
            -s "received TLS_EMPTY_RENEGOTIATION_INFO" \
            -s "found renegotiation extension" \
            -s "server hello, secure renegotiation extension" \
            -c "found renegotiation extension" \
            -c "=> renegotiate" \
            -s "=> renegotiate" \
            -s "write hello request"

requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
run_test    "Renegotiation: client-initiated, server-rejected" \
            "$P_SRV debug_level=3 exchanges=2 renegotiation=0 auth_mode=optional" \
            "$P_CLI debug_level=3 exchanges=2 renegotiation=1 renegotiate=1" \
            1 \
            -c "client hello, adding renegotiation extension" \
            -s "received TLS_EMPTY_RENEGOTIATION_INFO" \
            -S "found renegotiation extension" \
            -s "server hello, secure renegotiation extension" \
            -c "found renegotiation extension" \
            -c "=> renegotiate" \
            -S "=> renegotiate" \
            -S "write hello request" \
            -c "SSL - Unexpected message at ServerHello in renegotiation" \
            -c "failed"

requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
run_test    "Renegotiation: server-initiated, client-rejected, default" \
            "$P_SRV debug_level=3 exchanges=2 renegotiation=1 renegotiate=1 auth_mode=optional" \
            "$P_CLI debug_level=3 exchanges=2 renegotiation=0" \
            0 \
            -C "client hello, adding renegotiation extension" \
            -s "received TLS_EMPTY_RENEGOTIATION_INFO" \
            -S "found renegotiation extension" \
            -s "server hello, secure renegotiation extension" \
            -c "found renegotiation extension" \
            -C "=> renegotiate" \
            -S "=> renegotiate" \
            -s "write hello request" \
            -S "SSL - An unexpected message was received from our peer" \
            -S "failed"

requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
run_test    "Renegotiation: server-initiated, client-rejected, not enforced" \
            "$P_SRV debug_level=3 exchanges=2 renegotiation=1 renegotiate=1 \
             renego_delay=-1 auth_mode=optional" \
            "$P_CLI debug_level=3 exchanges=2 renegotiation=0" \
            0 \
            -C "client hello, adding renegotiation extension" \
            -s "received TLS_EMPTY_RENEGOTIATION_INFO" \
            -S "found renegotiation extension" \
            -s "server hello, secure renegotiation extension" \
            -c "found renegotiation extension" \
            -C "=> renegotiate" \
            -S "=> renegotiate" \
            -s "write hello request" \
            -S "SSL - An unexpected message was received from our peer" \
            -S "failed"

# delay 2 for 1 alert record + 1 application data record
requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
run_test    "Renegotiation: server-initiated, client-rejected, delay 2" \
            "$P_SRV debug_level=3 exchanges=2 renegotiation=1 renegotiate=1 \
             renego_delay=2 auth_mode=optional" \
            "$P_CLI debug_level=3 exchanges=2 renegotiation=0" \
            0 \
            -C "client hello, adding renegotiation extension" \
            -s "received TLS_EMPTY_RENEGOTIATION_INFO" \
            -S "found renegotiation extension" \
            -s "server hello, secure renegotiation extension" \
            -c "found renegotiation extension" \
            -C "=> renegotiate" \
            -S "=> renegotiate" \
            -s "write hello request" \
            -S "SSL - An unexpected message was received from our peer" \
            -S "failed"

requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
run_test    "Renegotiation: server-initiated, client-rejected, delay 0" \
            "$P_SRV debug_level=3 exchanges=2 renegotiation=1 renegotiate=1 \
             renego_delay=0 auth_mode=optional" \
            "$P_CLI debug_level=3 exchanges=2 renegotiation=0" \
            0 \
            -C "client hello, adding renegotiation extension" \
            -s "received TLS_EMPTY_RENEGOTIATION_INFO" \
            -S "found renegotiation extension" \
            -s "server hello, secure renegotiation extension" \
            -c "found renegotiation extension" \
            -C "=> renegotiate" \
            -S "=> renegotiate" \
            -s "write hello request" \
            -s "SSL - An unexpected message was received from our peer"

requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
run_test    "Renegotiation: server-initiated, client-accepted, delay 0" \
            "$P_SRV debug_level=3 exchanges=2 renegotiation=1 renegotiate=1 \
             renego_delay=0 auth_mode=optional" \
            "$P_CLI debug_level=3 exchanges=2 renegotiation=1" \
            0 \
            -c "client hello, adding renegotiation extension" \
            -s "received TLS_EMPTY_RENEGOTIATION_INFO" \
            -s "found renegotiation extension" \
            -s "server hello, secure renegotiation extension" \
            -c "found renegotiation extension" \
            -c "=> renegotiate" \
            -s "=> renegotiate" \
            -s "write hello request" \
            -S "SSL - An unexpected message was received from our peer" \
            -S "failed"

requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
run_test    "Renegotiation: periodic, just below period" \
            "$P_SRV debug_level=3 exchanges=9 renegotiation=1 renego_period=3 auth_mode=optional" \
            "$P_CLI debug_level=3 exchanges=2 renegotiation=1" \
            0 \
            -C "client hello, adding renegotiation extension" \
            -s "received TLS_EMPTY_RENEGOTIATION_INFO" \
            -S "found renegotiation extension" \
            -s "server hello, secure renegotiation extension" \
            -c "found renegotiation extension" \
            -S "record counter limit reached: renegotiate" \
            -C "=> renegotiate" \
            -S "=> renegotiate" \
            -S "write hello request" \
            -S "SSL - An unexpected message was received from our peer" \
            -S "failed"

# one extra exchange to be able to complete renego
requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
run_test    "Renegotiation: periodic, just above period" \
            "$P_SRV debug_level=3 exchanges=9 renegotiation=1 renego_period=3 auth_mode=optional" \
            "$P_CLI debug_level=3 exchanges=4 renegotiation=1" \
            0 \
            -c "client hello, adding renegotiation extension" \
            -s "received TLS_EMPTY_RENEGOTIATION_INFO" \
            -s "found renegotiation extension" \
            -s "server hello, secure renegotiation extension" \
            -c "found renegotiation extension" \
            -s "record counter limit reached: renegotiate" \
            -c "=> renegotiate" \
            -s "=> renegotiate" \
            -s "write hello request" \
            -S "SSL - An unexpected message was received from our peer" \
            -S "failed"

requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
run_test    "Renegotiation: periodic, two times period" \
            "$P_SRV debug_level=3 exchanges=9 renegotiation=1 renego_period=3 auth_mode=optional" \
            "$P_CLI debug_level=3 exchanges=7 renegotiation=1" \
            0 \
            -c "client hello, adding renegotiation extension" \
            -s "received TLS_EMPTY_RENEGOTIATION_INFO" \
            -s "found renegotiation extension" \
            -s "server hello, secure renegotiation extension" \
            -c "found renegotiation extension" \
            -s "record counter limit reached: renegotiate" \
            -c "=> renegotiate" \
            -s "=> renegotiate" \
            -s "write hello request" \
            -S "SSL - An unexpected message was received from our peer" \
            -S "failed"

requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
run_test    "Renegotiation: periodic, above period, disabled" \
            "$P_SRV debug_level=3 exchanges=9 renegotiation=0 renego_period=3 auth_mode=optional" \
            "$P_CLI debug_level=3 exchanges=4 renegotiation=1" \
            0 \
            -C "client hello, adding renegotiation extension" \
            -s "received TLS_EMPTY_RENEGOTIATION_INFO" \
            -S "found renegotiation extension" \
            -s "server hello, secure renegotiation extension" \
            -c "found renegotiation extension" \
            -S "record counter limit reached: renegotiate" \
            -C "=> renegotiate" \
            -S "=> renegotiate" \
            -S "write hello request" \
            -S "SSL - An unexpected message was received from our peer" \
            -S "failed"

requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
run_test    "Renegotiation: nbio, client-initiated" \
            "$P_SRV debug_level=3 nbio=2 exchanges=2 renegotiation=1 auth_mode=optional" \
            "$P_CLI debug_level=3 nbio=2 exchanges=2 renegotiation=1 renegotiate=1" \
            0 \
            -c "client hello, adding renegotiation extension" \
            -s "received TLS_EMPTY_RENEGOTIATION_INFO" \
            -s "found renegotiation extension" \
            -s "server hello, secure renegotiation extension" \
            -c "found renegotiation extension" \
            -c "=> renegotiate" \
            -s "=> renegotiate" \
            -S "write hello request"

requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
run_test    "Renegotiation: nbio, server-initiated" \
            "$P_SRV debug_level=3 nbio=2 exchanges=2 renegotiation=1 renegotiate=1 auth_mode=optional" \
            "$P_CLI debug_level=3 nbio=2 exchanges=2 renegotiation=1" \
            0 \
            -c "client hello, adding renegotiation extension" \
            -s "received TLS_EMPTY_RENEGOTIATION_INFO" \
            -s "found renegotiation extension" \
            -s "server hello, secure renegotiation extension" \
            -c "found renegotiation extension" \
            -c "=> renegotiate" \
            -s "=> renegotiate" \
            -s "write hello request"

requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
run_test    "Renegotiation: openssl server, client-initiated" \
            "$O_SRV -www" \
            "$P_CLI debug_level=3 exchanges=1 renegotiation=1 renegotiate=1" \
            0 \
            -c "client hello, adding renegotiation extension" \
            -c "found renegotiation extension" \
            -c "=> renegotiate" \
            -C "ssl_hanshake() returned" \
            -C "error" \
            -c "HTTP/1.0 200 [Oo][Kk]"

requires_gnutls
requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
run_test    "Renegotiation: gnutls server strict, client-initiated" \
            "$G_SRV --priority=NORMAL:%SAFE_RENEGOTIATION" \
            "$P_CLI debug_level=3 exchanges=1 renegotiation=1 renegotiate=1" \
            0 \
            -c "client hello, adding renegotiation extension" \
            -c "found renegotiation extension" \
            -c "=> renegotiate" \
            -C "ssl_hanshake() returned" \
            -C "error" \
            -c "HTTP/1.0 200 [Oo][Kk]"

requires_gnutls
requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
run_test    "Renegotiation: gnutls server unsafe, client-initiated default" \
            "$G_SRV --priority=NORMAL:%DISABLE_SAFE_RENEGOTIATION" \
            "$P_CLI debug_level=3 exchanges=1 renegotiation=1 renegotiate=1" \
            1 \
            -c "client hello, adding renegotiation extension" \
            -C "found renegotiation extension" \
            -c "=> renegotiate" \
            -c "mbedtls_ssl_handshake() returned" \
            -c "error" \
            -C "HTTP/1.0 200 [Oo][Kk]"

requires_gnutls
requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
run_test    "Renegotiation: gnutls server unsafe, client-inititated no legacy" \
            "$G_SRV --priority=NORMAL:%DISABLE_SAFE_RENEGOTIATION" \
            "$P_CLI debug_level=3 exchanges=1 renegotiation=1 renegotiate=1 \
             allow_legacy=0" \
            1 \
            -c "client hello, adding renegotiation extension" \
            -C "found renegotiation extension" \
            -c "=> renegotiate" \
            -c "mbedtls_ssl_handshake() returned" \
            -c "error" \
            -C "HTTP/1.0 200 [Oo][Kk]"

requires_gnutls
requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
run_test    "Renegotiation: gnutls server unsafe, client-inititated legacy" \
            "$G_SRV --priority=NORMAL:%DISABLE_SAFE_RENEGOTIATION" \
            "$P_CLI debug_level=3 exchanges=1 renegotiation=1 renegotiate=1 \
             allow_legacy=1" \
            0 \
            -c "client hello, adding renegotiation extension" \
            -C "found renegotiation extension" \
            -c "=> renegotiate" \
            -C "ssl_hanshake() returned" \
            -C "error" \
            -c "HTTP/1.0 200 [Oo][Kk]"

requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
run_test    "Renegotiation: DTLS, client-initiated" \
            "$P_SRV debug_level=3 dtls=1 exchanges=2 renegotiation=1" \
            "$P_CLI debug_level=3 dtls=1 exchanges=2 renegotiation=1 renegotiate=1" \
            0 \
            -c "client hello, adding renegotiation extension" \
            -s "received TLS_EMPTY_RENEGOTIATION_INFO" \
            -s "found renegotiation extension" \
            -s "server hello, secure renegotiation extension" \
            -c "found renegotiation extension" \
            -c "=> renegotiate" \
            -s "=> renegotiate" \
            -S "write hello request"

requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
run_test    "Renegotiation: DTLS, server-initiated" \
            "$P_SRV debug_level=3 dtls=1 exchanges=2 renegotiation=1 renegotiate=1" \
            "$P_CLI debug_level=3 dtls=1 exchanges=2 renegotiation=1 \
             read_timeout=1000 max_resend=2" \
            0 \
            -c "client hello, adding renegotiation extension" \
            -s "received TLS_EMPTY_RENEGOTIATION_INFO" \
            -s "found renegotiation extension" \
            -s "server hello, secure renegotiation extension" \
            -c "found renegotiation extension" \
            -c "=> renegotiate" \
            -s "=> renegotiate" \
            -s "write hello request"

requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
run_test    "Renegotiation: DTLS, renego_period overflow" \
            "$P_SRV debug_level=3 dtls=1 exchanges=4 renegotiation=1 renego_period=18446462598732840962 auth_mode=optional" \
            "$P_CLI debug_level=3 dtls=1 exchanges=4 renegotiation=1" \
            0 \
            -c "client hello, adding renegotiation extension" \
            -s "received TLS_EMPTY_RENEGOTIATION_INFO" \
            -s "found renegotiation extension" \
            -s "server hello, secure renegotiation extension" \
            -s "record counter limit reached: renegotiate" \
            -c "=> renegotiate" \
            -s "=> renegotiate" \
            -s "write hello request"

requires_gnutls
requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
run_test    "Renegotiation: DTLS, gnutls server, client-initiated" \
            "$G_SRV -u --mtu 4096" \
            "$P_CLI debug_level=3 dtls=1 exchanges=1 renegotiation=1 renegotiate=1" \
            0 \
            -c "client hello, adding renegotiation extension" \
            -c "found renegotiation extension" \
            -c "=> renegotiate" \
            -C "mbedtls_ssl_handshake returned" \
            -C "error" \
            -s "Extra-header:"

# Test for the "secure renegotation" extension only (no actual renegotiation)

requires_gnutls
run_test    "Renego ext: gnutls server strict, client default" \
            "$G_SRV --priority=NORMAL:%SAFE_RENEGOTIATION" \
            "$P_CLI debug_level=3" \
            0 \
            -c "found renegotiation extension" \
            -C "error" \
            -c "HTTP/1.0 200 [Oo][Kk]"

requires_gnutls
run_test    "Renego ext: gnutls server unsafe, client default" \
            "$G_SRV --priority=NORMAL:%DISABLE_SAFE_RENEGOTIATION" \
            "$P_CLI debug_level=3" \
            0 \
            -C "found renegotiation extension" \
            -C "error" \
            -c "HTTP/1.0 200 [Oo][Kk]"

requires_gnutls
run_test    "Renego ext: gnutls server unsafe, client break legacy" \
            "$G_SRV --priority=NORMAL:%DISABLE_SAFE_RENEGOTIATION" \
            "$P_CLI debug_level=3 allow_legacy=-1" \
            1 \
            -C "found renegotiation extension" \
            -c "error" \
            -C "HTTP/1.0 200 [Oo][Kk]"

requires_gnutls
run_test    "Renego ext: gnutls client strict, server default" \
            "$P_SRV debug_level=3" \
            "$G_CLI --priority=NORMAL:%SAFE_RENEGOTIATION localhost" \
            0 \
            -s "received TLS_EMPTY_RENEGOTIATION_INFO\|found renegotiation extension" \
            -s "server hello, secure renegotiation extension"

requires_gnutls
run_test    "Renego ext: gnutls client unsafe, server default" \
            "$P_SRV debug_level=3" \
            "$G_CLI --priority=NORMAL:%DISABLE_SAFE_RENEGOTIATION localhost" \
            0 \
            -S "received TLS_EMPTY_RENEGOTIATION_INFO\|found renegotiation extension" \
            -S "server hello, secure renegotiation extension"

requires_gnutls
run_test    "Renego ext: gnutls client unsafe, server break legacy" \
            "$P_SRV debug_level=3 allow_legacy=-1" \
            "$G_CLI --priority=NORMAL:%DISABLE_SAFE_RENEGOTIATION localhost" \
            1 \
            -S "received TLS_EMPTY_RENEGOTIATION_INFO\|found renegotiation extension" \
            -S "server hello, secure renegotiation extension"

# Tests for silently dropping trailing extra bytes in .der certificates

requires_gnutls
run_test    "DER format: no trailing bytes" \
            "$P_SRV crt_file=data_files/server5-der0.crt \
             key_file=data_files/server5.key" \
            "$G_CLI localhost" \
            0 \
            -c "Handshake was completed" \

requires_gnutls
run_test    "DER format: with a trailing zero byte" \
            "$P_SRV crt_file=data_files/server5-der1a.crt \
             key_file=data_files/server5.key" \
            "$G_CLI localhost" \
            0 \
            -c "Handshake was completed" \

requires_gnutls
run_test    "DER format: with a trailing random byte" \
            "$P_SRV crt_file=data_files/server5-der1b.crt \
             key_file=data_files/server5.key" \
            "$G_CLI localhost" \
            0 \
            -c "Handshake was completed" \

requires_gnutls
run_test    "DER format: with 2 trailing random bytes" \
            "$P_SRV crt_file=data_files/server5-der2.crt \
             key_file=data_files/server5.key" \
            "$G_CLI localhost" \
            0 \
            -c "Handshake was completed" \

requires_gnutls
run_test    "DER format: with 4 trailing random bytes" \
            "$P_SRV crt_file=data_files/server5-der4.crt \
             key_file=data_files/server5.key" \
            "$G_CLI localhost" \
            0 \
            -c "Handshake was completed" \

requires_gnutls
run_test    "DER format: with 8 trailing random bytes" \
            "$P_SRV crt_file=data_files/server5-der8.crt \
             key_file=data_files/server5.key" \
            "$G_CLI localhost" \
            0 \
            -c "Handshake was completed" \

requires_gnutls
run_test    "DER format: with 9 trailing random bytes" \
            "$P_SRV crt_file=data_files/server5-der9.crt \
             key_file=data_files/server5.key" \
            "$G_CLI localhost" \
            0 \
            -c "Handshake was completed" \

# Tests for auth_mode

run_test    "Authentication: server badcert, client required" \
            "$P_SRV crt_file=data_files/server5-badsign.crt \
             key_file=data_files/server5.key" \
            "$P_CLI debug_level=1 auth_mode=required" \
            1 \
            -c "x509_verify_cert() returned" \
            -c "! The certificate is not correctly signed by the trusted CA" \
            -c "! mbedtls_ssl_handshake returned" \
            -c "X509 - Certificate verification failed"

run_test    "Authentication: server badcert, client optional" \
            "$P_SRV crt_file=data_files/server5-badsign.crt \
             key_file=data_files/server5.key" \
            "$P_CLI debug_level=1 auth_mode=optional" \
            0 \
            -c "x509_verify_cert() returned" \
            -c "! The certificate is not correctly signed by the trusted CA" \
            -C "! mbedtls_ssl_handshake returned" \
            -C "X509 - Certificate verification failed"

run_test    "Authentication: server goodcert, client optional, no trusted CA" \
            "$P_SRV" \
            "$P_CLI debug_level=3 auth_mode=optional ca_file=none ca_path=none" \
            0 \
            -c "x509_verify_cert() returned" \
            -c "! The certificate is not correctly signed by the trusted CA" \
            -c "! Certificate verification flags"\
            -C "! mbedtls_ssl_handshake returned" \
            -C "X509 - Certificate verification failed" \
            -C "SSL - No CA Chain is set, but required to operate"

run_test    "Authentication: server goodcert, client required, no trusted CA" \
            "$P_SRV" \
            "$P_CLI debug_level=3 auth_mode=required ca_file=none ca_path=none" \
            1 \
            -c "x509_verify_cert() returned" \
            -c "! The certificate is not correctly signed by the trusted CA" \
            -c "! Certificate verification flags"\
            -c "! mbedtls_ssl_handshake returned" \
            -c "SSL - No CA Chain is set, but required to operate"

# The purpose of the next two tests is to test the client's behaviour when receiving a server
# certificate with an unsupported elliptic curve. This should usually not happen because
# the client informs the server about the supported curves - it does, though, in the
# corner case of a static ECDH suite, because the server doesn't check the curve on that
# occasion (to be fixed). If that bug's fixed, the test needs to be altered to use a
# different means to have the server ignoring the client's supported curve list.

requires_config_enabled MBEDTLS_ECP_C
run_test    "Authentication: server ECDH p256v1, client required, p256v1 unsupported" \
            "$P_SRV debug_level=1 key_file=data_files/server5.key \
             crt_file=data_files/server5.ku-ka.crt" \
            "$P_CLI debug_level=3 auth_mode=required curves=secp521r1" \
            1 \
            -c "bad certificate (EC key curve)"\
            -c "! Certificate verification flags"\
            -C "bad server certificate (ECDH curve)" # Expect failure at earlier verification stage

requires_config_enabled MBEDTLS_ECP_C
run_test    "Authentication: server ECDH p256v1, client optional, p256v1 unsupported" \
            "$P_SRV debug_level=1 key_file=data_files/server5.key \
             crt_file=data_files/server5.ku-ka.crt" \
            "$P_CLI debug_level=3 auth_mode=optional curves=secp521r1" \
            1 \
            -c "bad certificate (EC key curve)"\
            -c "! Certificate verification flags"\
            -c "bad server certificate (ECDH curve)" # Expect failure only at ECDH params check

run_test    "Authentication: server badcert, client none" \
            "$P_SRV crt_file=data_files/server5-badsign.crt \
             key_file=data_files/server5.key" \
            "$P_CLI debug_level=1 auth_mode=none" \
            0 \
            -C "x509_verify_cert() returned" \
            -C "! The certificate is not correctly signed by the trusted CA" \
            -C "! mbedtls_ssl_handshake returned" \
            -C "X509 - Certificate verification failed"

run_test    "Authentication: client SHA256, server required" \
            "$P_SRV auth_mode=required" \
            "$P_CLI debug_level=3 crt_file=data_files/server6.crt \
             key_file=data_files/server6.key \
             force_ciphersuite=TLS-ECDHE-ECDSA-WITH-AES-256-GCM-SHA384" \
            0 \
            -c "Supported Signature Algorithm found: 4," \
            -c "Supported Signature Algorithm found: 5,"

run_test    "Authentication: client SHA384, server required" \
            "$P_SRV auth_mode=required" \
            "$P_CLI debug_level=3 crt_file=data_files/server6.crt \
             key_file=data_files/server6.key \
             force_ciphersuite=TLS-ECDHE-ECDSA-WITH-AES-128-GCM-SHA256" \
            0 \
            -c "Supported Signature Algorithm found: 4," \
            -c "Supported Signature Algorithm found: 5,"

requires_config_enabled MBEDTLS_SSL_PROTO_SSL3
run_test    "Authentication: client has no cert, server required (SSLv3)" \
            "$P_SRV debug_level=3 min_version=ssl3 auth_mode=required" \
            "$P_CLI debug_level=3 force_version=ssl3 crt_file=none \
             key_file=data_files/server5.key" \
            1 \
            -S "skip write certificate request" \
            -C "skip parse certificate request" \
            -c "got a certificate request" \
            -c "got no certificate to send" \
            -S "x509_verify_cert() returned" \
            -s "client has no certificate" \
            -s "! mbedtls_ssl_handshake returned" \
            -c "! mbedtls_ssl_handshake returned" \
            -s "No client certification received from the client, but required by the authentication mode"

run_test    "Authentication: client has no cert, server required (TLS)" \
            "$P_SRV debug_level=3 auth_mode=required" \
            "$P_CLI debug_level=3 crt_file=none \
             key_file=data_files/server5.key" \
            1 \
            -S "skip write certificate request" \
            -C "skip parse certificate request" \
            -c "got a certificate request" \
            -c "= write certificate$" \
            -C "skip write certificate$" \
            -S "x509_verify_cert() returned" \
            -s "client has no certificate" \
            -s "! mbedtls_ssl_handshake returned" \
            -c "! mbedtls_ssl_handshake returned" \
            -s "No client certification received from the client, but required by the authentication mode"

run_test    "Authentication: client badcert, server required" \
            "$P_SRV debug_level=3 auth_mode=required" \
            "$P_CLI debug_level=3 crt_file=data_files/server5-badsign.crt \
             key_file=data_files/server5.key" \
            1 \
            -S "skip write certificate request" \
            -C "skip parse certificate request" \
            -c "got a certificate request" \
            -C "skip write certificate" \
            -C "skip write certificate verify" \
            -S "skip parse certificate verify" \
            -s "x509_verify_cert() returned" \
            -s "! The certificate is not correctly signed by the trusted CA" \
            -s "! mbedtls_ssl_handshake returned" \
            -s "send alert level=2 message=48" \
            -c "! mbedtls_ssl_handshake returned" \
            -s "X509 - Certificate verification failed"
# We don't check that the client receives the alert because it might
# detect that its write end of the connection is closed and abort
# before reading the alert message.

run_test    "Authentication: client cert not trusted, server required" \
            "$P_SRV debug_level=3 auth_mode=required" \
            "$P_CLI debug_level=3 crt_file=data_files/server5-selfsigned.crt \
             key_file=data_files/server5.key" \
            1 \
            -S "skip write certificate request" \
            -C "skip parse certificate request" \
            -c "got a certificate request" \
            -C "skip write certificate" \
            -C "skip write certificate verify" \
            -S "skip parse certificate verify" \
            -s "x509_verify_cert() returned" \
            -s "! The certificate is not correctly signed by the trusted CA" \
            -s "! mbedtls_ssl_handshake returned" \
            -c "! mbedtls_ssl_handshake returned" \
            -s "X509 - Certificate verification failed"

run_test    "Authentication: client badcert, server optional" \
            "$P_SRV debug_level=3 auth_mode=optional" \
            "$P_CLI debug_level=3 crt_file=data_files/server5-badsign.crt \
             key_file=data_files/server5.key" \
            0 \
            -S "skip write certificate request" \
            -C "skip parse certificate request" \
            -c "got a certificate request" \
            -C "skip write certificate" \
            -C "skip write certificate verify" \
            -S "skip parse certificate verify" \
            -s "x509_verify_cert() returned" \
            -s "! The certificate is not correctly signed by the trusted CA" \
            -S "! mbedtls_ssl_handshake returned" \
            -C "! mbedtls_ssl_handshake returned" \
            -S "X509 - Certificate verification failed"

run_test    "Authentication: client badcert, server none" \
            "$P_SRV debug_level=3 auth_mode=none" \
            "$P_CLI debug_level=3 crt_file=data_files/server5-badsign.crt \
             key_file=data_files/server5.key" \
            0 \
            -s "skip write certificate request" \
            -C "skip parse certificate request" \
            -c "got no certificate request" \
            -c "skip write certificate" \
            -c "skip write certificate verify" \
            -s "skip parse certificate verify" \
            -S "x509_verify_cert() returned" \
            -S "! The certificate is not correctly signed by the trusted CA" \
            -S "! mbedtls_ssl_handshake returned" \
            -C "! mbedtls_ssl_handshake returned" \
            -S "X509 - Certificate verification failed"

run_test    "Authentication: client no cert, server optional" \
            "$P_SRV debug_level=3 auth_mode=optional" \
            "$P_CLI debug_level=3 crt_file=none key_file=none" \
            0 \
            -S "skip write certificate request" \
            -C "skip parse certificate request" \
            -c "got a certificate request" \
            -C "skip write certificate$" \
            -C "got no certificate to send" \
            -S "SSLv3 client has no certificate" \
            -c "skip write certificate verify" \
            -s "skip parse certificate verify" \
            -s "! Certificate was missing" \
            -S "! mbedtls_ssl_handshake returned" \
            -C "! mbedtls_ssl_handshake returned" \
            -S "X509 - Certificate verification failed"

run_test    "Authentication: openssl client no cert, server optional" \
            "$P_SRV debug_level=3 auth_mode=optional" \
            "$O_CLI" \
            0 \
            -S "skip write certificate request" \
            -s "skip parse certificate verify" \
            -s "! Certificate was missing" \
            -S "! mbedtls_ssl_handshake returned" \
            -S "X509 - Certificate verification failed"

run_test    "Authentication: client no cert, openssl server optional" \
            "$O_SRV -verify 10" \
            "$P_CLI debug_level=3 crt_file=none key_file=none" \
            0 \
            -C "skip parse certificate request" \
            -c "got a certificate request" \
            -C "skip write certificate$" \
            -c "skip write certificate verify" \
            -C "! mbedtls_ssl_handshake returned"

run_test    "Authentication: client no cert, openssl server required" \
            "$O_SRV -Verify 10" \
            "$P_CLI debug_level=3 crt_file=none key_file=none" \
            1 \
            -C "skip parse certificate request" \
            -c "got a certificate request" \
            -C "skip write certificate$" \
            -c "skip write certificate verify" \
            -c "! mbedtls_ssl_handshake returned"

requires_config_enabled MBEDTLS_SSL_PROTO_SSL3
run_test    "Authentication: client no cert, ssl3" \
            "$P_SRV debug_level=3 auth_mode=optional force_version=ssl3" \
            "$P_CLI debug_level=3 crt_file=none key_file=none min_version=ssl3" \
            0 \
            -S "skip write certificate request" \
            -C "skip parse certificate request" \
            -c "got a certificate request" \
            -C "skip write certificate$" \
            -c "skip write certificate verify" \
            -c "got no certificate to send" \
            -s "SSLv3 client has no certificate" \
            -s "skip parse certificate verify" \
            -s "! Certificate was missing" \
            -S "! mbedtls_ssl_handshake returned" \
            -C "! mbedtls_ssl_handshake returned" \
            -S "X509 - Certificate verification failed"

# The "max_int chain" tests assume that MAX_INTERMEDIATE_CA is set to its
# default value (8)

MAX_IM_CA='8'
MAX_IM_CA_CONFIG=$( ../scripts/config.pl get MBEDTLS_X509_MAX_INTERMEDIATE_CA)

if [ -n "$MAX_IM_CA_CONFIG" ] && [ "$MAX_IM_CA_CONFIG" -ne "$MAX_IM_CA" ]; then
    printf "The ${CONFIG_H} file contains a value for the configuration of\n"
    printf "MBEDTLS_X509_MAX_INTERMEDIATE_CA that is different from the script’s\n"
    printf "test value of ${MAX_IM_CA}. \n"
    printf "\n"
    printf "The tests assume this value and if it changes, the tests in this\n"
    printf "script should also be adjusted.\n"
    printf "\n"

    exit 1
fi

requires_full_size_output_buffer
run_test    "Authentication: server max_int chain, client default" \
            "$P_SRV crt_file=data_files/dir-maxpath/c09.pem \
                    key_file=data_files/dir-maxpath/09.key" \
            "$P_CLI server_name=CA09 ca_file=data_files/dir-maxpath/00.crt" \
            0 \
            -C "X509 - A fatal error occured"

requires_full_size_output_buffer
run_test    "Authentication: server max_int+1 chain, client default" \
            "$P_SRV crt_file=data_files/dir-maxpath/c10.pem \
                    key_file=data_files/dir-maxpath/10.key" \
            "$P_CLI server_name=CA10 ca_file=data_files/dir-maxpath/00.crt" \
            1 \
            -c "X509 - A fatal error occured"

requires_full_size_output_buffer
run_test    "Authentication: server max_int+1 chain, client optional" \
            "$P_SRV crt_file=data_files/dir-maxpath/c10.pem \
                    key_file=data_files/dir-maxpath/10.key" \
            "$P_CLI server_name=CA10 ca_file=data_files/dir-maxpath/00.crt \
                    auth_mode=optional" \
            1 \
            -c "X509 - A fatal error occured"

requires_full_size_output_buffer
run_test    "Authentication: server max_int+1 chain, client none" \
            "$P_SRV crt_file=data_files/dir-maxpath/c10.pem \
                    key_file=data_files/dir-maxpath/10.key" \
            "$P_CLI server_name=CA10 ca_file=data_files/dir-maxpath/00.crt \
                    auth_mode=none" \
            0 \
            -C "X509 - A fatal error occured"

requires_full_size_output_buffer
run_test    "Authentication: client max_int+1 chain, server default" \
            "$P_SRV ca_file=data_files/dir-maxpath/00.crt" \
            "$P_CLI crt_file=data_files/dir-maxpath/c10.pem \
                    key_file=data_files/dir-maxpath/10.key" \
            0 \
            -S "X509 - A fatal error occured"

requires_full_size_output_buffer
run_test    "Authentication: client max_int+1 chain, server optional" \
            "$P_SRV ca_file=data_files/dir-maxpath/00.crt auth_mode=optional" \
            "$P_CLI crt_file=data_files/dir-maxpath/c10.pem \
                    key_file=data_files/dir-maxpath/10.key" \
            1 \
            -s "X509 - A fatal error occured"

requires_full_size_output_buffer
run_test    "Authentication: client max_int+1 chain, server required" \
            "$P_SRV ca_file=data_files/dir-maxpath/00.crt auth_mode=required" \
            "$P_CLI crt_file=data_files/dir-maxpath/c10.pem \
                    key_file=data_files/dir-maxpath/10.key" \
            1 \
            -s "X509 - A fatal error occured"

requires_full_size_output_buffer
run_test    "Authentication: client max_int chain, server required" \
            "$P_SRV ca_file=data_files/dir-maxpath/00.crt auth_mode=required" \
            "$P_CLI crt_file=data_files/dir-maxpath/c09.pem \
                    key_file=data_files/dir-maxpath/09.key" \
            0 \
            -S "X509 - A fatal error occured"

# Tests for CA list in CertificateRequest messages

run_test    "Authentication: send CA list in CertificateRequest  (default)" \
            "$P_SRV debug_level=3 auth_mode=required" \
            "$P_CLI crt_file=data_files/server6.crt \
             key_file=data_files/server6.key" \
            0 \
            -s "requested DN"

run_test    "Authentication: do not send CA list in CertificateRequest" \
            "$P_SRV debug_level=3 auth_mode=required cert_req_ca_list=0" \
            "$P_CLI crt_file=data_files/server6.crt \
             key_file=data_files/server6.key" \
            0 \
            -S "requested DN"

run_test    "Authentication: send CA list in CertificateRequest, client self signed" \
            "$P_SRV debug_level=3 auth_mode=required cert_req_ca_list=0" \
            "$P_CLI debug_level=3 crt_file=data_files/server5-selfsigned.crt \
             key_file=data_files/server5.key" \
            1 \
            -S "requested DN" \
            -s "x509_verify_cert() returned" \
            -s "! The certificate is not correctly signed by the trusted CA" \
            -s "! mbedtls_ssl_handshake returned" \
            -c "! mbedtls_ssl_handshake returned" \
            -s "X509 - Certificate verification failed"

# Tests for certificate selection based on SHA verson

run_test    "Certificate hash: client TLS 1.2 -> SHA-2" \
            "$P_SRV crt_file=data_files/server5.crt \
                    key_file=data_files/server5.key \
                    crt_file2=data_files/server5-sha1.crt \
                    key_file2=data_files/server5.key" \
            "$P_CLI force_version=tls1_2" \
            0 \
            -c "signed using.*ECDSA with SHA256" \
            -C "signed using.*ECDSA with SHA1"

run_test    "Certificate hash: client TLS 1.1 -> SHA-1" \
            "$P_SRV crt_file=data_files/server5.crt \
                    key_file=data_files/server5.key \
                    crt_file2=data_files/server5-sha1.crt \
                    key_file2=data_files/server5.key" \
            "$P_CLI force_version=tls1_1" \
            0 \
            -C "signed using.*ECDSA with SHA256" \
            -c "signed using.*ECDSA with SHA1"

run_test    "Certificate hash: client TLS 1.0 -> SHA-1" \
            "$P_SRV crt_file=data_files/server5.crt \
                    key_file=data_files/server5.key \
                    crt_file2=data_files/server5-sha1.crt \
                    key_file2=data_files/server5.key" \
            "$P_CLI force_version=tls1" \
            0 \
            -C "signed using.*ECDSA with SHA256" \
            -c "signed using.*ECDSA with SHA1"

run_test    "Certificate hash: client TLS 1.1, no SHA-1 -> SHA-2 (order 1)" \
            "$P_SRV crt_file=data_files/server5.crt \
                    key_file=data_files/server5.key \
                    crt_file2=data_files/server6.crt \
                    key_file2=data_files/server6.key" \
            "$P_CLI force_version=tls1_1" \
            0 \
            -c "serial number.*09" \
            -c "signed using.*ECDSA with SHA256" \
            -C "signed using.*ECDSA with SHA1"

run_test    "Certificate hash: client TLS 1.1, no SHA-1 -> SHA-2 (order 2)" \
            "$P_SRV crt_file=data_files/server6.crt \
                    key_file=data_files/server6.key \
                    crt_file2=data_files/server5.crt \
                    key_file2=data_files/server5.key" \
            "$P_CLI force_version=tls1_1" \
            0 \
            -c "serial number.*0A" \
            -c "signed using.*ECDSA with SHA256" \
            -C "signed using.*ECDSA with SHA1"

# tests for SNI

run_test    "SNI: no SNI callback" \
            "$P_SRV debug_level=3 \
             crt_file=data_files/server5.crt key_file=data_files/server5.key" \
            "$P_CLI server_name=localhost" \
            0 \
            -S "parse ServerName extension" \
            -c "issuer name *: C=NL, O=PolarSSL, CN=Polarssl Test EC CA" \
            -c "subject name *: C=NL, O=PolarSSL, CN=localhost"

run_test    "SNI: matching cert 1" \
            "$P_SRV debug_level=3 \
             crt_file=data_files/server5.crt key_file=data_files/server5.key \
             sni=localhost,data_files/server2.crt,data_files/server2.key,-,-,-,polarssl.example,data_files/server1-nospace.crt,data_files/server1.key,-,-,-" \
            "$P_CLI server_name=localhost" \
            0 \
            -s "parse ServerName extension" \
            -c "issuer name *: C=NL, O=PolarSSL, CN=PolarSSL Test CA" \
            -c "subject name *: C=NL, O=PolarSSL, CN=localhost"

run_test    "SNI: matching cert 2" \
            "$P_SRV debug_level=3 \
             crt_file=data_files/server5.crt key_file=data_files/server5.key \
             sni=localhost,data_files/server2.crt,data_files/server2.key,-,-,-,polarssl.example,data_files/server1-nospace.crt,data_files/server1.key,-,-,-" \
            "$P_CLI server_name=polarssl.example" \
            0 \
            -s "parse ServerName extension" \
            -c "issuer name *: C=NL, O=PolarSSL, CN=PolarSSL Test CA" \
            -c "subject name *: C=NL, O=PolarSSL, CN=polarssl.example"

run_test    "SNI: no matching cert" \
            "$P_SRV debug_level=3 \
             crt_file=data_files/server5.crt key_file=data_files/server5.key \
             sni=localhost,data_files/server2.crt,data_files/server2.key,-,-,-,polarssl.example,data_files/server1-nospace.crt,data_files/server1.key,-,-,-" \
            "$P_CLI server_name=nonesuch.example" \
            1 \
            -s "parse ServerName extension" \
            -s "ssl_sni_wrapper() returned" \
            -s "mbedtls_ssl_handshake returned" \
            -c "mbedtls_ssl_handshake returned" \
            -c "SSL - A fatal alert message was received from our peer"

run_test    "SNI: client auth no override: optional" \
            "$P_SRV debug_level=3 auth_mode=optional \
             crt_file=data_files/server5.crt key_file=data_files/server5.key \
             sni=localhost,data_files/server2.crt,data_files/server2.key,-,-,-" \
            "$P_CLI debug_level=3 server_name=localhost" \
            0 \
            -S "skip write certificate request" \
            -C "skip parse certificate request" \
            -c "got a certificate request" \
            -C "skip write certificate" \
            -C "skip write certificate verify" \
            -S "skip parse certificate verify"

run_test    "SNI: client auth override: none -> optional" \
            "$P_SRV debug_level=3 auth_mode=none \
             crt_file=data_files/server5.crt key_file=data_files/server5.key \
             sni=localhost,data_files/server2.crt,data_files/server2.key,-,-,optional" \
            "$P_CLI debug_level=3 server_name=localhost" \
            0 \
            -S "skip write certificate request" \
            -C "skip parse certificate request" \
            -c "got a certificate request" \
            -C "skip write certificate" \
            -C "skip write certificate verify" \
            -S "skip parse certificate verify"

run_test    "SNI: client auth override: optional -> none" \
            "$P_SRV debug_level=3 auth_mode=optional \
             crt_file=data_files/server5.crt key_file=data_files/server5.key \
             sni=localhost,data_files/server2.crt,data_files/server2.key,-,-,none" \
            "$P_CLI debug_level=3 server_name=localhost" \
            0 \
            -s "skip write certificate request" \
            -C "skip parse certificate request" \
            -c "got no certificate request" \
            -c "skip write certificate" \
            -c "skip write certificate verify" \
            -s "skip parse certificate verify"

run_test    "SNI: CA no override" \
            "$P_SRV debug_level=3 auth_mode=optional \
             crt_file=data_files/server5.crt key_file=data_files/server5.key \
             ca_file=data_files/test-ca.crt \
             sni=localhost,data_files/server2.crt,data_files/server2.key,-,-,required" \
            "$P_CLI debug_level=3 server_name=localhost \
             crt_file=data_files/server6.crt key_file=data_files/server6.key" \
            1 \
            -S "skip write certificate request" \
            -C "skip parse certificate request" \
            -c "got a certificate request" \
            -C "skip write certificate" \
            -C "skip write certificate verify" \
            -S "skip parse certificate verify" \
            -s "x509_verify_cert() returned" \
            -s "! The certificate is not correctly signed by the trusted CA" \
            -S "The certificate has been revoked (is on a CRL)"

run_test    "SNI: CA override" \
            "$P_SRV debug_level=3 auth_mode=optional \
             crt_file=data_files/server5.crt key_file=data_files/server5.key \
             ca_file=data_files/test-ca.crt \
             sni=localhost,data_files/server2.crt,data_files/server2.key,data_files/test-ca2.crt,-,required" \
            "$P_CLI debug_level=3 server_name=localhost \
             crt_file=data_files/server6.crt key_file=data_files/server6.key" \
            0 \
            -S "skip write certificate request" \
            -C "skip parse certificate request" \
            -c "got a certificate request" \
            -C "skip write certificate" \
            -C "skip write certificate verify" \
            -S "skip parse certificate verify" \
            -S "x509_verify_cert() returned" \
            -S "! The certificate is not correctly signed by the trusted CA" \
            -S "The certificate has been revoked (is on a CRL)"

run_test    "SNI: CA override with CRL" \
            "$P_SRV debug_level=3 auth_mode=optional \
             crt_file=data_files/server5.crt key_file=data_files/server5.key \
             ca_file=data_files/test-ca.crt \
             sni=localhost,data_files/server2.crt,data_files/server2.key,data_files/test-ca2.crt,data_files/crl-ec-sha256.pem,required" \
            "$P_CLI debug_level=3 server_name=localhost \
             crt_file=data_files/server6.crt key_file=data_files/server6.key" \
            1 \
            -S "skip write certificate request" \
            -C "skip parse certificate request" \
            -c "got a certificate request" \
            -C "skip write certificate" \
            -C "skip write certificate verify" \
            -S "skip parse certificate verify" \
            -s "x509_verify_cert() returned" \
            -S "! The certificate is not correctly signed by the trusted CA" \
            -s "The certificate has been revoked (is on a CRL)"

# Tests for SNI and DTLS

run_test    "SNI: DTLS, no SNI callback" \
            "$P_SRV debug_level=3 dtls=1 \
             crt_file=data_files/server5.crt key_file=data_files/server5.key" \
            "$P_CLI server_name=localhost dtls=1" \
            0 \
            -S "parse ServerName extension" \
            -c "issuer name *: C=NL, O=PolarSSL, CN=Polarssl Test EC CA" \
            -c "subject name *: C=NL, O=PolarSSL, CN=localhost"

run_test    "SNI: DTLS, matching cert 1" \
            "$P_SRV debug_level=3 dtls=1 \
             crt_file=data_files/server5.crt key_file=data_files/server5.key \
             sni=localhost,data_files/server2.crt,data_files/server2.key,-,-,-,polarssl.example,data_files/server1-nospace.crt,data_files/server1.key,-,-,-" \
            "$P_CLI server_name=localhost dtls=1" \
            0 \
            -s "parse ServerName extension" \
            -c "issuer name *: C=NL, O=PolarSSL, CN=PolarSSL Test CA" \
            -c "subject name *: C=NL, O=PolarSSL, CN=localhost"

run_test    "SNI: DTLS, matching cert 2" \
            "$P_SRV debug_level=3 dtls=1 \
             crt_file=data_files/server5.crt key_file=data_files/server5.key \
             sni=localhost,data_files/server2.crt,data_files/server2.key,-,-,-,polarssl.example,data_files/server1-nospace.crt,data_files/server1.key,-,-,-" \
            "$P_CLI server_name=polarssl.example dtls=1" \
            0 \
            -s "parse ServerName extension" \
            -c "issuer name *: C=NL, O=PolarSSL, CN=PolarSSL Test CA" \
            -c "subject name *: C=NL, O=PolarSSL, CN=polarssl.example"

run_test    "SNI: DTLS, no matching cert" \
            "$P_SRV debug_level=3 dtls=1 \
             crt_file=data_files/server5.crt key_file=data_files/server5.key \
             sni=localhost,data_files/server2.crt,data_files/server2.key,-,-,-,polarssl.example,data_files/server1-nospace.crt,data_files/server1.key,-,-,-" \
            "$P_CLI server_name=nonesuch.example dtls=1" \
            1 \
            -s "parse ServerName extension" \
            -s "ssl_sni_wrapper() returned" \
            -s "mbedtls_ssl_handshake returned" \
            -c "mbedtls_ssl_handshake returned" \
            -c "SSL - A fatal alert message was received from our peer"

run_test    "SNI: DTLS, client auth no override: optional" \
            "$P_SRV debug_level=3 auth_mode=optional dtls=1 \
             crt_file=data_files/server5.crt key_file=data_files/server5.key \
             sni=localhost,data_files/server2.crt,data_files/server2.key,-,-,-" \
            "$P_CLI debug_level=3 server_name=localhost dtls=1" \
            0 \
            -S "skip write certificate request" \
            -C "skip parse certificate request" \
            -c "got a certificate request" \
            -C "skip write certificate" \
            -C "skip write certificate verify" \
            -S "skip parse certificate verify"

run_test    "SNI: DTLS, client auth override: none -> optional" \
            "$P_SRV debug_level=3 auth_mode=none dtls=1 \
             crt_file=data_files/server5.crt key_file=data_files/server5.key \
             sni=localhost,data_files/server2.crt,data_files/server2.key,-,-,optional" \
            "$P_CLI debug_level=3 server_name=localhost dtls=1" \
            0 \
            -S "skip write certificate request" \
            -C "skip parse certificate request" \
            -c "got a certificate request" \
            -C "skip write certificate" \
            -C "skip write certificate verify" \
            -S "skip parse certificate verify"

run_test    "SNI: DTLS, client auth override: optional -> none" \
            "$P_SRV debug_level=3 auth_mode=optional dtls=1 \
             crt_file=data_files/server5.crt key_file=data_files/server5.key \
             sni=localhost,data_files/server2.crt,data_files/server2.key,-,-,none" \
            "$P_CLI debug_level=3 server_name=localhost dtls=1" \
            0 \
            -s "skip write certificate request" \
            -C "skip parse certificate request" \
            -c "got no certificate request" \
            -c "skip write certificate" \
            -c "skip write certificate verify" \
            -s "skip parse certificate verify"

run_test    "SNI: DTLS, CA no override" \
            "$P_SRV debug_level=3 auth_mode=optional dtls=1 \
             crt_file=data_files/server5.crt key_file=data_files/server5.key \
             ca_file=data_files/test-ca.crt \
             sni=localhost,data_files/server2.crt,data_files/server2.key,-,-,required" \
            "$P_CLI debug_level=3 server_name=localhost dtls=1 \
             crt_file=data_files/server6.crt key_file=data_files/server6.key" \
            1 \
            -S "skip write certificate request" \
            -C "skip parse certificate request" \
            -c "got a certificate request" \
            -C "skip write certificate" \
            -C "skip write certificate verify" \
            -S "skip parse certificate verify" \
            -s "x509_verify_cert() returned" \
            -s "! The certificate is not correctly signed by the trusted CA" \
            -S "The certificate has been revoked (is on a CRL)"

run_test    "SNI: DTLS, CA override" \
            "$P_SRV debug_level=3 auth_mode=optional dtls=1 \
             crt_file=data_files/server5.crt key_file=data_files/server5.key \
             ca_file=data_files/test-ca.crt \
             sni=localhost,data_files/server2.crt,data_files/server2.key,data_files/test-ca2.crt,-,required" \
            "$P_CLI debug_level=3 server_name=localhost dtls=1 \
             crt_file=data_files/server6.crt key_file=data_files/server6.key" \
            0 \
            -S "skip write certificate request" \
            -C "skip parse certificate request" \
            -c "got a certificate request" \
            -C "skip write certificate" \
            -C "skip write certificate verify" \
            -S "skip parse certificate verify" \
            -S "x509_verify_cert() returned" \
            -S "! The certificate is not correctly signed by the trusted CA" \
            -S "The certificate has been revoked (is on a CRL)"

run_test    "SNI: DTLS, CA override with CRL" \
            "$P_SRV debug_level=3 auth_mode=optional \
             crt_file=data_files/server5.crt key_file=data_files/server5.key dtls=1 \
             ca_file=data_files/test-ca.crt \
             sni=localhost,data_files/server2.crt,data_files/server2.key,data_files/test-ca2.crt,data_files/crl-ec-sha256.pem,required" \
            "$P_CLI debug_level=3 server_name=localhost dtls=1 \
             crt_file=data_files/server6.crt key_file=data_files/server6.key" \
            1 \
            -S "skip write certificate request" \
            -C "skip parse certificate request" \
            -c "got a certificate request" \
            -C "skip write certificate" \
            -C "skip write certificate verify" \
            -S "skip parse certificate verify" \
            -s "x509_verify_cert() returned" \
            -S "! The certificate is not correctly signed by the trusted CA" \
            -s "The certificate has been revoked (is on a CRL)"

# Tests for non-blocking I/O: exercise a variety of handshake flows

run_test    "Non-blocking I/O: basic handshake" \
            "$P_SRV nbio=2 tickets=0 auth_mode=none" \
            "$P_CLI nbio=2 tickets=0" \
            0 \
            -S "mbedtls_ssl_handshake returned" \
            -C "mbedtls_ssl_handshake returned" \
            -c "Read from server: .* bytes read"

run_test    "Non-blocking I/O: client auth" \
            "$P_SRV nbio=2 tickets=0 auth_mode=required" \
            "$P_CLI nbio=2 tickets=0" \
            0 \
            -S "mbedtls_ssl_handshake returned" \
            -C "mbedtls_ssl_handshake returned" \
            -c "Read from server: .* bytes read"

run_test    "Non-blocking I/O: ticket" \
            "$P_SRV nbio=2 tickets=1 auth_mode=none" \
            "$P_CLI nbio=2 tickets=1" \
            0 \
            -S "mbedtls_ssl_handshake returned" \
            -C "mbedtls_ssl_handshake returned" \
            -c "Read from server: .* bytes read"

run_test    "Non-blocking I/O: ticket + client auth" \
            "$P_SRV nbio=2 tickets=1 auth_mode=required" \
            "$P_CLI nbio=2 tickets=1" \
            0 \
            -S "mbedtls_ssl_handshake returned" \
            -C "mbedtls_ssl_handshake returned" \
            -c "Read from server: .* bytes read"

run_test    "Non-blocking I/O: ticket + client auth + resume" \
            "$P_SRV nbio=2 tickets=1 auth_mode=required" \
            "$P_CLI nbio=2 tickets=1 reconnect=1" \
            0 \
            -S "mbedtls_ssl_handshake returned" \
            -C "mbedtls_ssl_handshake returned" \
            -c "Read from server: .* bytes read"

run_test    "Non-blocking I/O: ticket + resume" \
            "$P_SRV nbio=2 tickets=1 auth_mode=none" \
            "$P_CLI nbio=2 tickets=1 reconnect=1" \
            0 \
            -S "mbedtls_ssl_handshake returned" \
            -C "mbedtls_ssl_handshake returned" \
            -c "Read from server: .* bytes read"

run_test    "Non-blocking I/O: session-id resume" \
            "$P_SRV nbio=2 tickets=0 auth_mode=none" \
            "$P_CLI nbio=2 tickets=0 reconnect=1" \
            0 \
            -S "mbedtls_ssl_handshake returned" \
            -C "mbedtls_ssl_handshake returned" \
            -c "Read from server: .* bytes read"

# Tests for event-driven I/O: exercise a variety of handshake flows

run_test    "Event-driven I/O: basic handshake" \
            "$P_SRV event=1 tickets=0 auth_mode=none" \
            "$P_CLI event=1 tickets=0" \
            0 \
            -S "mbedtls_ssl_handshake returned" \
            -C "mbedtls_ssl_handshake returned" \
            -c "Read from server: .* bytes read"

run_test    "Event-driven I/O: client auth" \
            "$P_SRV event=1 tickets=0 auth_mode=required" \
            "$P_CLI event=1 tickets=0" \
            0 \
            -S "mbedtls_ssl_handshake returned" \
            -C "mbedtls_ssl_handshake returned" \
            -c "Read from server: .* bytes read"

run_test    "Event-driven I/O: ticket" \
            "$P_SRV event=1 tickets=1 auth_mode=none" \
            "$P_CLI event=1 tickets=1" \
            0 \
            -S "mbedtls_ssl_handshake returned" \
            -C "mbedtls_ssl_handshake returned" \
            -c "Read from server: .* bytes read"

run_test    "Event-driven I/O: ticket + client auth" \
            "$P_SRV event=1 tickets=1 auth_mode=required" \
            "$P_CLI event=1 tickets=1" \
            0 \
            -S "mbedtls_ssl_handshake returned" \
            -C "mbedtls_ssl_handshake returned" \
            -c "Read from server: .* bytes read"

run_test    "Event-driven I/O: ticket + client auth + resume" \
            "$P_SRV event=1 tickets=1 auth_mode=required" \
            "$P_CLI event=1 tickets=1 reconnect=1" \
            0 \
            -S "mbedtls_ssl_handshake returned" \
            -C "mbedtls_ssl_handshake returned" \
            -c "Read from server: .* bytes read"

run_test    "Event-driven I/O: ticket + resume" \
            "$P_SRV event=1 tickets=1 auth_mode=none" \
            "$P_CLI event=1 tickets=1 reconnect=1" \
            0 \
            -S "mbedtls_ssl_handshake returned" \
            -C "mbedtls_ssl_handshake returned" \
            -c "Read from server: .* bytes read"

run_test    "Event-driven I/O: session-id resume" \
            "$P_SRV event=1 tickets=0 auth_mode=none" \
            "$P_CLI event=1 tickets=0 reconnect=1" \
            0 \
            -S "mbedtls_ssl_handshake returned" \
            -C "mbedtls_ssl_handshake returned" \
            -c "Read from server: .* bytes read"

run_test    "Event-driven I/O, DTLS: basic handshake" \
            "$P_SRV dtls=1 event=1 tickets=0 auth_mode=none" \
            "$P_CLI dtls=1 event=1 tickets=0" \
            0 \
            -c "Read from server: .* bytes read"

run_test    "Event-driven I/O, DTLS: client auth" \
            "$P_SRV dtls=1 event=1 tickets=0 auth_mode=required" \
            "$P_CLI dtls=1 event=1 tickets=0" \
            0 \
            -c "Read from server: .* bytes read"

run_test    "Event-driven I/O, DTLS: ticket" \
            "$P_SRV dtls=1 event=1 tickets=1 auth_mode=none" \
            "$P_CLI dtls=1 event=1 tickets=1" \
            0 \
            -c "Read from server: .* bytes read"

run_test    "Event-driven I/O, DTLS: ticket + client auth" \
            "$P_SRV dtls=1 event=1 tickets=1 auth_mode=required" \
            "$P_CLI dtls=1 event=1 tickets=1" \
            0 \
            -c "Read from server: .* bytes read"

run_test    "Event-driven I/O, DTLS: ticket + client auth + resume" \
            "$P_SRV dtls=1 event=1 tickets=1 auth_mode=required" \
            "$P_CLI dtls=1 event=1 tickets=1 reconnect=1" \
            0 \
            -c "Read from server: .* bytes read"

run_test    "Event-driven I/O, DTLS: ticket + resume" \
            "$P_SRV dtls=1 event=1 tickets=1 auth_mode=none" \
            "$P_CLI dtls=1 event=1 tickets=1 reconnect=1" \
            0 \
            -c "Read from server: .* bytes read"

run_test    "Event-driven I/O, DTLS: session-id resume" \
            "$P_SRV dtls=1 event=1 tickets=0 auth_mode=none" \
            "$P_CLI dtls=1 event=1 tickets=0 reconnect=1" \
            0 \
            -c "Read from server: .* bytes read"

# This test demonstrates the need for the mbedtls_ssl_check_pending function.
# During session resumption, the client will send its ApplicationData record
# within the same datagram as the Finished messages. In this situation, the
# server MUST NOT idle on the underlying transport after handshake completion,
# because the ApplicationData request has already been queued internally.
run_test    "Event-driven I/O, DTLS: session-id resume, UDP packing" \
            -p "$P_PXY pack=50" \
            "$P_SRV dtls=1 event=1 tickets=0 auth_mode=required" \
            "$P_CLI dtls=1 event=1 tickets=0 reconnect=1" \
            0 \
            -c "Read from server: .* bytes read"

# Tests for version negotiation

run_test    "Version check: all -> 1.2" \
            "$P_SRV" \
            "$P_CLI" \
            0 \
            -S "mbedtls_ssl_handshake returned" \
            -C "mbedtls_ssl_handshake returned" \
            -s "Protocol is TLSv1.2" \
            -c "Protocol is TLSv1.2"

run_test    "Version check: cli max 1.1 -> 1.1" \
            "$P_SRV" \
            "$P_CLI max_version=tls1_1" \
            0 \
            -S "mbedtls_ssl_handshake returned" \
            -C "mbedtls_ssl_handshake returned" \
            -s "Protocol is TLSv1.1" \
            -c "Protocol is TLSv1.1"

run_test    "Version check: srv max 1.1 -> 1.1" \
            "$P_SRV max_version=tls1_1" \
            "$P_CLI" \
            0 \
            -S "mbedtls_ssl_handshake returned" \
            -C "mbedtls_ssl_handshake returned" \
            -s "Protocol is TLSv1.1" \
            -c "Protocol is TLSv1.1"

run_test    "Version check: cli+srv max 1.1 -> 1.1" \
            "$P_SRV max_version=tls1_1" \
            "$P_CLI max_version=tls1_1" \
            0 \
            -S "mbedtls_ssl_handshake returned" \
            -C "mbedtls_ssl_handshake returned" \
            -s "Protocol is TLSv1.1" \
            -c "Protocol is TLSv1.1"

run_test    "Version check: cli max 1.1, srv min 1.1 -> 1.1" \
            "$P_SRV min_version=tls1_1" \
            "$P_CLI max_version=tls1_1" \
            0 \
            -S "mbedtls_ssl_handshake returned" \
            -C "mbedtls_ssl_handshake returned" \
            -s "Protocol is TLSv1.1" \
            -c "Protocol is TLSv1.1"

run_test    "Version check: cli min 1.1, srv max 1.1 -> 1.1" \
            "$P_SRV max_version=tls1_1" \
            "$P_CLI min_version=tls1_1" \
            0 \
            -S "mbedtls_ssl_handshake returned" \
            -C "mbedtls_ssl_handshake returned" \
            -s "Protocol is TLSv1.1" \
            -c "Protocol is TLSv1.1"

run_test    "Version check: cli min 1.2, srv max 1.1 -> fail" \
            "$P_SRV max_version=tls1_1" \
            "$P_CLI min_version=tls1_2" \
            1 \
            -s "mbedtls_ssl_handshake returned" \
            -c "mbedtls_ssl_handshake returned" \
            -c "SSL - Handshake protocol not within min/max boundaries"

run_test    "Version check: srv min 1.2, cli max 1.1 -> fail" \
            "$P_SRV min_version=tls1_2" \
            "$P_CLI max_version=tls1_1" \
            1 \
            -s "mbedtls_ssl_handshake returned" \
            -c "mbedtls_ssl_handshake returned" \
            -s "SSL - Handshake protocol not within min/max boundaries"

# Tests for ALPN extension

run_test    "ALPN: none" \
            "$P_SRV debug_level=3" \
            "$P_CLI debug_level=3" \
            0 \
            -C "client hello, adding alpn extension" \
            -S "found alpn extension" \
            -C "got an alert message, type: \\[2:120]" \
            -S "server hello, adding alpn extension" \
            -C "found alpn extension " \
            -C "Application Layer Protocol is" \
            -S "Application Layer Protocol is"

run_test    "ALPN: client only" \
            "$P_SRV debug_level=3" \
            "$P_CLI debug_level=3 alpn=abc,1234" \
            0 \
            -c "client hello, adding alpn extension" \
            -s "found alpn extension" \
            -C "got an alert message, type: \\[2:120]" \
            -S "server hello, adding alpn extension" \
            -C "found alpn extension " \
            -c "Application Layer Protocol is (none)" \
            -S "Application Layer Protocol is"

run_test    "ALPN: server only" \
            "$P_SRV debug_level=3 alpn=abc,1234" \
            "$P_CLI debug_level=3" \
            0 \
            -C "client hello, adding alpn extension" \
            -S "found alpn extension" \
            -C "got an alert message, type: \\[2:120]" \
            -S "server hello, adding alpn extension" \
            -C "found alpn extension " \
            -C "Application Layer Protocol is" \
            -s "Application Layer Protocol is (none)"

run_test    "ALPN: both, common cli1-srv1" \
            "$P_SRV debug_level=3 alpn=abc,1234" \
            "$P_CLI debug_level=3 alpn=abc,1234" \
            0 \
            -c "client hello, adding alpn extension" \
            -s "found alpn extension" \
            -C "got an alert message, type: \\[2:120]" \
            -s "server hello, adding alpn extension" \
            -c "found alpn extension" \
            -c "Application Layer Protocol is abc" \
            -s "Application Layer Protocol is abc"

run_test    "ALPN: both, common cli2-srv1" \
            "$P_SRV debug_level=3 alpn=abc,1234" \
            "$P_CLI debug_level=3 alpn=1234,abc" \
            0 \
            -c "client hello, adding alpn extension" \
            -s "found alpn extension" \
            -C "got an alert message, type: \\[2:120]" \
            -s "server hello, adding alpn extension" \
            -c "found alpn extension" \
            -c "Application Layer Protocol is abc" \
            -s "Application Layer Protocol is abc"

run_test    "ALPN: both, common cli1-srv2" \
            "$P_SRV debug_level=3 alpn=abc,1234" \
            "$P_CLI debug_level=3 alpn=1234,abcde" \
            0 \
            -c "client hello, adding alpn extension" \
            -s "found alpn extension" \
            -C "got an alert message, type: \\[2:120]" \
            -s "server hello, adding alpn extension" \
            -c "found alpn extension" \
            -c "Application Layer Protocol is 1234" \
            -s "Application Layer Protocol is 1234"

run_test    "ALPN: both, no common" \
            "$P_SRV debug_level=3 alpn=abc,123" \
            "$P_CLI debug_level=3 alpn=1234,abcde" \
            1 \
            -c "client hello, adding alpn extension" \
            -s "found alpn extension" \
            -c "got an alert message, type: \\[2:120]" \
            -S "server hello, adding alpn extension" \
            -C "found alpn extension" \
            -C "Application Layer Protocol is 1234" \
            -S "Application Layer Protocol is 1234"


# Tests for keyUsage in leaf certificates, part 1:
# server-side certificate/suite selection

run_test    "keyUsage srv: RSA, digitalSignature -> (EC)DHE-RSA" \
            "$P_SRV key_file=data_files/server2.key \
             crt_file=data_files/server2.ku-ds.crt" \
            "$P_CLI" \
            0 \
            -c "Ciphersuite is TLS-[EC]*DHE-RSA-WITH-"


run_test    "keyUsage srv: RSA, keyEncipherment -> RSA" \
            "$P_SRV key_file=data_files/server2.key \
             crt_file=data_files/server2.ku-ke.crt" \
            "$P_CLI" \
            0 \
            -c "Ciphersuite is TLS-RSA-WITH-"

run_test    "keyUsage srv: RSA, keyAgreement -> fail" \
            "$P_SRV key_file=data_files/server2.key \
             crt_file=data_files/server2.ku-ka.crt" \
            "$P_CLI" \
            1 \
            -C "Ciphersuite is "

run_test    "keyUsage srv: ECDSA, digitalSignature -> ECDHE-ECDSA" \
            "$P_SRV key_file=data_files/server5.key \
             crt_file=data_files/server5.ku-ds.crt" \
            "$P_CLI" \
            0 \
            -c "Ciphersuite is TLS-ECDHE-ECDSA-WITH-"


run_test    "keyUsage srv: ECDSA, keyAgreement -> ECDH-" \
            "$P_SRV key_file=data_files/server5.key \
             crt_file=data_files/server5.ku-ka.crt" \
            "$P_CLI" \
            0 \
            -c "Ciphersuite is TLS-ECDH-"

run_test    "keyUsage srv: ECDSA, keyEncipherment -> fail" \
            "$P_SRV key_file=data_files/server5.key \
             crt_file=data_files/server5.ku-ke.crt" \
            "$P_CLI" \
            1 \
            -C "Ciphersuite is "

# Tests for keyUsage in leaf certificates, part 2:
# client-side checking of server cert

run_test    "keyUsage cli: DigitalSignature+KeyEncipherment, RSA: OK" \
            "$O_SRV -key data_files/server2.key \
             -cert data_files/server2.ku-ds_ke.crt" \
            "$P_CLI debug_level=1 \
             force_ciphersuite=TLS-RSA-WITH-AES-128-CBC-SHA" \
            0 \
            -C "bad certificate (usage extensions)" \
            -C "Processing of the Certificate handshake message failed" \
            -c "Ciphersuite is TLS-"

run_test    "keyUsage cli: DigitalSignature+KeyEncipherment, DHE-RSA: OK" \
            "$O_SRV -key data_files/server2.key \
             -cert data_files/server2.ku-ds_ke.crt" \
            "$P_CLI debug_level=1 \
             force_ciphersuite=TLS-DHE-RSA-WITH-AES-128-CBC-SHA" \
            0 \
            -C "bad certificate (usage extensions)" \
            -C "Processing of the Certificate handshake message failed" \
            -c "Ciphersuite is TLS-"

run_test    "keyUsage cli: KeyEncipherment, RSA: OK" \
            "$O_SRV -key data_files/server2.key \
             -cert data_files/server2.ku-ke.crt" \
            "$P_CLI debug_level=1 \
             force_ciphersuite=TLS-RSA-WITH-AES-128-CBC-SHA" \
            0 \
            -C "bad certificate (usage extensions)" \
            -C "Processing of the Certificate handshake message failed" \
            -c "Ciphersuite is TLS-"

run_test    "keyUsage cli: KeyEncipherment, DHE-RSA: fail" \
            "$O_SRV -key data_files/server2.key \
             -cert data_files/server2.ku-ke.crt" \
            "$P_CLI debug_level=1 \
             force_ciphersuite=TLS-DHE-RSA-WITH-AES-128-CBC-SHA" \
            1 \
            -c "bad certificate (usage extensions)" \
            -c "Processing of the Certificate handshake message failed" \
            -C "Ciphersuite is TLS-"

run_test    "keyUsage cli: KeyEncipherment, DHE-RSA: fail, soft" \
            "$O_SRV -key data_files/server2.key \
             -cert data_files/server2.ku-ke.crt" \
            "$P_CLI debug_level=1 auth_mode=optional \
             force_ciphersuite=TLS-DHE-RSA-WITH-AES-128-CBC-SHA" \
            0 \
            -c "bad certificate (usage extensions)" \
            -C "Processing of the Certificate handshake message failed" \
            -c "Ciphersuite is TLS-" \
            -c "! Usage does not match the keyUsage extension"

run_test    "keyUsage cli: DigitalSignature, DHE-RSA: OK" \
            "$O_SRV -key data_files/server2.key \
             -cert data_files/server2.ku-ds.crt" \
            "$P_CLI debug_level=1 \
             force_ciphersuite=TLS-DHE-RSA-WITH-AES-128-CBC-SHA" \
            0 \
            -C "bad certificate (usage extensions)" \
            -C "Processing of the Certificate handshake message failed" \
            -c "Ciphersuite is TLS-"

run_test    "keyUsage cli: DigitalSignature, RSA: fail" \
            "$O_SRV -key data_files/server2.key \
             -cert data_files/server2.ku-ds.crt" \
            "$P_CLI debug_level=1 \
             force_ciphersuite=TLS-RSA-WITH-AES-128-CBC-SHA" \
            1 \
            -c "bad certificate (usage extensions)" \
            -c "Processing of the Certificate handshake message failed" \
            -C "Ciphersuite is TLS-"

run_test    "keyUsage cli: DigitalSignature, RSA: fail, soft" \
            "$O_SRV -key data_files/server2.key \
             -cert data_files/server2.ku-ds.crt" \
            "$P_CLI debug_level=1 auth_mode=optional \
             force_ciphersuite=TLS-RSA-WITH-AES-128-CBC-SHA" \
            0 \
            -c "bad certificate (usage extensions)" \
            -C "Processing of the Certificate handshake message failed" \
            -c "Ciphersuite is TLS-" \
            -c "! Usage does not match the keyUsage extension"

# Tests for keyUsage in leaf certificates, part 3:
# server-side checking of client cert

run_test    "keyUsage cli-auth: RSA, DigitalSignature: OK" \
            "$P_SRV debug_level=1 auth_mode=optional" \
            "$O_CLI -key data_files/server2.key \
             -cert data_files/server2.ku-ds.crt" \
            0 \
            -S "bad certificate (usage extensions)" \
            -S "Processing of the Certificate handshake message failed"

run_test    "keyUsage cli-auth: RSA, KeyEncipherment: fail (soft)" \
            "$P_SRV debug_level=1 auth_mode=optional" \
            "$O_CLI -key data_files/server2.key \
             -cert data_files/server2.ku-ke.crt" \
            0 \
            -s "bad certificate (usage extensions)" \
            -S "Processing of the Certificate handshake message failed"

run_test    "keyUsage cli-auth: RSA, KeyEncipherment: fail (hard)" \
            "$P_SRV debug_level=1 auth_mode=required" \
            "$O_CLI -key data_files/server2.key \
             -cert data_files/server2.ku-ke.crt" \
            1 \
            -s "bad certificate (usage extensions)" \
            -s "Processing of the Certificate handshake message failed"

run_test    "keyUsage cli-auth: ECDSA, DigitalSignature: OK" \
            "$P_SRV debug_level=1 auth_mode=optional" \
            "$O_CLI -key data_files/server5.key \
             -cert data_files/server5.ku-ds.crt" \
            0 \
            -S "bad certificate (usage extensions)" \
            -S "Processing of the Certificate handshake message failed"

run_test    "keyUsage cli-auth: ECDSA, KeyAgreement: fail (soft)" \
            "$P_SRV debug_level=1 auth_mode=optional" \
            "$O_CLI -key data_files/server5.key \
             -cert data_files/server5.ku-ka.crt" \
            0 \
            -s "bad certificate (usage extensions)" \
            -S "Processing of the Certificate handshake message failed"

# Tests for extendedKeyUsage, part 1: server-side certificate/suite selection

run_test    "extKeyUsage srv: serverAuth -> OK" \
            "$P_SRV key_file=data_files/server5.key \
             crt_file=data_files/server5.eku-srv.crt" \
            "$P_CLI" \
            0

run_test    "extKeyUsage srv: serverAuth,clientAuth -> OK" \
            "$P_SRV key_file=data_files/server5.key \
             crt_file=data_files/server5.eku-srv.crt" \
            "$P_CLI" \
            0

run_test    "extKeyUsage srv: codeSign,anyEKU -> OK" \
            "$P_SRV key_file=data_files/server5.key \
             crt_file=data_files/server5.eku-cs_any.crt" \
            "$P_CLI" \
            0

run_test    "extKeyUsage srv: codeSign -> fail" \
            "$P_SRV key_file=data_files/server5.key \
             crt_file=data_files/server5.eku-cli.crt" \
            "$P_CLI" \
            1

# Tests for extendedKeyUsage, part 2: client-side checking of server cert

run_test    "extKeyUsage cli: serverAuth -> OK" \
            "$O_SRV -key data_files/server5.key \
             -cert data_files/server5.eku-srv.crt" \
            "$P_CLI debug_level=1" \
            0 \
            -C "bad certificate (usage extensions)" \
            -C "Processing of the Certificate handshake message failed" \
            -c "Ciphersuite is TLS-"

run_test    "extKeyUsage cli: serverAuth,clientAuth -> OK" \
            "$O_SRV -key data_files/server5.key \
             -cert data_files/server5.eku-srv_cli.crt" \
            "$P_CLI debug_level=1" \
            0 \
            -C "bad certificate (usage extensions)" \
            -C "Processing of the Certificate handshake message failed" \
            -c "Ciphersuite is TLS-"

run_test    "extKeyUsage cli: codeSign,anyEKU -> OK" \
            "$O_SRV -key data_files/server5.key \
             -cert data_files/server5.eku-cs_any.crt" \
            "$P_CLI debug_level=1" \
            0 \
            -C "bad certificate (usage extensions)" \
            -C "Processing of the Certificate handshake message failed" \
            -c "Ciphersuite is TLS-"

run_test    "extKeyUsage cli: codeSign -> fail" \
            "$O_SRV -key data_files/server5.key \
             -cert data_files/server5.eku-cs.crt" \
            "$P_CLI debug_level=1" \
            1 \
            -c "bad certificate (usage extensions)" \
            -c "Processing of the Certificate handshake message failed" \
            -C "Ciphersuite is TLS-"

# Tests for extendedKeyUsage, part 3: server-side checking of client cert

run_test    "extKeyUsage cli-auth: clientAuth -> OK" \
            "$P_SRV debug_level=1 auth_mode=optional" \
            "$O_CLI -key data_files/server5.key \
             -cert data_files/server5.eku-cli.crt" \
            0 \
            -S "bad certificate (usage extensions)" \
            -S "Processing of the Certificate handshake message failed"

run_test    "extKeyUsage cli-auth: serverAuth,clientAuth -> OK" \
            "$P_SRV debug_level=1 auth_mode=optional" \
            "$O_CLI -key data_files/server5.key \
             -cert data_files/server5.eku-srv_cli.crt" \
            0 \
            -S "bad certificate (usage extensions)" \
            -S "Processing of the Certificate handshake message failed"

run_test    "extKeyUsage cli-auth: codeSign,anyEKU -> OK" \
            "$P_SRV debug_level=1 auth_mode=optional" \
            "$O_CLI -key data_files/server5.key \
             -cert data_files/server5.eku-cs_any.crt" \
            0 \
            -S "bad certificate (usage extensions)" \
            -S "Processing of the Certificate handshake message failed"

run_test    "extKeyUsage cli-auth: codeSign -> fail (soft)" \
            "$P_SRV debug_level=1 auth_mode=optional" \
            "$O_CLI -key data_files/server5.key \
             -cert data_files/server5.eku-cs.crt" \
            0 \
            -s "bad certificate (usage extensions)" \
            -S "Processing of the Certificate handshake message failed"

run_test    "extKeyUsage cli-auth: codeSign -> fail (hard)" \
            "$P_SRV debug_level=1 auth_mode=required" \
            "$O_CLI -key data_files/server5.key \
             -cert data_files/server5.eku-cs.crt" \
            1 \
            -s "bad certificate (usage extensions)" \
            -s "Processing of the Certificate handshake message failed"

# Tests for DHM parameters loading

run_test    "DHM parameters: reference" \
            "$P_SRV" \
            "$P_CLI force_ciphersuite=TLS-DHE-RSA-WITH-AES-128-CBC-SHA \
                    debug_level=3" \
            0 \
            -c "value of 'DHM: P ' (2048 bits)" \
            -c "value of 'DHM: G ' (2 bits)"

run_test    "DHM parameters: other parameters" \
            "$P_SRV dhm_file=data_files/dhparams.pem" \
            "$P_CLI force_ciphersuite=TLS-DHE-RSA-WITH-AES-128-CBC-SHA \
                    debug_level=3" \
            0 \
            -c "value of 'DHM: P ' (1024 bits)" \
            -c "value of 'DHM: G ' (2 bits)"

# Tests for DHM client-side size checking

run_test    "DHM size: server default, client default, OK" \
            "$P_SRV" \
            "$P_CLI force_ciphersuite=TLS-DHE-RSA-WITH-AES-128-CBC-SHA \
                    debug_level=1" \
            0 \
            -C "DHM prime too short:"

run_test    "DHM size: server default, client 2048, OK" \
            "$P_SRV" \
            "$P_CLI force_ciphersuite=TLS-DHE-RSA-WITH-AES-128-CBC-SHA \
                    debug_level=1 dhmlen=2048" \
            0 \
            -C "DHM prime too short:"

run_test    "DHM size: server 1024, client default, OK" \
            "$P_SRV dhm_file=data_files/dhparams.pem" \
            "$P_CLI force_ciphersuite=TLS-DHE-RSA-WITH-AES-128-CBC-SHA \
                    debug_level=1" \
            0 \
            -C "DHM prime too short:"

run_test    "DHM size: server 1000, client default, rejected" \
            "$P_SRV dhm_file=data_files/dh.1000.pem" \
            "$P_CLI force_ciphersuite=TLS-DHE-RSA-WITH-AES-128-CBC-SHA \
                    debug_level=1" \
            1 \
            -c "DHM prime too short:"

run_test    "DHM size: server default, client 2049, rejected" \
            "$P_SRV" \
            "$P_CLI force_ciphersuite=TLS-DHE-RSA-WITH-AES-128-CBC-SHA \
                    debug_level=1 dhmlen=2049" \
            1 \
            -c "DHM prime too short:"

# Tests for PSK callback

run_test    "PSK callback: psk, no callback" \
            "$P_SRV psk=abc123 psk_identity=foo" \
            "$P_CLI force_ciphersuite=TLS-PSK-WITH-AES-128-CBC-SHA \
            psk_identity=foo psk=abc123" \
            0 \
            -S "SSL - None of the common ciphersuites is usable" \
            -S "SSL - Unknown identity received" \
            -S "SSL - Verification of the message MAC failed"

run_test    "PSK callback: no psk, no callback" \
            "$P_SRV" \
            "$P_CLI force_ciphersuite=TLS-PSK-WITH-AES-128-CBC-SHA \
            psk_identity=foo psk=abc123" \
            1 \
            -s "SSL - None of the common ciphersuites is usable" \
            -S "SSL - Unknown identity received" \
            -S "SSL - Verification of the message MAC failed"

run_test    "PSK callback: callback overrides other settings" \
            "$P_SRV psk=abc123 psk_identity=foo psk_list=abc,dead,def,beef" \
            "$P_CLI force_ciphersuite=TLS-PSK-WITH-AES-128-CBC-SHA \
            psk_identity=foo psk=abc123" \
            1 \
            -S "SSL - None of the common ciphersuites is usable" \
            -s "SSL - Unknown identity received" \
            -S "SSL - Verification of the message MAC failed"

run_test    "PSK callback: first id matches" \
            "$P_SRV psk_list=abc,dead,def,beef" \
            "$P_CLI force_ciphersuite=TLS-PSK-WITH-AES-128-CBC-SHA \
            psk_identity=abc psk=dead" \
            0 \
            -S "SSL - None of the common ciphersuites is usable" \
            -S "SSL - Unknown identity received" \
            -S "SSL - Verification of the message MAC failed"

run_test    "PSK callback: second id matches" \
            "$P_SRV psk_list=abc,dead,def,beef" \
            "$P_CLI force_ciphersuite=TLS-PSK-WITH-AES-128-CBC-SHA \
            psk_identity=def psk=beef" \
            0 \
            -S "SSL - None of the common ciphersuites is usable" \
            -S "SSL - Unknown identity received" \
            -S "SSL - Verification of the message MAC failed"

run_test    "PSK callback: no match" \
            "$P_SRV psk_list=abc,dead,def,beef" \
            "$P_CLI force_ciphersuite=TLS-PSK-WITH-AES-128-CBC-SHA \
            psk_identity=ghi psk=beef" \
            1 \
            -S "SSL - None of the common ciphersuites is usable" \
            -s "SSL - Unknown identity received" \
            -S "SSL - Verification of the message MAC failed"

run_test    "PSK callback: wrong key" \
            "$P_SRV psk_list=abc,dead,def,beef" \
            "$P_CLI force_ciphersuite=TLS-PSK-WITH-AES-128-CBC-SHA \
            psk_identity=abc psk=beef" \
            1 \
            -S "SSL - None of the common ciphersuites is usable" \
            -S "SSL - Unknown identity received" \
            -s "SSL - Verification of the message MAC failed"

# Tests for EC J-PAKE

requires_config_enabled MBEDTLS_KEY_EXCHANGE_ECJPAKE
run_test    "ECJPAKE: client not configured" \
            "$P_SRV debug_level=3" \
            "$P_CLI debug_level=3" \
            0 \
            -C "add ciphersuite: c0ff" \
            -C "adding ecjpake_kkpp extension" \
            -S "found ecjpake kkpp extension" \
            -S "skip ecjpake kkpp extension" \
            -S "ciphersuite mismatch: ecjpake not configured" \
            -S "server hello, ecjpake kkpp extension" \
            -C "found ecjpake_kkpp extension" \
            -S "None of the common ciphersuites is usable"

requires_config_enabled MBEDTLS_KEY_EXCHANGE_ECJPAKE
run_test    "ECJPAKE: server not configured" \
            "$P_SRV debug_level=3" \
            "$P_CLI debug_level=3 ecjpake_pw=bla \
             force_ciphersuite=TLS-ECJPAKE-WITH-AES-128-CCM-8" \
            1 \
            -c "add ciphersuite: c0ff" \
            -c "adding ecjpake_kkpp extension" \
            -s "found ecjpake kkpp extension" \
            -s "skip ecjpake kkpp extension" \
            -s "ciphersuite mismatch: ecjpake not configured" \
            -S "server hello, ecjpake kkpp extension" \
            -C "found ecjpake_kkpp extension" \
            -s "None of the common ciphersuites is usable"

requires_config_enabled MBEDTLS_KEY_EXCHANGE_ECJPAKE
run_test    "ECJPAKE: working, TLS" \
            "$P_SRV debug_level=3 ecjpake_pw=bla" \
            "$P_CLI debug_level=3 ecjpake_pw=bla \
             force_ciphersuite=TLS-ECJPAKE-WITH-AES-128-CCM-8" \
            0 \
            -c "add ciphersuite: c0ff" \
            -c "adding ecjpake_kkpp extension" \
            -C "re-using cached ecjpake parameters" \
            -s "found ecjpake kkpp extension" \
            -S "skip ecjpake kkpp extension" \
            -S "ciphersuite mismatch: ecjpake not configured" \
            -s "server hello, ecjpake kkpp extension" \
            -c "found ecjpake_kkpp extension" \
            -S "None of the common ciphersuites is usable" \
            -S "SSL - Verification of the message MAC failed"

server_needs_more_time 1
requires_config_enabled MBEDTLS_KEY_EXCHANGE_ECJPAKE
run_test    "ECJPAKE: password mismatch, TLS" \
            "$P_SRV debug_level=3 ecjpake_pw=bla" \
            "$P_CLI debug_level=3 ecjpake_pw=bad \
             force_ciphersuite=TLS-ECJPAKE-WITH-AES-128-CCM-8" \
            1 \
            -C "re-using cached ecjpake parameters" \
            -s "SSL - Verification of the message MAC failed"

requires_config_enabled MBEDTLS_KEY_EXCHANGE_ECJPAKE
run_test    "ECJPAKE: working, DTLS" \
            "$P_SRV debug_level=3 dtls=1 ecjpake_pw=bla" \
            "$P_CLI debug_level=3 dtls=1 ecjpake_pw=bla \
             force_ciphersuite=TLS-ECJPAKE-WITH-AES-128-CCM-8" \
            0 \
            -c "re-using cached ecjpake parameters" \
            -S "SSL - Verification of the message MAC failed"

requires_config_enabled MBEDTLS_KEY_EXCHANGE_ECJPAKE
run_test    "ECJPAKE: working, DTLS, no cookie" \
            "$P_SRV debug_level=3 dtls=1 ecjpake_pw=bla cookies=0" \
            "$P_CLI debug_level=3 dtls=1 ecjpake_pw=bla \
             force_ciphersuite=TLS-ECJPAKE-WITH-AES-128-CCM-8" \
            0 \
            -C "re-using cached ecjpake parameters" \
            -S "SSL - Verification of the message MAC failed"

server_needs_more_time 1
requires_config_enabled MBEDTLS_KEY_EXCHANGE_ECJPAKE
run_test    "ECJPAKE: password mismatch, DTLS" \
            "$P_SRV debug_level=3 dtls=1 ecjpake_pw=bla" \
            "$P_CLI debug_level=3 dtls=1 ecjpake_pw=bad \
             force_ciphersuite=TLS-ECJPAKE-WITH-AES-128-CCM-8" \
            1 \
            -c "re-using cached ecjpake parameters" \
            -s "SSL - Verification of the message MAC failed"

# for tests with configs/config-thread.h
requires_config_enabled MBEDTLS_KEY_EXCHANGE_ECJPAKE
run_test    "ECJPAKE: working, DTLS, nolog" \
            "$P_SRV dtls=1 ecjpake_pw=bla" \
            "$P_CLI dtls=1 ecjpake_pw=bla \
             force_ciphersuite=TLS-ECJPAKE-WITH-AES-128-CCM-8" \
            0

# Tests for ciphersuites per version

requires_config_enabled MBEDTLS_SSL_PROTO_SSL3
requires_config_enabled MBEDTLS_CAMELLIA_C
requires_config_enabled MBEDTLS_AES_C
run_test    "Per-version suites: SSL3" \
            "$P_SRV min_version=ssl3 version_suites=TLS-RSA-WITH-CAMELLIA-128-CBC-SHA,TLS-RSA-WITH-AES-256-CBC-SHA,TLS-RSA-WITH-AES-128-CBC-SHA,TLS-RSA-WITH-AES-128-GCM-SHA256" \
            "$P_CLI force_version=ssl3" \
            0 \
            -c "Ciphersuite is TLS-RSA-WITH-CAMELLIA-128-CBC-SHA"

requires_config_enabled MBEDTLS_SSL_PROTO_TLS1
requires_config_enabled MBEDTLS_CAMELLIA_C
requires_config_enabled MBEDTLS_AES_C
run_test    "Per-version suites: TLS 1.0" \
            "$P_SRV version_suites=TLS-RSA-WITH-CAMELLIA-128-CBC-SHA,TLS-RSA-WITH-AES-256-CBC-SHA,TLS-RSA-WITH-AES-128-CBC-SHA,TLS-RSA-WITH-AES-128-GCM-SHA256" \
            "$P_CLI force_version=tls1 arc4=1" \
            0 \
            -c "Ciphersuite is TLS-RSA-WITH-AES-256-CBC-SHA"

requires_config_enabled MBEDTLS_SSL_PROTO_TLS1_1
requires_config_enabled MBEDTLS_CAMELLIA_C
requires_config_enabled MBEDTLS_AES_C
run_test    "Per-version suites: TLS 1.1" \
            "$P_SRV version_suites=TLS-RSA-WITH-CAMELLIA-128-CBC-SHA,TLS-RSA-WITH-AES-256-CBC-SHA,TLS-RSA-WITH-AES-128-CBC-SHA,TLS-RSA-WITH-AES-128-GCM-SHA256" \
            "$P_CLI force_version=tls1_1" \
            0 \
            -c "Ciphersuite is TLS-RSA-WITH-AES-128-CBC-SHA"

requires_config_enabled MBEDTLS_SSL_PROTO_TLS1_2
requires_config_enabled MBEDTLS_CAMELLIA_C
requires_config_enabled MBEDTLS_AES_C
run_test    "Per-version suites: TLS 1.2" \
            "$P_SRV version_suites=TLS-RSA-WITH-CAMELLIA-128-CBC-SHA,TLS-RSA-WITH-AES-256-CBC-SHA,TLS-RSA-WITH-AES-128-CBC-SHA,TLS-RSA-WITH-AES-128-GCM-SHA256" \
            "$P_CLI force_version=tls1_2" \
            0 \
            -c "Ciphersuite is TLS-RSA-WITH-AES-128-GCM-SHA256"

# Test for ClientHello without extensions

requires_gnutls
run_test    "ClientHello without extensions, SHA-1 allowed" \
            "$P_SRV debug_level=3" \
            "$G_CLI --priority=NORMAL:%NO_EXTENSIONS:%DISABLE_SAFE_RENEGOTIATION localhost" \
            0 \
            -s "dumping 'client hello extensions' (0 bytes)"

requires_gnutls
run_test    "ClientHello without extensions, SHA-1 forbidden in certificates on server" \
            "$P_SRV debug_level=3 key_file=data_files/server2.key crt_file=data_files/server2.crt allow_sha1=0" \
            "$G_CLI --priority=NORMAL:%NO_EXTENSIONS:%DISABLE_SAFE_RENEGOTIATION localhost" \
            0 \
            -s "dumping 'client hello extensions' (0 bytes)"

# Tests for mbedtls_ssl_get_bytes_avail()

run_test    "mbedtls_ssl_get_bytes_avail: no extra data" \
            "$P_SRV" \
            "$P_CLI request_size=100" \
            0 \
            -s "Read from client: 100 bytes read$"

run_test    "mbedtls_ssl_get_bytes_avail: extra data" \
            "$P_SRV" \
            "$P_CLI request_size=500" \
            0 \
            -s "Read from client: 500 bytes read (.*+.*)"

# Tests for small client packets

requires_config_enabled MBEDTLS_SSL_PROTO_SSL3
run_test    "Small client packet SSLv3 BlockCipher" \
            "$P_SRV min_version=ssl3" \
            "$P_CLI request_size=1 force_version=ssl3 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA" \
            0 \
            -s "Read from client: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_PROTO_SSL3
run_test    "Small client packet SSLv3 StreamCipher" \
            "$P_SRV min_version=ssl3 arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            "$P_CLI request_size=1 force_version=ssl3 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            0 \
            -s "Read from client: 1 bytes read"

run_test    "Small client packet TLS 1.0 BlockCipher" \
            "$P_SRV" \
            "$P_CLI request_size=1 force_version=tls1 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA" \
            0 \
            -s "Read from client: 1 bytes read"

run_test    "Small client packet TLS 1.0 BlockCipher, without EtM" \
            "$P_SRV" \
            "$P_CLI request_size=1 force_version=tls1 etm=0 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA" \
            0 \
            -s "Read from client: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Small client packet TLS 1.0 BlockCipher, truncated MAC" \
            "$P_SRV trunc_hmac=1" \
            "$P_CLI request_size=1 force_version=tls1 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA trunc_hmac=1" \
            0 \
            -s "Read from client: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Small client packet TLS 1.0 BlockCipher, without EtM, truncated MAC" \
            "$P_SRV trunc_hmac=1" \
            "$P_CLI request_size=1 force_version=tls1 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA trunc_hmac=1 etm=0" \
            0 \
            -s "Read from client: 1 bytes read"

run_test    "Small client packet TLS 1.0 StreamCipher" \
            "$P_SRV arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            "$P_CLI request_size=1 force_version=tls1 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            0 \
            -s "Read from client: 1 bytes read"

run_test    "Small client packet TLS 1.0 StreamCipher, without EtM" \
            "$P_SRV arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            "$P_CLI request_size=1 force_version=tls1 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA etm=0" \
            0 \
            -s "Read from client: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Small client packet TLS 1.0 StreamCipher, truncated MAC" \
            "$P_SRV arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1" \
            "$P_CLI request_size=1 force_version=tls1 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1" \
            0 \
            -s "Read from client: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Small client packet TLS 1.0 StreamCipher, without EtM, truncated MAC" \
            "$P_SRV arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1" \
            "$P_CLI request_size=1 force_version=tls1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA \
             trunc_hmac=1 etm=0" \
            0 \
            -s "Read from client: 1 bytes read"

run_test    "Small client packet TLS 1.1 BlockCipher" \
            "$P_SRV" \
            "$P_CLI request_size=1 force_version=tls1_1 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA" \
            0 \
            -s "Read from client: 1 bytes read"

run_test    "Small client packet TLS 1.1 BlockCipher, without EtM" \
            "$P_SRV" \
            "$P_CLI request_size=1 force_version=tls1_1 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA etm=0" \
            0 \
            -s "Read from client: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Small client packet TLS 1.1 BlockCipher, truncated MAC" \
            "$P_SRV trunc_hmac=1" \
            "$P_CLI request_size=1 force_version=tls1_1 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA trunc_hmac=1" \
            0 \
            -s "Read from client: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Small client packet TLS 1.1 BlockCipher, without EtM, truncated MAC" \
            "$P_SRV trunc_hmac=1" \
            "$P_CLI request_size=1 force_version=tls1_1 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA trunc_hmac=1 etm=0" \
            0 \
            -s "Read from client: 1 bytes read"

run_test    "Small client packet TLS 1.1 StreamCipher" \
            "$P_SRV arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            "$P_CLI request_size=1 force_version=tls1_1 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            0 \
            -s "Read from client: 1 bytes read"

run_test    "Small client packet TLS 1.1 StreamCipher, without EtM" \
            "$P_SRV arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            "$P_CLI request_size=1 force_version=tls1_1 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA etm=0" \
            0 \
            -s "Read from client: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Small client packet TLS 1.1 StreamCipher, truncated MAC" \
            "$P_SRV arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1" \
            "$P_CLI request_size=1 force_version=tls1_1 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1" \
            0 \
            -s "Read from client: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Small client packet TLS 1.1 StreamCipher, without EtM, truncated MAC" \
            "$P_SRV arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1" \
            "$P_CLI request_size=1 force_version=tls1_1 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1 etm=0" \
            0 \
            -s "Read from client: 1 bytes read"

run_test    "Small client packet TLS 1.2 BlockCipher" \
            "$P_SRV" \
            "$P_CLI request_size=1 force_version=tls1_2 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA" \
            0 \
            -s "Read from client: 1 bytes read"

run_test    "Small client packet TLS 1.2 BlockCipher, without EtM" \
            "$P_SRV" \
            "$P_CLI request_size=1 force_version=tls1_2 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA etm=0" \
            0 \
            -s "Read from client: 1 bytes read"

run_test    "Small client packet TLS 1.2 BlockCipher larger MAC" \
            "$P_SRV" \
            "$P_CLI request_size=1 force_version=tls1_2 \
             force_ciphersuite=TLS-ECDHE-RSA-WITH-AES-256-CBC-SHA384" \
            0 \
            -s "Read from client: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Small client packet TLS 1.2 BlockCipher, truncated MAC" \
            "$P_SRV trunc_hmac=1" \
            "$P_CLI request_size=1 force_version=tls1_2 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA trunc_hmac=1" \
            0 \
            -s "Read from client: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Small client packet TLS 1.2 BlockCipher, without EtM, truncated MAC" \
            "$P_SRV trunc_hmac=1" \
            "$P_CLI request_size=1 force_version=tls1_2 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA trunc_hmac=1 etm=0" \
            0 \
            -s "Read from client: 1 bytes read"

run_test    "Small client packet TLS 1.2 StreamCipher" \
            "$P_SRV arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            "$P_CLI request_size=1 force_version=tls1_2 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            0 \
            -s "Read from client: 1 bytes read"

run_test    "Small client packet TLS 1.2 StreamCipher, without EtM" \
            "$P_SRV arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            "$P_CLI request_size=1 force_version=tls1_2 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA etm=0" \
            0 \
            -s "Read from client: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Small client packet TLS 1.2 StreamCipher, truncated MAC" \
            "$P_SRV arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1" \
            "$P_CLI request_size=1 force_version=tls1_2 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1" \
            0 \
            -s "Read from client: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Small client packet TLS 1.2 StreamCipher, without EtM, truncated MAC" \
            "$P_SRV arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1" \
            "$P_CLI request_size=1 force_version=tls1_2 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1 etm=0" \
            0 \
            -s "Read from client: 1 bytes read"

run_test    "Small client packet TLS 1.2 AEAD" \
            "$P_SRV" \
            "$P_CLI request_size=1 force_version=tls1_2 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CCM" \
            0 \
            -s "Read from client: 1 bytes read"

run_test    "Small client packet TLS 1.2 AEAD shorter tag" \
            "$P_SRV" \
            "$P_CLI request_size=1 force_version=tls1_2 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CCM-8" \
            0 \
            -s "Read from client: 1 bytes read"

# Tests for small client packets in DTLS

requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
run_test    "Small client packet DTLS 1.0" \
            "$P_SRV dtls=1 force_version=dtls1" \
            "$P_CLI dtls=1 request_size=1 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA" \
            0 \
            -s "Read from client: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
run_test    "Small client packet DTLS 1.0, without EtM" \
            "$P_SRV dtls=1 force_version=dtls1 etm=0" \
            "$P_CLI dtls=1 request_size=1 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA" \
            0 \
            -s "Read from client: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Small client packet DTLS 1.0, truncated hmac" \
            "$P_SRV dtls=1 force_version=dtls1 trunc_hmac=1" \
            "$P_CLI dtls=1 request_size=1 trunc_hmac=1 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA" \
            0 \
            -s "Read from client: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Small client packet DTLS 1.0, without EtM, truncated MAC" \
            "$P_SRV dtls=1 force_version=dtls1 trunc_hmac=1 etm=0" \
            "$P_CLI dtls=1 request_size=1 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA trunc_hmac=1"\
            0 \
            -s "Read from client: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
run_test    "Small client packet DTLS 1.2" \
            "$P_SRV dtls=1 force_version=dtls1_2" \
            "$P_CLI dtls=1 request_size=1 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA" \
            0 \
            -s "Read from client: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
run_test    "Small client packet DTLS 1.2, without EtM" \
            "$P_SRV dtls=1 force_version=dtls1_2 etm=0" \
            "$P_CLI dtls=1 request_size=1 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA" \
            0 \
            -s "Read from client: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Small client packet DTLS 1.2, truncated hmac" \
            "$P_SRV dtls=1 force_version=dtls1_2 trunc_hmac=1" \
            "$P_CLI dtls=1 request_size=1 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA trunc_hmac=1" \
            0 \
            -s "Read from client: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Small client packet DTLS 1.2, without EtM, truncated MAC" \
            "$P_SRV dtls=1 force_version=dtls1_2 trunc_hmac=1 etm=0" \
            "$P_CLI dtls=1 request_size=1 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA trunc_hmac=1"\
            0 \
            -s "Read from client: 1 bytes read"

# Tests for small server packets

requires_config_enabled MBEDTLS_SSL_PROTO_SSL3
run_test    "Small server packet SSLv3 BlockCipher" \
            "$P_SRV response_size=1 min_version=ssl3" \
            "$P_CLI force_version=ssl3 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA" \
            0 \
            -c "Read from server: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_PROTO_SSL3
run_test    "Small server packet SSLv3 StreamCipher" \
            "$P_SRV response_size=1 min_version=ssl3 arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            "$P_CLI force_version=ssl3 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            0 \
            -c "Read from server: 1 bytes read"

run_test    "Small server packet TLS 1.0 BlockCipher" \
            "$P_SRV response_size=1" \
            "$P_CLI force_version=tls1 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA" \
            0 \
            -c "Read from server: 1 bytes read"

run_test    "Small server packet TLS 1.0 BlockCipher, without EtM" \
            "$P_SRV response_size=1" \
            "$P_CLI force_version=tls1 etm=0 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA" \
            0 \
            -c "Read from server: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Small server packet TLS 1.0 BlockCipher, truncated MAC" \
            "$P_SRV response_size=1 trunc_hmac=1" \
            "$P_CLI force_version=tls1 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA trunc_hmac=1" \
            0 \
            -c "Read from server: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Small server packet TLS 1.0 BlockCipher, without EtM, truncated MAC" \
            "$P_SRV response_size=1 trunc_hmac=1" \
            "$P_CLI force_version=tls1 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA trunc_hmac=1 etm=0" \
            0 \
            -c "Read from server: 1 bytes read"

run_test    "Small server packet TLS 1.0 StreamCipher" \
            "$P_SRV response_size=1 arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            "$P_CLI force_version=tls1 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            0 \
            -c "Read from server: 1 bytes read"

run_test    "Small server packet TLS 1.0 StreamCipher, without EtM" \
            "$P_SRV response_size=1 arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            "$P_CLI force_version=tls1 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA etm=0" \
            0 \
            -c "Read from server: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Small server packet TLS 1.0 StreamCipher, truncated MAC" \
            "$P_SRV response_size=1 arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1" \
            "$P_CLI force_version=tls1 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1" \
            0 \
            -c "Read from server: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Small server packet TLS 1.0 StreamCipher, without EtM, truncated MAC" \
            "$P_SRV response_size=1 arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1" \
            "$P_CLI force_version=tls1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA \
             trunc_hmac=1 etm=0" \
            0 \
            -c "Read from server: 1 bytes read"

run_test    "Small server packet TLS 1.1 BlockCipher" \
            "$P_SRV response_size=1" \
            "$P_CLI force_version=tls1_1 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA" \
            0 \
            -c "Read from server: 1 bytes read"

run_test    "Small server packet TLS 1.1 BlockCipher, without EtM" \
            "$P_SRV response_size=1" \
            "$P_CLI force_version=tls1_1 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA etm=0" \
            0 \
            -c "Read from server: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Small server packet TLS 1.1 BlockCipher, truncated MAC" \
            "$P_SRV response_size=1 trunc_hmac=1" \
            "$P_CLI force_version=tls1_1 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA trunc_hmac=1" \
            0 \
            -c "Read from server: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Small server packet TLS 1.1 BlockCipher, without EtM, truncated MAC" \
            "$P_SRV response_size=1 trunc_hmac=1" \
            "$P_CLI force_version=tls1_1 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA trunc_hmac=1 etm=0" \
            0 \
            -c "Read from server: 1 bytes read"

run_test    "Small server packet TLS 1.1 StreamCipher" \
            "$P_SRV response_size=1 arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            "$P_CLI force_version=tls1_1 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            0 \
            -c "Read from server: 1 bytes read"

run_test    "Small server packet TLS 1.1 StreamCipher, without EtM" \
            "$P_SRV response_size=1 arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            "$P_CLI force_version=tls1_1 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA etm=0" \
            0 \
            -c "Read from server: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Small server packet TLS 1.1 StreamCipher, truncated MAC" \
            "$P_SRV response_size=1 arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1" \
            "$P_CLI force_version=tls1_1 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1" \
            0 \
            -c "Read from server: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Small server packet TLS 1.1 StreamCipher, without EtM, truncated MAC" \
            "$P_SRV response_size=1 arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1" \
            "$P_CLI force_version=tls1_1 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1 etm=0" \
            0 \
            -c "Read from server: 1 bytes read"

run_test    "Small server packet TLS 1.2 BlockCipher" \
            "$P_SRV response_size=1" \
            "$P_CLI force_version=tls1_2 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA" \
            0 \
            -c "Read from server: 1 bytes read"

run_test    "Small server packet TLS 1.2 BlockCipher, without EtM" \
            "$P_SRV response_size=1" \
            "$P_CLI force_version=tls1_2 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA etm=0" \
            0 \
            -c "Read from server: 1 bytes read"

run_test    "Small server packet TLS 1.2 BlockCipher larger MAC" \
            "$P_SRV response_size=1" \
            "$P_CLI force_version=tls1_2 \
             force_ciphersuite=TLS-ECDHE-RSA-WITH-AES-256-CBC-SHA384" \
            0 \
            -c "Read from server: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Small server packet TLS 1.2 BlockCipher, truncated MAC" \
            "$P_SRV response_size=1 trunc_hmac=1" \
            "$P_CLI force_version=tls1_2 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA trunc_hmac=1" \
            0 \
            -c "Read from server: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Small server packet TLS 1.2 BlockCipher, without EtM, truncated MAC" \
            "$P_SRV response_size=1 trunc_hmac=1" \
            "$P_CLI force_version=tls1_2 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA trunc_hmac=1 etm=0" \
            0 \
            -c "Read from server: 1 bytes read"

run_test    "Small server packet TLS 1.2 StreamCipher" \
            "$P_SRV response_size=1 arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            "$P_CLI force_version=tls1_2 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            0 \
            -c "Read from server: 1 bytes read"

run_test    "Small server packet TLS 1.2 StreamCipher, without EtM" \
            "$P_SRV response_size=1 arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            "$P_CLI force_version=tls1_2 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA etm=0" \
            0 \
            -c "Read from server: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Small server packet TLS 1.2 StreamCipher, truncated MAC" \
            "$P_SRV response_size=1 arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1" \
            "$P_CLI force_version=tls1_2 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1" \
            0 \
            -c "Read from server: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Small server packet TLS 1.2 StreamCipher, without EtM, truncated MAC" \
            "$P_SRV response_size=1 arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1" \
            "$P_CLI force_version=tls1_2 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1 etm=0" \
            0 \
            -c "Read from server: 1 bytes read"

run_test    "Small server packet TLS 1.2 AEAD" \
            "$P_SRV response_size=1" \
            "$P_CLI force_version=tls1_2 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CCM" \
            0 \
            -c "Read from server: 1 bytes read"

run_test    "Small server packet TLS 1.2 AEAD shorter tag" \
            "$P_SRV response_size=1" \
            "$P_CLI force_version=tls1_2 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CCM-8" \
            0 \
            -c "Read from server: 1 bytes read"

# Tests for small server packets in DTLS

requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
run_test    "Small server packet DTLS 1.0" \
            "$P_SRV dtls=1 response_size=1 force_version=dtls1" \
            "$P_CLI dtls=1 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA" \
            0 \
            -c "Read from server: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
run_test    "Small server packet DTLS 1.0, without EtM" \
            "$P_SRV dtls=1 response_size=1 force_version=dtls1 etm=0" \
            "$P_CLI dtls=1 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA" \
            0 \
            -c "Read from server: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Small server packet DTLS 1.0, truncated hmac" \
            "$P_SRV dtls=1 response_size=1 force_version=dtls1 trunc_hmac=1" \
            "$P_CLI dtls=1 trunc_hmac=1 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA" \
            0 \
            -c "Read from server: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Small server packet DTLS 1.0, without EtM, truncated MAC" \
            "$P_SRV dtls=1 response_size=1 force_version=dtls1 trunc_hmac=1 etm=0" \
            "$P_CLI dtls=1 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA trunc_hmac=1"\
            0 \
            -c "Read from server: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
run_test    "Small server packet DTLS 1.2" \
            "$P_SRV dtls=1 response_size=1 force_version=dtls1_2" \
            "$P_CLI dtls=1 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA" \
            0 \
            -c "Read from server: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
run_test    "Small server packet DTLS 1.2, without EtM" \
            "$P_SRV dtls=1 response_size=1 force_version=dtls1_2 etm=0" \
            "$P_CLI dtls=1 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA" \
            0 \
            -c "Read from server: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Small server packet DTLS 1.2, truncated hmac" \
            "$P_SRV dtls=1 response_size=1 force_version=dtls1_2 trunc_hmac=1" \
            "$P_CLI dtls=1 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA trunc_hmac=1" \
            0 \
            -c "Read from server: 1 bytes read"

requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Small server packet DTLS 1.2, without EtM, truncated MAC" \
            "$P_SRV dtls=1 response_size=1 force_version=dtls1_2 trunc_hmac=1 etm=0" \
            "$P_CLI dtls=1 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA trunc_hmac=1"\
            0 \
            -c "Read from server: 1 bytes read"

# A test for extensions in SSLv3

requires_config_enabled MBEDTLS_SSL_PROTO_SSL3
run_test    "SSLv3 with extensions, server side" \
            "$P_SRV min_version=ssl3 debug_level=3" \
            "$P_CLI force_version=ssl3 tickets=1 max_frag_len=4096 alpn=abc,1234" \
            0 \
            -S "dumping 'client hello extensions'" \
            -S "server hello, total extension length:"

# Test for large client packets

# How many fragments do we expect to write $1 bytes?
fragments_for_write() {
    echo "$(( ( $1 + $MAX_OUT_LEN - 1 ) / $MAX_OUT_LEN ))"
}

requires_config_enabled MBEDTLS_SSL_PROTO_SSL3
run_test    "Large client packet SSLv3 BlockCipher" \
            "$P_SRV min_version=ssl3" \
            "$P_CLI request_size=16384 force_version=ssl3 recsplit=0 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA" \
            0 \
            -c "16384 bytes written in $(fragments_for_write 16384) fragments" \
            -s "Read from client: $MAX_CONTENT_LEN bytes read"

requires_config_enabled MBEDTLS_SSL_PROTO_SSL3
run_test    "Large client packet SSLv3 StreamCipher" \
            "$P_SRV min_version=ssl3 arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            "$P_CLI request_size=16384 force_version=ssl3 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            0 \
            -c "16384 bytes written in $(fragments_for_write 16384) fragments" \
            -s "Read from client: $MAX_CONTENT_LEN bytes read"

run_test    "Large client packet TLS 1.0 BlockCipher" \
            "$P_SRV" \
            "$P_CLI request_size=16384 force_version=tls1 recsplit=0 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA" \
            0 \
            -c "16384 bytes written in $(fragments_for_write 16384) fragments" \
            -s "Read from client: $MAX_CONTENT_LEN bytes read"

run_test    "Large client packet TLS 1.0 BlockCipher, without EtM" \
            "$P_SRV" \
            "$P_CLI request_size=16384 force_version=tls1 etm=0 recsplit=0 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA" \
            0 \
            -s "Read from client: $MAX_CONTENT_LEN bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Large client packet TLS 1.0 BlockCipher, truncated MAC" \
            "$P_SRV trunc_hmac=1" \
            "$P_CLI request_size=16384 force_version=tls1 recsplit=0 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA trunc_hmac=1" \
            0 \
            -c "16384 bytes written in $(fragments_for_write 16384) fragments" \
            -s "Read from client: $MAX_CONTENT_LEN bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Large client packet TLS 1.0 BlockCipher, without EtM, truncated MAC" \
            "$P_SRV trunc_hmac=1" \
            "$P_CLI request_size=16384 force_version=tls1 etm=0 recsplit=0 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA trunc_hmac=1" \
            0 \
            -s "Read from client: $MAX_CONTENT_LEN bytes read"

run_test    "Large client packet TLS 1.0 StreamCipher" \
            "$P_SRV arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            "$P_CLI request_size=16384 force_version=tls1 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            0 \
            -s "Read from client: $MAX_CONTENT_LEN bytes read"

run_test    "Large client packet TLS 1.0 StreamCipher, without EtM" \
            "$P_SRV arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            "$P_CLI request_size=16384 force_version=tls1 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA etm=0" \
            0 \
            -s "Read from client: $MAX_CONTENT_LEN bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Large client packet TLS 1.0 StreamCipher, truncated MAC" \
            "$P_SRV arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1" \
            "$P_CLI request_size=16384 force_version=tls1 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1" \
            0 \
            -s "Read from client: $MAX_CONTENT_LEN bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Large client packet TLS 1.0 StreamCipher, without EtM, truncated MAC" \
            "$P_SRV arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1" \
            "$P_CLI request_size=16384 force_version=tls1 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1 etm=0" \
            0 \
            -c "16384 bytes written in $(fragments_for_write 16384) fragments" \
            -s "Read from client: $MAX_CONTENT_LEN bytes read"

run_test    "Large client packet TLS 1.1 BlockCipher" \
            "$P_SRV" \
            "$P_CLI request_size=16384 force_version=tls1_1 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA" \
            0 \
            -c "16384 bytes written in $(fragments_for_write 16384) fragments" \
            -s "Read from client: $MAX_CONTENT_LEN bytes read"

run_test    "Large client packet TLS 1.1 BlockCipher, without EtM" \
            "$P_SRV" \
            "$P_CLI request_size=16384 force_version=tls1_1 etm=0 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA" \
            0 \
            -s "Read from client: $MAX_CONTENT_LEN bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Large client packet TLS 1.1 BlockCipher, truncated MAC" \
            "$P_SRV trunc_hmac=1" \
            "$P_CLI request_size=16384 force_version=tls1_1 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA trunc_hmac=1" \
            0 \
            -s "Read from client: $MAX_CONTENT_LEN bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Large client packet TLS 1.1 BlockCipher, without EtM, truncated MAC" \
            "$P_SRV trunc_hmac=1" \
            "$P_CLI request_size=16384 force_version=tls1_1 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA trunc_hmac=1 etm=0" \
            0 \
            -s "Read from client: $MAX_CONTENT_LEN bytes read"

run_test    "Large client packet TLS 1.1 StreamCipher" \
            "$P_SRV arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            "$P_CLI request_size=16384 force_version=tls1_1 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            0 \
            -c "16384 bytes written in $(fragments_for_write 16384) fragments" \
            -s "Read from client: $MAX_CONTENT_LEN bytes read"

run_test    "Large client packet TLS 1.1 StreamCipher, without EtM" \
            "$P_SRV arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            "$P_CLI request_size=16384 force_version=tls1_1 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA etm=0" \
            0 \
            -c "16384 bytes written in $(fragments_for_write 16384) fragments" \
            -s "Read from client: $MAX_CONTENT_LEN bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Large client packet TLS 1.1 StreamCipher, truncated MAC" \
            "$P_SRV arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1" \
            "$P_CLI request_size=16384 force_version=tls1_1 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1" \
            0 \
            -s "Read from client: $MAX_CONTENT_LEN bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Large client packet TLS 1.1 StreamCipher, without EtM, truncated MAC" \
            "$P_SRV arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1" \
            "$P_CLI request_size=16384 force_version=tls1_1 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1 etm=0" \
            0 \
            -c "16384 bytes written in $(fragments_for_write 16384) fragments" \
            -s "Read from client: $MAX_CONTENT_LEN bytes read"

run_test    "Large client packet TLS 1.2 BlockCipher" \
            "$P_SRV" \
            "$P_CLI request_size=16384 force_version=tls1_2 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA" \
            0 \
            -c "16384 bytes written in $(fragments_for_write 16384) fragments" \
            -s "Read from client: $MAX_CONTENT_LEN bytes read"

run_test    "Large client packet TLS 1.2 BlockCipher, without EtM" \
            "$P_SRV" \
            "$P_CLI request_size=16384 force_version=tls1_2 etm=0 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA" \
            0 \
            -s "Read from client: $MAX_CONTENT_LEN bytes read"

run_test    "Large client packet TLS 1.2 BlockCipher larger MAC" \
            "$P_SRV" \
            "$P_CLI request_size=16384 force_version=tls1_2 \
             force_ciphersuite=TLS-ECDHE-RSA-WITH-AES-256-CBC-SHA384" \
            0 \
            -c "16384 bytes written in $(fragments_for_write 16384) fragments" \
            -s "Read from client: $MAX_CONTENT_LEN bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Large client packet TLS 1.2 BlockCipher, truncated MAC" \
            "$P_SRV trunc_hmac=1" \
            "$P_CLI request_size=16384 force_version=tls1_2 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA trunc_hmac=1" \
            0 \
            -s "Read from client: $MAX_CONTENT_LEN bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Large client packet TLS 1.2 BlockCipher, without EtM, truncated MAC" \
            "$P_SRV trunc_hmac=1" \
            "$P_CLI request_size=16384 force_version=tls1_2 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA trunc_hmac=1 etm=0" \
            0 \
            -c "16384 bytes written in $(fragments_for_write 16384) fragments" \
            -s "Read from client: $MAX_CONTENT_LEN bytes read"

run_test    "Large client packet TLS 1.2 StreamCipher" \
            "$P_SRV arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            "$P_CLI request_size=16384 force_version=tls1_2 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            0 \
            -c "16384 bytes written in $(fragments_for_write 16384) fragments" \
            -s "Read from client: $MAX_CONTENT_LEN bytes read"

run_test    "Large client packet TLS 1.2 StreamCipher, without EtM" \
            "$P_SRV arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            "$P_CLI request_size=16384 force_version=tls1_2 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA etm=0" \
            0 \
            -s "Read from client: $MAX_CONTENT_LEN bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Large client packet TLS 1.2 StreamCipher, truncated MAC" \
            "$P_SRV arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1" \
            "$P_CLI request_size=16384 force_version=tls1_2 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1" \
            0 \
            -s "Read from client: $MAX_CONTENT_LEN bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Large client packet TLS 1.2 StreamCipher, without EtM, truncated MAC" \
            "$P_SRV arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1" \
            "$P_CLI request_size=16384 force_version=tls1_2 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1 etm=0" \
            0 \
            -c "16384 bytes written in $(fragments_for_write 16384) fragments" \
            -s "Read from client: $MAX_CONTENT_LEN bytes read"

run_test    "Large client packet TLS 1.2 AEAD" \
            "$P_SRV" \
            "$P_CLI request_size=16384 force_version=tls1_2 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CCM" \
            0 \
            -c "16384 bytes written in $(fragments_for_write 16384) fragments" \
            -s "Read from client: $MAX_CONTENT_LEN bytes read"

run_test    "Large client packet TLS 1.2 AEAD shorter tag" \
            "$P_SRV" \
            "$P_CLI request_size=16384 force_version=tls1_2 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CCM-8" \
            0 \
            -c "16384 bytes written in $(fragments_for_write 16384) fragments" \
            -s "Read from client: $MAX_CONTENT_LEN bytes read"

# Test for large server packets
requires_config_enabled MBEDTLS_SSL_PROTO_SSL3
run_test    "Large server packet SSLv3 StreamCipher" \
            "$P_SRV response_size=16384 min_version=ssl3 arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            "$P_CLI force_version=ssl3 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            0 \
            -c "Read from server: 16384 bytes read"

# Checking next 4 tests logs for 1n-1 split against BEAST too
requires_config_enabled MBEDTLS_SSL_PROTO_SSL3
run_test    "Large server packet SSLv3 BlockCipher" \
            "$P_SRV response_size=16384 min_version=ssl3" \
            "$P_CLI force_version=ssl3 recsplit=0 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA" \
            0 \
            -c "Read from server: 1 bytes read"\
            -c "16383 bytes read"\
            -C "Read from server: 16384 bytes read"

run_test    "Large server packet TLS 1.0 BlockCipher" \
            "$P_SRV response_size=16384" \
            "$P_CLI force_version=tls1 recsplit=0 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA" \
            0 \
            -c "Read from server: 1 bytes read"\
            -c "16383 bytes read"\
            -C "Read from server: 16384 bytes read"

run_test    "Large server packet TLS 1.0 BlockCipher, without EtM" \
            "$P_SRV response_size=16384" \
            "$P_CLI force_version=tls1 etm=0 recsplit=0 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA" \
            0 \
            -c "Read from server: 1 bytes read"\
            -c "16383 bytes read"\
            -C "Read from server: 16384 bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Large server packet TLS 1.0 BlockCipher truncated MAC" \
            "$P_SRV response_size=16384" \
            "$P_CLI force_version=tls1 recsplit=0 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA \
             trunc_hmac=1" \
            0 \
            -c "Read from server: 1 bytes read"\
            -c "16383 bytes read"\
            -C "Read from server: 16384 bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Large server packet TLS 1.0 StreamCipher truncated MAC" \
            "$P_SRV response_size=16384 arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            "$P_CLI force_version=tls1 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA \
             trunc_hmac=1" \
            0 \
            -s "16384 bytes written in 1 fragments" \
            -c "Read from server: 16384 bytes read"

run_test    "Large server packet TLS 1.0 StreamCipher" \
            "$P_SRV response_size=16384 arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            "$P_CLI force_version=tls1 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            0 \
            -s "16384 bytes written in 1 fragments" \
            -c "Read from server: 16384 bytes read"

run_test    "Large server packet TLS 1.0 StreamCipher, without EtM" \
            "$P_SRV response_size=16384 arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            "$P_CLI force_version=tls1 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA etm=0" \
            0 \
            -s "16384 bytes written in 1 fragments" \
            -c "Read from server: 16384 bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Large server packet TLS 1.0 StreamCipher, truncated MAC" \
            "$P_SRV response_size=16384 arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1" \
            "$P_CLI force_version=tls1 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1" \
            0 \
            -s "16384 bytes written in 1 fragments" \
            -c "Read from server: 16384 bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Large server packet TLS 1.0 StreamCipher, without EtM, truncated MAC" \
            "$P_SRV response_size=16384 arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1" \
            "$P_CLI force_version=tls1 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1 etm=0" \
            0 \
            -s "16384 bytes written in 1 fragments" \
            -c "Read from server: 16384 bytes read"

run_test    "Large server packet TLS 1.1 BlockCipher" \
            "$P_SRV response_size=16384" \
            "$P_CLI force_version=tls1_1 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA" \
            0 \
            -c "Read from server: 16384 bytes read"

run_test    "Large server packet TLS 1.1 BlockCipher, without EtM" \
            "$P_SRV response_size=16384" \
            "$P_CLI force_version=tls1_1 etm=0 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA" \
            0 \
            -s "16384 bytes written in 1 fragments" \
            -c "Read from server: 16384 bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Large server packet TLS 1.1 BlockCipher truncated MAC" \
            "$P_SRV response_size=16384" \
            "$P_CLI force_version=tls1_1 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA \
             trunc_hmac=1" \
            0 \
            -c "Read from server: 16384 bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Large server packet TLS 1.1 BlockCipher, without EtM, truncated MAC" \
            "$P_SRV response_size=16384 trunc_hmac=1" \
            "$P_CLI force_version=tls1_1 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA trunc_hmac=1 etm=0" \
            0 \
            -s "16384 bytes written in 1 fragments" \
            -c "Read from server: 16384 bytes read"

run_test    "Large server packet TLS 1.1 StreamCipher" \
            "$P_SRV response_size=16384 arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            "$P_CLI force_version=tls1_1 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            0 \
            -c "Read from server: 16384 bytes read"

run_test    "Large server packet TLS 1.1 StreamCipher, without EtM" \
            "$P_SRV response_size=16384 arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            "$P_CLI force_version=tls1_1 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA etm=0" \
            0 \
            -s "16384 bytes written in 1 fragments" \
            -c "Read from server: 16384 bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Large server packet TLS 1.1 StreamCipher truncated MAC" \
            "$P_SRV response_size=16384 arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            "$P_CLI force_version=tls1_1 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA \
             trunc_hmac=1" \
            0 \
            -c "Read from server: 16384 bytes read"

run_test    "Large server packet TLS 1.1 StreamCipher, without EtM, truncated MAC" \
            "$P_SRV response_size=16384 arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1" \
            "$P_CLI force_version=tls1_1 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1 etm=0" \
            0 \
            -s "16384 bytes written in 1 fragments" \
            -c "Read from server: 16384 bytes read"

run_test    "Large server packet TLS 1.2 BlockCipher" \
            "$P_SRV response_size=16384" \
            "$P_CLI force_version=tls1_2 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA" \
            0 \
            -c "Read from server: 16384 bytes read"

run_test    "Large server packet TLS 1.2 BlockCipher, without EtM" \
            "$P_SRV response_size=16384" \
            "$P_CLI force_version=tls1_2 etm=0 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA" \
            0 \
            -s "16384 bytes written in 1 fragments" \
            -c "Read from server: 16384 bytes read"

run_test    "Large server packet TLS 1.2 BlockCipher larger MAC" \
            "$P_SRV response_size=16384" \
            "$P_CLI force_version=tls1_2 \
             force_ciphersuite=TLS-ECDHE-RSA-WITH-AES-256-CBC-SHA384" \
            0 \
            -c "Read from server: 16384 bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Large server packet TLS 1.2 BlockCipher truncated MAC" \
            "$P_SRV response_size=16384" \
            "$P_CLI force_version=tls1_2 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA \
             trunc_hmac=1" \
            0 \
            -c "Read from server: 16384 bytes read"

run_test    "Large server packet TLS 1.2 BlockCipher, without EtM, truncated MAC" \
            "$P_SRV response_size=16384 trunc_hmac=1" \
            "$P_CLI force_version=tls1_2 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CBC-SHA trunc_hmac=1 etm=0" \
            0 \
            -s "16384 bytes written in 1 fragments" \
            -c "Read from server: 16384 bytes read"

run_test    "Large server packet TLS 1.2 StreamCipher" \
            "$P_SRV response_size=16384 arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            "$P_CLI force_version=tls1_2 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            0 \
            -s "16384 bytes written in 1 fragments" \
            -c "Read from server: 16384 bytes read"

run_test    "Large server packet TLS 1.2 StreamCipher, without EtM" \
            "$P_SRV response_size=16384 arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            "$P_CLI force_version=tls1_2 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA etm=0" \
            0 \
            -s "16384 bytes written in 1 fragments" \
            -c "Read from server: 16384 bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Large server packet TLS 1.2 StreamCipher truncated MAC" \
            "$P_SRV response_size=16384 arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA" \
            "$P_CLI force_version=tls1_2 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA \
             trunc_hmac=1" \
            0 \
            -c "Read from server: 16384 bytes read"

requires_config_enabled MBEDTLS_SSL_TRUNCATED_HMAC
run_test    "Large server packet TLS 1.2 StreamCipher, without EtM, truncated MAC" \
            "$P_SRV response_size=16384 arc4=1 force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1" \
            "$P_CLI force_version=tls1_2 \
             force_ciphersuite=TLS-RSA-WITH-RC4-128-SHA trunc_hmac=1 etm=0" \
            0 \
            -s "16384 bytes written in 1 fragments" \
            -c "Read from server: 16384 bytes read"

run_test    "Large server packet TLS 1.2 AEAD" \
            "$P_SRV response_size=16384" \
            "$P_CLI force_version=tls1_2 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CCM" \
            0 \
            -c "Read from server: 16384 bytes read"

run_test    "Large server packet TLS 1.2 AEAD shorter tag" \
            "$P_SRV response_size=16384" \
            "$P_CLI force_version=tls1_2 \
             force_ciphersuite=TLS-RSA-WITH-AES-256-CCM-8" \
            0 \
            -c "Read from server: 16384 bytes read"

# Tests for restartable ECC

requires_config_enabled MBEDTLS_ECP_RESTARTABLE
run_test    "EC restart: TLS, default" \
            "$P_SRV auth_mode=required" \
            "$P_CLI force_ciphersuite=TLS-ECDHE-ECDSA-WITH-AES-128-GCM-SHA256 \
             key_file=data_files/server5.key crt_file=data_files/server5.crt  \
             debug_level=1" \
            0 \
            -C "x509_verify_cert.*4b00" \
            -C "mbedtls_pk_verify.*4b00" \
            -C "mbedtls_ecdh_make_public.*4b00" \
            -C "mbedtls_pk_sign.*4b00"

requires_config_enabled MBEDTLS_ECP_RESTARTABLE
run_test    "EC restart: TLS, max_ops=0" \
            "$P_SRV auth_mode=required" \
            "$P_CLI force_ciphersuite=TLS-ECDHE-ECDSA-WITH-AES-128-GCM-SHA256 \
             key_file=data_files/server5.key crt_file=data_files/server5.crt  \
             debug_level=1 ec_max_ops=0" \
            0 \
            -C "x509_verify_cert.*4b00" \
            -C "mbedtls_pk_verify.*4b00" \
            -C "mbedtls_ecdh_make_public.*4b00" \
            -C "mbedtls_pk_sign.*4b00"

requires_config_enabled MBEDTLS_ECP_RESTARTABLE
run_test    "EC restart: TLS, max_ops=65535" \
            "$P_SRV auth_mode=required" \
            "$P_CLI force_ciphersuite=TLS-ECDHE-ECDSA-WITH-AES-128-GCM-SHA256 \
             key_file=data_files/server5.key crt_file=data_files/server5.crt  \
             debug_level=1 ec_max_ops=65535" \
            0 \
            -C "x509_verify_cert.*4b00" \
            -C "mbedtls_pk_verify.*4b00" \
            -C "mbedtls_ecdh_make_public.*4b00" \
            -C "mbedtls_pk_sign.*4b00"

requires_config_enabled MBEDTLS_ECP_RESTARTABLE
run_test    "EC restart: TLS, max_ops=1000" \
            "$P_SRV auth_mode=required" \
            "$P_CLI force_ciphersuite=TLS-ECDHE-ECDSA-WITH-AES-128-GCM-SHA256 \
             key_file=data_files/server5.key crt_file=data_files/server5.crt  \
             debug_level=1 ec_max_ops=1000" \
            0 \
            -c "x509_verify_cert.*4b00" \
            -c "mbedtls_pk_verify.*4b00" \
            -c "mbedtls_ecdh_make_public.*4b00" \
            -c "mbedtls_pk_sign.*4b00"

requires_config_enabled MBEDTLS_ECP_RESTARTABLE
run_test    "EC restart: TLS, max_ops=1000, badsign" \
            "$P_SRV auth_mode=required \
             crt_file=data_files/server5-badsign.crt \
             key_file=data_files/server5.key" \
            "$P_CLI force_ciphersuite=TLS-ECDHE-ECDSA-WITH-AES-128-GCM-SHA256 \
             key_file=data_files/server5.key crt_file=data_files/server5.crt  \
             debug_level=1 ec_max_ops=1000" \
            1 \
            -c "x509_verify_cert.*4b00" \
            -C "mbedtls_pk_verify.*4b00" \
            -C "mbedtls_ecdh_make_public.*4b00" \
            -C "mbedtls_pk_sign.*4b00" \
            -c "! The certificate is not correctly signed by the trusted CA" \
            -c "! mbedtls_ssl_handshake returned" \
            -c "X509 - Certificate verification failed"

requires_config_enabled MBEDTLS_ECP_RESTARTABLE
run_test    "EC restart: TLS, max_ops=1000, auth_mode=optional badsign" \
            "$P_SRV auth_mode=required \
             crt_file=data_files/server5-badsign.crt \
             key_file=data_files/server5.key" \
            "$P_CLI force_ciphersuite=TLS-ECDHE-ECDSA-WITH-AES-128-GCM-SHA256 \
             key_file=data_files/server5.key crt_file=data_files/server5.crt  \
             debug_level=1 ec_max_ops=1000 auth_mode=optional" \
            0 \
            -c "x509_verify_cert.*4b00" \
            -c "mbedtls_pk_verify.*4b00" \
            -c "mbedtls_ecdh_make_public.*4b00" \
            -c "mbedtls_pk_sign.*4b00" \
            -c "! The certificate is not correctly signed by the trusted CA" \
            -C "! mbedtls_ssl_handshake returned" \
            -C "X509 - Certificate verification failed"

requires_config_enabled MBEDTLS_ECP_RESTARTABLE
run_test    "EC restart: TLS, max_ops=1000, auth_mode=none badsign" \
            "$P_SRV auth_mode=required \
             crt_file=data_files/server5-badsign.crt \
             key_file=data_files/server5.key" \
            "$P_CLI force_ciphersuite=TLS-ECDHE-ECDSA-WITH-AES-128-GCM-SHA256 \
             key_file=data_files/server5.key crt_file=data_files/server5.crt  \
             debug_level=1 ec_max_ops=1000 auth_mode=none" \
            0 \
            -C "x509_verify_cert.*4b00" \
            -c "mbedtls_pk_verify.*4b00" \
            -c "mbedtls_ecdh_make_public.*4b00" \
            -c "mbedtls_pk_sign.*4b00" \
            -C "! The certificate is not correctly signed by the trusted CA" \
            -C "! mbedtls_ssl_handshake returned" \
            -C "X509 - Certificate verification failed"

requires_config_enabled MBEDTLS_ECP_RESTARTABLE
run_test    "EC restart: DTLS, max_ops=1000" \
            "$P_SRV auth_mode=required dtls=1" \
            "$P_CLI force_ciphersuite=TLS-ECDHE-ECDSA-WITH-AES-128-GCM-SHA256 \
             key_file=data_files/server5.key crt_file=data_files/server5.crt  \
             dtls=1 debug_level=1 ec_max_ops=1000" \
            0 \
            -c "x509_verify_cert.*4b00" \
            -c "mbedtls_pk_verify.*4b00" \
            -c "mbedtls_ecdh_make_public.*4b00" \
            -c "mbedtls_pk_sign.*4b00"

requires_config_enabled MBEDTLS_ECP_RESTARTABLE
run_test    "EC restart: TLS, max_ops=1000 no client auth" \
            "$P_SRV" \
            "$P_CLI force_ciphersuite=TLS-ECDHE-ECDSA-WITH-AES-128-GCM-SHA256 \
             debug_level=1 ec_max_ops=1000" \
            0 \
            -c "x509_verify_cert.*4b00" \
            -c "mbedtls_pk_verify.*4b00" \
            -c "mbedtls_ecdh_make_public.*4b00" \
            -C "mbedtls_pk_sign.*4b00"

requires_config_enabled MBEDTLS_ECP_RESTARTABLE
run_test    "EC restart: TLS, max_ops=1000, ECDHE-PSK" \
            "$P_SRV psk=abc123" \
            "$P_CLI force_ciphersuite=TLS-ECDHE-PSK-WITH-AES-128-CBC-SHA256 \
             psk=abc123 debug_level=1 ec_max_ops=1000" \
            0 \
            -C "x509_verify_cert.*4b00" \
            -C "mbedtls_pk_verify.*4b00" \
            -C "mbedtls_ecdh_make_public.*4b00" \
            -C "mbedtls_pk_sign.*4b00"

# Tests of asynchronous private key support in SSL

requires_config_enabled MBEDTLS_SSL_ASYNC_PRIVATE
run_test    "SSL async private: sign, delay=0" \
            "$P_SRV \
             async_operations=s async_private_delay1=0 async_private_delay2=0" \
            "$P_CLI" \
            0 \
            -s "Async sign callback: using key slot " \
            -s "Async resume (slot [0-9]): sign done, status=0"

requires_config_enabled MBEDTLS_SSL_ASYNC_PRIVATE
run_test    "SSL async private: sign, delay=1" \
            "$P_SRV \
             async_operations=s async_private_delay1=1 async_private_delay2=1" \
            "$P_CLI" \
            0 \
            -s "Async sign callback: using key slot " \
            -s "Async resume (slot [0-9]): call 0 more times." \
            -s "Async resume (slot [0-9]): sign done, status=0"

requires_config_enabled MBEDTLS_SSL_ASYNC_PRIVATE
run_test    "SSL async private: sign, delay=2" \
            "$P_SRV \
             async_operations=s async_private_delay1=2 async_private_delay2=2" \
            "$P_CLI" \
            0 \
            -s "Async sign callback: using key slot " \
            -U "Async sign callback: using key slot " \
            -s "Async resume (slot [0-9]): call 1 more times." \
            -s "Async resume (slot [0-9]): call 0 more times." \
            -s "Async resume (slot [0-9]): sign done, status=0"

# Test that the async callback correctly signs the 36-byte hash of TLS 1.0/1.1
# with RSA PKCS#1v1.5 as used in TLS 1.0/1.1.
requires_config_enabled MBEDTLS_SSL_ASYNC_PRIVATE
requires_config_enabled MBEDTLS_SSL_PROTO_TLS1_1
run_test    "SSL async private: sign, RSA, TLS 1.1" \
            "$P_SRV key_file=data_files/server2.key crt_file=data_files/server2.crt \
             async_operations=s async_private_delay1=0 async_private_delay2=0" \
            "$P_CLI force_version=tls1_1" \
            0 \
            -s "Async sign callback: using key slot " \
            -s "Async resume (slot [0-9]): sign done, status=0"

requires_config_enabled MBEDTLS_SSL_ASYNC_PRIVATE
run_test    "SSL async private: sign, SNI" \
            "$P_SRV debug_level=3 \
             async_operations=s async_private_delay1=0 async_private_delay2=0 \
             crt_file=data_files/server5.crt key_file=data_files/server5.key \
             sni=localhost,data_files/server2.crt,data_files/server2.key,-,-,-,polarssl.example,data_files/server1-nospace.crt,data_files/server1.key,-,-,-" \
            "$P_CLI server_name=polarssl.example" \
            0 \
            -s "Async sign callback: using key slot " \
            -s "Async resume (slot [0-9]): sign done, status=0" \
            -s "parse ServerName extension" \
            -c "issuer name *: C=NL, O=PolarSSL, CN=PolarSSL Test CA" \
            -c "subject name *: C=NL, O=PolarSSL, CN=polarssl.example"

requires_config_enabled MBEDTLS_SSL_ASYNC_PRIVATE
run_test    "SSL async private: decrypt, delay=0" \
            "$P_SRV \
             async_operations=d async_private_delay1=0 async_private_delay2=0" \
            "$P_CLI force_ciphersuite=TLS-RSA-WITH-AES-128-CBC-SHA" \
            0 \
            -s "Async decrypt callback: using key slot " \
            -s "Async resume (slot [0-9]): decrypt done, status=0"

requires_config_enabled MBEDTLS_SSL_ASYNC_PRIVATE
run_test    "SSL async private: decrypt, delay=1" \
            "$P_SRV \
             async_operations=d async_private_delay1=1 async_private_delay2=1" \
            "$P_CLI force_ciphersuite=TLS-RSA-WITH-AES-128-CBC-SHA" \
            0 \
            -s "Async decrypt callback: using key slot " \
            -s "Async resume (slot [0-9]): call 0 more times." \
            -s "Async resume (slot [0-9]): decrypt done, status=0"

requires_config_enabled MBEDTLS_SSL_ASYNC_PRIVATE
run_test    "SSL async private: decrypt RSA-PSK, delay=0" \
            "$P_SRV psk=abc123 \
             async_operations=d async_private_delay1=0 async_private_delay2=0" \
            "$P_CLI psk=abc123 \
             force_ciphersuite=TLS-RSA-PSK-WITH-AES-128-CBC-SHA256" \
            0 \
            -s "Async decrypt callback: using key slot " \
            -s "Async resume (slot [0-9]): decrypt done, status=0"

requires_config_enabled MBEDTLS_SSL_ASYNC_PRIVATE
run_test    "SSL async private: decrypt RSA-PSK, delay=1" \
            "$P_SRV psk=abc123 \
             async_operations=d async_private_delay1=1 async_private_delay2=1" \
            "$P_CLI psk=abc123 \
             force_ciphersuite=TLS-RSA-PSK-WITH-AES-128-CBC-SHA256" \
            0 \
            -s "Async decrypt callback: using key slot " \
            -s "Async resume (slot [0-9]): call 0 more times." \
            -s "Async resume (slot [0-9]): decrypt done, status=0"

requires_config_enabled MBEDTLS_SSL_ASYNC_PRIVATE
run_test    "SSL async private: sign callback not present" \
            "$P_SRV \
             async_operations=d async_private_delay1=1 async_private_delay2=1" \
            "$P_CLI; [ \$? -eq 1 ] &&
             $P_CLI force_ciphersuite=TLS-RSA-WITH-AES-128-CBC-SHA" \
            0 \
            -S "Async sign callback" \
            -s "! mbedtls_ssl_handshake returned" \
            -s "The own private key or pre-shared key is not set, but needed" \
            -s "Async resume (slot [0-9]): decrypt done, status=0" \
            -s "Successful connection"

requires_config_enabled MBEDTLS_SSL_ASYNC_PRIVATE
run_test    "SSL async private: decrypt callback not present" \
            "$P_SRV debug_level=1 \
             async_operations=s async_private_delay1=1 async_private_delay2=1" \
            "$P_CLI force_ciphersuite=TLS-RSA-WITH-AES-128-CBC-SHA;
             [ \$? -eq 1 ] && $P_CLI" \
            0 \
            -S "Async decrypt callback" \
            -s "! mbedtls_ssl_handshake returned" \
            -s "got no RSA private key" \
            -s "Async resume (slot [0-9]): sign done, status=0" \
            -s "Successful connection"

# key1: ECDSA, key2: RSA; use key1 from slot 0
requires_config_enabled MBEDTLS_SSL_ASYNC_PRIVATE
run_test    "SSL async private: slot 0 used with key1" \
            "$P_SRV \
             async_operations=s async_private_delay1=1 \
             key_file=data_files/server5.key crt_file=data_files/server5.crt \
             key_file2=data_files/server2.key crt_file2=data_files/server2.crt" \
            "$P_CLI force_ciphersuite=TLS-ECDHE-ECDSA-WITH-AES-128-CBC-SHA256" \
            0 \
            -s "Async sign callback: using key slot 0," \
            -s "Async resume (slot 0): call 0 more times." \
            -s "Async resume (slot 0): sign done, status=0"

# key1: ECDSA, key2: RSA; use key2 from slot 0
requires_config_enabled MBEDTLS_SSL_ASYNC_PRIVATE
run_test    "SSL async private: slot 0 used with key2" \
            "$P_SRV \
             async_operations=s async_private_delay2=1 \
             key_file=data_files/server5.key crt_file=data_files/server5.crt \
             key_file2=data_files/server2.key crt_file2=data_files/server2.crt" \
            "$P_CLI force_ciphersuite=TLS-ECDHE-RSA-WITH-AES-128-CBC-SHA256" \
            0 \
            -s "Async sign callback: using key slot 0," \
            -s "Async resume (slot 0): call 0 more times." \
            -s "Async resume (slot 0): sign done, status=0"

# key1: ECDSA, key2: RSA; use key2 from slot 1
requires_config_enabled MBEDTLS_SSL_ASYNC_PRIVATE
run_test    "SSL async private: slot 1 used with key2" \
            "$P_SRV \
             async_operations=s async_private_delay1=1 async_private_delay2=1 \
             key_file=data_files/server5.key crt_file=data_files/server5.crt \
             key_file2=data_files/server2.key crt_file2=data_files/server2.crt" \
            "$P_CLI force_ciphersuite=TLS-ECDHE-RSA-WITH-AES-128-CBC-SHA256" \
            0 \
            -s "Async sign callback: using key slot 1," \
            -s "Async resume (slot 1): call 0 more times." \
            -s "Async resume (slot 1): sign done, status=0"

# key1: ECDSA, key2: RSA; use key2 directly
requires_config_enabled MBEDTLS_SSL_ASYNC_PRIVATE
run_test    "SSL async private: fall back to transparent key" \
            "$P_SRV \
             async_operations=s async_private_delay1=1 \
             key_file=data_files/server5.key crt_file=data_files/server5.crt \
             key_file2=data_files/server2.key crt_file2=data_files/server2.crt " \
            "$P_CLI force_ciphersuite=TLS-ECDHE-RSA-WITH-AES-128-CBC-SHA256" \
            0 \
            -s "Async sign callback: no key matches this certificate."

requires_config_enabled MBEDTLS_SSL_ASYNC_PRIVATE
run_test    "SSL async private: sign, error in start" \
            "$P_SRV \
             async_operations=s async_private_delay1=1 async_private_delay2=1 \
             async_private_error=1" \
            "$P_CLI" \
            1 \
            -s "Async sign callback: injected error" \
            -S "Async resume" \
            -S "Async cancel" \
            -s "! mbedtls_ssl_handshake returned"

requires_config_enabled MBEDTLS_SSL_ASYNC_PRIVATE
run_test    "SSL async private: sign, cancel after start" \
            "$P_SRV \
             async_operations=s async_private_delay1=1 async_private_delay2=1 \
             async_private_error=2" \
            "$P_CLI" \
            1 \
            -s "Async sign callback: using key slot " \
            -S "Async resume" \
            -s "Async cancel"

requires_config_enabled MBEDTLS_SSL_ASYNC_PRIVATE
run_test    "SSL async private: sign, error in resume" \
            "$P_SRV \
             async_operations=s async_private_delay1=1 async_private_delay2=1 \
             async_private_error=3" \
            "$P_CLI" \
            1 \
            -s "Async sign callback: using key slot " \
            -s "Async resume callback: sign done but injected error" \
            -S "Async cancel" \
            -s "! mbedtls_ssl_handshake returned"

requires_config_enabled MBEDTLS_SSL_ASYNC_PRIVATE
run_test    "SSL async private: decrypt, error in start" \
            "$P_SRV \
             async_operations=d async_private_delay1=1 async_private_delay2=1 \
             async_private_error=1" \
            "$P_CLI force_ciphersuite=TLS-RSA-WITH-AES-128-CBC-SHA" \
            1 \
            -s "Async decrypt callback: injected error" \
            -S "Async resume" \
            -S "Async cancel" \
            -s "! mbedtls_ssl_handshake returned"

requires_config_enabled MBEDTLS_SSL_ASYNC_PRIVATE
run_test    "SSL async private: decrypt, cancel after start" \
            "$P_SRV \
             async_operations=d async_private_delay1=1 async_private_delay2=1 \
             async_private_error=2" \
            "$P_CLI force_ciphersuite=TLS-RSA-WITH-AES-128-CBC-SHA" \
            1 \
            -s "Async decrypt callback: using key slot " \
            -S "Async resume" \
            -s "Async cancel"

requires_config_enabled MBEDTLS_SSL_ASYNC_PRIVATE
run_test    "SSL async private: decrypt, error in resume" \
            "$P_SRV \
             async_operations=d async_private_delay1=1 async_private_delay2=1 \
             async_private_error=3" \
            "$P_CLI force_ciphersuite=TLS-RSA-WITH-AES-128-CBC-SHA" \
            1 \
            -s "Async decrypt callback: using key slot " \
            -s "Async resume callback: decrypt done but injected error" \
            -S "Async cancel" \
            -s "! mbedtls_ssl_handshake returned"

requires_config_enabled MBEDTLS_SSL_ASYNC_PRIVATE
run_test    "SSL async private: cancel after start then operate correctly" \
            "$P_SRV \
             async_operations=s async_private_delay1=1 async_private_delay2=1 \
             async_private_error=-2" \
            "$P_CLI; [ \$? -eq 1 ] && $P_CLI" \
            0 \
            -s "Async cancel" \
            -s "! mbedtls_ssl_handshake returned" \
            -s "Async resume" \
            -s "Successful connection"

requires_config_enabled MBEDTLS_SSL_ASYNC_PRIVATE
run_test    "SSL async private: error in resume then operate correctly" \
            "$P_SRV \
             async_operations=s async_private_delay1=1 async_private_delay2=1 \
             async_private_error=-3" \
            "$P_CLI; [ \$? -eq 1 ] && $P_CLI" \
            0 \
            -s "! mbedtls_ssl_handshake returned" \
            -s "Async resume" \
            -s "Successful connection"

# key1: ECDSA, key2: RSA; use key1 through async, then key2 directly
requires_config_enabled MBEDTLS_SSL_ASYNC_PRIVATE
run_test    "SSL async private: cancel after start then fall back to transparent key" \
            "$P_SRV \
             async_operations=s async_private_delay1=1 async_private_error=-2 \
             key_file=data_files/server5.key crt_file=data_files/server5.crt \
             key_file2=data_files/server2.key crt_file2=data_files/server2.crt" \
            "$P_CLI force_ciphersuite=TLS-ECDHE-ECDSA-WITH-AES-128-CBC-SHA256;
             [ \$? -eq 1 ] &&
             $P_CLI force_ciphersuite=TLS-ECDHE-RSA-WITH-AES-128-CBC-SHA256" \
            0 \
            -s "Async sign callback: using key slot 0" \
            -S "Async resume" \
            -s "Async cancel" \
            -s "! mbedtls_ssl_handshake returned" \
            -s "Async sign callback: no key matches this certificate." \
            -s "Successful connection"

# key1: ECDSA, key2: RSA; use key1 through async, then key2 directly
requires_config_enabled MBEDTLS_SSL_ASYNC_PRIVATE
run_test    "SSL async private: sign, error in resume then fall back to transparent key" \
            "$P_SRV \
             async_operations=s async_private_delay1=1 async_private_error=-3 \
             key_file=data_files/server5.key crt_file=data_files/server5.crt \
             key_file2=data_files/server2.key crt_file2=data_files/server2.crt" \
            "$P_CLI force_ciphersuite=TLS-ECDHE-ECDSA-WITH-AES-128-CBC-SHA256;
             [ \$? -eq 1 ] &&
             $P_CLI force_ciphersuite=TLS-ECDHE-RSA-WITH-AES-128-CBC-SHA256" \
            0 \
            -s "Async resume" \
            -s "! mbedtls_ssl_handshake returned" \
            -s "Async sign callback: no key matches this certificate." \
            -s "Successful connection"

requires_config_enabled MBEDTLS_SSL_ASYNC_PRIVATE
requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
run_test    "SSL async private: renegotiation: client-initiated; sign" \
            "$P_SRV \
             async_operations=s async_private_delay1=1 async_private_delay2=1 \
             exchanges=2 renegotiation=1" \
            "$P_CLI exchanges=2 renegotiation=1 renegotiate=1" \
            0 \
            -s "Async sign callback: using key slot " \
            -s "Async resume (slot [0-9]): sign done, status=0"

requires_config_enabled MBEDTLS_SSL_ASYNC_PRIVATE
requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
run_test    "SSL async private: renegotiation: server-initiated; sign" \
            "$P_SRV \
             async_operations=s async_private_delay1=1 async_private_delay2=1 \
             exchanges=2 renegotiation=1 renegotiate=1" \
            "$P_CLI exchanges=2 renegotiation=1" \
            0 \
            -s "Async sign callback: using key slot " \
            -s "Async resume (slot [0-9]): sign done, status=0"

requires_config_enabled MBEDTLS_SSL_ASYNC_PRIVATE
requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
run_test    "SSL async private: renegotiation: client-initiated; decrypt" \
            "$P_SRV \
             async_operations=d async_private_delay1=1 async_private_delay2=1 \
             exchanges=2 renegotiation=1" \
            "$P_CLI exchanges=2 renegotiation=1 renegotiate=1 \
             force_ciphersuite=TLS-RSA-WITH-AES-128-CBC-SHA" \
            0 \
            -s "Async decrypt callback: using key slot " \
            -s "Async resume (slot [0-9]): decrypt done, status=0"

requires_config_enabled MBEDTLS_SSL_ASYNC_PRIVATE
requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
run_test    "SSL async private: renegotiation: server-initiated; decrypt" \
            "$P_SRV \
             async_operations=d async_private_delay1=1 async_private_delay2=1 \
             exchanges=2 renegotiation=1 renegotiate=1" \
            "$P_CLI exchanges=2 renegotiation=1 \
             force_ciphersuite=TLS-RSA-WITH-AES-128-CBC-SHA" \
            0 \
            -s "Async decrypt callback: using key slot " \
            -s "Async resume (slot [0-9]): decrypt done, status=0"

# Tests for ECC extensions (rfc 4492)

requires_config_enabled MBEDTLS_AES_C
requires_config_enabled MBEDTLS_CIPHER_MODE_CBC
requires_config_enabled MBEDTLS_SHA256_C
requires_config_enabled MBEDTLS_KEY_EXCHANGE_RSA_ENABLED
run_test    "Force a non ECC ciphersuite in the client side" \
            "$P_SRV debug_level=3" \
            "$P_CLI debug_level=3 force_ciphersuite=TLS-RSA-WITH-AES-128-CBC-SHA256" \
            0 \
            -C "client hello, adding supported_elliptic_curves extension" \
            -C "client hello, adding supported_point_formats extension" \
            -S "found supported elliptic curves extension" \
            -S "found supported point formats extension"

requires_config_enabled MBEDTLS_AES_C
requires_config_enabled MBEDTLS_CIPHER_MODE_CBC
requires_config_enabled MBEDTLS_SHA256_C
requires_config_enabled MBEDTLS_KEY_EXCHANGE_RSA_ENABLED
run_test    "Force a non ECC ciphersuite in the server side" \
            "$P_SRV debug_level=3 force_ciphersuite=TLS-RSA-WITH-AES-128-CBC-SHA256" \
            "$P_CLI debug_level=3" \
            0 \
            -C "found supported_point_formats extension" \
            -S "server hello, supported_point_formats extension"

requires_config_enabled MBEDTLS_AES_C
requires_config_enabled MBEDTLS_CIPHER_MODE_CBC
requires_config_enabled MBEDTLS_SHA256_C
requires_config_enabled MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
run_test    "Force an ECC ciphersuite in the client side" \
            "$P_SRV debug_level=3" \
            "$P_CLI debug_level=3 force_ciphersuite=TLS-ECDHE-ECDSA-WITH-AES-128-CBC-SHA256" \
            0 \
            -c "client hello, adding supported_elliptic_curves extension" \
            -c "client hello, adding supported_point_formats extension" \
            -s "found supported elliptic curves extension" \
            -s "found supported point formats extension"

requires_config_enabled MBEDTLS_AES_C
requires_config_enabled MBEDTLS_CIPHER_MODE_CBC
requires_config_enabled MBEDTLS_SHA256_C
requires_config_enabled MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
run_test    "Force an ECC ciphersuite in the server side" \
            "$P_SRV debug_level=3 force_ciphersuite=TLS-ECDHE-ECDSA-WITH-AES-128-CBC-SHA256" \
            "$P_CLI debug_level=3" \
            0 \
            -c "found supported_point_formats extension" \
            -s "server hello, supported_point_formats extension"

# Tests for DTLS HelloVerifyRequest

run_test    "DTLS cookie: enabled" \
            "$P_SRV dtls=1 debug_level=2" \
            "$P_CLI dtls=1 debug_level=2" \
            0 \
            -s "cookie verification failed" \
            -s "cookie verification passed" \
            -S "cookie verification skipped" \
            -c "received hello verify request" \
            -s "hello verification requested" \
            -S "SSL - The requested feature is not available"

run_test    "DTLS cookie: disabled" \
            "$P_SRV dtls=1 debug_level=2 cookies=0" \
            "$P_CLI dtls=1 debug_level=2" \
            0 \
            -S "cookie verification failed" \
            -S "cookie verification passed" \
            -s "cookie verification skipped" \
            -C "received hello verify request" \
            -S "hello verification requested" \
            -S "SSL - The requested feature is not available"

run_test    "DTLS cookie: default (failing)" \
            "$P_SRV dtls=1 debug_level=2 cookies=-1" \
            "$P_CLI dtls=1 debug_level=2 hs_timeout=100-400" \
            1 \
            -s "cookie verification failed" \
            -S "cookie verification passed" \
            -S "cookie verification skipped" \
            -C "received hello verify request" \
            -S "hello verification requested" \
            -s "SSL - The requested feature is not available"

requires_ipv6
run_test    "DTLS cookie: enabled, IPv6" \
            "$P_SRV dtls=1 debug_level=2 server_addr=::1" \
            "$P_CLI dtls=1 debug_level=2 server_addr=::1" \
            0 \
            -s "cookie verification failed" \
            -s "cookie verification passed" \
            -S "cookie verification skipped" \
            -c "received hello verify request" \
            -s "hello verification requested" \
            -S "SSL - The requested feature is not available"

run_test    "DTLS cookie: enabled, nbio" \
            "$P_SRV dtls=1 nbio=2 debug_level=2" \
            "$P_CLI dtls=1 nbio=2 debug_level=2" \
            0 \
            -s "cookie verification failed" \
            -s "cookie verification passed" \
            -S "cookie verification skipped" \
            -c "received hello verify request" \
            -s "hello verification requested" \
            -S "SSL - The requested feature is not available"

# Tests for client reconnecting from the same port with DTLS

not_with_valgrind # spurious resend
run_test    "DTLS client reconnect from same port: reference" \
            "$P_SRV dtls=1 exchanges=2 read_timeout=1000" \
            "$P_CLI dtls=1 exchanges=2 debug_level=2 hs_timeout=500-1000" \
            0 \
            -C "resend" \
            -S "The operation timed out" \
            -S "Client initiated reconnection from same port"

not_with_valgrind # spurious resend
run_test    "DTLS client reconnect from same port: reconnect" \
            "$P_SRV dtls=1 exchanges=2 read_timeout=1000" \
            "$P_CLI dtls=1 exchanges=2 debug_level=2 hs_timeout=500-1000 reconnect_hard=1" \
            0 \
            -C "resend" \
            -S "The operation timed out" \
            -s "Client initiated reconnection from same port"

not_with_valgrind # server/client too slow to respond in time (next test has higher timeouts)
run_test    "DTLS client reconnect from same port: reconnect, nbio, no valgrind" \
            "$P_SRV dtls=1 exchanges=2 read_timeout=1000 nbio=2" \
            "$P_CLI dtls=1 exchanges=2 debug_level=2 hs_timeout=500-1000 reconnect_hard=1" \
            0 \
            -S "The operation timed out" \
            -s "Client initiated reconnection from same port"

only_with_valgrind # Only with valgrind, do previous test but with higher read_timeout and hs_timeout
run_test    "DTLS client reconnect from same port: reconnect, nbio, valgrind" \
            "$P_SRV dtls=1 exchanges=2 read_timeout=2000 nbio=2 hs_timeout=1500-6000" \
            "$P_CLI dtls=1 exchanges=2 debug_level=2 hs_timeout=1500-3000 reconnect_hard=1" \
            0 \
            -S "The operation timed out" \
            -s "Client initiated reconnection from same port"

run_test    "DTLS client reconnect from same port: no cookies" \
            "$P_SRV dtls=1 exchanges=2 read_timeout=1000 cookies=0" \
            "$P_CLI dtls=1 exchanges=2 debug_level=2 hs_timeout=500-8000 reconnect_hard=1" \
            0 \
            -s "The operation timed out" \
            -S "Client initiated reconnection from same port"

# Tests for various cases of client authentication with DTLS
# (focused on handshake flows and message parsing)

run_test    "DTLS client auth: required" \
            "$P_SRV dtls=1 auth_mode=required" \
            "$P_CLI dtls=1" \
            0 \
            -s "Verifying peer X.509 certificate... ok"

run_test    "DTLS client auth: optional, client has no cert" \
            "$P_SRV dtls=1 auth_mode=optional" \
            "$P_CLI dtls=1 crt_file=none key_file=none" \
            0 \
            -s "! Certificate was missing"

run_test    "DTLS client auth: none, client has no cert" \
            "$P_SRV dtls=1 auth_mode=none" \
            "$P_CLI dtls=1 crt_file=none key_file=none debug_level=2" \
            0 \
            -c "skip write certificate$" \
            -s "! Certificate verification was skipped"

run_test    "DTLS wrong PSK: badmac alert" \
            "$P_SRV dtls=1 psk=abc123 force_ciphersuite=TLS-PSK-WITH-AES-128-GCM-SHA256" \
            "$P_CLI dtls=1 psk=abc124" \
            1 \
            -s "SSL - Verification of the message MAC failed" \
            -c "SSL - A fatal alert message was received from our peer"

# Tests for receiving fragmented handshake messages with DTLS

requires_gnutls
run_test    "DTLS reassembly: no fragmentation (gnutls server)" \
            "$G_SRV -u --mtu 2048 -a" \
            "$P_CLI dtls=1 debug_level=2" \
            0 \
            -C "found fragmented DTLS handshake message" \
            -C "error"

requires_gnutls
run_test    "DTLS reassembly: some fragmentation (gnutls server)" \
            "$G_SRV -u --mtu 512" \
            "$P_CLI dtls=1 debug_level=2" \
            0 \
            -c "found fragmented DTLS handshake message" \
            -C "error"

requires_gnutls
run_test    "DTLS reassembly: more fragmentation (gnutls server)" \
            "$G_SRV -u --mtu 128" \
            "$P_CLI dtls=1 debug_level=2" \
            0 \
            -c "found fragmented DTLS handshake message" \
            -C "error"

requires_gnutls
run_test    "DTLS reassembly: more fragmentation, nbio (gnutls server)" \
            "$G_SRV -u --mtu 128" \
            "$P_CLI dtls=1 nbio=2 debug_level=2" \
            0 \
            -c "found fragmented DTLS handshake message" \
            -C "error"

requires_gnutls
requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
run_test    "DTLS reassembly: fragmentation, renego (gnutls server)" \
            "$G_SRV -u --mtu 256" \
            "$P_CLI debug_level=3 dtls=1 renegotiation=1 renegotiate=1" \
            0 \
            -c "found fragmented DTLS handshake message" \
            -c "client hello, adding renegotiation extension" \
            -c "found renegotiation extension" \
            -c "=> renegotiate" \
            -C "mbedtls_ssl_handshake returned" \
            -C "error" \
            -s "Extra-header:"

requires_gnutls
requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
run_test    "DTLS reassembly: fragmentation, nbio, renego (gnutls server)" \
            "$G_SRV -u --mtu 256" \
            "$P_CLI debug_level=3 nbio=2 dtls=1 renegotiation=1 renegotiate=1" \
            0 \
            -c "found fragmented DTLS handshake message" \
            -c "client hello, adding renegotiation extension" \
            -c "found renegotiation extension" \
            -c "=> renegotiate" \
            -C "mbedtls_ssl_handshake returned" \
            -C "error" \
            -s "Extra-header:"

run_test    "DTLS reassembly: no fragmentation (openssl server)" \
            "$O_SRV -dtls1 -mtu 2048" \
            "$P_CLI dtls=1 debug_level=2" \
            0 \
            -C "found fragmented DTLS handshake message" \
            -C "error"

run_test    "DTLS reassembly: some fragmentation (openssl server)" \
            "$O_SRV -dtls1 -mtu 768" \
            "$P_CLI dtls=1 debug_level=2" \
            0 \
            -c "found fragmented DTLS handshake message" \
            -C "error"

run_test    "DTLS reassembly: more fragmentation (openssl server)" \
            "$O_SRV -dtls1 -mtu 256" \
            "$P_CLI dtls=1 debug_level=2" \
            0 \
            -c "found fragmented DTLS handshake message" \
            -C "error"

run_test    "DTLS reassembly: fragmentation, nbio (openssl server)" \
            "$O_SRV -dtls1 -mtu 256" \
            "$P_CLI dtls=1 nbio=2 debug_level=2" \
            0 \
            -c "found fragmented DTLS handshake message" \
            -C "error"

# Tests for sending fragmented handshake messages with DTLS
#
# Use client auth when we need the client to send large messages,
# and use large cert chains on both sides too (the long chains we have all use
# both RSA and ECDSA, but ideally we should have long chains with either).
# Sizes reached (UDP payload):
# - 2037B for server certificate
# - 1542B for client certificate
# - 1013B for newsessionticket
# - all others below 512B
# All those tests assume MAX_CONTENT_LEN is at least 2048

requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
requires_config_enabled MBEDTLS_SSL_MAX_FRAGMENT_LENGTH
run_test    "DTLS fragmenting: none (for reference)" \
            "$P_SRV dtls=1 debug_level=2 auth_mode=required \
             crt_file=data_files/server7_int-ca.crt \
             key_file=data_files/server7.key \
             hs_timeout=2500-60000 \
             max_frag_len=4096" \
            "$P_CLI dtls=1 debug_level=2 \
             crt_file=data_files/server8_int-ca2.crt \
             key_file=data_files/server8.key \
             hs_timeout=2500-60000 \
             max_frag_len=4096" \
            0 \
            -S "found fragmented DTLS handshake message" \
            -C "found fragmented DTLS handshake message" \
            -C "error"

requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
requires_config_enabled MBEDTLS_SSL_MAX_FRAGMENT_LENGTH
run_test    "DTLS fragmenting: server only (max_frag_len)" \
            "$P_SRV dtls=1 debug_level=2 auth_mode=required \
             crt_file=data_files/server7_int-ca.crt \
             key_file=data_files/server7.key \
             hs_timeout=2500-60000 \
             max_frag_len=1024" \
            "$P_CLI dtls=1 debug_level=2 \
             crt_file=data_files/server8_int-ca2.crt \
             key_file=data_files/server8.key \
             hs_timeout=2500-60000 \
             max_frag_len=2048" \
            0 \
            -S "found fragmented DTLS handshake message" \
            -c "found fragmented DTLS handshake message" \
            -C "error"

# With the MFL extension, the server has no way of forcing
# the client to not exceed a certain MTU; hence, the following
# test can't be replicated with an MTU proxy such as the one
# `client-initiated, server only (max_frag_len)` below.
requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
requires_config_enabled MBEDTLS_SSL_MAX_FRAGMENT_LENGTH
run_test    "DTLS fragmenting: server only (more) (max_frag_len)" \
            "$P_SRV dtls=1 debug_level=2 auth_mode=required \
             crt_file=data_files/server7_int-ca.crt \
             key_file=data_files/server7.key \
             hs_timeout=2500-60000 \
             max_frag_len=512" \
            "$P_CLI dtls=1 debug_level=2 \
             crt_file=data_files/server8_int-ca2.crt \
             key_file=data_files/server8.key \
             hs_timeout=2500-60000 \
             max_frag_len=4096" \
            0 \
            -S "found fragmented DTLS handshake message" \
            -c "found fragmented DTLS handshake message" \
            -C "error"

requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
requires_config_enabled MBEDTLS_SSL_MAX_FRAGMENT_LENGTH
run_test    "DTLS fragmenting: client-initiated, server only (max_frag_len)" \
            "$P_SRV dtls=1 debug_level=2 auth_mode=none \
             crt_file=data_files/server7_int-ca.crt \
             key_file=data_files/server7.key \
             hs_timeout=2500-60000 \
             max_frag_len=2048" \
            "$P_CLI dtls=1 debug_level=2 \
             crt_file=data_files/server8_int-ca2.crt \
             key_file=data_files/server8.key \
             hs_timeout=2500-60000 \
             max_frag_len=1024" \
             0 \
            -S "found fragmented DTLS handshake message" \
            -c "found fragmented DTLS handshake message" \
            -C "error"

# While not required by the standard defining the MFL extension
# (according to which it only applies to records, not to datagrams),
# Mbed TLS will never send datagrams larger than MFL + { Max record expansion },
# as otherwise there wouldn't be any means to communicate MTU restrictions
# to the peer.
# The next test checks that no datagrams significantly larger than the
# negotiated MFL are sent.
requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
requires_config_enabled MBEDTLS_SSL_MAX_FRAGMENT_LENGTH
run_test    "DTLS fragmenting: client-initiated, server only (max_frag_len), proxy MTU" \
            -p "$P_PXY mtu=1110" \
            "$P_SRV dtls=1 debug_level=2 auth_mode=none \
             crt_file=data_files/server7_int-ca.crt \
             key_file=data_files/server7.key \
             hs_timeout=2500-60000 \
             max_frag_len=2048" \
            "$P_CLI dtls=1 debug_level=2 \
             crt_file=data_files/server8_int-ca2.crt \
             key_file=data_files/server8.key \
             hs_timeout=2500-60000 \
             max_frag_len=1024" \
            0 \
            -S "found fragmented DTLS handshake message" \
            -c "found fragmented DTLS handshake message" \
            -C "error"

requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
requires_config_enabled MBEDTLS_SSL_MAX_FRAGMENT_LENGTH
run_test    "DTLS fragmenting: client-initiated, both (max_frag_len)" \
            "$P_SRV dtls=1 debug_level=2 auth_mode=required \
             crt_file=data_files/server7_int-ca.crt \
             key_file=data_files/server7.key \
             hs_timeout=2500-60000 \
             max_frag_len=2048" \
            "$P_CLI dtls=1 debug_level=2 \
             crt_file=data_files/server8_int-ca2.crt \
             key_file=data_files/server8.key \
             hs_timeout=2500-60000 \
             max_frag_len=1024" \
            0 \
            -s "found fragmented DTLS handshake message" \
            -c "found fragmented DTLS handshake message" \
            -C "error"

# While not required by the standard defining the MFL extension
# (according to which it only applies to records, not to datagrams),
# Mbed TLS will never send datagrams larger than MFL + { Max record expansion },
# as otherwise there wouldn't be any means to communicate MTU restrictions
# to the peer.
# The next test checks that no datagrams significantly larger than the
# negotiated MFL are sent.
requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
requires_config_enabled MBEDTLS_SSL_MAX_FRAGMENT_LENGTH
run_test    "DTLS fragmenting: client-initiated, both (max_frag_len), proxy MTU" \
            -p "$P_PXY mtu=1110" \
            "$P_SRV dtls=1 debug_level=2 auth_mode=required \
             crt_file=data_files/server7_int-ca.crt \
             key_file=data_files/server7.key \
             hs_timeout=2500-60000 \
             max_frag_len=2048" \
            "$P_CLI dtls=1 debug_level=2 \
             crt_file=data_files/server8_int-ca2.crt \
             key_file=data_files/server8.key \
             hs_timeout=2500-60000 \
             max_frag_len=1024" \
            0 \
            -s "found fragmented DTLS handshake message" \
            -c "found fragmented DTLS handshake message" \
            -C "error"

requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
run_test    "DTLS fragmenting: none (for reference) (MTU)" \
            "$P_SRV dtls=1 debug_level=2 auth_mode=required \
             crt_file=data_files/server7_int-ca.crt \
             key_file=data_files/server7.key \
             hs_timeout=2500-60000 \
             mtu=4096" \
            "$P_CLI dtls=1 debug_level=2 \
             crt_file=data_files/server8_int-ca2.crt \
             key_file=data_files/server8.key \
             hs_timeout=2500-60000 \
             mtu=4096" \
            0 \
            -S "found fragmented DTLS handshake message" \
            -C "found fragmented DTLS handshake message" \
            -C "error"

requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
run_test    "DTLS fragmenting: client (MTU)" \
            "$P_SRV dtls=1 debug_level=2 auth_mode=required \
             crt_file=data_files/server7_int-ca.crt \
             key_file=data_files/server7.key \
             hs_timeout=3500-60000 \
             mtu=4096" \
            "$P_CLI dtls=1 debug_level=2 \
             crt_file=data_files/server8_int-ca2.crt \
             key_file=data_files/server8.key \
             hs_timeout=3500-60000 \
             mtu=1024" \
            0 \
            -s "found fragmented DTLS handshake message" \
            -C "found fragmented DTLS handshake message" \
            -C "error"

requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
run_test    "DTLS fragmenting: server (MTU)" \
            "$P_SRV dtls=1 debug_level=2 auth_mode=required \
             crt_file=data_files/server7_int-ca.crt \
             key_file=data_files/server7.key \
             hs_timeout=2500-60000 \
             mtu=512" \
            "$P_CLI dtls=1 debug_level=2 \
             crt_file=data_files/server8_int-ca2.crt \
             key_file=data_files/server8.key \
             hs_timeout=2500-60000 \
             mtu=2048" \
            0 \
            -S "found fragmented DTLS handshake message" \
            -c "found fragmented DTLS handshake message" \
            -C "error"

requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
run_test    "DTLS fragmenting: both (MTU=1024)" \
            -p "$P_PXY mtu=1024" \
            "$P_SRV dtls=1 debug_level=2 auth_mode=required \
             crt_file=data_files/server7_int-ca.crt \
             key_file=data_files/server7.key \
             hs_timeout=2500-60000 \
             mtu=1024" \
            "$P_CLI dtls=1 debug_level=2 \
             crt_file=data_files/server8_int-ca2.crt \
             key_file=data_files/server8.key \
             hs_timeout=2500-60000 \
             mtu=1024" \
            0 \
            -s "found fragmented DTLS handshake message" \
            -c "found fragmented DTLS handshake message" \
            -C "error"

# Forcing ciphersuite for this test to fit the MTU of 512 with full config.
requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
requires_config_enabled MBEDTLS_SHA256_C
requires_config_enabled MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA
requires_config_enabled MBEDTLS_AES_C
requires_config_enabled MBEDTLS_GCM_C
run_test    "DTLS fragmenting: both (MTU=512)" \
            -p "$P_PXY mtu=512" \
            "$P_SRV dtls=1 debug_level=2 auth_mode=required \
             crt_file=data_files/server7_int-ca.crt \
             key_file=data_files/server7.key \
             hs_timeout=2500-60000 \
             mtu=512" \
            "$P_CLI dtls=1 debug_level=2 \
             crt_file=data_files/server8_int-ca2.crt \
             key_file=data_files/server8.key \
             force_ciphersuite=TLS-ECDHE-ECDSA-WITH-AES-128-GCM-SHA256 \
             hs_timeout=2500-60000 \
             mtu=512" \
            0 \
            -s "found fragmented DTLS handshake message" \
            -c "found fragmented DTLS handshake message" \
            -C "error"

# Test for automatic MTU reduction on repeated resend.
# Forcing ciphersuite for this test to fit the MTU of 508 with full config.
# The ratio of max/min timeout should ideally equal 4 to accept two
# retransmissions, but in some cases (like both the server and client using
# fragmentation and auto-reduction) an extra retransmission might occur,
# hence the ratio of 8.
not_with_valgrind
requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
requires_config_enabled MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA
requires_config_enabled MBEDTLS_AES_C
requires_config_enabled MBEDTLS_GCM_C
run_test    "DTLS fragmenting: proxy MTU: auto-reduction" \
            -p "$P_PXY mtu=508" \
            "$P_SRV dtls=1 debug_level=2 auth_mode=required \
             crt_file=data_files/server7_int-ca.crt \
             key_file=data_files/server7.key \
             hs_timeout=400-3200" \
            "$P_CLI dtls=1 debug_level=2 \
             crt_file=data_files/server8_int-ca2.crt \
             key_file=data_files/server8.key \
             force_ciphersuite=TLS-ECDHE-ECDSA-WITH-AES-128-GCM-SHA256 \
             hs_timeout=400-3200" \
            0 \
            -s "found fragmented DTLS handshake message" \
            -c "found fragmented DTLS handshake message" \
            -C "error"

# Forcing ciphersuite for this test to fit the MTU of 508 with full config.
only_with_valgrind
requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
requires_config_enabled MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA
requires_config_enabled MBEDTLS_AES_C
requires_config_enabled MBEDTLS_GCM_C
run_test    "DTLS fragmenting: proxy MTU: auto-reduction" \
            -p "$P_PXY mtu=508" \
            "$P_SRV dtls=1 debug_level=2 auth_mode=required \
             crt_file=data_files/server7_int-ca.crt \
             key_file=data_files/server7.key \
             hs_timeout=250-10000" \
            "$P_CLI dtls=1 debug_level=2 \
             crt_file=data_files/server8_int-ca2.crt \
             key_file=data_files/server8.key \
             force_ciphersuite=TLS-ECDHE-ECDSA-WITH-AES-128-GCM-SHA256 \
             hs_timeout=250-10000" \
            0 \
            -s "found fragmented DTLS handshake message" \
            -c "found fragmented DTLS handshake message" \
            -C "error"

# the proxy shouldn't drop or mess up anything, so we shouldn't need to resend
# OTOH the client might resend if the server is to slow to reset after sending
# a HelloVerifyRequest, so only check for no retransmission server-side
not_with_valgrind # spurious autoreduction due to timeout
requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
run_test    "DTLS fragmenting: proxy MTU, simple handshake (MTU=1024)" \
            -p "$P_PXY mtu=1024" \
            "$P_SRV dtls=1 debug_level=2 auth_mode=required \
             crt_file=data_files/server7_int-ca.crt \
             key_file=data_files/server7.key \
             hs_timeout=10000-60000 \
             mtu=1024" \
            "$P_CLI dtls=1 debug_level=2 \
             crt_file=data_files/server8_int-ca2.crt \
             key_file=data_files/server8.key \
             hs_timeout=10000-60000 \
             mtu=1024" \
            0 \
            -S "autoreduction" \
            -s "found fragmented DTLS handshake message" \
            -c "found fragmented DTLS handshake message" \
            -C "error"

# Forcing ciphersuite for this test to fit the MTU of 512 with full config.
# the proxy shouldn't drop or mess up anything, so we shouldn't need to resend
# OTOH the client might resend if the server is to slow to reset after sending
# a HelloVerifyRequest, so only check for no retransmission server-side
not_with_valgrind # spurious autoreduction due to timeout
requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
requires_config_enabled MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA
requires_config_enabled MBEDTLS_AES_C
requires_config_enabled MBEDTLS_GCM_C
run_test    "DTLS fragmenting: proxy MTU, simple handshake (MTU=512)" \
            -p "$P_PXY mtu=512" \
            "$P_SRV dtls=1 debug_level=2 auth_mode=required \
             crt_file=data_files/server7_int-ca.crt \
             key_file=data_files/server7.key \
             hs_timeout=10000-60000 \
             mtu=512" \
            "$P_CLI dtls=1 debug_level=2 \
             crt_file=data_files/server8_int-ca2.crt \
             key_file=data_files/server8.key \
             force_ciphersuite=TLS-ECDHE-ECDSA-WITH-AES-128-GCM-SHA256 \
             hs_timeout=10000-60000 \
             mtu=512" \
            0 \
            -S "autoreduction" \
            -s "found fragmented DTLS handshake message" \
            -c "found fragmented DTLS handshake message" \
            -C "error"

not_with_valgrind # spurious autoreduction due to timeout
requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
run_test    "DTLS fragmenting: proxy MTU, simple handshake, nbio (MTU=1024)" \
            -p "$P_PXY mtu=1024" \
            "$P_SRV dtls=1 debug_level=2 auth_mode=required \
             crt_file=data_files/server7_int-ca.crt \
             key_file=data_files/server7.key \
             hs_timeout=10000-60000 \
             mtu=1024 nbio=2" \
            "$P_CLI dtls=1 debug_level=2 \
             crt_file=data_files/server8_int-ca2.crt \
             key_file=data_files/server8.key \
             hs_timeout=10000-60000 \
             mtu=1024 nbio=2" \
            0 \
            -S "autoreduction" \
            -s "found fragmented DTLS handshake message" \
            -c "found fragmented DTLS handshake message" \
            -C "error"

# Forcing ciphersuite for this test to fit the MTU of 512 with full config.
not_with_valgrind # spurious autoreduction due to timeout
requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
requires_config_enabled MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA
requires_config_enabled MBEDTLS_AES_C
requires_config_enabled MBEDTLS_GCM_C
run_test    "DTLS fragmenting: proxy MTU, simple handshake, nbio (MTU=512)" \
            -p "$P_PXY mtu=512" \
            "$P_SRV dtls=1 debug_level=2 auth_mode=required \
             crt_file=data_files/server7_int-ca.crt \
             key_file=data_files/server7.key \
             hs_timeout=10000-60000 \
             mtu=512 nbio=2" \
            "$P_CLI dtls=1 debug_level=2 \
             crt_file=data_files/server8_int-ca2.crt \
             key_file=data_files/server8.key \
             force_ciphersuite=TLS-ECDHE-ECDSA-WITH-AES-128-GCM-SHA256 \
             hs_timeout=10000-60000 \
             mtu=512 nbio=2" \
            0 \
            -S "autoreduction" \
            -s "found fragmented DTLS handshake message" \
            -c "found fragmented DTLS handshake message" \
            -C "error"

# Forcing ciphersuite for this test to fit the MTU of 1450 with full config.
# This ensures things still work after session_reset().
# It also exercises the "resumed handshake" flow.
# Since we don't support reading fragmented ClientHello yet,
# up the MTU to 1450 (larger than ClientHello with session ticket,
# but still smaller than client's Certificate to ensure fragmentation).
# An autoreduction on the client-side might happen if the server is
# slow to reset, therefore omitting '-C "autoreduction"' below.
# reco_delay avoids races where the client reconnects before the server has
# resumed listening, which would result in a spurious autoreduction.
not_with_valgrind # spurious autoreduction due to timeout
requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
requires_config_enabled MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA
requires_config_enabled MBEDTLS_AES_C
requires_config_enabled MBEDTLS_GCM_C
run_test    "DTLS fragmenting: proxy MTU, resumed handshake" \
            -p "$P_PXY mtu=1450" \
            "$P_SRV dtls=1 debug_level=2 auth_mode=required \
             crt_file=data_files/server7_int-ca.crt \
             key_file=data_files/server7.key \
             hs_timeout=10000-60000 \
             mtu=1450" \
            "$P_CLI dtls=1 debug_level=2 \
             crt_file=data_files/server8_int-ca2.crt \
             key_file=data_files/server8.key \
             hs_timeout=10000-60000 \
             force_ciphersuite=TLS-ECDHE-ECDSA-WITH-AES-128-GCM-SHA256 \
             mtu=1450 reconnect=1 reco_delay=1" \
            0 \
            -S "autoreduction" \
            -s "found fragmented DTLS handshake message" \
            -c "found fragmented DTLS handshake message" \
            -C "error"

# An autoreduction on the client-side might happen if the server is
# slow to reset, therefore omitting '-C "autoreduction"' below.
not_with_valgrind # spurious autoreduction due to timeout
requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
requires_config_enabled MBEDTLS_SHA256_C
requires_config_enabled MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA
requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
requires_config_enabled MBEDTLS_CHACHAPOLY_C
run_test    "DTLS fragmenting: proxy MTU, ChachaPoly renego" \
            -p "$P_PXY mtu=512" \
            "$P_SRV dtls=1 debug_level=2 auth_mode=required \
             crt_file=data_files/server7_int-ca.crt \
             key_file=data_files/server7.key \
             exchanges=2 renegotiation=1 \
             hs_timeout=10000-60000 \
             mtu=512" \
            "$P_CLI dtls=1 debug_level=2 \
             crt_file=data_files/server8_int-ca2.crt \
             key_file=data_files/server8.key \
             exchanges=2 renegotiation=1 renegotiate=1 \
             force_ciphersuite=TLS-ECDHE-ECDSA-WITH-AES-128-GCM-SHA256 \
             hs_timeout=10000-60000 \
             mtu=512" \
            0 \
            -S "autoreduction" \
            -s "found fragmented DTLS handshake message" \
            -c "found fragmented DTLS handshake message" \
            -C "error"

# An autoreduction on the client-side might happen if the server is
# slow to reset, therefore omitting '-C "autoreduction"' below.
not_with_valgrind # spurious autoreduction due to timeout
requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
requires_config_enabled MBEDTLS_SHA256_C
requires_config_enabled MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA
requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
requires_config_enabled MBEDTLS_AES_C
requires_config_enabled MBEDTLS_GCM_C
run_test    "DTLS fragmenting: proxy MTU, AES-GCM renego" \
            -p "$P_PXY mtu=512" \
            "$P_SRV dtls=1 debug_level=2 auth_mode=required \
             crt_file=data_files/server7_int-ca.crt \
             key_file=data_files/server7.key \
             exchanges=2 renegotiation=1 \
             hs_timeout=10000-60000 \
             mtu=512" \
            "$P_CLI dtls=1 debug_level=2 \
             crt_file=data_files/server8_int-ca2.crt \
             key_file=data_files/server8.key \
             exchanges=2 renegotiation=1 renegotiate=1 \
             force_ciphersuite=TLS-ECDHE-ECDSA-WITH-AES-128-GCM-SHA256 \
             hs_timeout=10000-60000 \
             mtu=512" \
            0 \
            -S "autoreduction" \
            -s "found fragmented DTLS handshake message" \
            -c "found fragmented DTLS handshake message" \
            -C "error"

# An autoreduction on the client-side might happen if the server is
# slow to reset, therefore omitting '-C "autoreduction"' below.
not_with_valgrind # spurious autoreduction due to timeout
requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
requires_config_enabled MBEDTLS_SHA256_C
requires_config_enabled MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA
requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
requires_config_enabled MBEDTLS_AES_C
requires_config_enabled MBEDTLS_CCM_C
run_test    "DTLS fragmenting: proxy MTU, AES-CCM renego" \
            -p "$P_PXY mtu=1024" \
            "$P_SRV dtls=1 debug_level=2 auth_mode=required \
             crt_file=data_files/server7_int-ca.crt \
             key_file=data_files/server7.key \
             exchanges=2 renegotiation=1 \
             force_ciphersuite=TLS-ECDHE-ECDSA-WITH-AES-128-CCM-8 \
             hs_timeout=10000-60000 \
             mtu=1024" \
            "$P_CLI dtls=1 debug_level=2 \
             crt_file=data_files/server8_int-ca2.crt \
             key_file=data_files/server8.key \
             exchanges=2 renegotiation=1 renegotiate=1 \
             hs_timeout=10000-60000 \
             mtu=1024" \
            0 \
            -S "autoreduction" \
            -s "found fragmented DTLS handshake message" \
            -c "found fragmented DTLS handshake message" \
            -C "error"

# An autoreduction on the client-side might happen if the server is
# slow to reset, therefore omitting '-C "autoreduction"' below.
not_with_valgrind # spurious autoreduction due to timeout
requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
requires_config_enabled MBEDTLS_SHA256_C
requires_config_enabled MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA
requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
requires_config_enabled MBEDTLS_AES_C
requires_config_enabled MBEDTLS_CIPHER_MODE_CBC
requires_config_enabled MBEDTLS_SSL_ENCRYPT_THEN_MAC
run_test    "DTLS fragmenting: proxy MTU, AES-CBC EtM renego" \
            -p "$P_PXY mtu=1024" \
            "$P_SRV dtls=1 debug_level=2 auth_mode=required \
             crt_file=data_files/server7_int-ca.crt \
             key_file=data_files/server7.key \
             exchanges=2 renegotiation=1 \
             force_ciphersuite=TLS-ECDHE-ECDSA-WITH-AES-128-CBC-SHA256 \
             hs_timeout=10000-60000 \
             mtu=1024" \
            "$P_CLI dtls=1 debug_level=2 \
             crt_file=data_files/server8_int-ca2.crt \
             key_file=data_files/server8.key \
             exchanges=2 renegotiation=1 renegotiate=1 \
             hs_timeout=10000-60000 \
             mtu=1024" \
            0 \
            -S "autoreduction" \
            -s "found fragmented DTLS handshake message" \
            -c "found fragmented DTLS handshake message" \
            -C "error"

# An autoreduction on the client-side might happen if the server is
# slow to reset, therefore omitting '-C "autoreduction"' below.
not_with_valgrind # spurious autoreduction due to timeout
requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
requires_config_enabled MBEDTLS_SHA256_C
requires_config_enabled MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA
requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
requires_config_enabled MBEDTLS_AES_C
requires_config_enabled MBEDTLS_CIPHER_MODE_CBC
run_test    "DTLS fragmenting: proxy MTU, AES-CBC non-EtM renego" \
            -p "$P_PXY mtu=1024" \
            "$P_SRV dtls=1 debug_level=2 auth_mode=required \
             crt_file=data_files/server7_int-ca.crt \
             key_file=data_files/server7.key \
             exchanges=2 renegotiation=1 \
             force_ciphersuite=TLS-ECDHE-ECDSA-WITH-AES-128-CBC-SHA256 etm=0 \
             hs_timeout=10000-60000 \
             mtu=1024" \
            "$P_CLI dtls=1 debug_level=2 \
             crt_file=data_files/server8_int-ca2.crt \
             key_file=data_files/server8.key \
             exchanges=2 renegotiation=1 renegotiate=1 \
             hs_timeout=10000-60000 \
             mtu=1024" \
            0 \
            -S "autoreduction" \
            -s "found fragmented DTLS handshake message" \
            -c "found fragmented DTLS handshake message" \
            -C "error"

# Forcing ciphersuite for this test to fit the MTU of 512 with full config.
requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
requires_config_enabled MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA
requires_config_enabled MBEDTLS_AES_C
requires_config_enabled MBEDTLS_GCM_C
client_needs_more_time 2
run_test    "DTLS fragmenting: proxy MTU + 3d" \
            -p "$P_PXY mtu=512 drop=8 delay=8 duplicate=8" \
            "$P_SRV dgram_packing=0 dtls=1 debug_level=2 auth_mode=required \
             crt_file=data_files/server7_int-ca.crt \
             key_file=data_files/server7.key \
             hs_timeout=250-10000 mtu=512" \
            "$P_CLI dgram_packing=0 dtls=1 debug_level=2 \
             crt_file=data_files/server8_int-ca2.crt \
             key_file=data_files/server8.key \
             force_ciphersuite=TLS-ECDHE-ECDSA-WITH-AES-128-GCM-SHA256 \
             hs_timeout=250-10000 mtu=512" \
            0 \
            -s "found fragmented DTLS handshake message" \
            -c "found fragmented DTLS handshake message" \
            -C "error"

# Forcing ciphersuite for this test to fit the MTU of 512 with full config.
requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
requires_config_enabled MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA
requires_config_enabled MBEDTLS_AES_C
requires_config_enabled MBEDTLS_GCM_C
client_needs_more_time 2
run_test    "DTLS fragmenting: proxy MTU + 3d, nbio" \
            -p "$P_PXY mtu=512 drop=8 delay=8 duplicate=8" \
            "$P_SRV dtls=1 debug_level=2 auth_mode=required \
             crt_file=data_files/server7_int-ca.crt \
             key_file=data_files/server7.key \
             hs_timeout=250-10000 mtu=512 nbio=2" \
            "$P_CLI dtls=1 debug_level=2 \
             crt_file=data_files/server8_int-ca2.crt \
             key_file=data_files/server8.key \
             force_ciphersuite=TLS-ECDHE-ECDSA-WITH-AES-128-GCM-SHA256 \
             hs_timeout=250-10000 mtu=512 nbio=2" \
            0 \
            -s "found fragmented DTLS handshake message" \
            -c "found fragmented DTLS handshake message" \
            -C "error"

# interop tests for DTLS fragmentating with reliable connection
#
# here and below we just want to test that the we fragment in a way that
# pleases other implementations, so we don't need the peer to fragment
requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
requires_config_enabled MBEDTLS_SSL_PROTO_TLS1_2
requires_gnutls
run_test    "DTLS fragmenting: gnutls server, DTLS 1.2" \
            "$G_SRV -u" \
            "$P_CLI dtls=1 debug_level=2 \
             crt_file=data_files/server8_int-ca2.crt \
             key_file=data_files/server8.key \
             mtu=512 force_version=dtls1_2" \
            0 \
            -c "fragmenting handshake message" \
            -C "error"

requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
requires_config_enabled MBEDTLS_SSL_PROTO_TLS1_1
requires_gnutls
run_test    "DTLS fragmenting: gnutls server, DTLS 1.0" \
            "$G_SRV -u" \
            "$P_CLI dtls=1 debug_level=2 \
             crt_file=data_files/server8_int-ca2.crt \
             key_file=data_files/server8.key \
             mtu=512 force_version=dtls1" \
            0 \
            -c "fragmenting handshake message" \
            -C "error"

# We use --insecure for the GnuTLS client because it expects
# the hostname / IP it connects to to be the name used in the
# certificate obtained from the server. Here, however, it
# connects to 127.0.0.1 while our test certificates use 'localhost'
# as the server name in the certificate. This will make the
# certifiate validation fail, but passing --insecure makes
# GnuTLS continue the connection nonetheless.
requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
requires_config_enabled MBEDTLS_SSL_PROTO_TLS1_2
requires_gnutls
requires_not_i686
run_test    "DTLS fragmenting: gnutls client, DTLS 1.2" \
            "$P_SRV dtls=1 debug_level=2 \
             crt_file=data_files/server7_int-ca.crt \
             key_file=data_files/server7.key \
             mtu=512 force_version=dtls1_2" \
            "$G_CLI -u --insecure 127.0.0.1" \
            0 \
            -s "fragmenting handshake message"

# See previous test for the reason to use --insecure
requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
requires_config_enabled MBEDTLS_SSL_PROTO_TLS1_1
requires_gnutls
requires_not_i686
run_test    "DTLS fragmenting: gnutls client, DTLS 1.0" \
            "$P_SRV dtls=1 debug_level=2 \
             crt_file=data_files/server7_int-ca.crt \
             key_file=data_files/server7.key \
             mtu=512 force_version=dtls1" \
            "$G_CLI -u --insecure 127.0.0.1" \
            0 \
            -s "fragmenting handshake message"

requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
requires_config_enabled MBEDTLS_SSL_PROTO_TLS1_2
run_test    "DTLS fragmenting: openssl server, DTLS 1.2" \
            "$O_SRV -dtls1_2 -verify 10" \
            "$P_CLI dtls=1 debug_level=2 \
             crt_file=data_files/server8_int-ca2.crt \
             key_file=data_files/server8.key \
             mtu=512 force_version=dtls1_2" \
            0 \
            -c "fragmenting handshake message" \
            -C "error"

requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
requires_config_enabled MBEDTLS_SSL_PROTO_TLS1_1
run_test    "DTLS fragmenting: openssl server, DTLS 1.0" \
            "$O_SRV -dtls1 -verify 10" \
            "$P_CLI dtls=1 debug_level=2 \
             crt_file=data_files/server8_int-ca2.crt \
             key_file=data_files/server8.key \
             mtu=512 force_version=dtls1" \
            0 \
            -c "fragmenting handshake message" \
            -C "error"

requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
requires_config_enabled MBEDTLS_SSL_PROTO_TLS1_2
run_test    "DTLS fragmenting: openssl client, DTLS 1.2" \
            "$P_SRV dtls=1 debug_level=2 \
             crt_file=data_files/server7_int-ca.crt \
             key_file=data_files/server7.key \
             mtu=512 force_version=dtls1_2" \
            "$O_CLI -dtls1_2" \
            0 \
            -s "fragmenting handshake message"

requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
requires_config_enabled MBEDTLS_SSL_PROTO_TLS1_1
run_test    "DTLS fragmenting: openssl client, DTLS 1.0" \
            "$P_SRV dtls=1 debug_level=2 \
             crt_file=data_files/server7_int-ca.crt \
             key_file=data_files/server7.key \
             mtu=512 force_version=dtls1" \
            "$O_CLI -dtls1" \
            0 \
            -s "fragmenting handshake message"

# interop tests for DTLS fragmentating with unreliable connection
#
# again we just want to test that the we fragment in a way that
# pleases other implementations, so we don't need the peer to fragment
requires_gnutls_next
requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
requires_config_enabled MBEDTLS_SSL_PROTO_TLS1_2
client_needs_more_time 4
run_test    "DTLS fragmenting: 3d, gnutls server, DTLS 1.2" \
            -p "$P_PXY drop=8 delay=8 duplicate=8" \
            "$G_NEXT_SRV -u" \
            "$P_CLI dgram_packing=0 dtls=1 debug_level=2 \
             crt_file=data_files/server8_int-ca2.crt \
             key_file=data_files/server8.key \
             hs_timeout=250-60000 mtu=512 force_version=dtls1_2" \
            0 \
            -c "fragmenting handshake message" \
            -C "error"

requires_gnutls_next
requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
requires_config_enabled MBEDTLS_SSL_PROTO_TLS1_1
client_needs_more_time 4
run_test    "DTLS fragmenting: 3d, gnutls server, DTLS 1.0" \
            -p "$P_PXY drop=8 delay=8 duplicate=8" \
            "$G_NEXT_SRV -u" \
            "$P_CLI dgram_packing=0 dtls=1 debug_level=2 \
             crt_file=data_files/server8_int-ca2.crt \
             key_file=data_files/server8.key \
             hs_timeout=250-60000 mtu=512 force_version=dtls1" \
            0 \
            -c "fragmenting handshake message" \
            -C "error"

requires_gnutls_next
requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
requires_config_enabled MBEDTLS_SSL_PROTO_TLS1_2
client_needs_more_time 4
run_test    "DTLS fragmenting: 3d, gnutls client, DTLS 1.2" \
            -p "$P_PXY drop=8 delay=8 duplicate=8" \
            "$P_SRV dtls=1 debug_level=2 \
             crt_file=data_files/server7_int-ca.crt \
             key_file=data_files/server7.key \
             hs_timeout=250-60000 mtu=512 force_version=dtls1_2" \
           "$G_NEXT_CLI -u --insecure 127.0.0.1" \
            0 \
            -s "fragmenting handshake message"

requires_gnutls_next
requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
requires_config_enabled MBEDTLS_SSL_PROTO_TLS1_1
client_needs_more_time 4
run_test    "DTLS fragmenting: 3d, gnutls client, DTLS 1.0" \
            -p "$P_PXY drop=8 delay=8 duplicate=8" \
            "$P_SRV dtls=1 debug_level=2 \
             crt_file=data_files/server7_int-ca.crt \
             key_file=data_files/server7.key \
             hs_timeout=250-60000 mtu=512 force_version=dtls1" \
           "$G_NEXT_CLI -u --insecure 127.0.0.1" \
            0 \
            -s "fragmenting handshake message"

## Interop test with OpenSSL might trigger a bug in recent versions (including
## all versions installed on the CI machines), reported here:
## Bug report: https://github.com/openssl/openssl/issues/6902
## They should be re-enabled once a fixed version of OpenSSL is available
## (this should happen in some 1.1.1_ release according to the ticket).
skip_next_test
requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
requires_config_enabled MBEDTLS_SSL_PROTO_TLS1_2
client_needs_more_time 4
run_test    "DTLS fragmenting: 3d, openssl server, DTLS 1.2" \
            -p "$P_PXY drop=8 delay=8 duplicate=8" \
            "$O_SRV -dtls1_2 -verify 10" \
            "$P_CLI dtls=1 debug_level=2 \
             crt_file=data_files/server8_int-ca2.crt \
             key_file=data_files/server8.key \
             hs_timeout=250-60000 mtu=512 force_version=dtls1_2" \
            0 \
            -c "fragmenting handshake message" \
            -C "error"

skip_next_test
requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
requires_config_enabled MBEDTLS_SSL_PROTO_TLS1_1
client_needs_more_time 4
run_test    "DTLS fragmenting: 3d, openssl server, DTLS 1.0" \
            -p "$P_PXY drop=8 delay=8 duplicate=8" \
            "$O_SRV -dtls1 -verify 10" \
            "$P_CLI dgram_packing=0 dtls=1 debug_level=2 \
             crt_file=data_files/server8_int-ca2.crt \
             key_file=data_files/server8.key \
             hs_timeout=250-60000 mtu=512 force_version=dtls1" \
            0 \
            -c "fragmenting handshake message" \
            -C "error"

skip_next_test
requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
requires_config_enabled MBEDTLS_SSL_PROTO_TLS1_2
client_needs_more_time 4
run_test    "DTLS fragmenting: 3d, openssl client, DTLS 1.2" \
            -p "$P_PXY drop=8 delay=8 duplicate=8" \
            "$P_SRV dtls=1 debug_level=2 \
             crt_file=data_files/server7_int-ca.crt \
             key_file=data_files/server7.key \
             hs_timeout=250-60000 mtu=512 force_version=dtls1_2" \
            "$O_CLI -dtls1_2" \
            0 \
            -s "fragmenting handshake message"

# -nbio is added to prevent s_client from blocking in case of duplicated
# messages at the end of the handshake
skip_next_test
requires_config_enabled MBEDTLS_SSL_PROTO_DTLS
requires_config_enabled MBEDTLS_RSA_C
requires_config_enabled MBEDTLS_ECDSA_C
requires_config_enabled MBEDTLS_SSL_PROTO_TLS1_1
client_needs_more_time 4
run_test    "DTLS fragmenting: 3d, openssl client, DTLS 1.0" \
            -p "$P_PXY drop=8 delay=8 duplicate=8" \
            "$P_SRV dgram_packing=0 dtls=1 debug_level=2 \
             crt_file=data_files/server7_int-ca.crt \
             key_file=data_files/server7.key \
             hs_timeout=250-60000 mtu=512 force_version=dtls1" \
            "$O_CLI -nbio -dtls1" \
            0 \
            -s "fragmenting handshake message"

# Tests for specific things with "unreliable" UDP connection

not_with_valgrind # spurious resend due to timeout
run_test    "DTLS proxy: reference" \
            -p "$P_PXY" \
            "$P_SRV dtls=1 debug_level=2" \
            "$P_CLI dtls=1 debug_level=2" \
            0 \
            -C "replayed record" \
            -S "replayed record" \
            -C "record from another epoch" \
            -S "record from another epoch" \
            -C "discarding invalid record" \
            -S "discarding invalid record" \
            -S "resend" \
            -s "Extra-header:" \
            -c "HTTP/1.0 200 OK"

not_with_valgrind # spurious resend due to timeout
run_test    "DTLS proxy: duplicate every packet" \
            -p "$P_PXY duplicate=1" \
            "$P_SRV dtls=1 dgram_packing=0 debug_level=2" \
            "$P_CLI dtls=1 dgram_packing=0 debug_level=2" \
            0 \
            -c "replayed record" \
            -s "replayed record" \
            -c "record from another epoch" \
            -s "record from another epoch" \
            -S "resend" \
            -s "Extra-header:" \
            -c "HTTP/1.0 200 OK"

run_test    "DTLS proxy: duplicate every packet, server anti-replay off" \
            -p "$P_PXY duplicate=1" \
            "$P_SRV dtls=1 dgram_packing=0 debug_level=2 anti_replay=0" \
            "$P_CLI dtls=1 dgram_packing=0 debug_level=2" \
            0 \
            -c "replayed record" \
            -S "replayed record" \
            -c "record from another epoch" \
            -s "record from another epoch" \
            -c "resend" \
            -s "resend" \
            -s "Extra-header:" \
            -c "HTTP/1.0 200 OK"

run_test    "DTLS proxy: multiple records in same datagram" \
            -p "$P_PXY pack=50" \
            "$P_SRV dtls=1 dgram_packing=0 debug_level=2" \
            "$P_CLI dtls=1 dgram_packing=0 debug_level=2" \
            0 \
            -c "next record in same datagram" \
            -s "next record in same datagram"

run_test    "DTLS proxy: multiple records in same datagram, duplicate every packet" \
            -p "$P_PXY pack=50 duplicate=1" \
            "$P_SRV dtls=1 dgram_packing=0 debug_level=2" \
            "$P_CLI dtls=1 dgram_packing=0 debug_level=2" \
            0 \
            -c "next record in same datagram" \
            -s "next record in same datagram"

run_test    "DTLS proxy: inject invalid AD record, default badmac_limit" \
            -p "$P_PXY bad_ad=1" \
            "$P_SRV dtls=1 dgram_packing=0 debug_level=1" \
            "$P_CLI dtls=1 dgram_packing=0 debug_level=1 read_timeout=100" \
            0 \
            -c "discarding invalid record (mac)" \
            -s "discarding invalid record (mac)" \
            -s "Extra-header:" \
            -c "HTTP/1.0 200 OK" \
            -S "too many records with bad MAC" \
            -S "Verification of the message MAC failed"

run_test    "DTLS proxy: inject invalid AD record, badmac_limit 1" \
            -p "$P_PXY bad_ad=1" \
            "$P_SRV dtls=1 dgram_packing=0 debug_level=1 badmac_limit=1" \
            "$P_CLI dtls=1 dgram_packing=0 debug_level=1 read_timeout=100" \
            1 \
            -C "discarding invalid record (mac)" \
            -S "discarding invalid record (mac)" \
            -S "Extra-header:" \
            -C "HTTP/1.0 200 OK" \
            -s "too many records with bad MAC" \
            -s "Verification of the message MAC failed"

run_test    "DTLS proxy: inject invalid AD record, badmac_limit 2" \
            -p "$P_PXY bad_ad=1" \
            "$P_SRV dtls=1 dgram_packing=0 debug_level=1 badmac_limit=2" \
            "$P_CLI dtls=1 dgram_packing=0 debug_level=1 read_timeout=100" \
            0 \
            -c "discarding invalid record (mac)" \
            -s "discarding invalid record (mac)" \
            -s "Extra-header:" \
            -c "HTTP/1.0 200 OK" \
            -S "too many records with bad MAC" \
            -S "Verification of the message MAC failed"

run_test    "DTLS proxy: inject invalid AD record, badmac_limit 2, exchanges 2"\
            -p "$P_PXY bad_ad=1" \
            "$P_SRV dtls=1 dgram_packing=0 debug_level=1 badmac_limit=2 exchanges=2" \
            "$P_CLI dtls=1 dgram_packing=0 debug_level=1 read_timeout=100 exchanges=2" \
            1 \
            -c "discarding invalid record (mac)" \
            -s "discarding invalid record (mac)" \
            -s "Extra-header:" \
            -c "HTTP/1.0 200 OK" \
            -s "too many records with bad MAC" \
            -s "Verification of the message MAC failed"

run_test    "DTLS proxy: delay ChangeCipherSpec" \
            -p "$P_PXY delay_ccs=1" \
            "$P_SRV dtls=1 debug_level=1 dgram_packing=0" \
            "$P_CLI dtls=1 debug_level=1 dgram_packing=0" \
            0 \
            -c "record from another epoch" \
            -s "record from another epoch" \
            -s "Extra-header:" \
            -c "HTTP/1.0 200 OK"

# Tests for reordering support with DTLS

run_test    "DTLS reordering: Buffer out-of-order handshake message on client" \
            -p "$P_PXY delay_srv=ServerHello" \
            "$P_SRV dgram_packing=0 cookies=0 dtls=1 debug_level=2 \
            hs_timeout=2500-60000" \
            "$P_CLI dgram_packing=0 dtls=1 debug_level=2 \
            hs_timeout=2500-60000" \
            0 \
            -c "Buffering HS message" \
            -c "Next handshake message has been buffered - load"\
            -S "Buffering HS message" \
            -S "Next handshake message has been buffered - load"\
            -C "Injecting buffered CCS message" \
            -C "Remember CCS message" \
            -S "Injecting buffered CCS message" \
            -S "Remember CCS message"

run_test    "DTLS reordering: Buffer out-of-order handshake message fragment on client" \
            -p "$P_PXY delay_srv=ServerHello" \
            "$P_SRV mtu=512 dgram_packing=0 cookies=0 dtls=1 debug_level=2 \
            hs_timeout=2500-60000" \
            "$P_CLI dgram_packing=0 dtls=1 debug_level=2 \
            hs_timeout=2500-60000" \
            0 \
            -c "Buffering HS message" \
            -c "found fragmented DTLS handshake message"\
            -c "Next handshake message 1 not or only partially bufffered" \
            -c "Next handshake message has been buffered - load"\
            -S "Buffering HS message" \
            -S "Next handshake message has been buffered - load"\
            -C "Injecting buffered CCS message" \
            -C "Remember CCS message" \
            -S "Injecting buffered CCS message" \
            -S "Remember CCS message"

# The client buffers the ServerKeyExchange before receiving the fragmented
# Certificate message; at the time of writing, together these are aroudn 1200b
# in size, so that the bound below ensures that the certificate can be reassembled
# while keeping the ServerKeyExchange.
requires_config_value_at_least "MBEDTLS_SSL_DTLS_MAX_BUFFERING" 1300
run_test    "DTLS reordering: Buffer out-of-order hs msg before reassembling next" \
            -p "$P_PXY delay_srv=Certificate delay_srv=Certificate" \
            "$P_SRV mtu=512 dgram_packing=0 cookies=0 dtls=1 debug_level=2 \
            hs_timeout=2500-60000" \
            "$P_CLI dgram_packing=0 dtls=1 debug_level=2 \
            hs_timeout=2500-60000" \
            0 \
            -c "Buffering HS message" \
            -c "Next handshake message has been buffered - load"\
            -C "attempt to make space by freeing buffered messages" \
            -S "Buffering HS message" \
            -S "Next handshake message has been buffered - load"\
            -C "Injecting buffered CCS message" \
            -C "Remember CCS message" \
            -S "Injecting buffered CCS message" \
            -S "Remember CCS message"

# The size constraints ensure that the delayed certificate message can't
# be reassembled while keeping the ServerKeyExchange message, but it can
# when dropping it first.
requires_config_value_at_least "MBEDTLS_SSL_DTLS_MAX_BUFFERING" 900
requires_config_value_at_most "MBEDTLS_SSL_DTLS_MAX_BUFFERING" 1299
run_test    "DTLS reordering: Buffer out-of-order hs msg before reassembling next, free buffered msg" \
            -p "$P_PXY delay_srv=Certificate delay_srv=Certificate" \
            "$P_SRV mtu=512 dgram_packing=0 cookies=0 dtls=1 debug_level=2 \
            hs_timeout=2500-60000" \
            "$P_CLI dgram_packing=0 dtls=1 debug_level=2 \
            hs_timeout=2500-60000" \
            0 \
            -c "Buffering HS message" \
            -c "attempt to make space by freeing buffered future messages" \
            -c "Enough space available after freeing buffered HS messages" \
            -S "Buffering HS message" \
            -S "Next handshake message has been buffered - load"\
            -C "Injecting buffered CCS message" \
            -C "Remember CCS message" \
            -S "Injecting buffered CCS message" \
            -S "Remember CCS message"

run_test    "DTLS reordering: Buffer out-of-order handshake message on server" \
            -p "$P_PXY delay_cli=Certificate" \
            "$P_SRV dgram_packing=0 auth_mode=required cookies=0 dtls=1 debug_level=2 \
            hs_timeout=2500-60000" \
            "$P_CLI dgram_packing=0 dtls=1 debug_level=2 \
            hs_timeout=2500-60000" \
            0 \
            -C "Buffering HS message" \
            -C "Next handshake message has been buffered - load"\
            -s "Buffering HS message" \
            -s "Next handshake message has been buffered - load" \
            -C "Injecting buffered CCS message" \
            -C "Remember CCS message" \
            -S "Injecting buffered CCS message" \
            -S "Remember CCS message"

run_test    "DTLS reordering: Buffer out-of-order CCS message on client"\
            -p "$P_PXY delay_srv=NewSessionTicket" \
            "$P_SRV dgram_packing=0 cookies=0 dtls=1 debug_level=2 \
            hs_timeout=2500-60000" \
            "$P_CLI dgram_packing=0 dtls=1 debug_level=2 \
            hs_timeout=2500-60000" \
            0 \
            -C "Buffering HS message" \
            -C "Next handshake message has been buffered - load"\
            -S "Buffering HS message" \
            -S "Next handshake message has been buffered - load" \
            -c "Injecting buffered CCS message" \
            -c "Remember CCS message" \
            -S "Injecting buffered CCS message" \
            -S "Remember CCS message"

run_test    "DTLS reordering: Buffer out-of-order CCS message on server"\
            -p "$P_PXY delay_cli=ClientKeyExchange" \
            "$P_SRV dgram_packing=0 cookies=0 dtls=1 debug_level=2 \
            hs_timeout=2500-60000" \
            "$P_CLI dgram_packing=0 dtls=1 debug_level=2 \
            hs_timeout=2500-60000" \
            0 \
            -C "Buffering HS message" \
            -C "Next handshake message has been buffered - load"\
            -S "Buffering HS message" \
            -S "Next handshake message has been buffered - load" \
            -C "Injecting buffered CCS message" \
            -C "Remember CCS message" \
            -s "Injecting buffered CCS message" \
            -s "Remember CCS message"

run_test    "DTLS reordering: Buffer encrypted Finished message" \
            -p "$P_PXY delay_ccs=1" \
            "$P_SRV dgram_packing=0 cookies=0 dtls=1 debug_level=2 \
            hs_timeout=2500-60000" \
            "$P_CLI dgram_packing=0 dtls=1 debug_level=2 \
            hs_timeout=2500-60000" \
            0 \
            -s "Buffer record from epoch 1" \
            -s "Found buffered record from current epoch - load" \
            -c "Buffer record from epoch 1" \
            -c "Found buffered record from current epoch - load"

# In this test, both the fragmented NewSessionTicket and the ChangeCipherSpec
# from the server are delayed, so that the encrypted Finished message
# is received and buffered. When the fragmented NewSessionTicket comes
# in afterwards, the encrypted Finished message must be freed in order
# to make space for the NewSessionTicket to be reassembled.
# This works only in very particular circumstances:
# - MBEDTLS_SSL_DTLS_MAX_BUFFERING must be large enough to allow buffering
#   of the NewSessionTicket, but small enough to also allow buffering of
#   the encrypted Finished message.
# - The MTU setting on the server must be so small that the NewSessionTicket
#   needs to be fragmented.
# - All messages sent by the server must be small enough to be either sent
#   without fragmentation or be reassembled within the bounds of
#   MBEDTLS_SSL_DTLS_MAX_BUFFERING. Achieve this by testing with a PSK-based
#   handshake, omitting CRTs.
requires_config_value_at_least "MBEDTLS_SSL_DTLS_MAX_BUFFERING" 240
requires_config_value_at_most "MBEDTLS_SSL_DTLS_MAX_BUFFERING" 280
run_test    "DTLS reordering: Buffer encrypted Finished message, drop for fragmented NewSessionTicket" \
            -p "$P_PXY delay_srv=NewSessionTicket delay_srv=NewSessionTicket delay_ccs=1" \
            "$P_SRV mtu=190 dgram_packing=0 psk=abc123 psk_identity=foo cookies=0 dtls=1 debug_level=2" \
            "$P_CLI dgram_packing=0 dtls=1 debug_level=2 force_ciphersuite=TLS-PSK-WITH-AES-128-CCM-8 psk=abc123 psk_identity=foo" \
            0 \
            -s "Buffer record from epoch 1" \
            -s "Found buffered record from current epoch - load" \
            -c "Buffer record from epoch 1" \
            -C "Found buffered record from current epoch - load" \
            -c "Enough space available after freeing future epoch record"

# Tests for "randomly unreliable connection": try a variety of flows and peers

client_needs_more_time 2
run_test    "DTLS proxy: 3d (drop, delay, duplicate), \"short\" PSK handshake" \
            -p "$P_PXY drop=5 delay=5 duplicate=5" \
            "$P_SRV dtls=1 dgram_packing=0 hs_timeout=500-10000 tickets=0 auth_mode=none \
             psk=abc123" \
            "$P_CLI dtls=1 dgram_packing=0 hs_timeout=500-10000 tickets=0 psk=abc123 \
             force_ciphersuite=TLS-PSK-WITH-AES-128-CCM-8" \
            0 \
            -s "Extra-header:" \
            -c "HTTP/1.0 200 OK"

client_needs_more_time 2
run_test    "DTLS proxy: 3d, \"short\" RSA handshake" \
            -p "$P_PXY drop=5 delay=5 duplicate=5" \
            "$P_SRV dtls=1 dgram_packing=0 hs_timeout=500-10000 tickets=0 auth_mode=none" \
            "$P_CLI dtls=1 dgram_packing=0 hs_timeout=500-10000 tickets=0 \
             force_ciphersuite=TLS-RSA-WITH-AES-128-CBC-SHA" \
            0 \
            -s "Extra-header:" \
            -c "HTTP/1.0 200 OK"

client_needs_more_time 2
run_test    "DTLS proxy: 3d, \"short\" (no ticket, no cli_auth) FS handshake" \
            -p "$P_PXY drop=5 delay=5 duplicate=5" \
            "$P_SRV dtls=1 dgram_packing=0 hs_timeout=500-10000 tickets=0 auth_mode=none" \
            "$P_CLI dtls=1 dgram_packing=0 hs_timeout=500-10000 tickets=0" \
            0 \
            -s "Extra-header:" \
            -c "HTTP/1.0 200 OK"

client_needs_more_time 2
run_test    "DTLS proxy: 3d, FS, client auth" \
            -p "$P_PXY drop=5 delay=5 duplicate=5" \
            "$P_SRV dtls=1 dgram_packing=0 hs_timeout=500-10000 tickets=0 auth_mode=required" \
            "$P_CLI dtls=1 dgram_packing=0 hs_timeout=500-10000 tickets=0" \
            0 \
            -s "Extra-header:" \
            -c "HTTP/1.0 200 OK"

client_needs_more_time 2
run_test    "DTLS proxy: 3d, FS, ticket" \
            -p "$P_PXY drop=5 delay=5 duplicate=5" \
            "$P_SRV dtls=1 dgram_packing=0 hs_timeout=500-10000 tickets=1 auth_mode=none" \
            "$P_CLI dtls=1 dgram_packing=0 hs_timeout=500-10000 tickets=1" \
            0 \
            -s "Extra-header:" \
            -c "HTTP/1.0 200 OK"

client_needs_more_time 2
run_test    "DTLS proxy: 3d, max handshake (FS, ticket + client auth)" \
            -p "$P_PXY drop=5 delay=5 duplicate=5" \
            "$P_SRV dtls=1 dgram_packing=0 hs_timeout=500-10000 tickets=1 auth_mode=required" \
            "$P_CLI dtls=1 dgram_packing=0 hs_timeout=500-10000 tickets=1" \
            0 \
            -s "Extra-header:" \
            -c "HTTP/1.0 200 OK"

client_needs_more_time 2
run_test    "DTLS proxy: 3d, max handshake, nbio" \
            -p "$P_PXY drop=5 delay=5 duplicate=5" \
            "$P_SRV dtls=1 dgram_packing=0 hs_timeout=500-10000 nbio=2 tickets=1 \
             auth_mode=required" \
            "$P_CLI dtls=1 dgram_packing=0 hs_timeout=500-10000 nbio=2 tickets=1" \
            0 \
            -s "Extra-header:" \
            -c "HTTP/1.0 200 OK"

client_needs_more_time 4
run_test    "DTLS proxy: 3d, min handshake, resumption" \
            -p "$P_PXY drop=5 delay=5 duplicate=5" \
            "$P_SRV dtls=1 dgram_packing=0 hs_timeout=500-10000 tickets=0 auth_mode=none \
             psk=abc123 debug_level=3" \
            "$P_CLI dtls=1 dgram_packing=0 hs_timeout=500-10000 tickets=0 psk=abc123 \
             debug_level=3 reconnect=1 read_timeout=1000 max_resend=10 \
             force_ciphersuite=TLS-PSK-WITH-AES-128-CCM-8" \
            0 \
            -s "a session has been resumed" \
            -c "a session has been resumed" \
            -s "Extra-header:" \
            -c "HTTP/1.0 200 OK"

client_needs_more_time 4
run_test    "DTLS proxy: 3d, min handshake, resumption, nbio" \
            -p "$P_PXY drop=5 delay=5 duplicate=5" \
            "$P_SRV dtls=1 dgram_packing=0 hs_timeout=500-10000 tickets=0 auth_mode=none \
             psk=abc123 debug_level=3 nbio=2" \
            "$P_CLI dtls=1 dgram_packing=0 hs_timeout=500-10000 tickets=0 psk=abc123 \
             debug_level=3 reconnect=1 read_timeout=1000 max_resend=10 \
             force_ciphersuite=TLS-PSK-WITH-AES-128-CCM-8 nbio=2" \
            0 \
            -s "a session has been resumed" \
            -c "a session has been resumed" \
            -s "Extra-header:" \
            -c "HTTP/1.0 200 OK"

client_needs_more_time 4
requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
run_test    "DTLS proxy: 3d, min handshake, client-initiated renego" \
            -p "$P_PXY drop=5 delay=5 duplicate=5" \
            "$P_SRV dtls=1 dgram_packing=0 hs_timeout=500-10000 tickets=0 auth_mode=none \
             psk=abc123 renegotiation=1 debug_level=2" \
            "$P_CLI dtls=1 dgram_packing=0 hs_timeout=500-10000 tickets=0 psk=abc123 \
             renegotiate=1 debug_level=2 \
             force_ciphersuite=TLS-PSK-WITH-AES-128-CCM-8" \
            0 \
            -c "=> renegotiate" \
            -s "=> renegotiate" \
            -s "Extra-header:" \
            -c "HTTP/1.0 200 OK"

client_needs_more_time 4
requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
run_test    "DTLS proxy: 3d, min handshake, client-initiated renego, nbio" \
            -p "$P_PXY drop=5 delay=5 duplicate=5" \
            "$P_SRV dtls=1 dgram_packing=0 hs_timeout=500-10000 tickets=0 auth_mode=none \
             psk=abc123 renegotiation=1 debug_level=2" \
            "$P_CLI dtls=1 dgram_packing=0 hs_timeout=500-10000 tickets=0 psk=abc123 \
             renegotiate=1 debug_level=2 \
             force_ciphersuite=TLS-PSK-WITH-AES-128-CCM-8" \
            0 \
            -c "=> renegotiate" \
            -s "=> renegotiate" \
            -s "Extra-header:" \
            -c "HTTP/1.0 200 OK"

client_needs_more_time 4
requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
run_test    "DTLS proxy: 3d, min handshake, server-initiated renego" \
            -p "$P_PXY drop=5 delay=5 duplicate=5" \
            "$P_SRV dtls=1 dgram_packing=0 hs_timeout=500-10000 tickets=0 auth_mode=none \
             psk=abc123 renegotiate=1 renegotiation=1 exchanges=4 \
             debug_level=2" \
            "$P_CLI dtls=1 dgram_packing=0 hs_timeout=500-10000 tickets=0 psk=abc123 \
             renegotiation=1 exchanges=4 debug_level=2 \
             force_ciphersuite=TLS-PSK-WITH-AES-128-CCM-8" \
            0 \
            -c "=> renegotiate" \
            -s "=> renegotiate" \
            -s "Extra-header:" \
            -c "HTTP/1.0 200 OK"

client_needs_more_time 4
requires_config_enabled MBEDTLS_SSL_RENEGOTIATION
run_test    "DTLS proxy: 3d, min handshake, server-initiated renego, nbio" \
            -p "$P_PXY drop=5 delay=5 duplicate=5" \
            "$P_SRV dtls=1 dgram_packing=0 hs_timeout=500-10000 tickets=0 auth_mode=none \
             psk=abc123 renegotiate=1 renegotiation=1 exchanges=4 \
             debug_level=2 nbio=2" \
            "$P_CLI dtls=1 dgram_packing=0 hs_timeout=500-10000 tickets=0 psk=abc123 \
             renegotiation=1 exchanges=4 debug_level=2 nbio=2 \
             force_ciphersuite=TLS-PSK-WITH-AES-128-CCM-8" \
            0 \
            -c "=> renegotiate" \
            -s "=> renegotiate" \
            -s "Extra-header:" \
            -c "HTTP/1.0 200 OK"

## Interop tests with OpenSSL might trigger a bug in recent versions (including
## all versions installed on the CI machines), reported here:
## Bug report: https://github.com/openssl/openssl/issues/6902
## They should be re-enabled once a fixed version of OpenSSL is available
## (this should happen in some 1.1.1_ release according to the ticket).
skip_next_test
client_needs_more_time 6
not_with_valgrind # risk of non-mbedtls peer timing out
run_test    "DTLS proxy: 3d, openssl server" \
            -p "$P_PXY drop=5 delay=5 duplicate=5 protect_hvr=1" \
            "$O_SRV -dtls1 -mtu 2048" \
            "$P_CLI dgram_packing=0 dtls=1 hs_timeout=500-60000 tickets=0" \
            0 \
            -c "HTTP/1.0 200 OK"

skip_next_test # see above
client_needs_more_time 8
not_with_valgrind # risk of non-mbedtls peer timing out
run_test    "DTLS proxy: 3d, openssl server, fragmentation" \
            -p "$P_PXY drop=5 delay=5 duplicate=5 protect_hvr=1" \
            "$O_SRV -dtls1 -mtu 768" \
            "$P_CLI dgram_packing=0 dtls=1 hs_timeout=500-60000 tickets=0" \
            0 \
            -c "HTTP/1.0 200 OK"

skip_next_test # see above
client_needs_more_time 8
not_with_valgrind # risk of non-mbedtls peer timing out
run_test    "DTLS proxy: 3d, openssl server, fragmentation, nbio" \
            -p "$P_PXY drop=5 delay=5 duplicate=5 protect_hvr=1" \
            "$O_SRV -dtls1 -mtu 768" \
            "$P_CLI dgram_packing=0 dtls=1 hs_timeout=500-60000 nbio=2 tickets=0" \
            0 \
            -c "HTTP/1.0 200 OK"

requires_gnutls
client_needs_more_time 6
not_with_valgrind # risk of non-mbedtls peer timing out
run_test    "DTLS proxy: 3d, gnutls server" \
            -p "$P_PXY drop=5 delay=5 duplicate=5" \
            "$G_SRV -u --mtu 2048 -a" \
            "$P_CLI dgram_packing=0 dtls=1 hs_timeout=500-60000" \
            0 \
            -s "Extra-header:" \
            -c "Extra-header:"

requires_gnutls_next
client_needs_more_time 8
not_with_valgrind # risk of non-mbedtls peer timing out
run_test    "DTLS proxy: 3d, gnutls server, fragmentation" \
            -p "$P_PXY drop=5 delay=5 duplicate=5" \
            "$G_NEXT_SRV -u --mtu 512" \
            "$P_CLI dgram_packing=0 dtls=1 hs_timeout=500-60000" \
            0 \
            -s "Extra-header:" \
            -c "Extra-header:"

requires_gnutls_next
client_needs_more_time 8
not_with_valgrind # risk of non-mbedtls peer timing out
run_test    "DTLS proxy: 3d, gnutls server, fragmentation, nbio" \
            -p "$P_PXY drop=5 delay=5 duplicate=5" \
            "$G_NEXT_SRV -u --mtu 512" \
            "$P_CLI dgram_packing=0 dtls=1 hs_timeout=500-60000 nbio=2" \
            0 \
            -s "Extra-header:" \
            -c "Extra-header:"

# Final report

echo "------------------------------------------------------------------------"

if [ $FAILS = 0 ]; then
    printf "PASSED"
else
    printf "FAILED"
fi
PASSES=$(( $TESTS - $FAILS ))
echo " ($PASSES / $TESTS tests ($SKIPS skipped))"

exit $FAILS
