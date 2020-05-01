#!/bin/bash

result=0

function run_test {
    printf '\e[0;36m'
    echo "running test: $command $@"
    printf '\e[0m'

    $command "$@"
    status=$?
    if [ $status -ne 0 ]; then
        printf '\e[0;31m'
        echo "test failed: $command $@"
        printf '\e[0m'
        echo
        result=1
    else
        printf '\e[0;32m'
        echo OK
        printf '\e[0m'
        echo
    fi
    return 0
}

[ -d src -a -x src/ss-local ] &&
    BIN="--bin src/"

if [ "$http_proxy" ]; then
    echo "SKIP: shadowsocks-libev does not support an upstream HTTP proxy"
    exit 0
fi

run_test python tests/test.py $BIN -c tests/aes.json
run_test python tests/test.py $BIN -c tests/aes-gcm.json
run_test python tests/test.py $BIN -c tests/aes-ctr.json
run_test python tests/test.py $BIN -c tests/rc4-md5.json
run_test python tests/test.py $BIN -c tests/salsa20.json
run_test python tests/test.py $BIN -c tests/chacha20.json
run_test python tests/test.py $BIN -c tests/chacha20-ietf.json
run_test python tests/test.py $BIN -c tests/chacha20-ietf-poly1305.json

exit $result
