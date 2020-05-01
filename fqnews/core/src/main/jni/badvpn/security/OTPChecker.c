/**
 * @file OTPChecker.c
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
 */

#include <string.h>

#include <misc/balloc.h>

#include <security/OTPChecker.h>

static void OTPChecker_Table_Empty (OTPChecker *mc, struct OTPChecker_table *t);
static void OTPChecker_Table_AddOTP (OTPChecker *mc, struct OTPChecker_table *t, otp_t otp);
static void OTPChecker_Table_Generate (OTPChecker *mc, struct OTPChecker_table *t, OTPCalculator *calc, uint8_t *key, uint8_t *iv);
static int OTPChecker_Table_CheckOTP (OTPChecker *mc, struct OTPChecker_table *t, otp_t otp);

void OTPChecker_Table_Empty (OTPChecker *mc, struct OTPChecker_table *t)
{
    for (int i = 0; i < mc->num_entries; i++) {
        t->entries[i].avail = -1;
    }
}

void OTPChecker_Table_AddOTP (OTPChecker *mc, struct OTPChecker_table *t, otp_t otp)
{
    // calculate starting index
    int start_index = otp % mc->num_entries;
    
    // try indexes starting with the base position
    for (int i = 0; i < mc->num_entries; i++) {
        int index = bmodadd_int(start_index, i, mc->num_entries);
        struct OTPChecker_entry *entry = &t->entries[index];
        
        // if we find a free index, use it
        if (entry->avail < 0) {
            entry->otp = otp;
            entry->avail = 1;
            return;
        }
        
        // if we find a used index with the same mac,
        // use it by incrementing its count
        if (entry->otp == otp) {
            entry->avail++;
            return;
        }
    }
    
    // will never add more macs than we can hold
    ASSERT(0)
}

void OTPChecker_Table_Generate (OTPChecker *mc, struct OTPChecker_table *t, OTPCalculator *calc, uint8_t *key, uint8_t *iv)
{
    // calculate values
    otp_t *otps = OTPCalculator_Generate(calc, key, iv, 0);
    
    // empty table
    OTPChecker_Table_Empty(mc ,t);
    
    // add calculated values to table
    for (int i = 0; i < mc->num_otps; i++) {
        OTPChecker_Table_AddOTP(mc, t, otps[i]);
    }
}

int OTPChecker_Table_CheckOTP (OTPChecker *mc, struct OTPChecker_table *t, otp_t otp)
{
    // calculate starting index
    int start_index = otp % mc->num_entries;
    
    // try indexes starting with the base position
    for (int i = 0; i < mc->num_entries; i++) {
        int index = bmodadd_int(start_index, i, mc->num_entries);
        struct OTPChecker_entry *entry = &t->entries[index];
        
        // if we find an empty entry, there is no such mac
        if (entry->avail < 0) {
            return 0;
        }
        
        // if we find a matching entry, check its count
        if (entry->otp == otp) {
            if (entry->avail > 0) {
                entry->avail--;
                return 1;
            }
            return 0;
        }
    }
    
    // there are always empty slots
    ASSERT(0)
    return 0;
}

static void work_func (OTPChecker *mc)
{
    struct OTPChecker_table *table = &mc->tables[mc->next_table];
    OTPChecker_Table_Generate(mc, table, &mc->calc, mc->tw_key, mc->tw_iv);
}

static void work_done_handler (OTPChecker *mc)
{
    ASSERT(mc->tw_have)
    DebugObject_Access(&mc->d_obj);
    
    // free work
    BThreadWork_Free(&mc->tw);
    mc->tw_have = 0;
    
    // update next table number
    mc->next_table = bmodadd_int(mc->next_table, 1, mc->num_tables);
    
    // update number of used tables if not all are used yet
    if (mc->tables_used < mc->num_tables) {
        mc->tables_used++;
    }
    
    // call handler
    if (mc->handler) {
        mc->handler(mc->user);
        return;
    }
}

