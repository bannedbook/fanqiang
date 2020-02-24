# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

INCLUDES = """
#include <openssl/engine.h>
"""

TYPES = """
static const long Cryptography_HAS_ENGINE_CRYPTODEV;

typedef ... ENGINE;
typedef ... RSA_METHOD;
typedef ... DSA_METHOD;
typedef ... ECDH_METHOD;
typedef ... ECDSA_METHOD;
typedef ... DH_METHOD;
typedef ... RAND_METHOD;
typedef ... STORE_METHOD;
typedef ... *ENGINE_GEN_INT_FUNC_PTR;
typedef ... *ENGINE_CTRL_FUNC_PTR;
typedef ... *ENGINE_LOAD_KEY_PTR;
typedef ... *ENGINE_CIPHERS_PTR;
typedef ... *ENGINE_DIGESTS_PTR;
typedef ... ENGINE_CMD_DEFN;
typedef ... UI_METHOD;

static const unsigned int ENGINE_METHOD_RSA;
static const unsigned int ENGINE_METHOD_DSA;
static const unsigned int ENGINE_METHOD_RAND;
static const unsigned int ENGINE_METHOD_ECDH;
static const unsigned int ENGINE_METHOD_ECDSA;
static const unsigned int ENGINE_METHOD_CIPHERS;
static const unsigned int ENGINE_METHOD_DIGESTS;
static const unsigned int ENGINE_METHOD_STORE;
static const unsigned int ENGINE_METHOD_ALL;
static const unsigned int ENGINE_METHOD_NONE;
"""

