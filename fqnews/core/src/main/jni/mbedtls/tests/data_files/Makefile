## This file contains a record of how some of the test data was
## generated. The final build products are committed to the repository
## as well to make sure that the test data is identical. You do not
## need to use this makefile unless you're extending mbed TLS's tests.

## Many data files were generated prior to the existence of this
## makefile, so the method of their generation was not recorded.

## Note that in addition to depending on the version of the data
## generation tool, many of the build outputs are randomized, so
## running this makefile twice would not produce the same results.

## Tools
OPENSSL ?= openssl
FAKETIME ?= faketime
MBEDTLS_CERT_WRITE ?= $(PWD)/../../programs/x509/cert_write
MBEDTLS_CERT_REQ ?= $(PWD)/../../programs/x509/cert_req

## Build the generated test data. Note that since the final outputs
## are committed to the repository, this target should do nothing on a
## fresh checkout. Furthermore, since the generation is randomized,
## re-running the same targets may result in differing files. The goal
## of this makefile is primarily to serve as a record of how the
## targets were generated in the first place.
default: all_final

all_intermediate := # temporary files
all_final := # files used by tests



################################################################
#### Generate certificates from existing keys
################################################################

test_ca_crt = test-ca.crt
test_ca_key_file_rsa = test-ca.key
test_ca_pwd_rsa = PolarSSLTest
test_ca_config_file = test-ca.opensslconf

test-ca.req.sha256: $(test_ca_key_file_rsa)
	$(MBEDTLS_CERT_REQ) output_file=$@ filename=$(test_ca_key_file_rsa) password=$(test_ca_pwd_rsa) subject_name="C=NL,O=PolarSSL,CN=PolarSSL Test CA" md=SHA256
all_intermediate += test-ca.req.sha256

test-ca.crt: $(test_ca_key_file_rsa) test-ca.req.sha256
	$(MBEDTLS_CERT_WRITE) is_ca=1 serial=3 request_file=test-ca.req.sha256 selfsign=1 issuer_name="C=NL,O=PolarSSL,CN=PolarSSL Test CA" issuer_key=$(test_ca_key_file_rsa) issuer_pwd=$(test_ca_pwd_rsa) not_before=20110212144400 not_after=20210212144400 md=SHA1 version=3 output_file=$@
all_final += test-ca.crt

test-ca-sha1.crt: $(test_ca_key_file_rsa) test-ca.req.sha256
	$(MBEDTLS_CERT_WRITE) is_ca=1 serial=3 request_file=test-ca.req.sha256 selfsign=1 issuer_name="C=NL,O=PolarSSL,CN=PolarSSL Test CA" issuer_key=$(test_ca_key_file_rsa) issuer_pwd=$(test_ca_pwd_rsa) not_before=20110212144400 not_after=20210212144400 md=SHA1 version=3 output_file=$@
all_final += test-ca-sha1.crt

test-ca-sha256.crt: $(test_ca_key_file_rsa) test-ca.req.sha256
	$(MBEDTLS_CERT_WRITE) is_ca=1 serial=3 request_file=test-ca.req.sha256 selfsign=1 issuer_name="C=NL,O=PolarSSL,CN=PolarSSL Test CA" issuer_key=$(test_ca_key_file_rsa) issuer_pwd=$(test_ca_pwd_rsa) not_before=20110212144400 not_after=20210212144400 md=SHA256 version=3 output_file=$@
all_final += test-ca-sha256.crt

test_ca_key_file_rsa_alt = test-ca-alt.key

$(test_ca_key_file_rsa_alt):
	$(OPENSSL) genrsa -out $@ 2048
test-ca-alt.csr: $(test_ca_key_file_rsa_alt) $(test_ca_config_file)
	$(OPENSSL) req -new -config $(test_ca_config_file) -key $(test_ca_key_file_rsa_alt) -subj "/C=NL/O=PolarSSL/CN=PolarSSL Test CA" -out $@
all_intermediate += test-ca-alt.csr
test-ca-alt.crt: $(test_ca_key_file_rsa_alt) $(test_ca_config_file) test-ca-alt.csr
	$(OPENSSL) req -x509 -config $(test_ca_config_file) -key $(test_ca_key_file_rsa_alt) -set_serial 0 -days 3653 -sha256 -in test-ca-alt.csr -out $@
all_final += test-ca-alt.crt
test-ca-alt-good.crt: test-ca-alt.crt test-ca-sha256.crt
	cat test-ca-alt.crt test-ca-sha256.crt > $@
all_final += test-ca-alt-good.crt
test-ca-good-alt.crt: test-ca-alt.crt test-ca-sha256.crt
	cat test-ca-sha256.crt test-ca-alt.crt > $@
all_final += test-ca-good-alt.crt

test_ca_crt_file_ec = test-ca2.crt
test_ca_key_file_ec = test-ca2.key

test_ca_crt_cat12 = test-ca_cat12.crt
$(test_ca_crt_cat12): $(test_ca_crt) $(test_ca_crt_file_ec)
	cat $(test_ca_crt) $(test_ca_crt_file_ec) > $@
all_final += $(test_ca_crt_cat12)

test_ca_crt_cat21 = test-ca_cat21.crt
$(test_ca_crt_cat21): $(test_ca_crt) $(test_ca_crt_file_ec)
	cat $(test_ca_crt_file_ec) $(test_ca_crt) > $@
all_final += $(test_ca_crt_cat21)

test-int-ca.csr: test-int-ca.key $(test_ca_config_file)
	$(OPENSSL) req -new -config $(test_ca_config_file) -key test-int-ca.key -subj "/C=NL/O=PolarSSL/CN=PolarSSL Test Intermediate CA" -out $@
all_intermediate += test-int-ca.csr
test-int-ca-exp.crt: $(test_ca_crt_file_ec) $(test_ca_key_file_ec) $(test_ca_config_file) test-int-ca.csr
	$(FAKETIME) -f -3653d $(OPENSSL) x509 -req -extfile $(test_ca_config_file) -extensions v3_ca -CA $(test_ca_crt_file_ec) -CAkey $(test_ca_key_file_ec) -set_serial 14 -days 3653 -sha256 -in test-int-ca.csr -out $@
all_final += test-int-ca-exp.crt

crl-idp.pem: $(test_ca_crt) $(test_ca_key_file_rsa) $(test_ca_config_file)
	$(OPENSSL) ca -gencrl -batch -cert $(test_ca_crt) -keyfile $(test_ca_key_file_rsa) -key $(test_ca_pwd_rsa) -config $(test_ca_config_file) -name test_ca -md sha256 -crldays 3653 -crlexts crl_ext_idp -out $@
all_final += crl-idp.pem
crl-idpnc.pem: $(test_ca_crt) $(test_ca_key_file_rsa) $(test_ca_config_file)
	$(OPENSSL) ca -gencrl -batch -cert $(test_ca_crt) -keyfile $(test_ca_key_file_rsa) -key $(test_ca_pwd_rsa) -config $(test_ca_config_file) -name test_ca -md sha256 -crldays 3653 -crlexts crl_ext_idp_nc -out $@
all_final += crl-idpnc.pem

cli_crt_key_file_rsa = cli-rsa.key
cli_crt_extensions_file = cli.opensslconf

cli-rsa.csr: $(cli_crt_key_file_rsa)
	$(MBEDTLS_CERT_REQ) output_file=$@ filename=$< subject_name="C=NL,O=PolarSSL,CN=PolarSSL Client 2" md=SHA1
all_intermediate += cli-rsa.csr

cli-rsa-sha1.crt: cli-rsa.csr
	$(MBEDTLS_CERT_WRITE) request_file=$< serial=4 issuer_crt=$(test_ca_crt) issuer_key=$(test_ca_key_file_rsa) issuer_pwd=$(test_ca_pwd_rsa) not_before=20110212144406 not_after=20210212144406 md=SHA1 version=3 output_file=$@

cli-rsa-sha256.crt: cli-rsa.csr
	$(MBEDTLS_CERT_WRITE) request_file=$< serial=4 issuer_crt=$(test_ca_crt) issuer_key=$(test_ca_key_file_rsa) issuer_pwd=$(test_ca_pwd_rsa) not_before=20110212144406 not_after=20210212144406 md=SHA256 version=3 output_file=$@
all_final += cli-rsa-sha256.crt

test_ca_int_rsa1 = test-int-ca.crt

server7.csr: server7.key
	$(OPENSSL) req -new -key server7.key -subj "/C=NL/O=PolarSSL/CN=localhost" -out $@
all_intermediate += server7.csr
server7-expired.crt: server7.csr $(test_ca_int_rsa1)
	$(FAKETIME) -f -3653d $(OPENSSL) x509 -req -extfile $(cli_crt_extensions_file) -extensions cli-rsa -CA $(test_ca_int_rsa1) -CAkey test-int-ca.key -set_serial 16 -days 3653 -sha256 -in server7.csr | cat - $(test_ca_int_rsa1) > $@
all_final += server7-expired.crt
server7-future.crt: server7.csr $(test_ca_int_rsa1)
	$(FAKETIME) -f +3653d $(OPENSSL) x509 -req -extfile $(cli_crt_extensions_file) -extensions cli-rsa -CA $(test_ca_int_rsa1) -CAkey test-int-ca.key -set_serial 16 -days 3653 -sha256 -in server7.csr | cat - $(test_ca_int_rsa1) > $@
