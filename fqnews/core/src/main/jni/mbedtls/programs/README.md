Mbed TLS sample programs
========================

This subdirectory mostly contains sample programs that illustrate specific features of the library, as well as a few test and support programs.

## Symmetric cryptography (AES) examples

* [`aes/aescrypt2.c`](aes/aescrypt2.c): file encryption and authentication with a key derived from a low-entropy secret, demonstrating the low-level AES interface, the digest interface and HMAC.  
  Warning: this program illustrates how to use low-level functions in the library. It should not be taken as an example of how to build a secure encryption mechanism. To derive a key from a low-entropy secret such as a password, use a standard key stretching mechanism such as PBKDF2 (provided by the `pkcs5` module). To encrypt and authenticate data, use a standard mode such as GCM or CCM (both available as library module).

* [`aes/crypt_and_hash.c`](aes/crypt_and_hash.c): file encryption and authentication, demonstrating the generic cipher interface and the generic hash interface.

## Hash (digest) examples

* [`hash/generic_sum.c`](hash/generic_sum.c): file hash calculator and verifier, demonstrating the message digest (`md`) interface.

* [`hash/hello.c`](hash/hello.c): hello-world program for MD5.

## Public-key cryptography examples

### Generic public-key cryptography (`pk`) examples

* [`pkey/gen_key.c`](pkey/gen_key.c): generates a key for any of the supported public-key algorithms (RSA or ECC) and writes it to a file that can be used by the other pk sample programs.

* [`pkey/key_app.c`](pkey/key_app.c): loads a PEM or DER public key or private key file and dumps its content.

* [`pkey/key_app_writer.c`](pkey/key_app_writer.c): loads a PEM or DER public key or private key file and writes it to a new PEM or DER file.

* [`pkey/pk_encrypt.c`](pkey/pk_encrypt.c), [`pkey/pk_decrypt.c`](pkey/pk_decrypt.c): loads a PEM or DER public/private key file and uses the key to encrypt/decrypt a short string through the generic public-key interface.

* [`pkey/pk_sign.c`](pkey/pk_sign.c), [`pkey/pk_verify.c`](pkey/pk_verify.c): loads a PEM or DER private/public key file and uses the key to sign/verify a short string.

### ECDSA and RSA signature examples

* [`pkey/ecdsa.c`](pkey/ecdsa.c): generates an ECDSA key, signs a fixed message and verifies the signature.

* [`pkey/rsa_encrypt.c`](pkey/rsa_encrypt.c), [`pkey/rsa_decrypt.c`](pkey/rsa_decrypt.c): loads an RSA public/private key and uses it to encrypt/decrypt a short string through the low-level RSA interface.

* [`pkey/rsa_genkey.c`](pkey/rsa_genkey.c): generates an RSA key and writes it to a file that can be used with the other RSA sample programs.

* [`pkey/rsa_sign.c`](pkey/rsa_sign.c), [`pkey/rsa_verify.c`](pkey/rsa_verify.c): loads an RSA private/public key and uses it to sign/verify a short string with the RSA PKCS#1 v1.5 algorithm.

* [`pkey/rsa_sign_pss.c`](pkey/rsa_sign_pss.c), [`pkey/rsa_verify_pss.c`](pkey/rsa_verify_pss.c): loads an RSA private/public key and uses it to sign/verify a short string with the RSASSA-PSS algorithm.

### Diffie-Hellman key exchange examples

* [`pkey/dh_client.c`](pkey/dh_client.c), [`pkey/dh_server.c`](pkey/dh_server.c): secure channel demonstrators (client, server). This pair of programs illustrates how to set up a secure channel using RSA for authentication and Diffie-Hellman to generate a shared AES session key.

* [`pkey/ecdh_curve25519.c`](pkey/ecdh_curve25519.c): demonstration of a elliptic curve Diffie-Hellman (ECDH) key agreement.

### Bignum (`mpi`) usage examples

* [`pkey/dh_genprime.c`](pkey/dh_genprime.c): shows how to use the bignum (`mpi`) interface to generate Diffie-Hellman parameters.

* [`pkey/mpi_demo.c`](pkey/mpi_demo.c): demonstrates operations on big integers.

## Random number generator (RNG) examples

* [`random/gen_entropy.c`](random/gen_entropy.c): shows how to use the default entropy sources to generate random data.  
  Note: most applications should only use the entropy generator to seed a cryptographic pseudorandom generator, as illustrated by `random/gen_random_ctr_drbg.c`.

