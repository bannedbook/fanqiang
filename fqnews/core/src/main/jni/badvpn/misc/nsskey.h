/**
 * @file nsskey.h
 * @author Ambroz Bizjak <ambrop7@gmail.com>
 * 
 * @section LICENSE
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * @section DESCRIPTION
 * 
 * Function for opening a NSS certificate and its private key.
 */

#ifndef BADVPN_MISC_NSSKEY_H
#define BADVPN_MISC_NSSKEY_H

#include <stdlib.h>

#include <prerror.h>
#include <nss/cert.h>
#include <nss/keyhi.h>
#include <nss/pk11func.h>

#include <base/BLog.h>

#include <generated/blog_channel_nsskey.h>

/**
 * Opens a NSS certificate and its private key.
 * 
 * @param name name of the certificate
 * @param out_cert on success, the certificate will be returned here. Should be
 *                 released with CERT_DestroyCertificate.
 * @param out_key on success, the private key will be returned here. Should be
 *                released with SECKEY_DestroyPrivateKey.
 * @return 1 on success, 0 on failure
 */
static int open_nss_cert_and_key (char *name, CERTCertificate **out_cert, SECKEYPrivateKey **out_key) WARN_UNUSED;

static SECKEYPrivateKey * find_nss_private_key (char *name)
{
    SECKEYPrivateKey *key = NULL;

    PK11SlotList *slot_list = PK11_GetAllTokens(CKM_INVALID_MECHANISM, PR_FALSE, PR_FALSE, NULL);
    if (!slot_list) {
        return NULL;
    }
    
    PK11SlotListElement *slot_entry;
    for (slot_entry = slot_list->head; !key && slot_entry; slot_entry = slot_entry->next) {
        SECKEYPrivateKeyList *key_list = PK11_ListPrivKeysInSlot(slot_entry->slot, name, NULL);
        if (!key_list) {
            BLog(BLOG_ERROR, "PK11_ListPrivKeysInSlot failed");
            continue;
        }
        
        SECKEYPrivateKeyListNode *key_node;
        for (key_node = PRIVKEY_LIST_HEAD(key_list); !key && !PRIVKEY_LIST_END(key_node, key_list); key_node = PRIVKEY_LIST_NEXT(key_node)) {
            char *key_name = PK11_GetPrivateKeyNickname(key_node->key);
            if (!key_name || strcmp(key_name, name)) {
                PORT_Free((void *)key_name);
                continue;
            }
            PORT_Free((void *)key_name);
            
            key = SECKEY_CopyPrivateKey(key_node->key);
        }
        
        SECKEY_DestroyPrivateKeyList(key_list);
    }
    
    PK11_FreeSlotList(slot_list);
    
    return key;
}

int open_nss_cert_and_key (char *name, CERTCertificate **out_cert, SECKEYPrivateKey **out_key)
{
    CERTCertificate *cert;
    cert = CERT_FindCertByNicknameOrEmailAddr(CERT_GetDefaultCertDB(), name);
    if (!cert) {
        BLog(BLOG_ERROR, "CERT_FindCertByName failed (%d)", (int)PR_GetError());
        return 0;
    }
    
    SECKEYPrivateKey *key = find_nss_private_key(name);
    if (!key) {
        BLog(BLOG_ERROR, "Failed to find private key");
        CERT_DestroyCertificate(cert);
        return 0;
    }
    
    *out_cert = cert;
    *out_key = key;
    return 1;
}

#endif