all_final += server7-future.crt
server7-badsign.crt: server7.crt $(test_ca_int_rsa1)
	{ head -n-2 $<; tail -n-2 $< | sed -e '1s/0\(=*\)$$/_\1/' -e '1s/[^_=]\(=*\)$$/0\1/' -e '1s/_/1/'; cat $(test_ca_int_rsa1); } > $@
all_final += server7-badsign.crt
server7_int-ca-exp.crt: server7.crt test-int-ca-exp.crt
	cat server7.crt test-int-ca-exp.crt > $@
all_final += server7_int-ca-exp.crt

server5-ss-expired.crt: server5.key
	$(FAKETIME) -f -3653d $(OPENSSL) req -x509 -new -subj "/C=UK/O=mbed TLS/OU=testsuite/CN=localhost" -days 3653 -sha256 -key $< -out $@
all_final += server5-ss-expired.crt

# try to forge a copy of test-int-ca3 with different key
server5-ss-forgeca.crt: server5.key
	$(FAKETIME) '2015-09-01 14:08:43' $(OPENSSL) req -x509 -new -subj "/C=UK/O=mbed TLS/CN=mbed TLS Test intermediate CA 3" -set_serial 77 -config $(test_ca_config_file) -extensions noext_ca -days 3650 -sha256 -key $< -out $@
all_final += server5-ss-forgeca.crt

server10-badsign.crt: server10.crt
	{ head -n-2 $<; tail -n-2 $< | sed -e '1s/0\(=*\)$$/_\1/' -e '1s/[^_=]\(=*\)$$/0\1/' -e '1s/_/1/'; } > $@
all_final += server10-badsign.crt
server10-bs_int3.pem: server10-badsign.crt test-int-ca3.crt
	cat server10-badsign.crt test-int-ca3.crt > $@
all_final += server10-bs_int3.pem
test-int-ca3-badsign.crt: test-int-ca3.crt
	{ head -n-2 $<; tail -n-2 $< | sed -e '1s/0\(=*\)$$/_\1/' -e '1s/[^_=]\(=*\)$$/0\1/' -e '1s/_/1/'; } > $@
all_final += test-int-ca3-badsign.crt
server10_int3-bs.pem: server10.crt test-int-ca3-badsign.crt
	cat server10.crt test-int-ca3-badsign.crt > $@
all_final += server10-bs_int3-bs.pem

rsa_pkcs1_2048_public.pem: server8.key
	$(OPENSSL)  rsa -in $< -outform PEM -RSAPublicKey_out -out $@
all_final += rsa_pkcs1_2048_public.pem

rsa_pkcs1_2048_public.der: rsa_pkcs1_2048_public.pem
	$(OPENSSL) rsa -RSAPublicKey_in -in $< -outform DER -RSAPublicKey_out -out $@
all_final += rsa_pkcs1_2048_public.der

rsa_pkcs8_2048_public.pem: server8.key
	$(OPENSSL)  rsa -in $< -outform PEM -pubout -out $@
all_final += rsa_pkcs8_2048_public.pem

rsa_pkcs8_2048_public.der: rsa_pkcs8_2048_public.pem
	$(OPENSSL) rsa -pubin -in $< -outform DER -pubout -out $@
all_final += rsa_pkcs8_2048_public.der

################################################################
#### Generate various RSA keys
################################################################

### Password used for PKCS1-encoded encrypted RSA keys
keys_rsa_basic_pwd = testkey

### Password used for PKCS8-encoded encrypted RSA keys
keys_rsa_pkcs8_pwd = PolarSSLTest

### Basic 1024-, 2048- and 4096-bit unencrypted RSA keys from which
### all other encrypted RSA keys are derived.
rsa_pkcs1_1024_clear.pem:
	$(OPENSSL) genrsa -out $@ 1024
all_final += rsa_pkcs1_1024_clear.pem
rsa_pkcs1_2048_clear.pem:
	$(OPENSSL) genrsa -out $@ 2048
all_final += rsa_pkcs1_2048_clear.pem
rsa_pkcs1_4096_clear.pem:
	$(OPENSSL) genrsa -out $@ 4096
all_final += rsa_pkcs1_4096_clear.pem

###
### PKCS1-encoded, encrypted RSA keys
###

### 1024-bit
rsa_pkcs1_1024_des.pem: rsa_pkcs1_1024_clear.pem
	$(OPENSSL) rsa -des -in $< -out $@ -passout "pass:$(keys_rsa_basic_pwd)"
all_final += rsa_pkcs1_1024_des.pem
rsa_pkcs1_1024_3des.pem: rsa_pkcs1_1024_clear.pem
	$(OPENSSL) rsa -des3 -in $< -out $@ -passout "pass:$(keys_rsa_basic_pwd)"
all_final += rsa_pkcs1_1024_3des.pem
rsa_pkcs1_1024_aes128.pem: rsa_pkcs1_1024_clear.pem
	$(OPENSSL) rsa -aes128 -in $< -out $@ -passout "pass:$(keys_rsa_basic_pwd)"
all_final += rsa_pkcs1_1024_aes128.pem
rsa_pkcs1_1024_aes192.pem: rsa_pkcs1_1024_clear.pem
	$(OPENSSL) rsa -aes192 -in $< -out $@ -passout "pass:$(keys_rsa_basic_pwd)"
all_final += rsa_pkcs1_1024_aes192.pem
rsa_pkcs1_1024_aes256.pem: rsa_pkcs1_1024_clear.pem
	$(OPENSSL) rsa -aes256 -in $< -out $@ -passout "pass:$(keys_rsa_basic_pwd)"
all_final += rsa_pkcs1_1024_aes256.pem
keys_rsa_enc_basic_1024: rsa_pkcs1_1024_des.pem rsa_pkcs1_1024_3des.pem rsa_pkcs1_1024_aes128.pem rsa_pkcs1_1024_aes192.pem rsa_pkcs1_1024_aes256.pem

# 2048-bit
rsa_pkcs1_2048_des.pem: rsa_pkcs1_2048_clear.pem
	$(OPENSSL) rsa -des -in $< -out $@ -passout "pass:$(keys_rsa_basic_pwd)"
all_final += rsa_pkcs1_2048_des.pem
rsa_pkcs1_2048_3des.pem: rsa_pkcs1_2048_clear.pem
	$(OPENSSL) rsa -des3 -in $< -out $@ -passout "pass:$(keys_rsa_basic_pwd)"
all_final += rsa_pkcs1_2048_3des.pem
rsa_pkcs1_2048_aes128.pem: rsa_pkcs1_2048_clear.pem
	$(OPENSSL) rsa -aes128 -in $< -out $@ -passout "pass:$(keys_rsa_basic_pwd)"
all_final += rsa_pkcs1_2048_aes128.pem
rsa_pkcs1_2048_aes192.pem: rsa_pkcs1_2048_clear.pem
	$(OPENSSL) rsa -aes192 -in $< -out $@ -passout "pass:$(keys_rsa_basic_pwd)"
all_final += rsa_pkcs1_2048_aes192.pem
rsa_pkcs1_2048_aes256.pem: rsa_pkcs1_2048_clear.pem
	$(OPENSSL) rsa -aes256 -in $< -out $@ -passout "pass:$(keys_rsa_basic_pwd)"
all_final += rsa_pkcs1_2048_aes256.pem
keys_rsa_enc_basic_2048: rsa_pkcs1_2048_des.pem rsa_pkcs1_2048_3des.pem rsa_pkcs1_2048_aes128.pem rsa_pkcs1_2048_aes192.pem rsa_pkcs1_2048_aes256.pem

# 4096-bit
rsa_pkcs1_4096_des.pem: rsa_pkcs1_4096_clear.pem
	$(OPENSSL) rsa -des -in $< -out $@ -passout "pass:$(keys_rsa_basic_pwd)"
all_final += rsa_pkcs1_4096_des.pem
rsa_pkcs1_4096_3des.pem: rsa_pkcs1_4096_clear.pem
	$(OPENSSL) rsa -des3 -in $< -out $@ -passout "pass:$(keys_rsa_basic_pwd)"
all_final += rsa_pkcs1_4096_3des.pem
rsa_pkcs1_4096_aes128.pem: rsa_pkcs1_4096_clear.pem
	$(OPENSSL) rsa -aes128 -in $< -out $@ -passout "pass:$(keys_rsa_basic_pwd)"
all_final += rsa_pkcs1_4096_aes128.pem
rsa_pkcs1_4096_aes192.pem: rsa_pkcs1_4096_clear.pem
	$(OPENSSL) rsa -aes192 -in $< -out $@ -passout "pass:$(keys_rsa_basic_pwd)"
all_final += rsa_pkcs1_4096_aes192.pem
rsa_pkcs1_4096_aes256.pem: rsa_pkcs1_4096_clear.pem
	$(OPENSSL) rsa -aes256 -in $< -out $@ -passout "pass:$(keys_rsa_basic_pwd)"
all_final += rsa_pkcs1_4096_aes256.pem
keys_rsa_enc_basic_4096: rsa_pkcs1_4096_des.pem rsa_pkcs1_4096_3des.pem rsa_pkcs1_4096_aes128.pem rsa_pkcs1_4096_aes192.pem rsa_pkcs1_4096_aes256.pem

###
### PKCS8-v1 encoded, encrypted RSA keys
###

### 1024-bit
rsa_pkcs8_pbe_sha1_1024_3des.der: rsa_pkcs1_1024_clear.pem
	$(OPENSSL) pkcs8 -inform PEM -in $< -outform DER -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)" -topk8 -v1 PBE-SHA1-3DES
