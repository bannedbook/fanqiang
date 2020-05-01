/**
 * @file NCDAst.h
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

#ifndef BADVPN_NCDAST_H
#define BADVPN_NCDAST_H

#include <stdint.h>
#include <stddef.h>

#include <misc/debug.h>
#include <structure/LinkedList1.h>

typedef struct NCDValue_s NCDValue;
typedef struct NCDProgram_s NCDProgram;
typedef struct NCDProgramElem_s NCDProgramElem;
typedef struct NCDProcess_s NCDProcess;
typedef struct NCDBlock_s NCDBlock;
typedef struct NCDStatement_s NCDStatement;
typedef struct NCDIfBlock_s NCDIfBlock;
typedef struct NCDIf_s NCDIf;

struct NCDValue_s {
    int type;
    union {
        struct {
            uint8_t *string;
            size_t string_len;
        };
        struct {
            LinkedList1 list;
            size_t list_count;
        };
        struct {
            LinkedList1 map_list;
            size_t map_count;
        };
        struct {
            char *var_name;
        };
        struct {
            NCDValue *invoc_func;
            NCDValue *invoc_arg;
        };
    };
};

struct NCDProgram_s {
    LinkedList1 elems_list;
    size_t num_elems;
};

struct NCDBlock_s {
    LinkedList1 statements_list;
    size_t count;
};

struct NCDProcess_s {
    int is_template;
    char *name;
    NCDBlock block;
};

struct NCDProgramElem_s {
    int type;
    union {
        NCDProcess process;
        struct {
            char *path_data;
            size_t path_length;
        } include;
        struct {
            char *id_data;
            size_t id_length;
        } include_guard;
    };
};

struct NCDIfBlock_s {
    LinkedList1 ifs_list;
};

struct NCDStatement_s {
    int type;
    char *name;
    union {
        struct {
            char *objname;
            char *cmdname;
            NCDValue args;
        } reg;
        struct {
            NCDIfBlock ifblock;
            int have_else;
            NCDBlock else_block;
            int iftype;
        } ifc;
        struct {
            NCDValue collection;
            char *name1;
            char *name2;
            NCDBlock block;
            int is_grabbed;
        } foreach;
        struct {
            NCDBlock block;
            int is_grabbed;
        } block;
    };
};

struct NCDIf_s {
    NCDValue cond;
    NCDBlock block;
};

//

#define NCDVALUE_STRING 1
#define NCDVALUE_LIST 2
#define NCDVALUE_MAP 3
#define NCDVALUE_VAR 4
#define NCDVALUE_INVOC 5

#define NCDPROGRAMELEM_PROCESS 1
#define NCDPROGRAMELEM_INCLUDE 2
#define NCDPROGRAMELEM_INCLUDE_GUARD 3

#define NCDSTATEMENT_REG 1
#define NCDSTATEMENT_IF 2
#define NCDSTATEMENT_FOREACH 3
#define NCDSTATEMENT_BLOCK 4

#define NCDIFTYPE_IF 1
#define NCDIFTYPE_DO 2

void NCDValue_Free (NCDValue *o);
int NCDValue_Type (NCDValue *o);
int NCDValue_InitString (NCDValue *o, const char *str) WARN_UNUSED;
int NCDValue_InitStringBin (NCDValue *o, const uint8_t *str, size_t len) WARN_UNUSED;
const char * NCDValue_StringValue (NCDValue *o);
size_t NCDValue_StringLength (NCDValue *o);
void NCDValue_InitList (NCDValue *o);
size_t NCDValue_ListCount (NCDValue *o);
int NCDValue_ListAppend (NCDValue *o, NCDValue v) WARN_UNUSED;
int NCDValue_ListPrepend (NCDValue *o, NCDValue v) WARN_UNUSED;
NCDValue * NCDValue_ListFirst (NCDValue *o);
NCDValue * NCDValue_ListNext (NCDValue *o, NCDValue *ev);
void NCDValue_InitMap (NCDValue *o);
size_t NCDValue_MapCount (NCDValue *o);
int NCDValue_MapPrepend (NCDValue *o, NCDValue key, NCDValue val) WARN_UNUSED;
NCDValue * NCDValue_MapFirstKey (NCDValue *o);
NCDValue * NCDValue_MapNextKey (NCDValue *o, NCDValue *ekey);
NCDValue * NCDValue_MapKeyValue (NCDValue *o, NCDValue *ekey);
int NCDValue_InitVar (NCDValue *o, const char *var_name) WARN_UNUSED;
const char * NCDValue_VarName (NCDValue *o);
int NCDValue_InitInvoc (NCDValue *o, NCDValue func, NCDValue arg) WARN_UNUSED;
NCDValue * NCDValue_InvocFunc (NCDValue *o);
NCDValue * NCDValue_InvocArg (NCDValue *o);

void NCDProgram_Init (NCDProgram *o);
void NCDProgram_Free (NCDProgram *o);
NCDProgramElem * NCDProgram_PrependElem (NCDProgram *o, NCDProgramElem elem) WARN_UNUSED;
NCDProgramElem * NCDProgram_FirstElem (NCDProgram *o);
NCDProgramElem * NCDProgram_NextElem (NCDProgram *o, NCDProgramElem *ee);
size_t NCDProgram_NumElems (NCDProgram *o);
int NCDProgram_ContainsElemType (NCDProgram *o, int elem_type);
void NCDProgram_RemoveElem (NCDProgram *o, NCDProgramElem *ee);
int NCDProgram_ReplaceElemWithProgram (NCDProgram *o, NCDProgramElem *ee, NCDProgram replace_prog) WARN_UNUSED;

void NCDProgramElem_InitProcess (NCDProgramElem *o, NCDProcess process);
int NCDProgramElem_InitInclude (NCDProgramElem *o, const char *path_data, size_t path_length) WARN_UNUSED;
int NCDProgramElem_InitIncludeGuard (NCDProgramElem *o, const char *id_data, size_t id_length) WARN_UNUSED;
void NCDProgramElem_Free (NCDProgramElem *o);
int NCDProgramElem_Type (NCDProgramElem *o);
NCDProcess * NCDProgramElem_Process (NCDProgramElem *o);
const char * NCDProgramElem_IncludePathData (NCDProgramElem *o);
size_t NCDProgramElem_IncludePathLength (NCDProgramElem *o);
const char * NCDProgramElem_IncludeGuardIdData (NCDProgramElem *o);
size_t NCDProgramElem_IncludeGuardIdLength (NCDProgramElem *o);

int NCDProcess_Init (NCDProcess *o, int is_template, const char *name, NCDBlock block) WARN_UNUSED;
void NCDProcess_Free (NCDProcess *o);
int NCDProcess_IsTemplate (NCDProcess *o);
const char * NCDProcess_Name (NCDProcess *o);
NCDBlock * NCDProcess_Block (NCDProcess *o);

void NCDBlock_Init (NCDBlock *o);
void NCDBlock_Free (NCDBlock *o);
int NCDBlock_PrependStatement (NCDBlock *o, NCDStatement s) WARN_UNUSED;
int NCDBlock_InsertStatementAfter (NCDBlock *o, NCDStatement *after, NCDStatement s) WARN_UNUSED;
NCDStatement * NCDBlock_ReplaceStatement (NCDBlock *o, NCDStatement *es, NCDStatement s);
NCDStatement * NCDBlock_FirstStatement (NCDBlock *o);
NCDStatement * NCDBlock_NextStatement (NCDBlock *o, NCDStatement *es);
size_t NCDBlock_NumStatements (NCDBlock *o);

int NCDStatement_InitReg (NCDStatement *o, const char *name, const char *objname, const char *cmdname, NCDValue args) WARN_UNUSED;
int NCDStatement_InitIf (NCDStatement *o, const char *name, NCDIfBlock ifblock, int iftype) WARN_UNUSED;
int NCDStatement_InitForeach (NCDStatement *o, const char *name, NCDValue collection, const char *name1, const char *name2, NCDBlock block) WARN_UNUSED;
int NCDStatement_InitBlock (NCDStatement *o, const char *name, NCDBlock block) WARN_UNUSED;
void NCDStatement_Free (NCDStatement *o);
int NCDStatement_Type (NCDStatement *o);
const char * NCDStatement_Name (NCDStatement *o);
const char * NCDStatement_RegObjName (NCDStatement *o);
const char * NCDStatement_RegCmdName (NCDStatement *o);
NCDValue * NCDStatement_RegArgs (NCDStatement *o);
int NCDStatement_IfType (NCDStatement *o);
NCDIfBlock * NCDStatement_IfBlock (NCDStatement *o);
void NCDStatement_IfAddElse (NCDStatement *o, NCDBlock else_block);
NCDBlock * NCDStatement_IfElse (NCDStatement *o);
NCDBlock NCDStatement_IfGrabElse (NCDStatement *o);
NCDValue * NCDStatement_ForeachCollection (NCDStatement *o);
const char * NCDStatement_ForeachName1 (NCDStatement *o);
const char * NCDStatement_ForeachName2 (NCDStatement *o);
NCDBlock * NCDStatement_ForeachBlock (NCDStatement *o);
void NCDStatement_ForeachGrab (NCDStatement *o, NCDValue *out_collection, NCDBlock *out_block);
NCDBlock * NCDStatement_BlockBlock (NCDStatement *o);
NCDBlock NCDStatement_BlockGrabBlock (NCDStatement *o);

void NCDIfBlock_Init (NCDIfBlock *o);
void NCDIfBlock_Free (NCDIfBlock *o);
int NCDIfBlock_PrependIf (NCDIfBlock *o, NCDIf ifc) WARN_UNUSED;
NCDIf * NCDIfBlock_FirstIf (NCDIfBlock *o);
NCDIf * NCDIfBlock_NextIf (NCDIfBlock *o, NCDIf *ei);
NCDIf NCDIfBlock_GrabIf (NCDIfBlock *o, NCDIf *ei);

void NCDIf_Init (NCDIf *o, NCDValue cond, NCDBlock block);
void NCDIf_InitBlock (NCDIf *o, NCDBlock block);
void NCDIf_Free (NCDIf *o);
void NCDIf_FreeGrab (NCDIf *o, NCDValue *out_cond, NCDBlock *out_block);
NCDBlock NCDIf_FreeGrabBlock (NCDIf *o);
NCDValue * NCDIf_Cond (NCDIf *o);
NCDBlock * NCDIf_Block (NCDIf *o);

#endif
