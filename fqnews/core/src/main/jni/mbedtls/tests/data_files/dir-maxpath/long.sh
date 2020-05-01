#!/bin/sh

set -eu

: ${OPENSSL:=openssl}
NB=20

OPT="-days 3653 -sha256"

# generate self-signed root
$OPENSSL ecparam -name prime256v1 -genkey -out 00.key
$OPENSSL req -new -x509 -subj "/C=UK/O=mbed TLS/CN=CA00" $OPT \
             -key 00.key -out 00.crt

# cXX.pem is the chain starting at XX
cp 00.crt c00.pem

# generate long chain
i=1
while [ $i -le $NB ]; do
    UP=$( printf "%02d" $((i-1)) )
    ME=$( printf "%02d" $i )

    $OPENSSL ecparam -name prime256v1 -genkey -out ${ME}.key
    $OPENSSL req -new -subj "/C=UK/O=mbed TLS/CN=CA${ME}" \
                 -key ${ME}.key -out ${ME}.csr
    $OPENSSL x509 -req -CA ${UP}.crt -CAkey ${UP}.key -set_serial 1 $OPT \
                  -extfile int.opensslconf -extensions int \
                  -in ${ME}.csr -out ${ME}.crt

    cat ${ME}.crt c${UP}.pem > c${ME}.pem

    rm ${ME}.csr
    i=$((i+1))
done