all_final += rsa_pkcs8_pbe_sha1_1024_3des.der
rsa_pkcs8_pbe_sha1_1024_3des.pem: rsa_pkcs1_1024_clear.pem
	$(OPENSSL) pkcs8 -inform PEM -in $< -outform PEM -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)" -topk8 -v1 PBE-SHA1-3DES
all_final += rsa_pkcs8_pbe_sha1_1024_3des.pem
keys_rsa_enc_pkcs8_v1_1024_3des: rsa_pkcs8_pbe_sha1_1024_3des.pem rsa_pkcs8_pbe_sha1_1024_3des.der

rsa_pkcs8_pbe_sha1_1024_2des.der: rsa_pkcs1_1024_clear.pem
	$(OPENSSL) pkcs8 -inform PEM -in $< -outform DER -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)" -topk8 -v1 PBE-SHA1-2DES
all_final += rsa_pkcs8_pbe_sha1_1024_2des.der
rsa_pkcs8_pbe_sha1_1024_2des.pem: rsa_pkcs1_1024_clear.pem
	$(OPENSSL) pkcs8 -inform PEM -in $< -outform PEM -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)" -topk8 -v1 PBE-SHA1-2DES
all_final += rsa_pkcs8_pbe_sha1_1024_2des.pem
keys_rsa_enc_pkcs8_v1_1024_2des: rsa_pkcs8_pbe_sha1_1024_2des.pem rsa_pkcs8_pbe_sha1_1024_2des.der

rsa_pkcs8_pbe_sha1_1024_rc4_128.der: rsa_pkcs1_1024_clear.pem
	$(OPENSSL) pkcs8 -inform PEM -in $< -outform DER -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)" -topk8 -v1 PBE-SHA1-RC4-128
all_final += rsa_pkcs8_pbe_sha1_1024_rc4_128.der
rsa_pkcs8_pbe_sha1_1024_rc4_128.pem: rsa_pkcs1_1024_clear.pem
	$(OPENSSL) pkcs8 -inform PEM -in $< -outform PEM -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)" -topk8 -v1 PBE-SHA1-RC4-128
all_final += rsa_pkcs8_pbe_sha1_1024_rc4_128.pem
keys_rsa_enc_pkcs8_v1_1024_rc4_128: rsa_pkcs8_pbe_sha1_1024_rc4_128.pem rsa_pkcs8_pbe_sha1_1024_rc4_128.der

keys_rsa_enc_pkcs8_v1_1024: keys_rsa_enc_pkcs8_v1_1024_3des keys_rsa_enc_pkcs8_v1_1024_2des keys_rsa_enc_pkcs8_v1_1024_rc4_128

### 2048-bit
rsa_pkcs8_pbe_sha1_2048_3des.der: rsa_pkcs1_2048_clear.pem
	$(OPENSSL) pkcs8 -inform PEM -in $< -outform DER -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)" -topk8 -v1 PBE-SHA1-3DES
all_final += rsa_pkcs8_pbe_sha1_2048_3des.der
rsa_pkcs8_pbe_sha1_2048_3des.pem: rsa_pkcs1_2048_clear.pem
	$(OPENSSL) pkcs8 -inform PEM -in $< -outform PEM -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)" -topk8 -v1 PBE-SHA1-3DES
all_final += rsa_pkcs8_pbe_sha1_2048_3des.pem
keys_rsa_enc_pkcs8_v1_2048_3des: rsa_pkcs8_pbe_sha1_2048_3des.pem rsa_pkcs8_pbe_sha1_2048_3des.der

rsa_pkcs8_pbe_sha1_2048_2des.der: rsa_pkcs1_2048_clear.pem
	$(OPENSSL) pkcs8 -inform PEM -in $< -outform DER -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)" -topk8 -v1 PBE-SHA1-2DES
all_final += rsa_pkcs8_pbe_sha1_2048_2des.der
rsa_pkcs8_pbe_sha1_2048_2des.pem: rsa_pkcs1_2048_clear.pem
	$(OPENSSL) pkcs8 -inform PEM -in $< -outform PEM -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)" -topk8 -v1 PBE-SHA1-2DES
all_final += rsa_pkcs8_pbe_sha1_2048_2des.pem
keys_rsa_enc_pkcs8_v1_2048_2des: rsa_pkcs8_pbe_sha1_2048_2des.pem rsa_pkcs8_pbe_sha1_2048_2des.der

rsa_pkcs8_pbe_sha1_2048_rc4_128.der: rsa_pkcs1_2048_clear.pem
	$(OPENSSL) pkcs8 -inform PEM -in $< -outform DER -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)" -topk8 -v1 PBE-SHA1-RC4-128
all_final += rsa_pkcs8_pbe_sha1_2048_rc4_128.der
rsa_pkcs8_pbe_sha1_2048_rc4_128.pem: rsa_pkcs1_2048_clear.pem
	$(OPENSSL) pkcs8 -inform PEM -in $< -outform PEM -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)" -topk8 -v1 PBE-SHA1-RC4-128
all_final += rsa_pkcs8_pbe_sha1_2048_rc4_128.pem
keys_rsa_enc_pkcs8_v1_2048_rc4_128: rsa_pkcs8_pbe_sha1_2048_rc4_128.pem rsa_pkcs8_pbe_sha1_2048_rc4_128.der

keys_rsa_enc_pkcs8_v1_2048: keys_rsa_enc_pkcs8_v1_2048_3des keys_rsa_enc_pkcs8_v1_2048_2des keys_rsa_enc_pkcs8_v1_2048_rc4_128

### 4096-bit
rsa_pkcs8_pbe_sha1_4096_3des.der: rsa_pkcs1_4096_clear.pem
	$(OPENSSL) pkcs8 -inform PEM -in $< -outform DER -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)" -topk8 -v1 PBE-SHA1-3DES
all_final += rsa_pkcs8_pbe_sha1_4096_3des.der
rsa_pkcs8_pbe_sha1_4096_3des.pem: rsa_pkcs1_4096_clear.pem
	$(OPENSSL) pkcs8 -inform PEM -in $< -outform PEM -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)" -topk8 -v1 PBE-SHA1-3DES
all_final += rsa_pkcs8_pbe_sha1_4096_3des.pem
keys_rsa_enc_pkcs8_v1_4096_3des: rsa_pkcs8_pbe_sha1_4096_3des.pem rsa_pkcs8_pbe_sha1_4096_3des.der

rsa_pkcs8_pbe_sha1_4096_2des.der: rsa_pkcs1_4096_clear.pem
	$(OPENSSL) pkcs8 -inform PEM -in $< -outform DER -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)" -topk8 -v1 PBE-SHA1-2DES
all_final += rsa_pkcs8_pbe_sha1_4096_2des.der
rsa_pkcs8_pbe_sha1_4096_2des.pem: rsa_pkcs1_4096_clear.pem
	$(OPENSSL) pkcs8 -inform PEM -in $< -outform PEM -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)" -topk8 -v1 PBE-SHA1-2DES
all_final += rsa_pkcs8_pbe_sha1_4096_2des.pem
keys_rsa_enc_pkcs8_v1_4096_2des: rsa_pkcs8_pbe_sha1_4096_2des.pem rsa_pkcs8_pbe_sha1_4096_2des.der

rsa_pkcs8_pbe_sha1_4096_rc4_128.der: rsa_pkcs1_4096_clear.pem
	$(OPENSSL) pkcs8 -inform PEM -in $< -outform DER -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)" -topk8 -v1 PBE-SHA1-RC4-128
all_final += rsa_pkcs8_pbe_sha1_4096_rc4_128.der
rsa_pkcs8_pbe_sha1_4096_rc4_128.pem: rsa_pkcs1_4096_clear.pem
	$(OPENSSL) pkcs8 -inform PEM -in $< -outform PEM -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)" -topk8 -v1 PBE-SHA1-RC4-128
all_final += rsa_pkcs8_pbe_sha1_4096_rc4_128.pem
keys_rsa_enc_pkcs8_v1_4096_rc4_128: rsa_pkcs8_pbe_sha1_4096_rc4_128.pem rsa_pkcs8_pbe_sha1_4096_rc4_128.der

keys_rsa_enc_pkcs8_v1_4096: keys_rsa_enc_pkcs8_v1_4096_3des keys_rsa_enc_pkcs8_v1_4096_2des keys_rsa_enc_pkcs8_v1_4096_rc4_128

###
### PKCS8-v2 encoded, encrypted RSA keys, no PRF specified (default for OpenSSL1.0: hmacWithSHA1)
###

### 1024-bit
rsa_pkcs8_pbes2_pbkdf2_1024_3des.der: rsa_pkcs1_1024_clear.pem
	$(OPENSSL) pkcs8 -topk8 -v2 des3 -inform PEM -in $< -outform DER -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_1024_3des.der
rsa_pkcs8_pbes2_pbkdf2_1024_3des.pem: rsa_pkcs1_1024_clear.pem
	$(OPENSSL) pkcs8  -topk8 -v2 des3 -inform PEM -in $< -outform PEM -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_1024_3des.pem
keys_rsa_enc_pkcs8_v2_1024_3des: rsa_pkcs8_pbes2_pbkdf2_1024_3des.der rsa_pkcs8_pbes2_pbkdf2_1024_3des.pem

rsa_pkcs8_pbes2_pbkdf2_1024_des.der: rsa_pkcs1_1024_clear.pem
	$(OPENSSL) pkcs8 -topk8 -v2 des -inform PEM -in $< -outform DER -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_1024_des.der
rsa_pkcs8_pbes2_pbkdf2_1024_des.pem: rsa_pkcs1_1024_clear.pem
	$(OPENSSL) pkcs8  -topk8 -v2 des -inform PEM -in $< -outform PEM -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_1024_des.pem