FUNCTIONS = """
ENGINE *ENGINE_get_first(void);
ENGINE *ENGINE_get_last(void);
ENGINE *ENGINE_get_next(ENGINE *);
ENGINE *ENGINE_get_prev(ENGINE *);
int ENGINE_add(ENGINE *);
int ENGINE_remove(ENGINE *);
ENGINE *ENGINE_by_id(const char *);
int ENGINE_init(ENGINE *);
int ENGINE_finish(ENGINE *);
void ENGINE_load_openssl(void);
void ENGINE_load_dynamic(void);
void ENGINE_load_builtin_engines(void);
void ENGINE_cleanup(void);
ENGINE *ENGINE_get_default_RSA(void);
ENGINE *ENGINE_get_default_DSA(void);
ENGINE *ENGINE_get_default_ECDH(void);
ENGINE *ENGINE_get_default_ECDSA(void);
ENGINE *ENGINE_get_default_DH(void);
ENGINE *ENGINE_get_default_RAND(void);
ENGINE *ENGINE_get_cipher_engine(int);
ENGINE *ENGINE_get_digest_engine(int);
int ENGINE_set_default_RSA(ENGINE *);
int ENGINE_set_default_DSA(ENGINE *);
int ENGINE_set_default_ECDH(ENGINE *);
int ENGINE_set_default_ECDSA(ENGINE *);
int ENGINE_set_default_DH(ENGINE *);
int ENGINE_set_default_RAND(ENGINE *);
int ENGINE_set_default_ciphers(ENGINE *);
int ENGINE_set_default_digests(ENGINE *);
int ENGINE_set_default_string(ENGINE *, const char *);
int ENGINE_set_default(ENGINE *, unsigned int);
unsigned int ENGINE_get_table_flags(void);
void ENGINE_set_table_flags(unsigned int);
int ENGINE_register_RSA(ENGINE *);
void ENGINE_unregister_RSA(ENGINE *);
void ENGINE_register_all_RSA(void);
int ENGINE_register_DSA(ENGINE *);
void ENGINE_unregister_DSA(ENGINE *);
void ENGINE_register_all_DSA(void);
int ENGINE_register_ECDH(ENGINE *);
void ENGINE_unregister_ECDH(ENGINE *);
void ENGINE_register_all_ECDH(void);
int ENGINE_register_ECDSA(ENGINE *);
void ENGINE_unregister_ECDSA(ENGINE *);
void ENGINE_register_all_ECDSA(void);
int ENGINE_register_DH(ENGINE *);
void ENGINE_unregister_DH(ENGINE *);
void ENGINE_register_all_DH(void);
int ENGINE_register_RAND(ENGINE *);
void ENGINE_unregister_RAND(ENGINE *);
void ENGINE_register_all_RAND(void);
int ENGINE_register_STORE(ENGINE *);
void ENGINE_unregister_STORE(ENGINE *);
void ENGINE_register_all_STORE(void);
int ENGINE_register_ciphers(ENGINE *);
void ENGINE_unregister_ciphers(ENGINE *);
void ENGINE_register_all_ciphers(void);
int ENGINE_register_digests(ENGINE *);
void ENGINE_unregister_digests(ENGINE *);
void ENGINE_register_all_digests(void);
int ENGINE_register_complete(ENGINE *);
int ENGINE_register_all_complete(void);
int ENGINE_ctrl(ENGINE *, int, long, void *, void (*)(void));
int ENGINE_cmd_is_executable(ENGINE *, int);
int ENGINE_ctrl_cmd(ENGINE *, const char *, long, void *, void (*)(void), int);
int ENGINE_ctrl_cmd_string(ENGINE *, const char *, const char *, int);

ENGINE *ENGINE_new(void);
int ENGINE_free(ENGINE *);
int ENGINE_up_ref(ENGINE *);
int ENGINE_set_id(ENGINE *, const char *);
int ENGINE_set_name(ENGINE *, const char *);
int ENGINE_set_RSA(ENGINE *, const RSA_METHOD *);
int ENGINE_set_DSA(ENGINE *, const DSA_METHOD *);
int ENGINE_set_ECDH(ENGINE *, const ECDH_METHOD *);
int ENGINE_set_ECDSA(ENGINE *, const ECDSA_METHOD *);
int ENGINE_set_DH(ENGINE *, const DH_METHOD *);
int ENGINE_set_RAND(ENGINE *, const RAND_METHOD *);
int ENGINE_set_STORE(ENGINE *, const STORE_METHOD *);
int ENGINE_set_destroy_function(ENGINE *, ENGINE_GEN_INT_FUNC_PTR);
int ENGINE_set_init_function(ENGINE *, ENGINE_GEN_INT_FUNC_PTR);
int ENGINE_set_finish_function(ENGINE *, ENGINE_GEN_INT_FUNC_PTR);
int ENGINE_set_ctrl_function(ENGINE *, ENGINE_CTRL_FUNC_PTR);
int ENGINE_set_load_privkey_function(ENGINE *, ENGINE_LOAD_KEY_PTR);
int ENGINE_set_load_pubkey_function(ENGINE *, ENGINE_LOAD_KEY_PTR);
int ENGINE_set_ciphers(ENGINE *, ENGINE_CIPHERS_PTR);
int ENGINE_set_digests(ENGINE *, ENGINE_DIGESTS_PTR);
int ENGINE_set_flags(ENGINE *, int);
int ENGINE_set_cmd_defns(ENGINE *, const ENGINE_CMD_DEFN *);
const char *ENGINE_get_id(const ENGINE *);
const char *ENGINE_get_name(const ENGINE *);
const RSA_METHOD *ENGINE_get_RSA(const ENGINE *);
const DSA_METHOD *ENGINE_get_DSA(const ENGINE *);
const ECDH_METHOD *ENGINE_get_ECDH(const ENGINE *);
const ECDSA_METHOD *ENGINE_get_ECDSA(const ENGINE *);
const DH_METHOD *ENGINE_get_DH(const ENGINE *);
const RAND_METHOD *ENGINE_get_RAND(const ENGINE *);
const STORE_METHOD *ENGINE_get_STORE(const ENGINE *);

const EVP_CIPHER *ENGINE_get_cipher(ENGINE *, int);
const EVP_MD *ENGINE_get_digest(ENGINE *, int);
int ENGINE_get_flags(const ENGINE *);
const ENGINE_CMD_DEFN *ENGINE_get_cmd_defns(const ENGINE *);
EVP_PKEY *ENGINE_load_private_key(ENGINE *, const char *, UI_METHOD *, void *);
EVP_PKEY *ENGINE_load_public_key(ENGINE *, const char *, UI_METHOD *, void *);
void ENGINE_add_conf_module(void);
"""

MACROS = """
void ENGINE_load_cryptodev(void);
"""

CUSTOMIZATIONS = """
#if defined(LIBRESSL_VERSION_NUMBER)
static const long Cryptography_HAS_ENGINE_CRYPTODEV = 0;
void (*ENGINE_load_cryptodev)(void) = NULL;
#else
static const long Cryptography_HAS_ENGINE_CRYPTODEV = 1;
#endif
"""

CONDITIONAL_NAMES = {
    "Cryptography_HAS_ENGINE_CRYPTODEV": [
        "ENGINE_load_cryptodev"
    ]
}
