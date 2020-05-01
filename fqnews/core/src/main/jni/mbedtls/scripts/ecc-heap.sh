#!/bin/sh

# Measure heap usage (and performance) of ECC operations with various values of
# the relevant tunable compile-time parameters.
#
# Usage (preferably on a 32-bit platform):
# cmake -D CMAKE_BUILD_TYPE=Release .
# scripts/ecc-heap.sh | tee ecc-heap.log

set -eu

CONFIG_H='include/mbedtls/config.h'

if [ -r $CONFIG_H ]; then :; else
    echo "$CONFIG_H not found" >&2
    exit 1
fi

if grep -i cmake Makefile >/dev/null; then :; else
    echo "Needs Cmake" >&2
    exit 1
fi

if git status | grep -F $CONFIG_H >/dev/null 2>&1; then
    echo "config.h not clean" >&2
    exit 1
fi

CONFIG_BAK=${CONFIG_H}.bak
cp $CONFIG_H $CONFIG_BAK

cat << EOF >$CONFIG_H
#define MBEDTLS_PLATFORM_C
#define MBEDTLS_PLATFORM_MEMORY
#define MBEDTLS_MEMORY_BUFFER_ALLOC_C
#define MBEDTLS_MEMORY_DEBUG

#define MBEDTLS_TIMING_C

#define MBEDTLS_BIGNUM_C
#define MBEDTLS_ECP_C
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_ASN1_WRITE_C
#define MBEDTLS_ECDSA_C
#define MBEDTLS_ECDH_C

#define MBEDTLS_ECP_DP_SECP192R1_ENABLED
#define MBEDTLS_ECP_DP_SECP224R1_ENABLED
#define MBEDTLS_ECP_DP_SECP256R1_ENABLED
#define MBEDTLS_ECP_DP_SECP384R1_ENABLED
#define MBEDTLS_ECP_DP_SECP521R1_ENABLED
#define MBEDTLS_ECP_DP_CURVE25519_ENABLED

#include "check_config.h"

//#define MBEDTLS_ECP_WINDOW_SIZE            6
//#define MBEDTLS_ECP_FIXED_POINT_OPTIM      1
EOF

for F in 0 1; do
    for W in 2 3 4 5 6; do
        scripts/config.pl set MBEDTLS_ECP_WINDOW_SIZE $W
        scripts/config.pl set MBEDTLS_ECP_FIXED_POINT_OPTIM $F
        make benchmark >/dev/null 2>&1
        echo "fixed point optim = $F, max window size = $W"
        echo "--------------------------------------------"
        programs/test/benchmark
    done
done

# cleanup

mv $CONFIG_BAK $CONFIG_H
make clean