keys_rsa_enc_pkcs8_v2_1024_des: rsa_pkcs8_pbes2_pbkdf2_1024_des.der rsa_pkcs8_pbes2_pbkdf2_1024_des.pem

keys_rsa_enc_pkcs8_v2_1024: keys_rsa_enc_pkcs8_v2_1024_3des keys_rsa_enc_pkcs8_v2_1024_des

### 2048-bit
rsa_pkcs8_pbes2_pbkdf2_2048_3des.der: rsa_pkcs1_2048_clear.pem
	$(OPENSSL) pkcs8 -topk8 -v2 des3 -inform PEM -in $< -outform DER -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_2048_3des.der
rsa_pkcs8_pbes2_pbkdf2_2048_3des.pem: rsa_pkcs1_2048_clear.pem
	$(OPENSSL) pkcs8  -topk8 -v2 des3 -inform PEM -in $< -outform PEM -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_2048_3des.pem
keys_rsa_enc_pkcs8_v2_2048_3des: rsa_pkcs8_pbes2_pbkdf2_2048_3des.der rsa_pkcs8_pbes2_pbkdf2_2048_3des.pem

rsa_pkcs8_pbes2_pbkdf2_2048_des.der: rsa_pkcs1_2048_clear.pem
	$(OPENSSL) pkcs8 -topk8 -v2 des -inform PEM -in $< -outform DER -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_2048_des.der
rsa_pkcs8_pbes2_pbkdf2_2048_des.pem: rsa_pkcs1_2048_clear.pem
	$(OPENSSL) pkcs8  -topk8 -v2 des -inform PEM -in $< -outform PEM -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_2048_des.pem
keys_rsa_enc_pkcs8_v2_2048_des: rsa_pkcs8_pbes2_pbkdf2_2048_des.der rsa_pkcs8_pbes2_pbkdf2_2048_des.pem

keys_rsa_enc_pkcs8_v2_2048: keys_rsa_enc_pkcs8_v2_2048_3des keys_rsa_enc_pkcs8_v2_2048_des

### 4096-bit
rsa_pkcs8_pbes2_pbkdf2_4096_3des.der: rsa_pkcs1_4096_clear.pem
	$(OPENSSL) pkcs8 -topk8 -v2 des3 -inform PEM -in $< -outform DER -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_4096_3des.der
rsa_pkcs8_pbes2_pbkdf2_4096_3des.pem: rsa_pkcs1_4096_clear.pem
	$(OPENSSL) pkcs8  -topk8 -v2 des3 -inform PEM -in $< -outform PEM -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_4096_3des.pem
keys_rsa_enc_pkcs8_v2_4096_3des: rsa_pkcs8_pbes2_pbkdf2_4096_3des.der rsa_pkcs8_pbes2_pbkdf2_4096_3des.pem

rsa_pkcs8_pbes2_pbkdf2_4096_des.der: rsa_pkcs1_4096_clear.pem
	$(OPENSSL) pkcs8 -topk8 -v2 des -inform PEM -in $< -outform DER -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_4096_des.der
rsa_pkcs8_pbes2_pbkdf2_4096_des.pem: rsa_pkcs1_4096_clear.pem
	$(OPENSSL) pkcs8  -topk8 -v2 des -inform PEM -in $< -outform PEM -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_4096_des.pem
keys_rsa_enc_pkcs8_v2_4096_des: rsa_pkcs8_pbes2_pbkdf2_4096_des.der rsa_pkcs8_pbes2_pbkdf2_4096_des.pem

keys_rsa_enc_pkcs8_v2_4096: keys_rsa_enc_pkcs8_v2_4096_3des keys_rsa_enc_pkcs8_v2_4096_des

###
### PKCS8-v2 encoded, encrypted RSA keys, PRF hmacWithSHA224
###

### 1024-bit
rsa_pkcs8_pbes2_pbkdf2_1024_3des_sha224.der: rsa_pkcs1_1024_clear.pem
	$(OPENSSL) pkcs8 -topk8 -v2 des3 -v2prf hmacWithSHA224 -inform PEM -in $< -outform DER -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_1024_3des_sha224.der
rsa_pkcs8_pbes2_pbkdf2_1024_3des_sha224.pem: rsa_pkcs1_1024_clear.pem
	$(OPENSSL) pkcs8  -topk8 -v2 des3 -v2prf hmacWithSHA224 -inform PEM -in $< -outform PEM -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_1024_3des_sha224.pem
keys_rsa_enc_pkcs8_v2_1024_3des_sha224: rsa_pkcs8_pbes2_pbkdf2_1024_3des_sha224.der rsa_pkcs8_pbes2_pbkdf2_1024_3des_sha224.pem

rsa_pkcs8_pbes2_pbkdf2_1024_des_sha224.der: rsa_pkcs1_1024_clear.pem
	$(OPENSSL) pkcs8 -topk8 -v2 des -v2prf hmacWithSHA224 -inform PEM -in $< -outform DER -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_1024_des_sha224.der
rsa_pkcs8_pbes2_pbkdf2_1024_des_sha224.pem: rsa_pkcs1_1024_clear.pem
	$(OPENSSL) pkcs8  -topk8 -v2 des -v2prf hmacWithSHA224 -inform PEM -in $< -outform PEM -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_1024_des_sha224.pem
keys_rsa_enc_pkcs8_v2_1024_des_sha224: rsa_pkcs8_pbes2_pbkdf2_1024_des_sha224.der rsa_pkcs8_pbes2_pbkdf2_1024_des_sha224.pem

keys_rsa_enc_pkcs8_v2_1024_sha224: keys_rsa_enc_pkcs8_v2_1024_3des_sha224 keys_rsa_enc_pkcs8_v2_1024_des_sha224

### 2048-bit
rsa_pkcs8_pbes2_pbkdf2_2048_3des_sha224.der: rsa_pkcs1_2048_clear.pem
	$(OPENSSL) pkcs8 -topk8 -v2 des3 -v2prf hmacWithSHA224 -inform PEM -in $< -outform DER -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_2048_3des_sha224.der
rsa_pkcs8_pbes2_pbkdf2_2048_3des_sha224.pem: rsa_pkcs1_2048_clear.pem
	$(OPENSSL) pkcs8  -topk8 -v2 des3 -v2prf hmacWithSHA224 -inform PEM -in $< -outform PEM -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_2048_3des_sha224.pem
keys_rsa_enc_pkcs8_v2_2048_3des_sha224: rsa_pkcs8_pbes2_pbkdf2_2048_3des_sha224.der rsa_pkcs8_pbes2_pbkdf2_2048_3des_sha224.pem

rsa_pkcs8_pbes2_pbkdf2_2048_des_sha224.der: rsa_pkcs1_2048_clear.pem
	$(OPENSSL) pkcs8 -topk8 -v2 des -v2prf hmacWithSHA224 -inform PEM -in $< -outform DER -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_2048_des_sha224.der
rsa_pkcs8_pbes2_pbkdf2_2048_des_sha224.pem: rsa_pkcs1_2048_clear.pem
	$(OPENSSL) pkcs8  -topk8 -v2 des -v2prf hmacWithSHA224 -inform PEM -in $< -outform PEM -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_2048_des_sha224.pem
keys_rsa_enc_pkcs8_v2_2048_des_sha224: rsa_pkcs8_pbes2_pbkdf2_2048_des_sha224.der rsa_pkcs8_pbes2_pbkdf2_2048_des_sha224.pem

keys_rsa_enc_pkcs8_v2_2048_sha224: keys_rsa_enc_pkcs8_v2_2048_3des_sha224 keys_rsa_enc_pkcs8_v2_2048_des_sha224

### 4096-bit
rsa_pkcs8_pbes2_pbkdf2_4096_3des_sha224.der: rsa_pkcs1_4096_clear.pem
	$(OPENSSL) pkcs8 -topk8 -v2 des3 -v2prf hmacWithSHA224 -inform PEM -in $< -outform DER -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_4096_3des_sha224.der
rsa_pkcs8_pbes2_pbkdf2_4096_3des_sha224.pem: rsa_pkcs1_4096_clear.pem
	$(OPENSSL) pkcs8  -topk8 -v2 des3 -v2prf hmacWithSHA224 -inform PEM -in $< -outform PEM -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_4096_3des_sha224.pem
keys_rsa_enc_pkcs8_v2_4096_3des_sha224: rsa_pkcs8_pbes2_pbkdf2_4096_3des_sha224.der rsa_pkcs8_pbes2_pbkdf2_4096_3des_sha224.pem

rsa_pkcs8_pbes2_pbkdf2_4096_des_sha224.der: rsa_pkcs1_4096_clear.pem
	$(OPENSSL) pkcs8 -topk8 -v2 des -v2prf hmacWithSHA224 -inform PEM -in $< -outform DER -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_4096_des_sha224.der
rsa_pkcs8_pbes2_pbkdf2_4096_des_sha224.pem: rsa_pkcs1_4096_clear.pem
	$(OPENSSL) pkcs8  -topk8 -v2 des -v2prf hmacWithSHA224 -inform PEM -in $< -outform PEM -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_4096_des_sha224.pem
keys_rsa_enc_pkcs8_v2_4096_des_sha224: rsa_pkcs8_pbes2_pbkdf2_4096_des_sha224.der rsa_pkcs8_pbes2_pbkdf2_4096_des_sha224.pem