* [`random/gen_random_ctr_drbg.c`](random/gen_random_ctr_drbg.c): shows how to use the default entropy sources to seed a pseudorandom generator, and how to use the resulting random generator to generate random data.

* [`random/gen_random_havege.c`](random/gen_random_havege.c): demonstrates the HAVEGE entropy collector.

## SSL/TLS examples

### SSL/TLS sample applications

* [`ssl/dtls_client.c`](ssl/dtls_client.c): a simple DTLS client program, which sends one datagram to the server and reads one datagram in response.

* [`ssl/dtls_server.c`](ssl/dtls_server.c): a simple DTLS server program, which expects one datagram from the client and writes one datagram in response. This program supports DTLS cookies for hello verification.

* [`ssl/mini_client.c`](ssl/mini_client.c): a minimalistic SSL client, which sends a short string and disconnects. This is primarily intended as a benchmark; for a better example of a typical TLS client, see `ssl/ssl_client1.c`.

* [`ssl/ssl_client1.c`](ssl/ssl_client1.c): a simple HTTPS client that sends a fixed request and displays the response.

* [`ssl/ssl_fork_server.c`](ssl/ssl_fork_server.c): a simple HTTPS server using one process per client to send a fixed response. This program requires a Unix/POSIX environment implementing the `fork` system call.

* [`ssl/ssl_mail_client.c`](ssl/ssl_mail_client.c): a simple SMTP-over-TLS or SMTP-STARTTLS client. This client sends an email with fixed content.

* [`ssl/ssl_pthread_server.c`](ssl/ssl_pthread_server.c): a simple HTTPS server using one thread per client to send a fixed response. This program requires the pthread library.

* [`ssl/ssl_server.c`](ssl/ssl_server.c): a simple HTTPS server that sends a fixed response. It serves a single client at a time.

### SSL/TLS feature demonstrators

Note: unlike most of the other programs under the `programs/` directory, these two programs are not intended as a basis for writing an application. They combine most of the features supported by the library, and most applications require only a few features. To write a new application, we recommended that you start with `ssl_client1.c` or `ssl_server.c`, and then look inside `ssl/ssl_client2.c` or `ssl/ssl_server2.c` to see how to use the specific features that your application needs.

* [`ssl/ssl_client2.c`](ssl/ssl_client2.c): an HTTPS client that sends a fixed request and displays the response, with options to select TLS protocol features and Mbed TLS library features.

* [`ssl/ssl_server2.c`](ssl/ssl_server2.c): an HTTPS server that sends a fixed response, with options to select TLS protocol features and Mbed TLS library features.

In addition to providing options for testing client-side features, the `ssl_client2` program has options that allow you to trigger certain behaviors in the server. For example, there are options to select ciphersuites, or to force a renegotiation. These options are useful for testing the corresponding features in a TLS server. Likewise, `ssl_server2` has options to activate certain behaviors that are useful for testing a TLS client.

## Test utilities

* [`test/benchmark.c`](test/benchmark.c): benchmark for cryptographic algorithms.

* [`test/selftest.c`](test/selftest.c): runs the self-test function in each library module.

* [`test/ssl_cert_test.c`](test/ssl_cert_test.c): demonstrates how to verify X.509 certificates, and (for RSA keys only) how to check that each certificate matches the corresponding private key. This program requires some test data which is not provided.

* [`test/udp_proxy.c`](test/udp_proxy.c): a UDP proxy that can inject certain failures (delay, duplicate, drop). Useful for testing DTLS.

* [`test/zeroize.c`](test/zeroize.c): a test program for `mbedtls_platform_zeroize`, used by [`tests/scripts/test_zeroize.gdb`](tests/scripts/test_zeroize.gdb).

## Development utilities

* [`util/pem2der.c`](util/pem2der.c): a PEM to DER converter. Mbed TLS can read PEM files directly, but this utility can be useful for interacting with other tools or with minimal Mbed TLS builds that lack PEM support.

* [`util/strerror.c`](util/strerror.c): prints the error description corresponding to an integer status returned by an Mbed TLS function.

## X.509 certificate examples

* [`x509/cert_app.c`](x509/cert_app.c): connects to a TLS server and verifies its certificate chain.

* [`x509/cert_req.c`](x509/cert_req.c): generates a certificate signing request (CSR) for a private key.

* [`x509/cert_write.c`](x509/cert_write.c): signs a certificate signing request, or self-signs a certificate.

* [`x509/crl_app.c`](x509/crl_app.c): loads and dumps a certificate revocation list (CRL).

* [`x509/req_app.c`](x509/req_app.c): loads and dumps a certificate signing request (CSR).

