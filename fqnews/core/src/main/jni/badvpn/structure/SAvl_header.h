/**
 * @file SAvl_header.h
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

// Preprocessor inputs. All types must be typedef names unless stated otherwise.
// SAVL_PARAM_NAME - name of this data structure
// SAVL_PARAM_FEATURE_COUNTS - whether to keep count information (0 or 1)
// SAVL_PARAM_FEATURE_NOKEYS - define to 1 if there is no need for a lookup operation
// SAVL_PARAM_TYPE_ENTRY - type of entry. This can also be a struct (e.g.: struct Foo).
// SAVL_PARAM_TYPE_KEY - type of key (ignored if SAVL_PARAM_FEATURE_NOKEYS)
// SAVL_PARAM_TYPE_ARG - type of argument pass through to callbacks
// SAVL_PARAM_TYPE_COUNT - type of count (only if SAVL_PARAM_FEATURE_COUNTS)
// SAVL_PARAM_VALUE_COUNT_MAX - maximum value of count (of type SAVL_PARAM_TYPE_COUNT) (only if SAVL_PARAM_FEATURE_COUNTS)
// SAVL_PARAM_FUN_COMPARE_ENTRIES(arg, entry1, entry2) - compare two entries; returns -1/0/1
// SAVL_PARAM_FUN_COMPARE_KEY_ENTRY(arg, key1, entry2) - compare key and entry; returns -1/0/1 (ignored if SAVL_PARAM_FEATURE_NOKEYS) 
// SAVL_PARAM_MEMBER_NODE - node member in entry

// types
#define SAvl SAVL_PARAM_NAME
#define SAvlEntry SAVL_PARAM_TYPE_ENTRY
#define SAvlArg SAVL_PARAM_TYPE_ARG
#define SAvlKey SAVL_PARAM_TYPE_KEY
#define SAvlCount SAVL_PARAM_TYPE_COUNT
#define SAvlNode MERGE(SAvl, Node)

// public functions
#define SAvl_Init MERGE(SAvl, _Init)
#define SAvl_Insert MERGE(SAvl, _Insert)
#define SAvl_Remove MERGE(SAvl, _Remove)
#define SAvl_Lookup MERGE(SAvl, _Lookup)
#define SAvl_LookupExact MERGE(SAvl, _LookupExact)
#define SAvl_GetFirstGreater MERGE(SAvl, _GetFirstGreater)
#define SAvl_GetLastLesser MERGE(SAvl, _GetLastLesser)
#define SAvl_GetFirstGreaterEqual MERGE(SAvl, _GetFirstGreaterEqual)
#define SAvl_GetLastLesserEqual MERGE(SAvl, _GetLastLesserEqual)
#define SAvl_GetFirst MERGE(SAvl, _GetFirst)
#define SAvl_GetLast MERGE(SAvl, _GetLast)
#define SAvl_GetNext MERGE(SAvl, _GetNext)
#define SAvl_GetPrev MERGE(SAvl, _GetPrev)
#define SAvl_IsEmpty MERGE(SAvl, _IsEmpty)
#define SAvl_Verify MERGE(SAvl, _Verify)
#define SAvl_Count MERGE(SAvl, _Count)
#define SAvl_IndexOf MERGE(SAvl, _IndexOf)
#define SAvl_GetAt MERGE(SAvl, _GetAt)

// internal stuff
#define SAvl__Tree MERGE(SAvl, __Tree)
#define SAvl__TreeRef MERGE(SAvl, __TreeRef)
#define SAvl__TreeLink MERGE(SAvl, __TreeLink)
#define SAvl__TreeDeref MERGE(SAvl, __TreeDeref)
#define SAvl__Tree_Init MERGE(SAvl, __Tree_Init)
#define SAvl__Tree_Insert MERGE(SAvl, __Tree_Insert)
#define SAvl__Tree_Remove MERGE(SAvl, __Tree_Remove)
#define SAvl__Tree_Lookup MERGE(SAvl, __Tree_Lookup)
#define SAvl__Tree_LookupExact MERGE(SAvl, __Tree_LookupExact)
#define SAvl__Tree_GetFirstGreater MERGE(SAvl, __Tree_GetFirstGreater)
#define SAvl__Tree_GetLastLesser MERGE(SAvl, __Tree_GetLastLesser)
#define SAvl__Tree_GetFirstGreaterEqual MERGE(SAvl, __Tree_GetFirstGreaterEqual)
#define SAvl__Tree_GetLastLesserEqual MERGE(SAvl, __Tree_GetLastLesserEqual)
#define SAvl__Tree_GetFirst MERGE(SAvl, __Tree_GetFirst)
#define SAvl__Tree_GetLast MERGE(SAvl, __Tree_GetLast)
#define SAvl__Tree_GetNext MERGE(SAvl, __Tree_GetNext)
#define SAvl__Tree_GetPrev MERGE(SAvl, __Tree_GetPrev)
#define SAvl__Tree_IsEmpty MERGE(SAvl, __Tree_IsEmpty)
#define SAvl__Tree_Verify MERGE(SAvl, __Tree_Verify)
#define SAvl__Tree_Count MERGE(SAvl, __Tree_Count)
#define SAvl__Tree_IndexOf MERGE(SAvl, __Tree_IndexOf)
#define SAvl__Tree_GetAt MERGE(SAvl, __Tree_GetAt)