keys_rsa_enc_pkcs8_v2_4096_sha224: keys_rsa_enc_pkcs8_v2_4096_3des_sha224 keys_rsa_enc_pkcs8_v2_4096_des_sha224

###
### PKCS8-v2 encoded, encrypted RSA keys, PRF hmacWithSHA256
###

### 1024-bit
rsa_pkcs8_pbes2_pbkdf2_1024_3des_sha256.der: rsa_pkcs1_1024_clear.pem
	$(OPENSSL) pkcs8 -topk8 -v2 des3 -v2prf hmacWithSHA256 -inform PEM -in $< -outform DER -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_1024_3des_sha256.der
rsa_pkcs8_pbes2_pbkdf2_1024_3des_sha256.pem: rsa_pkcs1_1024_clear.pem
	$(OPENSSL) pkcs8  -topk8 -v2 des3 -v2prf hmacWithSHA256 -inform PEM -in $< -outform PEM -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_1024_3des_sha256.pem
keys_rsa_enc_pkcs8_v2_1024_3des_sha256: rsa_pkcs8_pbes2_pbkdf2_1024_3des_sha256.der rsa_pkcs8_pbes2_pbkdf2_1024_3des_sha256.pem

rsa_pkcs8_pbes2_pbkdf2_1024_des_sha256.der: rsa_pkcs1_1024_clear.pem
	$(OPENSSL) pkcs8 -topk8 -v2 des -v2prf hmacWithSHA256 -inform PEM -in $< -outform DER -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_1024_des_sha256.der
rsa_pkcs8_pbes2_pbkdf2_1024_des_sha256.pem: rsa_pkcs1_1024_clear.pem
	$(OPENSSL) pkcs8  -topk8 -v2 des -v2prf hmacWithSHA256 -inform PEM -in $< -outform PEM -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_1024_des_sha256.pem
keys_rsa_enc_pkcs8_v2_1024_des_sha256: rsa_pkcs8_pbes2_pbkdf2_1024_des_sha256.der rsa_pkcs8_pbes2_pbkdf2_1024_des_sha256.pem

keys_rsa_enc_pkcs8_v2_1024_sha256: keys_rsa_enc_pkcs8_v2_1024_3des_sha256 keys_rsa_enc_pkcs8_v2_1024_des_sha256

### 2048-bit
rsa_pkcs8_pbes2_pbkdf2_2048_3des_sha256.der: rsa_pkcs1_2048_clear.pem
	$(OPENSSL) pkcs8 -topk8 -v2 des3 -v2prf hmacWithSHA256 -inform PEM -in $< -outform DER -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_2048_3des_sha256.der
rsa_pkcs8_pbes2_pbkdf2_2048_3des_sha256.pem: rsa_pkcs1_2048_clear.pem
	$(OPENSSL) pkcs8  -topk8 -v2 des3 -v2prf hmacWithSHA256 -inform PEM -in $< -outform PEM -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_2048_3des_sha256.pem
keys_rsa_enc_pkcs8_v2_2048_3des_sha256: rsa_pkcs8_pbes2_pbkdf2_2048_3des_sha256.der rsa_pkcs8_pbes2_pbkdf2_2048_3des_sha256.pem

rsa_pkcs8_pbes2_pbkdf2_2048_des_sha256.der: rsa_pkcs1_2048_clear.pem
	$(OPENSSL) pkcs8 -topk8 -v2 des -v2prf hmacWithSHA256 -inform PEM -in $< -outform DER -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_2048_des_sha256.der
rsa_pkcs8_pbes2_pbkdf2_2048_des_sha256.pem: rsa_pkcs1_2048_clear.pem
	$(OPENSSL) pkcs8  -topk8 -v2 des -v2prf hmacWithSHA256 -inform PEM -in $< -outform PEM -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_2048_des_sha256.pem
keys_rsa_enc_pkcs8_v2_2048_des_sha256: rsa_pkcs8_pbes2_pbkdf2_2048_des_sha256.der rsa_pkcs8_pbes2_pbkdf2_2048_des_sha256.pem

keys_rsa_enc_pkcs8_v2_2048_sha256: keys_rsa_enc_pkcs8_v2_2048_3des_sha256 keys_rsa_enc_pkcs8_v2_2048_des_sha256

### 4096-bit
rsa_pkcs8_pbes2_pbkdf2_4096_3des_sha256.der: rsa_pkcs1_4096_clear.pem
	$(OPENSSL) pkcs8 -topk8 -v2 des3 -v2prf hmacWithSHA256 -inform PEM -in $< -outform DER -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_4096_3des_sha256.der
rsa_pkcs8_pbes2_pbkdf2_4096_3des_sha256.pem: rsa_pkcs1_4096_clear.pem
	$(OPENSSL) pkcs8  -topk8 -v2 des3 -v2prf hmacWithSHA256 -inform PEM -in $< -outform PEM -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_4096_3des_sha256.pem
keys_rsa_enc_pkcs8_v2_4096_3des_sha256: rsa_pkcs8_pbes2_pbkdf2_4096_3des_sha256.der rsa_pkcs8_pbes2_pbkdf2_4096_3des_sha256.pem

rsa_pkcs8_pbes2_pbkdf2_4096_des_sha256.der: rsa_pkcs1_4096_clear.pem
	$(OPENSSL) pkcs8 -topk8 -v2 des -v2prf hmacWithSHA256 -inform PEM -in $< -outform DER -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_4096_des_sha256.der
rsa_pkcs8_pbes2_pbkdf2_4096_des_sha256.pem: rsa_pkcs1_4096_clear.pem
	$(OPENSSL) pkcs8  -topk8 -v2 des -v2prf hmacWithSHA256 -inform PEM -in $< -outform PEM -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_4096_des_sha256.pem
keys_rsa_enc_pkcs8_v2_4096_des_sha256: rsa_pkcs8_pbes2_pbkdf2_4096_des_sha256.der rsa_pkcs8_pbes2_pbkdf2_4096_des_sha256.pem

keys_rsa_enc_pkcs8_v2_4096_sha256: keys_rsa_enc_pkcs8_v2_4096_3des_sha256 keys_rsa_enc_pkcs8_v2_4096_des_sha256

###
### PKCS8-v2 encoded, encrypted RSA keys, PRF hmacWithSHA384
###

### 1024-bit
rsa_pkcs8_pbes2_pbkdf2_1024_3des_sha384.der: rsa_pkcs1_1024_clear.pem
	$(OPENSSL) pkcs8 -topk8 -v2 des3 -v2prf hmacWithSHA384 -inform PEM -in $< -outform DER -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_1024_3des_sha384.der
rsa_pkcs8_pbes2_pbkdf2_1024_3des_sha384.pem: rsa_pkcs1_1024_clear.pem
	$(OPENSSL) pkcs8  -topk8 -v2 des3 -v2prf hmacWithSHA384 -inform PEM -in $< -outform PEM -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_1024_3des_sha384.pem
keys_rsa_enc_pkcs8_v2_1024_3des_sha384: rsa_pkcs8_pbes2_pbkdf2_1024_3des_sha384.der rsa_pkcs8_pbes2_pbkdf2_1024_3des_sha384.pem

rsa_pkcs8_pbes2_pbkdf2_1024_des_sha384.der: rsa_pkcs1_1024_clear.pem
	$(OPENSSL) pkcs8 -topk8 -v2 des -v2prf hmacWithSHA384 -inform PEM -in $< -outform DER -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_1024_des_sha384.der
rsa_pkcs8_pbes2_pbkdf2_1024_des_sha384.pem: rsa_pkcs1_1024_clear.pem
	$(OPENSSL) pkcs8  -topk8 -v2 des -v2prf hmacWithSHA384 -inform PEM -in $< -outform PEM -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_1024_des_sha384.pem
keys_rsa_enc_pkcs8_v2_1024_des_sha384: rsa_pkcs8_pbes2_pbkdf2_1024_des_sha384.der rsa_pkcs8_pbes2_pbkdf2_1024_des_sha384.pem

keys_rsa_enc_pkcs8_v2_1024_sha384: keys_rsa_enc_pkcs8_v2_1024_3des_sha384 keys_rsa_enc_pkcs8_v2_1024_des_sha384

### 2048-bit
rsa_pkcs8_pbes2_pbkdf2_2048_3des_sha384.der: rsa_pkcs1_2048_clear.pem
	$(OPENSSL) pkcs8 -topk8 -v2 des3 -v2prf hmacWithSHA384 -inform PEM -in $< -outform DER -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_2048_3des_sha384.der
rsa_pkcs8_pbes2_pbkdf2_2048_3des_sha384.pem: rsa_pkcs1_2048_clear.pem
	$(OPENSSL) pkcs8  -topk8 -v2 des3 -v2prf hmacWithSHA384 -inform PEM -in $< -outform PEM -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_2048_3des_sha384.pem
keys_rsa_enc_pkcs8_v2_2048_3des_sha384: rsa_pkcs8_pbes2_pbkdf2_2048_3des_sha384.der rsa_pkcs8_pbes2_pbkdf2_2048_3des_sha384.pem

rsa_pkcs8_pbes2_pbkdf2_2048_des_sha384.der: rsa_pkcs1_2048_clear.pem
	$(OPENSSL) pkcs8 -topk8 -v2 des -v2prf hmacWithSHA384 -inform PEM -in $< -outform DER -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_2048_des_sha384.der
rsa_pkcs8_pbes2_pbkdf2_2048_des_sha384.pem: rsa_pkcs1_2048_clear.pem
	$(OPENSSL) pkcs8  -topk8 -v2 des -v2prf hmacWithSHA384 -inform PEM -in $< -outform PEM -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_2048_des_sha384.pem
