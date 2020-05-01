This documents the X.509 CAs, certificates, and CRLS used for testing.

Certification authorities
-------------------------

There are two main CAs for use as trusted roots:
- test-ca.crt aka "C=NL, O=PolarSSL, CN=PolarSSL Test CA"
  uses a RSA-2048 key
  test-ca-sha1.crt and test-ca-sha256.crt use the same key, signed with
  different hashes.
- test-ca2*.crt aka "C=NL, O=PolarSSL, CN=Polarssl Test EC CA"
  uses an EC key with NIST P-384 (aka secp384r1)
  variants used to test the keyUsage extension
The files test-ca_cat12 and test-ca_cat21 contain them concatenated both ways.

Two intermediate CAs are signed by them:
- test-int-ca.crt "C=NL, O=PolarSSL, CN=PolarSSL Test Intermediate CA"
  uses RSA-4096, signed by test-ca2
    - test-int-ca-exp.crt is a copy that is expired
- test-int-ca2.crt "C=NL, O=PolarSSL, CN=PolarSSL Test Intermediate EC CA"
  uses an EC key with NIST P-384, signed by test-ca

A third intermediate CA is signed by test-int-ca2.crt:
- test-int-ca3.crt "C=UK, O=mbed TLS, CN=mbed TLS Test intermediate CA 3"
  uses an EC key with NIST P-256, signed by test-int-ca2

Finally, other CAs for specific purposes:
- enco-ca-prstr.pem: has its CN encoded as a printable string, but child cert
  enco-cert-utf8str.pem has its issuer's CN encoded as a UTF-8 string.
- test-ca-v1.crt: v1 "CA", signs
    server1-v1.crt: v1 "intermediate CA", signs
        server2-v1*.crt: EE cert (without of with chain in same file)
- keyUsage.decipherOnly.crt: has the decipherOnly keyUsage bit set

End-entity certificates
-----------------------

Short information fields:

- name or pattern
- issuing CA:   1   -> test-ca.crt
                2   -> test-ca2.crt
                I1  -> test-int-ca.crt
                I2  -> test-int-ca2.crt
                I3  -> test-int-ca3.crt
                O   -> other
- key type: R -> RSA, E -> EC
- C -> there is a CRL revoking this cert (see below)
- L -> CN=localhost (useful for local test servers)
- P1, P2 if the file includes parent (resp. parent + grandparent)
- free-form comments

List of certificates:

- cert_example_multi*.crt: 1/O R: subjectAltName
- cert_example_wildcard.crt: 1 R: wildcard in subject's CN
- cert_md*.crt, cert_sha*.crt: 1 R: signature hash
- cert_v1_with_ext.crt: 1 R: v1 with extensions (illegal)
- cli2.crt: 2 E: basic
- cli-rsa.key, cli-rsa-*.crt: RSA key used for test clients, signed by
  the RSA test CA.
- enco-cert-utf8str.pem: see enco-ca-prstr.pem above
- server1*.crt: 1* R C* P1*: misc *(server1-v1 see test-ca-v1.crt above)
    *CRL for: .cert_type.crt, .crt, .key_usage.crt, .v1.crt
    P1 only for _ca.crt
- server2-v1*.crt: O R: see test-ca-v1.crt above
- server2*.crt: 1 R L: misc
- server3.crt: 1 E L: EC cert signed by RSA CA
- server4.crt: 2 R L: RSA cert signed by EC CA
- server5*.crt: 2* E L: misc *(except -selfsigned and -ss-*)
    -sha*: hashes
    .eku*: extendeKeyUsage (cli/srv = www client/server, cs = codesign, etc)
    .ku*: keyUsage (ds = signatures, ke/ka = key exchange/agreement)
    .req*: CSR, not certificate
    -der*: trailing bytes in der (?)
    -badsign.crt: S5 with corrupted signature
    -expired.crt: S5 with "not after" date in the past
    -future.crt: S5 with "not before" date in the future
    -selfsigned.crt: Self-signed cert with S5 key
    -ss-expired.crt: Self-signed cert with S5 key, expired
    -ss-forgeca.crt: Copy of test-int-ca3 self-signed with S5 key
- server6-ss-child.crt: O E: "child" of non-CA server5-selfsigned
- server6.crt, server6.pem: 2 E L C: revoked
- server7.crt: I1 E L P1(usually): EC signed by RSA signed by EC
    -badsign.crt: S7 with corrupted signature + I1
    -expired.crt: S7 with "not after" date in the past + I1
    -future.crt: S7 with "not before" date in the future + I1
    _int-ca-exp.crt: S7 + expired I1
    _int-ca.crt: S7 + I1
    _int-ca_ca2.crt: S7 + I1 + 2
    _all_space.crt: S7 + I1 both with misplaced spaces (invalid PEM)
    _pem_space.crt: S7 with misplace space (invalid PEM) + I1
    _trailing_space.crt: S7 + I1 both with trainling space (valid PEM)
    _spurious_int-ca.crt: S7 + I2(spurious) + I1
- server8*.crt: I2 R L: RSA signed by EC signed by RSA (P1 for _int-ca2)
- server9*.crt: 1 R C* L P1*: signed using RSASSA-PSS
    *CRL for: 9.crt, -badsign, -with-ca (P1)
- server10.crt: I3 E L
    -badsign.crt: S10 with corrupted signature
    -bs_int3.pem: S10-badsign + I3
    _int3-bs.pem: S10 + I3-badsign
    _int3_int-ca2.crt: S10 + I3 + I2
    _int3_int-ca2_ca.crt: S10 + I3 + I2 + 1
    _int3_spurious_int-ca2.crt: S10 + I3 + I1(spurious) + I2

Certificate revocation lists
----------------------------

Signing CA in parentheses (same meaning as certificates).

- crl-ec-sha*.pem: (2) server6.crt
- crl-future.pem: (2) server6.crt + unknown
- crl-rsa-pss-*.pem: (1) server9{,badsign,with-ca}.crt + cert_sha384.crt + unknown
- crl.pem, crl_expired.pem: (1) server1{,.cert_type,.key_usage,.v1}.crt + unknown
- crl_md*.pem: crl_sha*.pem: (1) same as crl.pem
- crt_cat_*.pem: (1+2) concatenations in various orders:
    ec = crl-ec-sha256.pem, ecfut = crl-future.pem
    rsa = crl.pem, rsabadpem = same with pem error, rsaexp = crl_expired.pem

Note: crl_future would revoke server9 and cert_sha384.crt if signed by CA 1
      crl-rsa-pss* would revoke server6.crt if signed by CA 2

Generation
----------

Newer test files have been generated through commands in the Makefile. The
resulting files are committed to the repository so that the tests can
run without having to re-do the generation and so that the output is the
same for everyone (the generation process is randomized).

The origin of older certificates has not been recorded.
