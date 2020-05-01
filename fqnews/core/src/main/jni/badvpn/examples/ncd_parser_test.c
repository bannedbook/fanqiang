/**
 * @file ncd_parser_test.c
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

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include <misc/debug.h>
#include <misc/expstring.h>
#include <base/BLog.h>
#include <ncd/NCDConfigParser.h>
#include <ncd/NCDValGenerator.h>
#include <ncd/NCDSugar.h>

static int generate_val (NCDValue *value, ExpString *out_str)
{
    switch (NCDValue_Type(value)) {
        case NCDVALUE_STRING: {
            const char *str = NCDValue_StringValue(value);
            size_t len = NCDValue_StringLength(value);
            
            if (!ExpString_AppendChar(out_str, '"')) {
                goto fail;
            }
            
            for (size_t i = 0; i < len; i++) {
                if (str[i] == '\0') {
                    char buf[5];
                    snprintf(buf, sizeof(buf), "\\x%02"PRIx8, (uint8_t)str[i]);
                    
                    if (!ExpString_Append(out_str, buf)) {
                        goto fail;
                    }
                    
                    continue;
                }
                
                if (str[i] == '"' || str[i] == '\\') {
                    if (!ExpString_AppendChar(out_str, '\\')) {
                        goto fail;
                    }
                }
                
                if (!ExpString_AppendChar(out_str, str[i])) {
                    goto fail;
                }
            }
            
            if (!ExpString_AppendChar(out_str, '"')) {
                goto fail;
            }
        } break;
        
        case NCDVALUE_LIST: {
            if (!ExpString_AppendChar(out_str, '{')) {
                goto fail;
            }
            
            int is_first = 1;
            
            for (NCDValue *e = NCDValue_ListFirst(value); e; e = NCDValue_ListNext(value, e)) {
                if (!is_first) {
                    if (!ExpString_Append(out_str, ", ")) {
                        goto fail;
                    }
                }
                
                if (!generate_val(e, out_str)) {
                    goto fail;
                }
                
                is_first = 0;
            }
            
            if (!ExpString_AppendChar(out_str, '}')) {
                goto fail;
            }
        } break;
        
        case NCDVALUE_MAP: {
            if (!ExpString_AppendChar(out_str, '[')) {
                goto fail;
            }
            
            int is_first = 1;
            
            for (NCDValue *ekey = NCDValue_MapFirstKey(value); ekey; ekey = NCDValue_MapNextKey(value, ekey)) {
                NCDValue *eval = NCDValue_MapKeyValue(value, ekey);
                
                if (!is_first) {
                    if (!ExpString_Append(out_str, ", ")) {
                        goto fail;
                    }
                }
                
                if (!generate_val(ekey, out_str)) {
                    goto fail;
                }
                
                if (!ExpString_AppendChar(out_str, ':')) {
                    goto fail;
                }
                
                if (!generate_val(eval, out_str)) {
                    goto fail;
                }
                
                is_first = 0;
            }
            
            if (!ExpString_AppendChar(out_str, ']')) {
                goto fail;
            }
        } break;
        
        case NCDVALUE_VAR: {
            if (!ExpString_Append(out_str, NCDValue_VarName(value))) {
                goto fail;
            }
        } break;
        
        case NCDVALUE_INVOC: {
            if (!generate_val(NCDValue_InvocFunc(value), out_str)) {
                goto fail;
            }
            
            if (!ExpString_AppendChar(out_str, '(')) {
                goto fail;
            }
            
            if (!generate_val(NCDValue_InvocArg(value), out_str)) {
                goto fail;
            }
            
            if (!ExpString_AppendChar(out_str, ')')) {
                goto fail;
            }
        } break;
        
        default: ASSERT(0);
    }
    
    return 1;
    
fail:
    return 0;
}

static void print_indent (unsigned int indent)
{
    while (indent > 0) {
        printf("  ");
        indent--;
    }
}

static void print_value (NCDValue *v, unsigned int indent)
{
    ExpString estr;
    if (!ExpString_Init(&estr)) {
        DEBUG("ExpString_Init failed");
        exit(1);
    }
    
    if (!generate_val(v, &estr)) {
        DEBUG("generate_val failed");
        exit(1);
    }
    
    print_indent(indent);
    printf("%s\n", ExpString_Get(&estr));
    
    ExpString_Free(&estr);
}

static void print_block (NCDBlock *block, unsigned int indent)
{
    for (NCDStatement *st = NCDBlock_FirstStatement(block); st; st = NCDBlock_NextStatement(block, st)) {
        const char *name = NCDStatement_Name(st) ? NCDStatement_Name(st) : "";
        
        switch (NCDStatement_Type(st)) {
            case NCDSTATEMENT_REG: {
                const char *objname = NCDStatement_RegObjName(st) ? NCDStatement_RegObjName(st) : "";
                const char *cmdname = NCDStatement_RegCmdName(st);
                
                print_indent(indent);
                printf("reg name=%s objname=%s cmdname=%s args:\n", name, objname, cmdname);
                
                print_value(NCDStatement_RegArgs(st), indent + 2);
            } break;
            
            case NCDSTATEMENT_IF: {
                print_indent(indent);
                printf("if name=%s\n", name);
                
                NCDIfBlock *ifb = NCDStatement_IfBlock(st);
                
                for (NCDIf *ifc = NCDIfBlock_FirstIf(ifb); ifc; ifc = NCDIfBlock_NextIf(ifb, ifc)) {
                    print_indent(indent + 2);
                    printf("if\n");
                    
                    print_value(NCDIf_Cond(ifc), indent + 4);
                    
                    print_indent(indent + 2);
                    printf("then\n");
                    
                    print_block(NCDIf_Block(ifc), indent + 4);
                }
                
                if (NCDStatement_IfElse(st)) {
                    print_indent(indent + 2);
                    printf("else\n");
                    
                    print_block(NCDStatement_IfElse(st), indent + 4);
                }
            } break;
            
            case NCDSTATEMENT_FOREACH: {
                const char *name1 = NCDStatement_ForeachName1(st);
                const char *name2 = NCDStatement_ForeachName2(st) ? NCDStatement_ForeachName2(st) : "";
                
                print_indent(indent);
                printf("foreach name=%s name1=%s name2=%s\n", name, name1, name2);
                
                print_block(NCDStatement_ForeachBlock(st), indent + 2);
            } break;
            
            case NCDSTATEMENT_BLOCK: {
                print_indent(indent);
                printf("block name=%s\n", name);
                
                print_block(NCDStatement_BlockBlock(st), indent + 2);
            } break;
            
            default: {
                print_indent(indent);
                printf("unknown_statement_type name=%s\n", name);
            } break;
        }
    }
}

int main (int argc, char **argv)
{
    int res = 1;
    
    if (argc != 3) {
        printf("Usage: %s <desugar=0/1> <string>\n", (argc > 0 ? argv[0] : ""));
        goto fail0;
    }
    
    int desugar = atoi(argv[1]);
    char *text = argv[2];
    
    BLog_InitStdout();
    
    // parse
    NCDProgram prog;
    if (!NCDConfigParser_Parse(text, strlen(text), &prog)) {
        DEBUG("NCDConfigParser_Parse failed");
        goto fail1;
    }
    
    // desugar
    if (desugar) {
        if (!NCDSugar_Desugar(&prog)) {
            DEBUG("NCDSugar_Desugar failed");
            goto fail2;
        }
    }
    
    // print
    for (NCDProgramElem *elem = NCDProgram_FirstElem(&prog); elem; elem = NCDProgram_NextElem(&prog, elem)) {
        switch (NCDProgramElem_Type(elem)) {
            case NCDPROGRAMELEM_PROCESS: {
                NCDProcess *p = NCDProgramElem_Process(elem);
                printf("process name=%s is_template=%d\n", NCDProcess_Name(p), NCDProcess_IsTemplate(p));
                print_block(NCDProcess_Block(p), 2);
            } break;
            
            case NCDPROGRAMELEM_INCLUDE: {
                printf("include path=%s\n", NCDProgramElem_IncludePathData(elem));
            } break;
            
            case NCDPROGRAMELEM_INCLUDE_GUARD: {
                printf("include_guard id=%s\n", NCDProgramElem_IncludeGuardIdData(elem));
            } break;
            
            default: ASSERT(0);
        }
    }
    
    res = 0;
fail2:
    NCDProgram_Free(&prog);
fail1:
    BLog_Free();
fail0:
    return res;
}