keys_rsa_enc_pkcs8_v2_2048_des_sha384: rsa_pkcs8_pbes2_pbkdf2_2048_des_sha384.der rsa_pkcs8_pbes2_pbkdf2_2048_des_sha384.pem

keys_rsa_enc_pkcs8_v2_2048_sha384: keys_rsa_enc_pkcs8_v2_2048_3des_sha384 keys_rsa_enc_pkcs8_v2_2048_des_sha384

### 4096-bit
rsa_pkcs8_pbes2_pbkdf2_4096_3des_sha384.der: rsa_pkcs1_4096_clear.pem
	$(OPENSSL) pkcs8 -topk8 -v2 des3 -v2prf hmacWithSHA384 -inform PEM -in $< -outform DER -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_4096_3des_sha384.der
rsa_pkcs8_pbes2_pbkdf2_4096_3des_sha384.pem: rsa_pkcs1_4096_clear.pem
	$(OPENSSL) pkcs8  -topk8 -v2 des3 -v2prf hmacWithSHA384 -inform PEM -in $< -outform PEM -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_4096_3des_sha384.pem
keys_rsa_enc_pkcs8_v2_4096_3des_sha384: rsa_pkcs8_pbes2_pbkdf2_4096_3des_sha384.der rsa_pkcs8_pbes2_pbkdf2_4096_3des_sha384.pem

rsa_pkcs8_pbes2_pbkdf2_4096_des_sha384.der: rsa_pkcs1_4096_clear.pem
	$(OPENSSL) pkcs8 -topk8 -v2 des -v2prf hmacWithSHA384 -inform PEM -in $< -outform DER -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_4096_des_sha384.der
rsa_pkcs8_pbes2_pbkdf2_4096_des_sha384.pem: rsa_pkcs1_4096_clear.pem
	$(OPENSSL) pkcs8  -topk8 -v2 des -v2prf hmacWithSHA384 -inform PEM -in $< -outform PEM -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_4096_des_sha384.pem
keys_rsa_enc_pkcs8_v2_4096_des_sha384: rsa_pkcs8_pbes2_pbkdf2_4096_des_sha384.der rsa_pkcs8_pbes2_pbkdf2_4096_des_sha384.pem

keys_rsa_enc_pkcs8_v2_4096_sha384: keys_rsa_enc_pkcs8_v2_4096_3des_sha384 keys_rsa_enc_pkcs8_v2_4096_des_sha384

###
### PKCS8-v2 encoded, encrypted RSA keys, PRF hmacWithSHA512
###

### 1024-bit
rsa_pkcs8_pbes2_pbkdf2_1024_3des_sha512.der: rsa_pkcs1_1024_clear.pem
	$(OPENSSL) pkcs8 -topk8 -v2 des3 -v2prf hmacWithSHA512 -inform PEM -in $< -outform DER -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_1024_3des_sha512.der
rsa_pkcs8_pbes2_pbkdf2_1024_3des_sha512.pem: rsa_pkcs1_1024_clear.pem
	$(OPENSSL) pkcs8  -topk8 -v2 des3 -v2prf hmacWithSHA512 -inform PEM -in $< -outform PEM -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_1024_3des_sha512.pem
keys_rsa_enc_pkcs8_v2_1024_3des_sha512: rsa_pkcs8_pbes2_pbkdf2_1024_3des_sha512.der rsa_pkcs8_pbes2_pbkdf2_1024_3des_sha512.pem

rsa_pkcs8_pbes2_pbkdf2_1024_des_sha512.der: rsa_pkcs1_1024_clear.pem
	$(OPENSSL) pkcs8 -topk8 -v2 des -v2prf hmacWithSHA512 -inform PEM -in $< -outform DER -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_1024_des_sha512.der
rsa_pkcs8_pbes2_pbkdf2_1024_des_sha512.pem: rsa_pkcs1_1024_clear.pem
	$(OPENSSL) pkcs8  -topk8 -v2 des -v2prf hmacWithSHA512 -inform PEM -in $< -outform PEM -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_1024_des_sha512.pem
keys_rsa_enc_pkcs8_v2_1024_des_sha512: rsa_pkcs8_pbes2_pbkdf2_1024_des_sha512.der rsa_pkcs8_pbes2_pbkdf2_1024_des_sha512.pem

keys_rsa_enc_pkcs8_v2_1024_sha512: keys_rsa_enc_pkcs8_v2_1024_3des_sha512 keys_rsa_enc_pkcs8_v2_1024_des_sha512

### 2048-bit
rsa_pkcs8_pbes2_pbkdf2_2048_3des_sha512.der: rsa_pkcs1_2048_clear.pem
	$(OPENSSL) pkcs8 -topk8 -v2 des3 -v2prf hmacWithSHA512 -inform PEM -in $< -outform DER -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_2048_3des_sha512.der
rsa_pkcs8_pbes2_pbkdf2_2048_3des_sha512.pem: rsa_pkcs1_2048_clear.pem
	$(OPENSSL) pkcs8  -topk8 -v2 des3 -v2prf hmacWithSHA512 -inform PEM -in $< -outform PEM -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_2048_3des_sha512.pem
keys_rsa_enc_pkcs8_v2_2048_3des_sha512: rsa_pkcs8_pbes2_pbkdf2_2048_3des_sha512.der rsa_pkcs8_pbes2_pbkdf2_2048_3des_sha512.pem

rsa_pkcs8_pbes2_pbkdf2_2048_des_sha512.der: rsa_pkcs1_2048_clear.pem
	$(OPENSSL) pkcs8 -topk8 -v2 des -v2prf hmacWithSHA512 -inform PEM -in $< -outform DER -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_2048_des_sha512.der
rsa_pkcs8_pbes2_pbkdf2_2048_des_sha512.pem: rsa_pkcs1_2048_clear.pem
	$(OPENSSL) pkcs8  -topk8 -v2 des -v2prf hmacWithSHA512 -inform PEM -in $< -outform PEM -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_2048_des_sha512.pem
keys_rsa_enc_pkcs8_v2_2048_des_sha512: rsa_pkcs8_pbes2_pbkdf2_2048_des_sha512.der rsa_pkcs8_pbes2_pbkdf2_2048_des_sha512.pem

keys_rsa_enc_pkcs8_v2_2048_sha512: keys_rsa_enc_pkcs8_v2_2048_3des_sha512 keys_rsa_enc_pkcs8_v2_2048_des_sha512

### 4096-bit
rsa_pkcs8_pbes2_pbkdf2_4096_3des_sha512.der: rsa_pkcs1_4096_clear.pem
	$(OPENSSL) pkcs8 -topk8 -v2 des3 -v2prf hmacWithSHA512 -inform PEM -in $< -outform DER -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_4096_3des_sha512.der
rsa_pkcs8_pbes2_pbkdf2_4096_3des_sha512.pem: rsa_pkcs1_4096_clear.pem
	$(OPENSSL) pkcs8  -topk8 -v2 des3 -v2prf hmacWithSHA512 -inform PEM -in $< -outform PEM -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_4096_3des_sha512.pem
keys_rsa_enc_pkcs8_v2_4096_3des_sha512: rsa_pkcs8_pbes2_pbkdf2_4096_3des_sha512.der rsa_pkcs8_pbes2_pbkdf2_4096_3des_sha512.pem

rsa_pkcs8_pbes2_pbkdf2_4096_des_sha512.der: rsa_pkcs1_4096_clear.pem
	$(OPENSSL) pkcs8 -topk8 -v2 des -v2prf hmacWithSHA512 -inform PEM -in $< -outform DER -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_4096_des_sha512.der
rsa_pkcs8_pbes2_pbkdf2_4096_des_sha512.pem: rsa_pkcs1_4096_clear.pem
	$(OPENSSL) pkcs8  -topk8 -v2 des -v2prf hmacWithSHA512 -inform PEM -in $< -outform PEM -out $@ -passout "pass:$(keys_rsa_pkcs8_pwd)"
all_final += rsa_pkcs8_pbes2_pbkdf2_4096_des_sha512.pem
keys_rsa_enc_pkcs8_v2_4096_des_sha512: rsa_pkcs8_pbes2_pbkdf2_4096_des_sha512.der rsa_pkcs8_pbes2_pbkdf2_4096_des_sha512.pem

keys_rsa_enc_pkcs8_v2_4096_sha512: keys_rsa_enc_pkcs8_v2_4096_3des_sha512 keys_rsa_enc_pkcs8_v2_4096_des_sha512

###
### Rules to generate all RSA keys from a particular class
###

### Generate basic unencrypted RSA keys
keys_rsa_unenc: rsa_pkcs1_1024_clear.pem rsa_pkcs1_2048_clear.pem rsa_pkcs1_4096_clear.pem

### Generate PKCS1-encoded encrypted RSA keys
keys_rsa_enc_basic: keys_rsa_enc_basic_1024 keys_rsa_enc_basic_2048 keys_rsa_enc_basic_4096

### Generate PKCS8-v1 encrypted RSA keys
keys_rsa_enc_pkcs8_v1: keys_rsa_enc_pkcs8_v1_1024 keys_rsa_enc_pkcs8_v1_2048 keys_rsa_enc_pkcs8_v1_4096