int OTPChecker_Init (OTPChecker *mc, int num_otps, int cipher, int num_tables, BThreadWorkDispatcher *twd)
{
    ASSERT(num_otps > 0)
    ASSERT(BEncryption_cipher_valid(cipher))
    ASSERT(num_tables > 0)
    
    // init arguments
    mc->num_otps = num_otps;
    mc->cipher = cipher;
    mc->num_tables = num_tables;
    mc->twd = twd;
    
    // set no handlers
    mc->handler = NULL;
    
    // set number of entries
    if (mc->num_otps > INT_MAX / 2) {
        goto fail0;
    }
    mc->num_entries = 2 * mc->num_otps;
    
    // set no tables used
    mc->tables_used = 0;
    mc->next_table = 0;
    
    // initialize calculator
    if (!OTPCalculator_Init(&mc->calc, mc->num_otps, cipher)) {
        goto fail0;
    }
    
    // allocate tables
    if (!(mc->tables = (struct OTPChecker_table *)BAllocArray(mc->num_tables, sizeof(mc->tables[0])))) {
        goto fail1;
    }
    
    // allocate entries
    if (!(mc->entries = (struct OTPChecker_entry *)BAllocArray2(mc->num_tables, mc->num_entries, sizeof(mc->entries[0])))) {
        goto fail2;
    }
    
    // initialize tables
    for (int i = 0; i < mc->num_tables; i++) {
        struct OTPChecker_table *table = &mc->tables[i];
        table->entries = mc->entries + (size_t)i * mc->num_entries;
        OTPChecker_Table_Empty(mc, table);
    }
    
    // have no work
    mc->tw_have = 0;
    
    DebugObject_Init(&mc->d_obj);
    return 1;
    
fail2:
    BFree(mc->tables);
fail1:
    OTPCalculator_Free(&mc->calc);
fail0:
    return 0;
}

void OTPChecker_Free (OTPChecker *mc)
{
    DebugObject_Free(&mc->d_obj);
    
    // free work
    if (mc->tw_have) {
        BThreadWork_Free(&mc->tw);
    }
    
    // free entries
    BFree(mc->entries);
    
    // free tables
    BFree(mc->tables);
    
    // free calculator
    OTPCalculator_Free(&mc->calc);
}

void OTPChecker_AddSeed (OTPChecker *mc, uint16_t seed_id, uint8_t *key, uint8_t *iv)
{
    ASSERT(mc->next_table >= 0)
    ASSERT(mc->next_table < mc->num_tables)
    DebugObject_Access(&mc->d_obj);
    
    // free existing work
    if (mc->tw_have) {
        BThreadWork_Free(&mc->tw);
    }
    
    // set table's seed ID
    mc->tables[mc->next_table].id = seed_id;
    
    // copy key and IV
    memcpy(mc->tw_key, key, BEncryption_cipher_key_size(mc->cipher));
    memcpy(mc->tw_iv, iv, BEncryption_cipher_block_size(mc->cipher));
    
    // start work
    BThreadWork_Init(&mc->tw, mc->twd, (BThreadWork_handler_done)work_done_handler, mc, (BThreadWork_work_func)work_func, mc);
    
    // set have work
    mc->tw_have = 1;
}

void OTPChecker_RemoveSeeds (OTPChecker *mc)
{
    DebugObject_Access(&mc->d_obj);
    
    // free existing work
    if (mc->tw_have) {
        BThreadWork_Free(&mc->tw);
        mc->tw_have = 0;
    }
    
    mc->tables_used = 0;
    mc->next_table = 0;
}

int OTPChecker_CheckOTP (OTPChecker *mc, uint16_t seed_id, otp_t otp)
{
    DebugObject_Access(&mc->d_obj);
    
    // try tables in reverse order
    for (int i = 1; i <= mc->tables_used; i++) {
        int table_index = bmodadd_int(mc->next_table, mc->num_tables - i, mc->num_tables);
        if (table_index == mc->next_table && mc->tw_have) {
            // ignore table that is being generated
            continue;
        }
        
        struct OTPChecker_table *table = &mc->tables[table_index];
        if (table->id == seed_id) {
            return OTPChecker_Table_CheckOTP(mc, table, otp);
        }
    }
    
    return 0;
}

void OTPChecker_SetHandlers (OTPChecker *mc, OTPChecker_handler handler, void *user)
{
    DebugObject_Access(&mc->d_obj);
    
    mc->handler = handler;
    mc->user = user;
}