### Generate PKCS8-v2 encrypted RSA keys
keys_rsa_enc_pkcs8_v2: keys_rsa_enc_pkcs8_v2_1024 keys_rsa_enc_pkcs8_v2_2048 keys_rsa_enc_pkcs8_v2_4096 keys_rsa_enc_pkcs8_v2_1024_sha224 keys_rsa_enc_pkcs8_v2_2048_sha224 keys_rsa_enc_pkcs8_v2_4096_sha224 keys_rsa_enc_pkcs8_v2_1024_sha256 keys_rsa_enc_pkcs8_v2_2048_sha256 keys_rsa_enc_pkcs8_v2_4096_sha256 keys_rsa_enc_pkcs8_v2_1024_sha384 keys_rsa_enc_pkcs8_v2_2048_sha384 keys_rsa_enc_pkcs8_v2_4096_sha384 keys_rsa_enc_pkcs8_v2_1024_sha512 keys_rsa_enc_pkcs8_v2_2048_sha512 keys_rsa_enc_pkcs8_v2_4096_sha512

### Generate all RSA keys
keys_rsa_all: keys_rsa_unenc keys_rsa_enc_basic keys_rsa_enc_pkcs8_v1 keys_rsa_enc_pkcs8_v2

################################################################
#### Generate various EC keys
################################################################

###
### PKCS8 encoded
###

ec_prv.pk8.der:
	$(OPENSSL) genpkey -algorithm EC -pkeyopt ec_paramgen_curve:prime192v1 -pkeyopt ec_param_enc:named_curve -out $@ -outform DER
all_final += ec_prv.pk8.der

# ### Instructions for creating `ec_prv.pk8nopub.der`,
# ### `ec_prv.pk8nopubparam.der`, and `ec_prv.pk8param.der` by hand from
# ### `ec_prv.pk8.der`.
#
# These instructions assume you are familiar with ASN.1 DER encoding and can
# use a hex editor to manipulate DER.
#
# The relevant ASN.1 definitions for a PKCS#8 encoded Elliptic Curve key are:
#
# PrivateKeyInfo ::= SEQUENCE {
#   version                   Version,
#   privateKeyAlgorithm       PrivateKeyAlgorithmIdentifier,
#   privateKey                PrivateKey,
#   attributes           [0]  IMPLICIT Attributes OPTIONAL
# }
#
# AlgorithmIdentifier  ::=  SEQUENCE  {
#   algorithm   OBJECT IDENTIFIER,
#   parameters  ANY DEFINED BY algorithm OPTIONAL
# }
#
# ECParameters ::= CHOICE {
#   namedCurve         OBJECT IDENTIFIER
#   -- implicitCurve   NULL
#   -- specifiedCurve  SpecifiedECDomain
# }
#
# ECPrivateKey ::= SEQUENCE {
#   version        INTEGER { ecPrivkeyVer1(1) } (ecPrivkeyVer1),
#   privateKey     OCTET STRING,
#   parameters [0] ECParameters {{ NamedCurve }} OPTIONAL,
#   publicKey  [1] BIT STRING OPTIONAL
# }
#
# `ec_prv.pk8.der` as generatde above by OpenSSL should have the following
# fields:
#
# * privateKeyAlgorithm       namedCurve
# * privateKey.parameters     NOT PRESENT
# * privateKey.publicKey      PRESENT
# * attributes                NOT PRESENT
#
# # ec_prv.pk8nopub.der
#
# Take `ec_prv.pk8.der` and remove `privateKey.publicKey`.
#
# # ec_prv.pk8nopubparam.der
#
# Take `ec_prv.pk8nopub.der` and add `privateKey.parameters`, the same value as
# `privateKeyAlgorithm.namedCurve`. Don't forget to add the explicit tag.
#
# # ec_prv.pk8param.der
#
# Take `ec_prv.pk8.der` and add `privateKey.parameters`, the same value as
# `privateKeyAlgorithm.namedCurve`. Don't forget to add the explicit tag.

ec_prv.pk8.pem: ec_prv.pk8.der
	$(OPENSSL) pkey -in $< -inform DER -out $@
all_final += ec_prv.pk8.pem
ec_prv.pk8nopub.pem: ec_prv.pk8nopub.der
	$(OPENSSL) pkey -in $< -inform DER -out $@
all_final += ec_prv.pk8nopub.pem
ec_prv.pk8nopubparam.pem: ec_prv.pk8nopubparam.der
	$(OPENSSL) pkey -in $< -inform DER -out $@
all_final += ec_prv.pk8nopubparam.pem
ec_prv.pk8param.pem: ec_prv.pk8param.der
	$(OPENSSL) pkey -in $< -inform DER -out $@
all_final += ec_prv.pk8param.pem

# server5*

# The use of 'Server 1' in the DN is intentional here, as the DN is hardcoded in the x509_write test suite.'
server5.req.ku.sha1: server5.key
	$(MBEDTLS_CERT_REQ) output_file=$@ filename=$< key_usage=digital_signature,non_repudiation subject_name="C=NL,O=PolarSSL,CN=PolarSSL Server 1" md=SHA1
all_final += server5.req.ku.sha1

################################################################
### Generate CSRs for X.509 write test suite
################################################################

server1.req.cert_type: server1.key
	$(MBEDTLS_CERT_REQ) output_file=$@ filename=$< ns_cert_type=ssl_server subject_name="C=NL,O=PolarSSL,CN=PolarSSL Server 1" md=SHA1
all_final += server1.req.cert_type

server1.req.key_usage: server1.key
	$(MBEDTLS_CERT_REQ) output_file=$@ filename=$< key_usage=digital_signature,non_repudiation,key_encipherment subject_name="C=NL,O=PolarSSL,CN=PolarSSL Server 1" md=SHA1
all_final += server1.req.key_usage

server1.req.ku-ct: server1.key
	$(MBEDTLS_CERT_REQ) output_file=$@ filename=$< key_usage=digital_signature,non_repudiation,key_encipherment ns_cert_type=ssl_server subject_name="C=NL,O=PolarSSL,CN=PolarSSL Server 1" md=SHA1
all_final += server1.req.ku-ct

server1.req.key_usage_empty: server1.key
	$(MBEDTLS_CERT_REQ) output_file=$@ filename=$< subject_name="C=NL,O=PolarSSL,CN=PolarSSL Server 1" md=SHA1 force_key_usage=1
all_final += server1.req.key_usage_empty

server1.req.cert_type_empty: server1.key
	$(MBEDTLS_CERT_REQ) output_file=$@ filename=$< subject_name="C=NL,O=PolarSSL,CN=PolarSSL Server 1" md=SHA1 force_ns_cert_type=1
all_final += server1.req.cert_type_empty

################################################################
### Generate CSRs for X.509 write test suite
################################################################

server1.req.sha1: server1.key
	$(MBEDTLS_CERT_REQ) output_file=$@ filename=$< subject_name="C=NL,O=PolarSSL,CN=PolarSSL Server 1" md=SHA1
all_final += server1.req.sha1

server1.req.md4: server1.key
	$(MBEDTLS_CERT_REQ) output_file=$@ filename=$< subject_name="C=NL,O=PolarSSL,CN=PolarSSL Server 1" md=MD4
all_final += server1.req.md4

server1.req.md5: server1.key
	$(MBEDTLS_CERT_REQ) output_file=$@ filename=$< subject_name="C=NL,O=PolarSSL,CN=PolarSSL Server 1" md=MD5
all_final += server1.req.md5

server1.req.sha224: server1.key
	$(MBEDTLS_CERT_REQ) output_file=$@ filename=$< subject_name="C=NL,O=PolarSSL,CN=PolarSSL Server 1" md=SHA224
all_final += server1.req.sha224

server1.req.sha256: server1.key
	$(MBEDTLS_CERT_REQ) output_file=$@ filename=$< subject_name="C=NL,O=PolarSSL,CN=PolarSSL Server 1" md=SHA256
all_final += server1.req.sha256

server1.req.sha384: server1.key
	$(MBEDTLS_CERT_REQ) output_file=$@ filename=$< subject_name="C=NL,O=PolarSSL,CN=PolarSSL Server 1" md=SHA384
all_final += server1.req.sha384

server1.req.sha512: server1.key
	$(MBEDTLS_CERT_REQ) output_file=$@ filename=$< subject_name="C=NL,O=PolarSSL,CN=PolarSSL Server 1" md=SHA512
all_final += server1.req.sha512

server1.req.cert_type: server1.key
	$(MBEDTLS_CERT_REQ) output_file=$@ filename=$< ns_cert_type=ssl_server subject_name="C=NL,O=PolarSSL,CN=PolarSSL Server 1" md=SHA1
all_final += server1.req.cert_type

server1.req.key_usage: server1.key
	$(MBEDTLS_CERT_REQ) output_file=$@ filename=$< key_usage=digital_signature,non_repudiation,key_encipherment subject_name="C=NL,O=PolarSSL,CN=PolarSSL Server 1" md=SHA1
all_final += server1.req.key_usage

server1.req.ku-ct: server1.key
	$(MBEDTLS_CERT_REQ) output_file=$@ filename=$< key_usage=digital_signature,non_repudiation,key_encipherment ns_cert_type=ssl_server subject_name="C=NL,O=PolarSSL,CN=PolarSSL Server 1" md=SHA1
all_final += server1.req.ku-ct

# server2*

server2.req.sha256: server2.key
	$(MBEDTLS_CERT_REQ) output_file=$@ filename=$< subject_name="C=NL,O=PolarSSL,CN=localhost" md=SHA256
all_intermediate += server2.req.sha256

# server5*

# The use of 'Server 1' in the DN is intentional here, as the DN is hardcoded in the x509_write test suite.'
server5.req.ku.sha1: server5.key
	$(MBEDTLS_CERT_REQ) output_file=$@ filename=$< key_usage=digital_signature,non_repudiation subject_name="C=NL,O=PolarSSL,CN=PolarSSL Server 1" md=SHA1
all_final += server5.req.ku.sha1

################################################################
### Generate certificates for CRT write check tests
################################################################

### The test files use the Mbed TLS generated certificates server1*.crt,
### but for comparison with OpenSSL also rules for OpenSSL-generated
### certificates server1*.crt.openssl are offered.
###
### Known differences:
### * OpenSSL encodes trailing zero-bits in bit-strings occurring in X.509 extension
###   as unused bits, while Mbed TLS doesn't.

test_ca_server1_db = test-ca.server1.db
test_ca_server1_serial = test-ca.server1.serial
test_ca_server1_config_file = test-ca.server1.opensslconf

# server1*

server1.crt: server1.key server1.req.sha256 $(test_ca_crt) $(test_ca_key_file_rsa)
	$(MBEDTLS_CERT_WRITE) request_file=server1.req.sha256 issuer_crt=$(test_ca_crt) issuer_key=$(test_ca_key_file_rsa) issuer_pwd=$(test_ca_pwd_rsa) version=1 not_before=20110212144406 not_after=20210212144406 md=SHA1 version=3 output_file=$@
server1.noauthid.crt: server1.key server1.req.sha256 $(test_ca_crt) $(test_ca_key_file_rsa)
	$(MBEDTLS_CERT_WRITE) request_file=server1.req.sha256 issuer_crt=$(test_ca_crt) issuer_key=$(test_ca_key_file_rsa) issuer_pwd=$(test_ca_pwd_rsa) not_before=20110212144406 not_after=20210212144406 md=SHA1 authority_identifier=0 version=3 output_file=$@
server1.der: server1.crt
	$(OPENSSL) x509 -inform PEM -in $< -outform DER -out $@
all_final += server1.crt server1.noauthid.crt server1.der

server1.key_usage.crt: server1.key server1.req.sha256 $(test_ca_crt) $(test_ca_key_file_rsa)
	$(MBEDTLS_CERT_WRITE) request_file=server1.req.sha256 issuer_crt=$(test_ca_crt) issuer_key=$(test_ca_key_file_rsa) issuer_pwd=$(test_ca_pwd_rsa) version=1 not_before=20110212144406 not_after=20210212144406 md=SHA1 key_usage=digital_signature,non_repudiation,key_encipherment version=3 output_file=$@
server1.key_usage_noauthid.crt: server1.key server1.req.sha256 $(test_ca_crt) $(test_ca_key_file_rsa)
	$(MBEDTLS_CERT_WRITE) request_file=server1.req.sha256 issuer_crt=$(test_ca_crt) issuer_key=$(test_ca_key_file_rsa) issuer_pwd=$(test_ca_pwd_rsa) version=1 not_before=20110212144406 not_after=20210212144406 md=SHA1 key_usage=digital_signature,non_repudiation,key_encipherment authority_identifier=0 version=3 output_file=$@
server1.key_usage.der: server1.key_usage.crt
	$(OPENSSL) x509 -inform PEM -in $< -outform DER -out $@
all_final += server1.key_usage.crt server1.key_usage_noauthid.crt server1.key_usage.der

server1.cert_type.crt: server1.key server1.req.sha256 $(test_ca_crt) $(test_ca_key_file_rsa)
	$(MBEDTLS_CERT_WRITE) request_file=server1.req.sha256 issuer_crt=$(test_ca_crt) issuer_key=$(test_ca_key_file_rsa) issuer_pwd=$(test_ca_pwd_rsa) version=1 not_before=20110212144406 not_after=20210212144406 md=SHA1 ns_cert_type=ssl_server version=3 output_file=$@
server1.cert_type_noauthid.crt: server1.key server1.req.sha256 $(test_ca_crt) $(test_ca_key_file_rsa)
	$(MBEDTLS_CERT_WRITE) request_file=server1.req.sha256 issuer_crt=$(test_ca_crt) issuer_key=$(test_ca_key_file_rsa) issuer_pwd=$(test_ca_pwd_rsa) version=1 not_before=20110212144406 not_after=20210212144406 md=SHA1 ns_cert_type=ssl_server authority_identifier=0 version=3 output_file=$@
server1.cert_type.der: server1.cert_type.crt
	$(OPENSSL) x509 -inform PEM -in $< -outform DER -out $@
all_final += server1.cert_type.crt server1.cert_type_noauthid.crt server1.cert_type.der

server1.v1.crt: server1.key server1.req.sha256 $(test_ca_crt) $(test_ca_key_file_rsa)
	$(MBEDTLS_CERT_WRITE) request_file=server1.req.sha256 issuer_crt=$(test_ca_crt) issuer_key=$(test_ca_key_file_rsa) issuer_pwd=$(test_ca_pwd_rsa) version=1 not_before=20110212144406 not_after=20210212144406 md=SHA1 version=1 output_file=$@
server1.v1.der: server1.v1.crt
	$(OPENSSL) x509 -inform PEM -in $< -outform DER -out $@
all_final += server1.v1.crt server1.v1.der

# OpenSSL-generated certificates for comparison
# Also provide certificates in DER format to allow
# direct binary comparison using e.g. dumpasn1
server1.crt.openssl server1.key_usage.crt.openssl server1.cert_type.crt.openssl: server1.key server1.req.sha256 $(test_ca_crt) $(test_ca_key_file_rsa) $(test_ca_server1_config_file)
	echo "01" > $(test_ca_server1_serial)
	rm -f $(test_ca_server1_db)
	touch $(test_ca_server1_db)
	$(OPENSSL) ca -batch -passin "pass:$(test_ca_pwd_rsa)" -config $(test_ca_server1_config_file) -in server1.req.sha256 -extensions v3_ext -extfile $@.v3_ext -out $@
server1.der.openssl: server1.crt.openssl
	$(OPENSSL) x509 -inform PEM -in $< -outform DER -out $@
server1.key_usage.der.openssl: server1.key_usage.crt.openssl
	$(OPENSSL) x509 -inform PEM -in $< -outform DER -out $@
server1.cert_type.der.openssl: server1.cert_type.crt.openssl
	$(OPENSSL) x509 -inform PEM -in $< -outform DER -out $@

server1.v1.crt.openssl: server1.key server1.req.sha256 $(test_ca_crt) $(test_ca_key_file_rsa) $(test_ca_server1_config_file)
	echo "01" > $(test_ca_server1_serial)
	rm -f $(test_ca_server1_db)
	touch $(test_ca_server1_db)
	$(OPENSSL) ca -batch -passin "pass:$(test_ca_pwd_rsa)" -config $(test_ca_server1_config_file) -in server1.req.sha256 -out $@
server1.v1.der.openssl: server1.v1.crt.openssl
	$(OPENSSL) x509 -inform PEM -in $< -outform DER -out $@

server1_all: server1.crt server1.noauthid.crt server1.crt.openssl server1.v1.crt server1.v1.crt.openssl server1.key_usage.crt server1.key_usage_noauthid.crt server1.key_usage.crt.openssl server1.cert_type.crt server1.cert_type_noauthid.crt server1.cert_type.crt.openssl server1.der server1.der.openssl server1.v1.der server1.v1.der.openssl server1.key_usage.der server1.key_usage.der.openssl server1.cert_type.der server1.cert_type.der.openssl

# server2*

server2.crt: server2.req.sha256
	$(MBEDTLS_CERT_WRITE) request_file=server2.req.sha256 serial=2 issuer_crt=$(test_ca_crt) issuer_key=$(test_ca_key_file_rsa) issuer_pwd=$(test_ca_pwd_rsa) not_before=20110212144406 not_after=20210212144406 md=SHA1 version=3 output_file=$@
all_final += server2.crt

server2-sha256.crt: server2.req.sha256
	$(MBEDTLS_CERT_WRITE) request_file=server2.req.sha256 serial=2 issuer_crt=$(test_ca_crt) issuer_key=$(test_ca_key_file_rsa) issuer_pwd=$(test_ca_pwd_rsa) not_before=20110212144406 not_after=20210212144406 md=SHA256 version=3 output_file=$@
all_final += server2-sha256.crt



################################################################
#### Meta targets
################################################################

all_final: $(all_final)
all: $(all_intermediate) $(all_final)

.PHONY: default all_final all
.PHONY: keys_rsa_all
.PHONY: keys_rsa_unenc keys_rsa_enc_basic
.PHONY: keys_rsa_enc_pkcs8_v1 keys_rsa_enc_pkcs8_v2
.PHONY: keys_rsa_enc_basic_1024 keys_rsa_enc_basic_2048 keys_rsa_enc_basic_4096
.PHONY: keys_rsa_enc_pkcs8_v1_1024 keys_rsa_enc_pkcs8_v2_1024
.PHONY: keys_rsa_enc_pkcs8_v1_2048 keys_rsa_enc_pkcs8_v2_2048
.PHONY: keys_rsa_enc_pkcs8_v1_4096 keys_rsa_enc_pkcs8_v2_4096
.PHONY: server1_all

# These files should not be committed to the repository.
list_intermediate:
	@printf '%s\n' $(all_intermediate) | sort
# These files should be committed to the repository so that the test data is
# available upon checkout without running a randomized process depending on
# third-party tools.
list_final:
	@printf '%s\n' $(all_final) | sort
.PHONY: list_intermediate list_final

## Remove intermediate files
clean:
	rm -f $(all_intermediate)
## Remove all build products, even the ones that are committed
neat: clean
	rm -f $(all_final)
.PHONY: clean neat
