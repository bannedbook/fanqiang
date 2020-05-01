/*************************************************
*      Perl-Compatible Regular Expressions       *
*************************************************/

/* PCRE is a library of functions to support regular expressions whose syntax
and semantics are as close as possible to those of the Perl 5 language.

                  Main Library written by Philip Hazel
           Copyright (c) 1997-2012 University of Cambridge

  This JIT compiler regression test program was written by Zoltan Herczeg
                      Copyright (c) 2010-2012

-----------------------------------------------------------------------------
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    * Neither the name of the University of Cambridge nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-----------------------------------------------------------------------------
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include "pcre.h"


#include "pcre_internal.h"

/*
 Letter characters:
   \xe6\x92\xad = 0x64ad = 25773 (kanji)
 Non-letter characters:
   \xc2\xa1 = 0xa1 =  (Inverted Exclamation Mark)
   \xf3\xa9\xb7\x80 = 0xe9dc0 = 957888
   \xed\xa0\x80 = 55296 = 0xd800 (Invalid UTF character)
   \xed\xb0\x80 = 56320 = 0xdc00 (Invalid UTF character)
 Newlines:
   \xc2\x85 = 0x85 = 133 (NExt Line = NEL)
   \xe2\x80\xa8 = 0x2028 = 8232 (Line Separator)
 Othercase pairs:
   \xc3\xa9 = 0xe9 = 233 (e')
      \xc3\x89 = 0xc9 = 201 (E')
   \xc3\xa1 = 0xe1 = 225 (a')
      \xc3\x81 = 0xc1 = 193 (A')
   \x53 = 0x53 = S
     \x73 = 0x73 = s
     \xc5\xbf = 0x17f = 383 (long S)
   \xc8\xba = 0x23a = 570
      \xe2\xb1\xa5 = 0x2c65 = 11365
   \xe1\xbd\xb8 = 0x1f78 = 8056
      \xe1\xbf\xb8 = 0x1ff8 = 8184
   \xf0\x90\x90\x80 = 0x10400 = 66560
      \xf0\x90\x90\xa8 = 0x10428 = 66600
   \xc7\x84 = 0x1c4 = 452
     \xc7\x85 = 0x1c5 = 453
     \xc7\x86 = 0x1c6 = 454
 Caseless sets:
   ucp_Armenian - \x{531}-\x{556} -> \x{561}-\x{586}
   ucp_Coptic - \x{2c80}-\x{2ce3} -> caseless: XOR 0x1
   ucp_Latin - \x{ff21}-\x{ff3a} -> \x{ff41]-\x{ff5a}

 Mark property:
   \xcc\x8d = 0x30d = 781
 Special:
   \xc2\x80 = 0x80 = 128 (lowest 2 byte character)
   \xdf\xbf = 0x7ff = 2047 (highest 2 byte character)
   \xe0\xa0\x80 = 0x800 = 2048 (lowest 2 byte character)
   \xef\xbf\xbf = 0xffff = 65535 (highest 3 byte character)
   \xf0\x90\x80\x80 = 0x10000 = 65536 (lowest 4 byte character)
   \xf4\x8f\xbf\xbf = 0x10ffff = 1114111 (highest allowed utf character)
*/

static int regression_tests(void);

int main(void)
{
	int jit = 0;
#if defined SUPPORT_PCRE8
	pcre_config(PCRE_CONFIG_JIT, &jit);
#elif defined SUPPORT_PCRE16
	pcre16_config(PCRE_CONFIG_JIT, &jit);
#elif defined SUPPORT_PCRE32
	pcre32_config(PCRE_CONFIG_JIT, &jit);
#endif
	if (!jit) {
		printf("JIT must be enabled to run pcre_jit_test\n");
		return 1;
	}
	return regression_tests();
}

/* --------------------------------------------------------------------------------------- */

#if !(defined SUPPORT_PCRE8) && !(defined SUPPORT_PCRE16) && !(defined SUPPORT_PCRE32)
#error SUPPORT_PCRE8 or SUPPORT_PCRE16 or SUPPORT_PCRE32 must be defined
#endif

#define MUA	(PCRE_MULTILINE | PCRE_UTF8 | PCRE_NEWLINE_ANYCRLF)
#define MUAP	(PCRE_MULTILINE | PCRE_UTF8 | PCRE_NEWLINE_ANYCRLF | PCRE_UCP)
#define CMUA	(PCRE_CASELESS | PCRE_MULTILINE | PCRE_UTF8 | PCRE_NEWLINE_ANYCRLF)
#define CMUAP	(PCRE_CASELESS | PCRE_MULTILINE | PCRE_UTF8 | PCRE_NEWLINE_ANYCRLF | PCRE_UCP)
#define MA	(PCRE_MULTILINE | PCRE_NEWLINE_ANYCRLF)
#define MAP	(PCRE_MULTILINE | PCRE_NEWLINE_ANYCRLF | PCRE_UCP)
#define CMA	(PCRE_CASELESS | PCRE_MULTILINE | PCRE_NEWLINE_ANYCRLF)

#define OFFSET_MASK	0x00ffff
#define F_NO8		0x010000
#define F_NO16		0x020000
#define F_NO32		0x020000
#define F_NOMATCH	0x040000
#define F_DIFF		0x080000
#define F_FORCECONV	0x100000
#define F_PROPERTY	0x200000
#define F_STUDY		0x400000

struct regression_test_case {
	int flags;
	int start_offset;
	const char *pattern;
	const char *input;
};

static struct regression_test_case regression_test_cases[] = {
	/* Constant strings. */
	{ MUA, 0, "AbC", "AbAbC" },
	{ MUA, 0, "ACCEPT", "AACACCACCEACCEPACCEPTACCEPTT" },
	{ CMUA, 0, "aA#\xc3\xa9\xc3\x81", "aA#Aa#\xc3\x89\xc3\xa1" },
	{ MA, 0, "[^a]", "aAbB" },
	{ CMA, 0, "[^m]", "mMnN" },
	{ MA, 0, "a[^b][^#]", "abacd" },
	{ CMA, 0, "A[^B][^E]", "abacd" },
	{ CMUA, 0, "[^x][^#]", "XxBll" },
	{ MUA, 0, "[^a]", "aaa\xc3\xa1#Ab" },
	{ CMUA, 0, "[^A]", "aA\xe6\x92\xad" },
	{ MUA, 0, "\\W(\\W)?\\w", "\r\n+bc" },
	{ MUA, 0, "\\W(\\W)?\\w", "\n\r+bc" },
	{ MUA, 0, "\\W(\\W)?\\w", "\r\r+bc" },
	{ MUA, 0, "\\W(\\W)?\\w", "\n\n+bc" },
	{ MUA, 0, "[axd]", "sAXd" },
	{ CMUA, 0, "[axd]", "sAXd" },
	{ CMUA, 0 | F_NOMATCH, "[^axd]", "DxA" },
	{ MUA, 0, "[a-dA-C]", "\xe6\x92\xad\xc3\xa9.B" },
	{ MUA, 0, "[^a-dA-C]", "\xe6\x92\xad\xc3\xa9" },
	{ CMUA, 0, "[^\xc3\xa9]", "\xc3\xa9\xc3\x89." },
	{ MUA, 0, "[^\xc3\xa9]", "\xc3\xa9\xc3\x89." },
	{ MUA, 0, "[^a]", "\xc2\x80[]" },
	{ CMUA, 0, "\xf0\x90\x90\xa7", "\xf0\x90\x91\x8f" },
	{ CMA, 0, "1a2b3c4", "1a2B3c51A2B3C4" },
	{ PCRE_CASELESS, 0, "\xff#a", "\xff#\xff\xfe##\xff#A" },
	{ PCRE_CASELESS, 0, "\xfe", "\xff\xfc#\xfe\xfe" },
	{ PCRE_CASELESS, 0, "a1", "Aa1" },
	{ MA, 0, "\\Ca", "cda" },
	{ CMA, 0, "\\Ca", "CDA" },
	{ MA, 0 | F_NOMATCH, "\\Cx", "cda" },
	{ CMA, 0 | F_NOMATCH, "\\Cx", "CDA" },
	{ CMUAP, 0, "\xf0\x90\x90\x80\xf0\x90\x90\xa8", "\xf0\x90\x90\xa8\xf0\x90\x90\x80" },
	{ CMUAP, 0, "\xf0\x90\x90\x80{2}", "\xf0\x90\x90\x80#\xf0\x90\x90\xa8\xf0\x90\x90\x80" },
	{ CMUAP, 0, "\xf0\x90\x90\xa8{2}", "\xf0\x90\x90\x80#\xf0\x90\x90\xa8\xf0\x90\x90\x80" },
	{ CMUAP, 0, "\xe1\xbd\xb8\xe1\xbf\xb8", "\xe1\xbf\xb8\xe1\xbd\xb8" },
	{ MA, 0, "[3-57-9]", "5" },

	/* Assertions. */
	{ MUA, 0, "\\b[^A]", "A_B#" },
	{ MA, 0 | F_NOMATCH, "\\b\\W", "\n*" },
	{ MUA, 0, "\\B[^,]\\b[^s]\\b", "#X" },
	{ MAP, 0, "\\B", "_\xa1" },
	{ MAP, 0, "\\b_\\b[,A]\\B", "_," },
	{ MUAP, 0, "\\b", "\xe6\x92\xad!" },
	{ MUAP, 0, "\\B", "_\xc2\xa1\xc3\xa1\xc2\x85" },
	{ MUAP, 0, "\\b[^A]\\B[^c]\\b[^_]\\B", "_\xc3\xa1\xe2\x80\xa8" },
	{ MUAP, 0, "\\b\\w+\\B", "\xc3\x89\xc2\xa1\xe6\x92\xad\xc3\x81\xc3\xa1" },
	{ MUA, 0 | F_NOMATCH, "\\b.", "\xcd\xbe" },
	{ CMUAP, 0, "\\By", "\xf0\x90\x90\xa8y" },
	{ MA, 0 | F_NOMATCH, "\\R^", "\n" },
	{ MA, 1 | F_NOMATCH, "^", "\n" },
	{ 0, 0, "^ab", "ab" },
	{ 0, 0 | F_NOMATCH, "^ab", "aab" },
	{ PCRE_MULTILINE | PCRE_NEWLINE_CRLF, 0, "^a", "\r\raa\n\naa\r\naa" },
	{ PCRE_MULTILINE | PCRE_UTF8 | PCRE_NEWLINE_ANYCRLF, 0, "^-", "\xe2\x80\xa8--\xc2\x85-\r\n-" },
	{ PCRE_MULTILINE | PCRE_NEWLINE_ANY, 0, "^-", "a--b--\x85--" },
	{ PCRE_MULTILINE | PCRE_UTF8 | PCRE_NEWLINE_ANY, 0, "^-", "a--\xe2\x80\xa8--" },
	{ PCRE_MULTILINE | PCRE_UTF8 | PCRE_NEWLINE_ANY, 0, "^-", "a--\xc2\x85--" },
	{ 0, 0, "ab$", "ab" },
	{ 0, 0 | F_NOMATCH, "ab$", "abab\n\n" },
	{ PCRE_DOLLAR_ENDONLY, 0 | F_NOMATCH, "ab$", "abab\r\n" },
	{ PCRE_MULTILINE | PCRE_NEWLINE_CRLF, 0, "a$", "\r\raa\n\naa\r\naa" },
	{ PCRE_MULTILINE | PCRE_NEWLINE_ANY, 0, "a$", "aaa" },
	{ PCRE_MULTILINE | PCRE_UTF8 | PCRE_NEWLINE_ANYCRLF, 0, "#$", "#\xc2\x85###\r#" },
	{ PCRE_MULTILINE | PCRE_UTF8 | PCRE_NEWLINE_ANY, 0, "#$", "#\xe2\x80\xa9" },
	{ PCRE_NOTBOL | PCRE_NEWLINE_ANY, 0 | F_NOMATCH, "^a", "aa\naa" },
	{ PCRE_NOTBOL | PCRE_MULTILINE | PCRE_NEWLINE_ANY, 0, "^a", "aa\naa" },
	{ PCRE_NOTEOL | PCRE_NEWLINE_ANY, 0 | F_NOMATCH, "a$", "aa\naa" },
	{ PCRE_NOTEOL | PCRE_NEWLINE_ANY, 0 | F_NOMATCH, "a$", "aa\r\n" },
	{ PCRE_UTF8 | PCRE_DOLLAR_ENDONLY | PCRE_NEWLINE_ANY, 0 | F_PROPERTY, "\\p{Any}{2,}$", "aa\r\n" },
	{ PCRE_NOTEOL | PCRE_MULTILINE | PCRE_NEWLINE_ANY, 0, "a$", "aa\naa" },
	{ PCRE_NEWLINE_CR, 0, ".\\Z", "aaa" },
	{ PCRE_NEWLINE_CR | PCRE_UTF8, 0, "a\\Z", "aaa\r" },
	{ PCRE_NEWLINE_CR, 0, ".\\Z", "aaa\n" },
	{ PCRE_NEWLINE_CRLF, 0, ".\\Z", "aaa\r" },
	{ PCRE_NEWLINE_CRLF | PCRE_UTF8, 0, ".\\Z", "aaa\n" },
	{ PCRE_NEWLINE_CRLF, 0, ".\\Z", "aaa\r\n" },
	{ PCRE_NEWLINE_ANYCRLF | PCRE_UTF8, 0, ".\\Z", "aaa" },
	{ PCRE_NEWLINE_ANYCRLF | PCRE_UTF8, 0, ".\\Z", "aaa\r" },
	{ PCRE_NEWLINE_ANYCRLF | PCRE_UTF8, 0, ".\\Z", "aaa\n" },
	{ PCRE_NEWLINE_ANYCRLF | PCRE_UTF8, 0, ".\\Z", "aaa\r\n" },
	{ PCRE_NEWLINE_ANYCRLF | PCRE_UTF8, 0, ".\\Z", "aaa\xe2\x80\xa8" },
	{ PCRE_NEWLINE_ANYCRLF | PCRE_UTF8, 0, ".\\Z", "aaa" },
	{ PCRE_NEWLINE_ANYCRLF | PCRE_UTF8, 0, ".\\Z", "aaa\r" },
	{ PCRE_NEWLINE_ANYCRLF | PCRE_UTF8, 0, ".\\Z", "aaa\n" },
	{ PCRE_NEWLINE_ANYCRLF | PCRE_UTF8, 0, ".\\Z", "aaa\r\n" },
	{ PCRE_NEWLINE_ANY | PCRE_UTF8, 0, ".\\Z", "aaa\xc2\x85" },
	{ PCRE_NEWLINE_ANY | PCRE_UTF8, 0, ".\\Z", "aaa\xe2\x80\xa8" },
	{ MA, 0, "\\Aa", "aaa" },
	{ MA, 1 | F_NOMATCH, "\\Aa", "aaa" },
	{ MA, 1, "\\Ga", "aaa" },
	{ MA, 1 | F_NOMATCH, "\\Ga", "aba" },
	{ MA, 0, "a\\z", "aaa" },
	{ MA, 0 | F_NOMATCH, "a\\z", "aab" },

	/* Brackets. */
	{ MUA, 0, "(ab|bb|cd)", "bacde" },
	{ MUA, 0, "(?:ab|a)(bc|c)", "ababc" },
	{ MUA, 0, "((ab|(cc))|(bb)|(?:cd|efg))", "abac" },
	{ CMUA, 0, "((aB|(Cc))|(bB)|(?:cd|EFg))", "AcCe" },
	{ MUA, 0, "((ab|(cc))|(bb)|(?:cd|ebg))", "acebebg" },
	{ MUA, 0, "(?:(a)|(?:b))(cc|(?:d|e))(a|b)k", "accabdbbccbk" },

	/* Greedy and non-greedy ? operators. */
	{ MUA, 0, "(?:a)?a", "laab" },
	{ CMUA, 0, "(A)?A", "llaab" },
	{ MUA, 0, "(a)?\?a", "aab" }, /* ?? is the prefix of trygraphs in GCC. */
	{ MUA, 0, "(a)?a", "manm" },
	{ CMUA, 0, "(a|b)?\?d((?:e)?)", "ABABdx" },
	{ MUA, 0, "(a|b)?\?d((?:e)?)", "abcde" },
	{ MUA, 0, "((?:ab)?\?g|b(?:g(nn|d)?\?)?)?\?(?:n)?m", "abgnbgnnbgdnmm" },

	/* Greedy and non-greedy + operators */
	{ MUA, 0, "(aa)+aa", "aaaaaaa" },
	{ MUA, 0, "(aa)+?aa", "aaaaaaa" },
	{ MUA, 0, "(?:aba|ab|a)+l", "ababamababal" },
	{ MUA, 0, "(?:aba|ab|a)+?l", "ababamababal" },
	{ MUA, 0, "(a(?:bc|cb|b|c)+?|ss)+e", "accssabccbcacbccbbXaccssabccbcacbccbbe" },
	{ MUA, 0, "(a(?:bc|cb|b|c)+|ss)+?e", "accssabccbcacbccbbXaccssabccbcacbccbbe" },
	{ MUA, 0, "(?:(b(c)+?)+)?\?(?:(bc)+|(cb)+)+(?:m)+", "bccbcccbcbccbcbPbccbcccbcbccbcbmmn" },

	/* Greedy and non-greedy * operators */
	{ CMUA, 0, "(?:AA)*AB", "aaaaaaamaaaaaaab" },
	{ MUA, 0, "(?:aa)*?ab", "aaaaaaamaaaaaaab" },
	{ MUA, 0, "(aa|ab)*ab", "aaabaaab" },
	{ CMUA, 0, "(aa|Ab)*?aB", "aaabaaab" },
	{ MUA, 0, "(a|b)*(?:a)*(?:b)*m", "abbbaaababanabbbaaababamm" },
	{ MUA, 0, "(a|b)*?(?:a)*?(?:b)*?m", "abbbaaababanabbbaaababamm" },
	{ MA, 0, "a(a(\\1*)a|(b)b+){0}a", "aa" },
	{ MA, 0, "((?:a|)*){0}a", "a" },

	/* Combining ? + * operators */
	{ MUA, 0, "((bm)+)?\?(?:a)*(bm)+n|((am)+?)?(?:a)+(am)*n", "bmbmabmamaaamambmaman" },
	{ MUA, 0, "(((ab)?cd)*ef)+g", "abcdcdefcdefefmabcdcdefcdefefgg" },
	{ MUA, 0, "(((ab)?\?cd)*?ef)+?g", "abcdcdefcdefefmabcdcdefcdefefgg" },
	{ MUA, 0, "(?:(ab)?c|(?:ab)+?d)*g", "ababcdccababddg" },
	{ MUA, 0, "(?:(?:ab)?\?c|(ab)+d)*?g", "ababcdccababddg" },

	/* Single character iterators. */
	{ MUA, 0, "(a+aab)+aaaab", "aaaabcaaaabaabcaabcaaabaaaab" },
	{ MUA, 0, "(a*a*aab)+x", "aaaaabaabaaabmaabx" },
	{ MUA, 0, "(a*?(b|ab)a*?)+x", "aaaabcxbbaabaacbaaabaabax" },
	{ MUA, 0, "(a+(ab|ad)a+)+x", "aaabaaaadaabaaabaaaadaaax" },
	{ MUA, 0, "(a?(a)a?)+(aaa)", "abaaabaaaaaaaa" },
	{ MUA, 0, "(a?\?(a)a?\?)+(b)", "aaaacaaacaacacbaaab" },
	{ MUA, 0, "(a{0,4}(b))+d", "aaaaaabaabcaaaaabaaaaabd" },
	{ MUA, 0, "(a{0,4}?[^b])+d+(a{0,4}[^b])d+", "aaaaadaaaacaadddaaddd" },
	{ MUA, 0, "(ba{2})+c", "baabaaabacbaabaac" },
	{ MUA, 0, "(a*+bc++)+", "aaabbcaaabcccab" },
	{ MUA, 0, "(a?+[^b])+", "babaacacb" },
	{ MUA, 0, "(a{0,3}+b)(a{0,3}+b)(a{0,3}+)[^c]", "abaabaaacbaabaaaac" },
	{ CMUA, 0, "([a-c]+[d-f]+?)+?g", "aBdacdehAbDaFgA" },
	{ CMUA, 0, "[c-f]+k", "DemmFke" },
	{ MUA, 0, "([DGH]{0,4}M)+", "GGDGHDGMMHMDHHGHM" },
	{ MUA, 0, "([a-c]{4,}s)+", "abasabbasbbaabsbba" },
	{ CMUA, 0, "[ace]{3,7}", "AcbDAcEEcEd" },
	{ CMUA, 0, "[ace]{3,7}?", "AcbDAcEEcEd" },
	{ CMUA, 0, "[ace]{3,}", "AcbDAcEEcEd" },
	{ CMUA, 0, "[ace]{3,}?", "AcbDAcEEcEd" },
	{ MUA, 0, "[ckl]{2,}?g", "cdkkmlglglkcg" },
	{ CMUA, 0, "[ace]{5}?", "AcCebDAcEEcEd" },
	{ MUA, 0, "([AbC]{3,5}?d)+", "BACaAbbAEAACCbdCCbdCCAAbb" },
	{ MUA, 0, "([^ab]{0,}s){2}", "abaabcdsABamsDDs" },
	{ MUA, 0, "\\b\\w+\\B", "x,a_cd" },
	{ MUAP, 0, "\\b[^\xc2\xa1]+\\B", "\xc3\x89\xc2\xa1\xe6\x92\xad\xc3\x81\xc3\xa1" },
	{ CMUA, 0, "[^b]+(a*)([^c]?d{3})", "aaaaddd" },
	{ CMUAP, 0, "\xe1\xbd\xb8{2}", "\xe1\xbf\xb8#\xe1\xbf\xb8\xe1\xbd\xb8" },
	{ CMUA, 0, "[^\xf0\x90\x90\x80]{2,4}@", "\xf0\x90\x90\xa8\xf0\x90\x90\x80###\xf0\x90\x90\x80@@@" },
	{ CMUA, 0, "[^\xe1\xbd\xb8][^\xc3\xa9]", "\xe1\xbd\xb8\xe1\xbf\xb8\xc3\xa9\xc3\x89#" },
	{ MUA, 0, "[^\xe1\xbd\xb8][^\xc3\xa9]", "\xe1\xbd\xb8\xe1\xbf\xb8\xc3\xa9\xc3\x89#" },
	{ MUA, 0, "[^\xe1\xbd\xb8]{3,}?", "##\xe1\xbd\xb8#\xe1\xbd\xb8#\xc3\x89#\xe1\xbd\xb8" },

	/* Bracket repeats with limit. */
	{ MUA, 0, "(?:(ab){2}){5}M", "abababababababababababM" },
	{ MUA, 0, "(?:ab|abab){1,5}M", "abababababababababababM" },
	{ MUA, 0, "(?>ab|abab){1,5}M", "abababababababababababM" },
	{ MUA, 0, "(?:ab|abab){1,5}?M", "abababababababababababM" },
	{ MUA, 0, "(?>ab|abab){1,5}?M", "abababababababababababM" },
	{ MUA, 0, "(?:(ab){1,4}?){1,3}?M", "abababababababababababababM" },
	{ MUA, 0, "(?:(ab){1,4}){1,3}abababababababababababM", "ababababababababababababM" },
	{ MUA, 0 | F_NOMATCH, "(?:(ab){1,4}){1,3}abababababababababababM", "abababababababababababM" },
	{ MUA, 0, "(ab){4,6}?M", "abababababababM" },

	/* Basic character sets. */
	{ MUA, 0, "(?:\\s)+(?:\\S)+", "ab \t\xc3\xa9\xe6\x92\xad " },
	{ MUA, 0, "(\\w)*(k)(\\W)?\?", "abcdef abck11" },
	{ MUA, 0, "\\((\\d)+\\)\\D", "a() (83 (8)2 (9)ab" },
	{ MUA, 0, "\\w(\\s|(?:\\d)*,)+\\w\\wb", "a 5, 4,, bb 5, 4,, aab" },
	{ MUA, 0, "(\\v+)(\\V+)", "\x0e\xc2\x85\xe2\x80\xa8\x0b\x09\xe2\x80\xa9" },
	{ MUA, 0, "(\\h+)(\\H+)", "\xe2\x80\xa8\xe2\x80\x80\x20\xe2\x80\x8a\xe2\x81\x9f\xe3\x80\x80\x09\x20\xc2\xa0\x0a" },
	{ MUA, 0, "x[bcef]+", "xaxdxecbfg" },
	{ MUA, 0, "x[bcdghij]+", "xaxexfxdgbjk" },
	{ MUA, 0, "x[^befg]+", "xbxexacdhg" },
	{ MUA, 0, "x[^bcdl]+", "xlxbxaekmd" },
	{ MUA, 0, "x[^bcdghi]+", "xbxdxgxaefji" },
	{ MUA, 0, "x[B-Fb-f]+", "xaxAxgxbfBFG" },
	{ CMUA, 0, "\\x{e9}+", "#\xf0\x90\x90\xa8\xc3\xa8\xc3\xa9\xc3\x89\xc3\x88" },
	{ CMUA, 0, "[^\\x{e9}]+", "\xc3\xa9#\xf0\x90\x90\xa8\xc3\xa8\xc3\x88\xc3\x89" },
	{ MUA, 0, "[\\x02\\x7e]+", "\xc3\x81\xe1\xbf\xb8\xf0\x90\x90\xa8\x01\x02\x7e\x7f" },
	{ MUA, 0, "[^\\x02\\x7e]+", "\x02\xc3\x81\xe1\xbf\xb8\xf0\x90\x90\xa8\x01\x7f\x7e" },
	{ MUA, 0, "[\\x{81}-\\x{7fe}]+", "#\xe1\xbf\xb8\xf0\x90\x90\xa8\xc2\x80\xc2\x81\xdf\xbe\xdf\xbf" },
	{ MUA, 0, "[^\\x{81}-\\x{7fe}]+", "\xc2\x81#\xe1\xbf\xb8\xf0\x90\x90\xa8\xc2\x80\xdf\xbf\xdf\xbe" },
	{ MUA, 0, "[\\x{801}-\\x{fffe}]+", "#\xc3\xa9\xf0\x90\x90\x80\xe0\xa0\x80\xe0\xa0\x81\xef\xbf\xbe\xef\xbf\xbf" },
	{ MUA, 0, "[^\\x{801}-\\x{fffe}]+", "\xe0\xa0\x81#\xc3\xa9\xf0\x90\x90\x80\xe0\xa0\x80\xef\xbf\xbf\xef\xbf\xbe" },
	{ MUA, 0, "[\\x{10001}-\\x{10fffe}]+", "#\xc3\xa9\xe2\xb1\xa5\xf0\x90\x80\x80\xf0\x90\x80\x81\xf4\x8f\xbf\xbe\xf4\x8f\xbf\xbf" },
	{ MUA, 0, "[^\\x{10001}-\\x{10fffe}]+", "\xf0\x90\x80\x81#\xc3\xa9\xe2\xb1\xa5\xf0\x90\x80\x80\xf4\x8f\xbf\xbf\xf4\x8f\xbf\xbe" },

	/* Unicode properties. */
	{ MUAP, 0, "[1-5\xc3\xa9\\w]", "\xc3\xa1_" },
	{ MUAP, 0 | F_PROPERTY, "[\xc3\x81\\p{Ll}]", "A_\xc3\x89\xc3\xa1" },
	{ MUAP, 0, "[\\Wd-h_x-z]+", "a\xc2\xa1#_yhzdxi" },
	{ MUAP, 0 | F_NOMATCH | F_PROPERTY, "[\\P{Any}]", "abc" },
	{ MUAP, 0 | F_NOMATCH | F_PROPERTY, "[^\\p{Any}]", "abc" },
	{ MUAP, 0 | F_NOMATCH | F_PROPERTY, "[\\P{Any}\xc3\xa1-\xc3\xa8]", "abc" },
	{ MUAP, 0 | F_NOMATCH | F_PROPERTY, "[^\\p{Any}\xc3\xa1-\xc3\xa8]", "abc" },
	{ MUAP, 0 | F_NOMATCH | F_PROPERTY, "[\xc3\xa1-\xc3\xa8\\P{Any}]", "abc" },
	{ MUAP, 0 | F_NOMATCH | F_PROPERTY, "[^\xc3\xa1-\xc3\xa8\\p{Any}]", "abc" },
	{ MUAP, 0 | F_PROPERTY, "[\xc3\xa1-\xc3\xa8\\p{Any}]", "abc" },
	{ MUAP, 0 | F_PROPERTY, "[^\xc3\xa1-\xc3\xa8\\P{Any}]", "abc" },
	{ MUAP, 0, "[b-\xc3\xa9\\s]", "a\xc\xe6\x92\xad" },
	{ CMUAP, 0, "[\xc2\x85-\xc2\x89\xc3\x89]", "\xc2\x84\xc3\xa9" },
	{ MUAP, 0, "[^b-d^&\\s]{3,}", "db^ !a\xe2\x80\xa8_ae" },
	{ MUAP, 0 | F_PROPERTY, "[^\\S\\P{Any}][\\sN]{1,3}[\\P{N}]{4}", "\xe2\x80\xaa\xa N\x9\xc3\xa9_0" },
	{ MUA, 0 | F_PROPERTY, "[^\\P{L}\x9!D-F\xa]{2,3}", "\x9,.DF\xa.CG\xc3\x81" },
	{ CMUAP, 0, "[\xc3\xa1-\xc3\xa9_\xe2\x80\xa0-\xe2\x80\xaf]{1,5}[^\xe2\x80\xa0-\xe2\x80\xaf]", "\xc2\xa1\xc3\x89\xc3\x89\xe2\x80\xaf_\xe2\x80\xa0" },
	{ MUAP, 0 | F_PROPERTY, "[\xc3\xa2-\xc3\xa6\xc3\x81-\xc3\x84\xe2\x80\xa8-\xe2\x80\xa9\xe6\x92\xad\\p{Zs}]{2,}", "\xe2\x80\xa7\xe2\x80\xa9\xe6\x92\xad \xe6\x92\xae" },
	{ MUAP, 0 | F_PROPERTY, "[\\P{L&}]{2}[^\xc2\x85-\xc2\x89\\p{Ll}\\p{Lu}]{2}", "\xc3\xa9\xe6\x92\xad.a\xe6\x92\xad|\xc2\x8a#" },
	{ PCRE_UCP, 0, "[a-b\\s]{2,5}[^a]", "AB  baaa" },

	/* Possible empty brackets. */
	{ MUA, 0, "(?:|ab||bc|a)+d", "abcxabcabd" },
	{ MUA, 0, "(|ab||bc|a)+d", "abcxabcabd" },
	{ MUA, 0, "(?:|ab||bc|a)*d", "abcxabcabd" },
	{ MUA, 0, "(|ab||bc|a)*d", "abcxabcabd" },
	{ MUA, 0, "(?:|ab||bc|a)+?d", "abcxabcabd" },
	{ MUA, 0, "(|ab||bc|a)+?d", "abcxabcabd" },
	{ MUA, 0, "(?:|ab||bc|a)*?d", "abcxabcabd" },
	{ MUA, 0, "(|ab||bc|a)*?d", "abcxabcabd" },
	{ MUA, 0, "(((a)*?|(?:ba)+)+?|(?:|c|ca)*)*m", "abaacaccabacabalabaacaccabacabamm" },
	{ MUA, 0, "(?:((?:a)*|(ba)+?)+|(|c|ca)*?)*?m", "abaacaccabacabalabaacaccabacabamm" },

	/* Start offset. */
	{ MUA, 3, "(\\d|(?:\\w)*\\w)+", "0ac01Hb" },
	{ MUA, 4 | F_NOMATCH, "(\\w\\W\\w)+", "ab#d" },
	{ MUA, 2 | F_NOMATCH, "(\\w\\W\\w)+", "ab#d" },
	{ MUA, 1, "(\\w\\W\\w)+", "ab#d" },

	/* Newline. */
	{ PCRE_MULTILINE | PCRE_NEWLINE_CRLF, 0, "\\W{0,2}[^#]{3}", "\r\n#....." },
	{ PCRE_MULTILINE | PCRE_NEWLINE_CR, 0, "\\W{0,2}[^#]{3}", "\r\n#....." },
	{ PCRE_MULTILINE | PCRE_NEWLINE_CRLF, 0, "\\W{1,3}[^#]", "\r\n##...." },
	{ MUA | PCRE_NO_UTF8_CHECK, 1, "^.a", "\n\x80\nxa" },
	{ MUA, 1, "^", "\r\n" },
	{ PCRE_MULTILINE | PCRE_NEWLINE_CRLF, 1 | F_NOMATCH, "^", "\r\n" },
	{ PCRE_MULTILINE | PCRE_NEWLINE_CRLF, 1, "^", "\r\na" },

	/* Any character except newline or any newline. */
	{ PCRE_NEWLINE_CRLF, 0, ".", "\r" },
	{ PCRE_NEWLINE_CRLF | PCRE_UTF8, 0, ".(.).", "a\xc3\xa1\r\n\n\r\r" },
	{ PCRE_NEWLINE_ANYCRLF, 0, ".(.)", "a\rb\nc\r\n\xc2\x85\xe2\x80\xa8" },
	{ PCRE_NEWLINE_ANYCRLF | PCRE_UTF8, 0, ".(.)", "a\rb\nc\r\n\xc2\x85\xe2\x80\xa8" },
	{ PCRE_NEWLINE_ANY | PCRE_UTF8, 0, "(.).", "a\rb\nc\r\n\xc2\x85\xe2\x80\xa9$de" },
	{ PCRE_NEWLINE_ANYCRLF | PCRE_UTF8, 0 | F_NOMATCH, ".(.).", "\xe2\x80\xa8\nb\r" },
	{ PCRE_NEWLINE_ANY, 0, "(.)(.)", "#\x85#\r#\n#\r\n#\x84" },
	{ PCRE_NEWLINE_ANY | PCRE_UTF8, 0, "(.+)#", "#\rMn\xc2\x85#\n###" },
	{ PCRE_BSR_ANYCRLF, 0, "\\R", "\r" },
	{ PCRE_BSR_ANYCRLF, 0, "\\R", "\x85#\r\n#" },
	{ PCRE_BSR_UNICODE | PCRE_UTF8, 0, "\\R", "ab\xe2\x80\xa8#c" },
	{ PCRE_BSR_UNICODE | PCRE_UTF8, 0, "\\R", "ab\r\nc" },
	{ PCRE_NEWLINE_CRLF | PCRE_BSR_UNICODE | PCRE_UTF8, 0, "(\\R.)+", "\xc2\x85\r\n#\xe2\x80\xa8\n\r\n\r" },
	{ MUA, 0 | F_NOMATCH, "\\R+", "ab" },
	{ MUA, 0, "\\R+", "ab\r\n\r" },
	{ MUA, 0, "\\R*", "ab\r\n\r" },
	{ MUA, 0, "\\R*", "\r\n\r" },
	{ MUA, 0, "\\R{2,4}", "\r\nab\r\r" },
	{ MUA, 0, "\\R{2,4}", "\r\nab\n\n\n\r\r\r" },
	{ MUA, 0, "\\R{2,}", "\r\nab\n\n\n\r\r\r" },
	{ MUA, 0, "\\R{0,3}", "\r\n\r\n\r\n\r\n\r\n" },
	{ MUA, 0 | F_NOMATCH, "\\R+\\R\\R", "\r\n\r\n" },
	{ MUA, 0, "\\R+\\R\\R", "\r\r\r" },
	{ MUA, 0, "\\R*\\R\\R", "\n\r" },
	{ MUA, 0 | F_NOMATCH, "\\R{2,4}\\R\\R", "\r\r\r" },
	{ MUA, 0, "\\R{2,4}\\R\\R", "\r\r\r\r" },

	/* Atomic groups (no fallback from "next" direction). */
	{ MUA, 0 | F_NOMATCH, "(?>ab)ab", "bab" },
	{ MUA, 0 | F_NOMATCH, "(?>(ab))ab", "bab" },
	{ MUA, 0, "(?>ab)+abc(?>de)*def(?>gh)?ghe(?>ij)+?k(?>lm)*?n(?>op)?\?op",
			"bababcdedefgheijijklmlmnop" },
	{ MUA, 0, "(?>a(b)+a|(ab)?\?(b))an", "abban" },
	{ MUA, 0, "(?>ab+a|(?:ab)?\?b)an", "abban" },
	{ MUA, 0, "((?>ab|ad|)*?)(?>|c)*abad", "abababcababad" },
	{ MUA, 0, "(?>(aa|b|)*+(?>(##)|###)*d|(aa)(?>(baa)?)m)", "aabaa#####da" },
	{ MUA, 0, "((?>a|)+?)b", "aaacaaab" },
	{ MUA, 0, "(?>x|)*$", "aaa" },
	{ MUA, 0, "(?>(x)|)*$", "aaa" },
	{ MUA, 0, "(?>x|())*$", "aaa" },
	{ MUA, 0, "((?>[cxy]a|[a-d])*?)b", "aaa+ aaab" },
	{ MUA, 0, "((?>[cxy](a)|[a-d])*?)b", "aaa+ aaab" },
	{ MUA, 0, "(?>((?>(a+))))bab|(?>((?>(a+))))bb", "aaaabaaabaabab" },
	{ MUA, 0, "(?>(?>a+))bab|(?>(?>a+))bb", "aaaabaaabaabab" },
	{ MUA, 0, "(?>(a)c|(?>(c)|(a))a)b*?bab", "aaaabaaabaabab" },
	{ MUA, 0, "(?>ac|(?>c|a)a)b*?bab", "aaaabaaabaabab" },
	{ MUA, 0, "(?>(b)b|(a))*b(?>(c)|d)?x", "ababcaaabdbx" },
	{ MUA, 0, "(?>bb|a)*b(?>c|d)?x", "ababcaaabdbx" },
	{ MUA, 0, "(?>(bb)|a)*b(?>c|(d))?x", "ababcaaabdbx" },
	{ MUA, 0, "(?>(a))*?(?>(a))+?(?>(a))??x", "aaaaaacccaaaaabax" },
	{ MUA, 0, "(?>a)*?(?>a)+?(?>a)??x", "aaaaaacccaaaaabax" },
	{ MUA, 0, "(?>(a)|)*?(?>(a)|)+?(?>(a)|)??x", "aaaaaacccaaaaabax" },
	{ MUA, 0, "(?>a|)*?(?>a|)+?(?>a|)??x", "aaaaaacccaaaaabax" },
	{ MUA, 0, "(?>a(?>(a{0,2}))*?b|aac)+b", "aaaaaaacaaaabaaaaacaaaabaacaaabb" },
	{ CMA, 0, "(?>((?>a{32}|b+|(a*))?(?>c+|d*)?\?)+e)+?f", "aaccebbdde bbdaaaccebbdee bbdaaaccebbdeef" },
	{ MUA, 0, "(?>(?:(?>aa|a||x)+?b|(?>aa|a||(x))+?c)?(?>[ad]{0,2})*?d)+d", "aaacdbaabdcabdbaaacd aacaabdbdcdcaaaadaabcbaadd" },
	{ MUA, 0, "(?>(?:(?>aa|a||(x))+?b|(?>aa|a||x)+?c)?(?>[ad]{0,2})*?d)+d", "aaacdbaabdcabdbaaacd aacaabdbdcdcaaaadaabcbaadd" },
	{ MUA, 0 | F_PROPERTY, "\\X", "\xcc\x8d\xcc\x8d" },
	{ MUA, 0 | F_PROPERTY, "\\X", "\xcc\x8d\xcc\x8d#\xcc\x8d\xcc\x8d" },
	{ MUA, 0 | F_PROPERTY, "\\X+..", "\xcc\x8d#\xcc\x8d#\xcc\x8d\xcc\x8d" },
	{ MUA, 0 | F_PROPERTY, "\\X{2,4}", "abcdef" },
	{ MUA, 0 | F_PROPERTY, "\\X{2,4}?", "abcdef" },
	{ MUA, 0 | F_NOMATCH | F_PROPERTY, "\\X{2,4}..", "#\xcc\x8d##" },
	{ MUA, 0 | F_PROPERTY, "\\X{2,4}..", "#\xcc\x8d#\xcc\x8d##" },
	{ MUA, 0, "(c(ab)?+ab)+", "cabcababcab" },
	{ MUA, 0, "(?>(a+)b)+aabab", "aaaabaaabaabab" },

	/* Possessive quantifiers. */
	{ MUA, 0, "(?:a|b)++m", "mababbaaxababbaam" },
	{ MUA, 0, "(?:a|b)*+m", "mababbaaxababbaam" },
	{ MUA, 0, "(?:a|b)*+m", "ababbaaxababbaam" },
	{ MUA, 0, "(a|b)++m", "mababbaaxababbaam" },
	{ MUA, 0, "(a|b)*+m", "mababbaaxababbaam" },
	{ MUA, 0, "(a|b)*+m", "ababbaaxababbaam" },
	{ MUA, 0, "(a|b(*ACCEPT))++m", "maaxab" },
	{ MUA, 0, "(?:b*)++m", "bxbbxbbbxm" },
	{ MUA, 0, "(?:b*)++m", "bxbbxbbbxbbm" },
	{ MUA, 0, "(?:b*)*+m", "bxbbxbbbxm" },
	{ MUA, 0, "(?:b*)*+m", "bxbbxbbbxbbm" },
	{ MUA, 0, "(b*)++m", "bxbbxbbbxm" },
	{ MUA, 0, "(b*)++m", "bxbbxbbbxbbm" },
	{ MUA, 0, "(b*)*+m", "bxbbxbbbxm" },
	{ MUA, 0, "(b*)*+m", "bxbbxbbbxbbm" },
	{ MUA, 0, "(?:a|(b))++m", "mababbaaxababbaam" },
	{ MUA, 0, "(?:(a)|b)*+m", "mababbaaxababbaam" },
	{ MUA, 0, "(?:(a)|(b))*+m", "ababbaaxababbaam" },
	{ MUA, 0, "(a|(b))++m", "mababbaaxababbaam" },
	{ MUA, 0, "((a)|b)*+m", "mababbaaxababbaam" },
	{ MUA, 0, "((a)|(b))*+m", "ababbaaxababbaam" },
	{ MUA, 0, "(a|(b)(*ACCEPT))++m", "maaxab" },
	{ MUA, 0, "(?:(b*))++m", "bxbbxbbbxm" },
	{ MUA, 0, "(?:(b*))++m", "bxbbxbbbxbbm" },
	{ MUA, 0, "(?:(b*))*+m", "bxbbxbbbxm" },
	{ MUA, 0, "(?:(b*))*+m", "bxbbxbbbxbbm" },
	{ MUA, 0, "((b*))++m", "bxbbxbbbxm" },
	{ MUA, 0, "((b*))++m", "bxbbxbbbxbbm" },
	{ MUA, 0, "((b*))*+m", "bxbbxbbbxm" },
	{ MUA, 0, "((b*))*+m", "bxbbxbbbxbbm" },
	{ MUA, 0 | F_NOMATCH, "(?>(b{2,4}))(?:(?:(aa|c))++m|(?:(aa|c))+n)", "bbaacaaccaaaacxbbbmbn" },
	{ MUA, 0, "((?:b)++a)+(cd)*+m", "bbababbacdcdnbbababbacdcdm" },
	{ MUA, 0, "((?:(b))++a)+((c)d)*+m", "bbababbacdcdnbbababbacdcdm" },
	{ MUA, 0, "(?:(?:(?:ab)*+k)++(?:n(?:cd)++)*+)*+m", "ababkkXababkkabkncXababkkabkncdcdncdXababkkabkncdcdncdkkabkncdXababkkabkncdcdncdkkabkncdm" },
	{ MUA, 0, "(?:((ab)*+(k))++(n(?:c(d))++)*+)*+m", "ababkkXababkkabkncXababkkabkncdcdncdXababkkabkncdcdncdkkabkncdXababkkabkncdcdncdkkabkncdm" },

	/* Back references. */
	{ MUA, 0, "(aa|bb)(\\1*)(ll|)(\\3*)bbbbbbc", "aaaaaabbbbbbbbc" },
	{ CMUA, 0, "(aa|bb)(\\1+)(ll|)(\\3+)bbbbbbc", "bBbbBbCbBbbbBbbcbbBbbbBBbbC" },
	{ CMA, 0, "(a{2,4})\\1", "AaAaaAaA" },
	{ MUA, 0, "(aa|bb)(\\1?)aa(\\1?)(ll|)(\\4+)bbc", "aaaaaaaabbaabbbbaabbbbc" },
	{ MUA, 0, "(aa|bb)(\\1{0,5})(ll|)(\\3{0,5})cc", "bbxxbbbbxxaaaaaaaaaaaaaaaacc" },
	{ MUA, 0, "(aa|bb)(\\1{3,5})(ll|)(\\3{3,5})cc", "bbbbbbbbbbbbaaaaaaccbbbbbbbbbbbbbbcc" },
	{ MUA, 0, "(aa|bb)(\\1{3,})(ll|)(\\3{3,})cc", "bbbbbbbbbbbbaaaaaaccbbbbbbbbbbbbbbcc" },
	{ MUA, 0, "(\\w+)b(\\1+)c", "GabGaGaDbGaDGaDc" },
	{ MUA, 0, "(?:(aa)|b)\\1?b", "bb" },
	{ CMUA, 0, "(aa|bb)(\\1*?)aa(\\1+?)", "bBBbaaAAaaAAaa" },
	{ MUA, 0, "(aa|bb)(\\1*?)(dd|)cc(\\3+?)", "aaaaaccdd" },
	{ CMUA, 0, "(?:(aa|bb)(\\1?\?)cc){2}(\\1?\?)", "aAaABBbbAAaAcCaAcCaA" },
	{ MUA, 0, "(?:(aa|bb)(\\1{3,5}?)){2}(dd|)(\\3{3,5}?)", "aaaaaabbbbbbbbbbaaaaaaaaaaaaaa" },
	{ CMA, 0, "(?:(aa|bb)(\\1{3,}?)){2}(dd|)(\\3{3,}?)", "aaaaaabbbbbbbbbbaaaaaaaaaaaaaa" },
	{ MUA, 0, "(?:(aa|bb)(\\1{0,3}?)){2}(dd|)(\\3{0,3}?)b(\\1{0,3}?)(\\1{0,3})", "aaaaaaaaaaaaaaabaaaaa" },
	{ MUA, 0, "(a(?:\\1|)a){3}b", "aaaaaaaaaaab" },
	{ MA, 0, "(a?)b(\\1\\1*\\1+\\1?\\1*?\\1+?\\1??\\1*+\\1++\\1?+\\1{4}\\1{3,5}\\1{4,}\\1{0,5}\\1{3,5}?\\1{4,}?\\1{0,5}?\\1{3,5}+\\1{4,}+\\1{0,5}+#){2}d", "bb#b##d" },
	{ MUAP, 0 | F_PROPERTY, "(\\P{N})\\1{2,}", ".www." },
	{ MUAP, 0 | F_PROPERTY, "(\\P{N})\\1{0,2}", "wwwww." },
	{ MUAP, 0 | F_PROPERTY, "(\\P{N})\\1{1,2}ww", "wwww" },
	{ MUAP, 0 | F_PROPERTY, "(\\P{N})\\1{1,2}ww", "wwwww" },
	{ PCRE_UCP, 0 | F_PROPERTY, "(\\P{N})\\1{2,}", ".www." },
	{ CMUAP, 0, "(\xf0\x90\x90\x80)\\1", "\xf0\x90\x90\xa8\xf0\x90\x90\xa8" },
	{ MUA | PCRE_DUPNAMES, 0 | F_NOMATCH, "\\k<A>{1,3}(?<A>aa)(?<A>bb)", "aabb" },
	{ MUA | PCRE_DUPNAMES | PCRE_JAVASCRIPT_COMPAT, 0, "\\k<A>{1,3}(?<A>aa)(?<A>bb)", "aabb" },
	{ MUA | PCRE_DUPNAMES | PCRE_JAVASCRIPT_COMPAT, 0, "\\k<A>*(?<A>aa)(?<A>bb)", "aabb" },
	{ MUA | PCRE_DUPNAMES, 0, "(?<A>aa)(?<A>bb)\\k<A>{0,3}aaaaaa", "aabbaaaaaa" },
	{ MUA | PCRE_DUPNAMES, 0, "(?<A>aa)(?<A>bb)\\k<A>{2,5}bb", "aabbaaaabb" },
	{ MUA | PCRE_DUPNAMES, 0, "(?:(?<A>aa)|(?<A>bb))\\k<A>{0,3}m", "aaaaaaaabbbbaabbbbm" },
	{ MUA | PCRE_DUPNAMES, 0 | F_NOMATCH, "\\k<A>{1,3}?(?<A>aa)(?<A>bb)", "aabb" },
	{ MUA | PCRE_DUPNAMES | PCRE_JAVASCRIPT_COMPAT, 0, "\\k<A>{1,3}?(?<A>aa)(?<A>bb)", "aabb" },
	{ MUA | PCRE_DUPNAMES, 0, "\\k<A>*?(?<A>aa)(?<A>bb)", "aabb" },
	{ MUA | PCRE_DUPNAMES, 0, "(?:(?<A>aa)|(?<A>bb))\\k<A>{0,3}?m", "aaaaaabbbbbbaabbbbbbbbbbm" },
	{ MUA | PCRE_DUPNAMES, 0, "(?:(?<A>aa)|(?<A>bb))\\k<A>*?m", "aaaaaabbbbbbaabbbbbbbbbbm" },
	{ MUA | PCRE_DUPNAMES, 0, "(?:(?<A>aa)|(?<A>bb))\\k<A>{2,3}?", "aaaabbbbaaaabbbbbbbbbb" },
	{ CMUA | PCRE_DUPNAMES, 0, "(?:(?<A>AA)|(?<A>BB))\\k<A>{0,3}M", "aaaaaaaabbbbaabbbbm" },
	{ CMUA | PCRE_DUPNAMES, 0, "(?:(?<A>AA)|(?<A>BB))\\k<A>{1,3}M", "aaaaaaaabbbbaabbbbm" },
	{ CMUA | PCRE_DUPNAMES, 0, "(?:(?<A>AA)|(?<A>BB))\\k<A>{0,3}?M", "aaaaaabbbbbbaabbbbbbbbbbm" },
	{ CMUA | PCRE_DUPNAMES, 0, "(?:(?<A>AA)|(?<A>BB))\\k<A>{2,3}?", "aaaabbbbaaaabbbbbbbbbb" },

	/* Assertions. */
	{ MUA, 0, "(?=xx|yy|zz)\\w{4}", "abczzdefg" },
	{ MUA, 0, "(?=((\\w+)b){3}|ab)", "dbbbb ab" },
	{ MUA, 0, "(?!ab|bc|cd)[a-z]{2}", "Xabcdef" },
	{ MUA, 0, "(?<=aaa|aa|a)a", "aaa" },
	{ MUA, 2, "(?<=aaa|aa|a)a", "aaa" },
	{ MA, 0, "(?<=aaa|aa|a)a", "aaa" },
	{ MA, 2, "(?<=aaa|aa|a)a", "aaa" },
	{ MUA, 0, "(\\d{2})(?!\\w+c|(((\\w?)m){2}n)+|\\1)", "x5656" },
	{ MUA, 0, "((?=((\\d{2,6}\\w){2,}))\\w{5,20}K){2,}", "567v09708K12l00M00 567v09708K12l00M00K45K" },
	{ MUA, 0, "(?=(?:(?=\\S+a)\\w*(b)){3})\\w+\\d", "bba bbab nbbkba nbbkba0kl" },
	{ MUA, 0, "(?>a(?>(b+))a(?=(..)))*?k", "acabbcabbaabacabaabbakk" },
	{ MUA, 0, "((?(?=(a))a)+k)", "bbak" },
	{ MUA, 0, "((?(?=a)a)+k)", "bbak" },
	{ MUA, 0 | F_NOMATCH, "(?=(?>(a))m)amk", "a k" },
	{ MUA, 0 | F_NOMATCH, "(?!(?>(a))m)amk", "a k" },
	{ MUA, 0 | F_NOMATCH, "(?>(?=(a))am)amk", "a k" },
	{ MUA, 0, "(?=(?>a|(?=(?>(b+))a|c)[a-c]+)*?m)[a-cm]+k", "aaam bbam baaambaam abbabba baaambaamk" },
	{ MUA, 0, "(?> ?\?\\b(?(?=\\w{1,4}(a))m)\\w{0,8}bc){2,}?", "bca ssbc mabd ssbc mabc" },
	{ MUA, 0, "(?:(?=ab)?[^n][^n])+m", "ababcdabcdcdabnababcdabcdcdabm" },
	{ MUA, 0, "(?:(?=a(b))?[^n][^n])+m", "ababcdabcdcdabnababcdabcdcdabm" },
	{ MUA, 0, "(?:(?=.(.))??\\1.)+m", "aabbbcbacccanaabbbcbacccam" },
	{ MUA, 0, "(?:(?=.)??[a-c])+m", "abacdcbacacdcaccam" },
	{ MUA, 0, "((?!a)?(?!([^a]))?)+$", "acbab" },
	{ MUA, 0, "((?!a)?\?(?!([^a]))?\?)+$", "acbab" },

	/* Not empty, ACCEPT, FAIL */
	{ MUA | PCRE_NOTEMPTY, 0 | F_NOMATCH, "a*", "bcx" },
	{ MUA | PCRE_NOTEMPTY, 0, "a*", "bcaad" },
	{ MUA | PCRE_NOTEMPTY, 0, "a*?", "bcaad" },
	{ MUA | PCRE_NOTEMPTY_ATSTART, 0, "a*", "bcaad" },
	{ MUA, 0, "a(*ACCEPT)b", "ab" },
	{ MUA | PCRE_NOTEMPTY, 0 | F_NOMATCH, "a*(*ACCEPT)b", "bcx" },
	{ MUA | PCRE_NOTEMPTY, 0, "a*(*ACCEPT)b", "bcaad" },
	{ MUA | PCRE_NOTEMPTY, 0, "a*?(*ACCEPT)b", "bcaad" },
	{ MUA | PCRE_NOTEMPTY, 0 | F_NOMATCH, "(?:z|a*(*ACCEPT)b)", "bcx" },
	{ MUA | PCRE_NOTEMPTY, 0, "(?:z|a*(*ACCEPT)b)", "bcaad" },
	{ MUA | PCRE_NOTEMPTY, 0, "(?:z|a*?(*ACCEPT)b)", "bcaad" },
	{ MUA | PCRE_NOTEMPTY_ATSTART, 0, "a*(*ACCEPT)b", "bcx" },
	{ MUA | PCRE_NOTEMPTY_ATSTART, 0 | F_NOMATCH, "a*(*ACCEPT)b", "" },
	{ MUA, 0, "((a(*ACCEPT)b))", "ab" },
	{ MUA, 0, "(a(*FAIL)a|a)", "aaa" },
	{ MUA, 0, "(?=ab(*ACCEPT)b)a", "ab" },
	{ MUA, 0, "(?=(?:x|ab(*ACCEPT)b))", "ab" },
	{ MUA, 0, "(?=(a(b(*ACCEPT)b)))a", "ab" },
	{ MUA | PCRE_NOTEMPTY, 0, "(?=a*(*ACCEPT))c", "c" },

	/* Conditional blocks. */
	{ MUA, 0, "(?(?=(a))a|b)+k", "ababbalbbadabak" },
	{ MUA, 0, "(?(?!(b))a|b)+k", "ababbalbbadabak" },
	{ MUA, 0, "(?(?=a)a|b)+k", "ababbalbbadabak" },
	{ MUA, 0, "(?(?!b)a|b)+k", "ababbalbbadabak" },
	{ MUA, 0, "(?(?=(a))a*|b*)+k", "ababbalbbadabak" },
	{ MUA, 0, "(?(?!(b))a*|b*)+k", "ababbalbbadabak" },
	{ MUA, 0, "(?(?!(b))(?:aaaaaa|a)|(?:bbbbbb|b))+aaaak", "aaaaaaaaaaaaaa bbbbbbbbbbbbbbb aaaaaaak" },
	{ MUA, 0, "(?(?!b)(?:aaaaaa|a)|(?:bbbbbb|b))+aaaak", "aaaaaaaaaaaaaa bbbbbbbbbbbbbbb aaaaaaak" },
	{ MUA, 0 | F_DIFF, "(?(?!(b))(?:aaaaaa|a)|(?:bbbbbb|b))+bbbbk", "aaaaaaaaaaaaaa bbbbbbbbbbbbbbb bbbbbbbk" },
	{ MUA, 0, "(?(?!b)(?:aaaaaa|a)|(?:bbbbbb|b))+bbbbk", "aaaaaaaaaaaaaa bbbbbbbbbbbbbbb bbbbbbbk" },
	{ MUA, 0, "(?(?=a)a*|b*)+k", "ababbalbbadabak" },
	{ MUA, 0, "(?(?!b)a*|b*)+k", "ababbalbbadabak" },
	{ MUA, 0, "(?(?=a)ab)", "a" },
	{ MUA, 0, "(?(?<!b)c)", "b" },
	{ MUA, 0, "(?(DEFINE)a(b))", "a" },
	{ MUA, 0, "a(?(DEFINE)(?:b|(?:c?)+)*)", "a" },
	{ MUA, 0, "(?(?=.[a-c])[k-l]|[A-D])", "kdB" },
	{ MUA, 0, "(?(?!.{0,4}[cd])(aa|bb)|(cc|dd))+", "aabbccddaa" },
	{ MUA, 0, "(?(?=[^#@]*@)(aaab|aa|aba)|(aba|aab)){3,}", "aaabaaaba#aaabaaaba#aaabaaaba@" },
	{ MUA, 0, "((?=\\w{5})\\w(?(?=\\w*k)\\d|[a-f_])*\\w\\s)+", "mol m10kk m088k _f_a_ mbkkl" },
	{ MUA, 0, "(c)?\?(?(1)a|b)", "cdcaa" },
	{ MUA, 0, "(c)?\?(?(1)a|b)", "cbb" },
	{ MUA, 0 | F_DIFF, "(?(?=(a))(aaaa|a?))+aak", "aaaaab aaaaak" },
	{ MUA, 0, "(?(?=a)(aaaa|a?))+aak", "aaaaab aaaaak" },
	{ MUA, 0, "(?(?!(b))(aaaa|a?))+aak", "aaaaab aaaaak" },
	{ MUA, 0, "(?(?!b)(aaaa|a?))+aak", "aaaaab aaaaak" },
	{ MUA, 0 | F_DIFF, "(?(?=(a))a*)+aak", "aaaaab aaaaak" },
	{ MUA, 0, "(?(?=a)a*)+aak", "aaaaab aaaaak" },
	{ MUA, 0, "(?(?!(b))a*)+aak", "aaaaab aaaaak" },
	{ MUA, 0, "(?(?!b)a*)+aak", "aaaaab aaaaak" },
	{ MUA, 0, "(?(?=(?=(?!(x))a)aa)aaa|(?(?=(?!y)bb)bbb))*k", "abaabbaaabbbaaabbb abaabbaaabbbaaabbbk" },
	{ MUA, 0, "(?P<Name>a)?(?P<Name2>b)?(?(Name)c|d)*l", "bc ddd abccabccl" },
	{ MUA, 0, "(?P<Name>a)?(?P<Name2>b)?(?(Name)c|d)+?dd", "bcabcacdb bdddd" },
	{ MUA, 0, "(?P<Name>a)?(?P<Name2>b)?(?(Name)c|d)+l", "ababccddabdbccd abcccl" },
	{ MUA, 0, "((?:a|aa)(?(1)aaa))x", "aax" },
	{ MUA, 0, "(?(?!)a|b)", "ab" },
	{ MUA, 0, "(?(?!)a)", "ab" },
	{ MUA, 0 | F_NOMATCH, "(?(?!)a|b)", "ac" },

	/* Set start of match. */
	{ MUA, 0, "(?:\\Ka)*aaaab", "aaaaaaaa aaaaaaabb" },
	{ MUA, 0, "(?>\\Ka\\Ka)*aaaab", "aaaaaaaa aaaaaaaaaabb" },
	{ MUA, 0, "a+\\K(?<=\\Gaa)a", "aaaaaa" },
	{ MUA | PCRE_NOTEMPTY, 0 | F_NOMATCH, "a\\K(*ACCEPT)b", "aa" },
	{ MUA | PCRE_NOTEMPTY_ATSTART, 0, "a\\K(*ACCEPT)b", "aa" },

	/* First line. */
	{ MUA | PCRE_FIRSTLINE, 0 | F_PROPERTY, "\\p{Any}a", "bb\naaa" },
	{ MUA | PCRE_FIRSTLINE, 0 | F_NOMATCH | F_PROPERTY, "\\p{Any}a", "bb\r\naaa" },
	{ MUA | PCRE_FIRSTLINE, 0, "(?<=a)", "a" },
	{ MUA | PCRE_FIRSTLINE, 0 | F_NOMATCH, "[^a][^b]", "ab" },
	{ MUA | PCRE_FIRSTLINE, 0 | F_NOMATCH, "a", "\na" },
	{ MUA | PCRE_FIRSTLINE, 0 | F_NOMATCH, "[abc]", "\na" },
	{ MUA | PCRE_FIRSTLINE, 0 | F_NOMATCH, "^a", "\na" },
	{ MUA | PCRE_FIRSTLINE, 0 | F_NOMATCH, "^(?<=\n)", "\na" },
	{ MUA | PCRE_FIRSTLINE, 0, "\xf0\x90\x90\x80", "\xf0\x90\x90\x80" },
	{ PCRE_MULTILINE | PCRE_UTF8 | PCRE_NEWLINE_ANY | PCRE_FIRSTLINE, 0 | F_NOMATCH, "#", "\xc2\x85#" },
	{ PCRE_MULTILINE | PCRE_NEWLINE_ANY | PCRE_FIRSTLINE, 0 | F_NOMATCH, "#", "\x85#" },
	{ PCRE_MULTILINE | PCRE_UTF8 | PCRE_NEWLINE_ANY | PCRE_FIRSTLINE, 0 | F_NOMATCH, "^#", "\xe2\x80\xa8#" },
	{ PCRE_MULTILINE | PCRE_UTF8 | PCRE_NEWLINE_CRLF | PCRE_FIRSTLINE, 0 | F_PROPERTY, "\\p{Any}", "\r\na" },
	{ PCRE_MULTILINE | PCRE_UTF8 | PCRE_NEWLINE_CRLF | PCRE_FIRSTLINE, 0, ".", "\r" },
	{ PCRE_MULTILINE | PCRE_UTF8 | PCRE_NEWLINE_CRLF | PCRE_FIRSTLINE, 0, "a", "\ra" },
	{ PCRE_MULTILINE | PCRE_UTF8 | PCRE_NEWLINE_CRLF | PCRE_FIRSTLINE, 0 | F_NOMATCH, "ba", "bbb\r\nba" },
	{ PCRE_MULTILINE | PCRE_UTF8 | PCRE_NEWLINE_CRLF | PCRE_FIRSTLINE, 0 | F_NOMATCH | F_PROPERTY, "\\p{Any}{4}|a", "\r\na" },
	{ PCRE_MULTILINE | PCRE_UTF8 | PCRE_NEWLINE_CRLF | PCRE_FIRSTLINE, 1, ".", "\r\n" },
	{ PCRE_FIRSTLINE | PCRE_NEWLINE_LF | PCRE_DOTALL, 0 | F_NOMATCH, "ab.", "ab" },
	{ MUA | PCRE_FIRSTLINE, 1 | F_NOMATCH, "^[a-d0-9]", "\nxx\nd" },

	/* Recurse. */
	{ MUA, 0, "(a)(?1)", "aa" },
	{ MUA, 0, "((a))(?1)", "aa" },
	{ MUA, 0, "(b|a)(?1)", "aa" },
	{ MUA, 0, "(b|(a))(?1)", "aa" },
	{ MUA, 0 | F_NOMATCH, "((a)(b)(?:a*))(?1)", "aba" },
	{ MUA, 0, "((a)(b)(?:a*))(?1)", "abab" },
	{ MUA, 0, "((a+)c(?2))b(?1)", "aacaabaca" },
	{ MUA, 0, "((?2)b|(a)){2}(?1)", "aabab" },
	{ MUA, 0, "(?1)(a)*+(?2)(b(?1))", "aababa" },
	{ MUA, 0, "(?1)(((a(*ACCEPT)))b)", "axaa" },
	{ MUA, 0, "(?1)(?(DEFINE) (((ac(*ACCEPT)))b) )", "akaac" },
	{ MUA, 0, "(a+)b(?1)b\\1", "abaaabaaaaa" },
	{ MUA, 0 | F_NOMATCH, "(?(DEFINE)(aa|a))(?1)ab", "aab" },
	{ MUA, 0, "(?(DEFINE)(a\\Kb))(?1)+ababc", "abababxabababc" },
	{ MUA, 0, "(a\\Kb)(?1)+ababc", "abababxababababc" },
	{ MUA, 0 | F_NOMATCH, "(a\\Kb)(?1)+ababc", "abababxababababxc" },
	{ MUA, 0, "b|<(?R)*>", "<<b>" },
	{ MUA, 0, "(a\\K){0}(?:(?1)b|ac)", "ac" },
	{ MUA, 0, "(?(DEFINE)(a(?2)|b)(b(?1)|(a)))(?:(?1)|(?2))m", "ababababnababababaam" },
	{ MUA, 0, "(a)((?(R)a|b))(?2)", "aabbabaa" },
	{ MUA, 0, "(a)((?(R2)a|b))(?2)", "aabbabaa" },
	{ MUA, 0, "(a)((?(R1)a|b))(?2)", "ababba" },
	{ MUA, 0, "(?(R0)aa|bb(?R))", "abba aabb bbaa" },
	{ MUA, 0, "((?(R)(?:aaaa|a)|(?:(aaaa)|(a)))+)(?1)$", "aaaaaaaaaa aaaa" },
	{ MUA, 0, "(?P<Name>a(?(R&Name)a|b))(?1)", "aab abb abaa" },
	{ MUA, 0, "((?(R)a|(?1)){3})", "XaaaaaaaaaX" },
	{ MUA, 0, "((?:(?(R)a|(?1))){3})", "XaaaaaaaaaX" },
	{ MUA, 0, "((?(R)a|(?1)){1,3})aaaaaa", "aaaaaaaaXaaaaaaaaa" },
	{ MUA, 0, "((?(R)a|(?1)){1,3}?)M", "aaaM" },

	/* 16 bit specific tests. */
	{ CMA, 0 | F_FORCECONV, "\xc3\xa1", "\xc3\x81\xc3\xa1" },
	{ CMA, 0 | F_FORCECONV, "\xe1\xbd\xb8", "\xe1\xbf\xb8\xe1\xbd\xb8" },
	{ CMA, 0 | F_FORCECONV, "[\xc3\xa1]", "\xc3\x81\xc3\xa1" },
	{ CMA, 0 | F_FORCECONV, "[\xe1\xbd\xb8]", "\xe1\xbf\xb8\xe1\xbd\xb8" },
	{ CMA, 0 | F_FORCECONV, "[a-\xed\xb0\x80]", "A" },
	{ CMA, 0 | F_NO8 | F_FORCECONV, "[a-\\x{dc00}]", "B" },
	{ CMA, 0 | F_NO8 | F_NOMATCH | F_FORCECONV, "[b-\\x{dc00}]", "a" },
	{ CMA, 0 | F_NO8 | F_FORCECONV, "\xed\xa0\x80\\x{d800}\xed\xb0\x80\\x{dc00}", "\xed\xa0\x80\xed\xa0\x80\xed\xb0\x80\xed\xb0\x80" },
	{ CMA, 0 | F_NO8 | F_FORCECONV, "[\xed\xa0\x80\\x{d800}]{1,2}?[\xed\xb0\x80\\x{dc00}]{1,2}?#", "\xed\xa0\x80\xed\xa0\x80\xed\xb0\x80\xed\xb0\x80#" },
	{ CMA, 0 | F_FORCECONV, "[\xed\xa0\x80\xed\xb0\x80#]{0,3}(?<=\xed\xb0\x80.)", "\xed\xa0\x80#\xed\xa0\x80##\xed\xb0\x80\xed\xa0\x80" },
	{ CMA, 0 | F_FORCECONV, "[\xed\xa0\x80-\xed\xb3\xbf]", "\xed\x9f\xbf\xed\xa0\x83" },
	{ CMA, 0 | F_FORCECONV, "[\xed\xa0\x80-\xed\xb3\xbf]", "\xed\xb4\x80\xed\xb3\xb0" },
	{ CMA, 0 | F_NO8 | F_FORCECONV, "[\\x{d800}-\\x{dcff}]", "\xed\x9f\xbf\xed\xa0\x83" },
	{ CMA, 0 | F_NO8 | F_FORCECONV, "[\\x{d800}-\\x{dcff}]", "\xed\xb4\x80\xed\xb3\xb0" },
	{ CMA, 0 | F_FORCECONV, "[\xed\xa0\x80-\xef\xbf\xbf]+[\x1-\xed\xb0\x80]+#", "\xed\xa0\x85\xc3\x81\xed\xa0\x85\xef\xbf\xb0\xc2\x85\xed\xa9\x89#" },
	{ CMA, 0 | F_FORCECONV, "[\xed\xa0\x80][\xed\xb0\x80]{2,}", "\xed\xa0\x80\xed\xb0\x80\xed\xa0\x80\xed\xb0\x80\xed\xb0\x80\xed\xb0\x80" },
	{ MA, 0 | F_FORCECONV, "[^\xed\xb0\x80]{3,}?", "##\xed\xb0\x80#\xed\xb0\x80#\xc3\x89#\xed\xb0\x80" },
	{ MA, 0 | F_NO8 | F_FORCECONV, "[^\\x{dc00}]{3,}?", "##\xed\xb0\x80#\xed\xb0\x80#\xc3\x89#\xed\xb0\x80" },
	{ CMA, 0 | F_FORCECONV, ".\\B.", "\xed\xa0\x80\xed\xb0\x80" },
	{ CMA, 0 | F_FORCECONV, "\\D+(?:\\d+|.)\\S+(?:\\s+|.)\\W+(?:\\w+|.)\xed\xa0\x80\xed\xa0\x80", "\xed\xa0\x80\xed\xa0\x80\xed\xa0\x80\xed\xa0\x80\xed\xa0\x80\xed\xa0\x80\xed\xa0\x80\xed\xa0\x80" },
	{ CMA, 0 | F_FORCECONV, "\\d*\\s*\\w*\xed\xa0\x80\xed\xa0\x80", "\xed\xa0\x80\xed\xa0\x80" },
	{ CMA, 0 | F_FORCECONV | F_NOMATCH, "\\d*?\\D*?\\s*?\\S*?\\w*?\\W*?##", "\xed\xa0\x80\xed\xa0\x80\xed\xa0\x80\xed\xa0\x80#" },
	{ CMA | PCRE_EXTENDED, 0 | F_FORCECONV, "\xed\xa0\x80 \xed\xb0\x80 !", "\xed\xa0\x80\xed\xb0\x80!" },
	{ CMA, 0 | F_FORCECONV, "\xed\xa0\x80+#[^#]+\xed\xa0\x80", "\xed\xa0\x80#a\xed\xa0\x80" },
	{ CMA, 0 | F_FORCECONV, "(\xed\xa0\x80+)#\\1", "\xed\xa0\x80\xed\xa0\x80#\xed\xa0\x80\xed\xa0\x80" },
	{ PCRE_MULTILINE | PCRE_NEWLINE_ANY, 0 | F_NO8 | F_FORCECONV, "^-", "a--\xe2\x80\xa8--" },
	{ PCRE_BSR_UNICODE, 0 | F_NO8 | F_FORCECONV, "\\R", "ab\xe2\x80\xa8" },
	{ 0, 0 | F_NO8 | F_FORCECONV, "\\v", "ab\xe2\x80\xa9" },
	{ 0, 0 | F_NO8 | F_FORCECONV, "\\h", "ab\xe1\xa0\x8e" },
	{ 0, 0 | F_NO8 | F_FORCECONV, "\\v+?\\V+?#", "\xe2\x80\xa9\xe2\x80\xa9\xef\xbf\xbf\xef\xbf\xbf#" },
	{ 0, 0 | F_NO8 | F_FORCECONV, "\\h+?\\H+?#", "\xe1\xa0\x8e\xe1\xa0\x8e\xef\xbf\xbf\xef\xbf\xbf#" },

	/* Partial matching. */
	{ MUA | PCRE_PARTIAL_SOFT, 0, "ab", "a" },
	{ MUA | PCRE_PARTIAL_SOFT, 0, "ab|a", "a" },
	{ MUA | PCRE_PARTIAL_HARD, 0, "ab|a", "a" },
	{ MUA | PCRE_PARTIAL_SOFT, 0, "\\b#", "a" },
	{ MUA | PCRE_PARTIAL_SOFT, 0, "(?<=a)b", "a" },
	{ MUA | PCRE_PARTIAL_SOFT, 0, "abc|(?<=xxa)bc", "xxab" },
	{ MUA | PCRE_PARTIAL_SOFT, 0, "a\\B", "a" },
	{ MUA | PCRE_PARTIAL_HARD, 0, "a\\b", "a" },

	/* (*MARK) verb. */
	{ MUA, 0, "a(*MARK:aa)a", "ababaa" },
	{ MUA, 0 | F_NOMATCH, "a(*:aa)a", "abab" },
	{ MUA, 0, "a(*:aa)(b(*:bb)b|bc)", "abc" },
	{ MUA, 0 | F_NOMATCH, "a(*:1)x|b(*:2)y", "abc" },
	{ MUA, 0, "(?>a(*:aa))b|ac", "ac" },
	{ MUA, 0, "(?(DEFINE)(a(*:aa)))(?1)", "a" },
	{ MUA, 0 | F_NOMATCH, "(?(DEFINE)((a)(*:aa)))(?1)b", "aa" },
	{ MUA, 0, "(?(DEFINE)(a(*:aa)))a(?1)b|aac", "aac" },
	{ MUA, 0, "(a(*:aa)){0}(?:b(?1)b|c)+c", "babbab cc" },
	{ MUA, 0, "(a(*:aa)){0}(?:b(?1)b)+", "babba" },
	{ MUA, 0 | F_NOMATCH | F_STUDY, "(a(*:aa)){0}(?:b(?1)b)+", "ba" },
	{ MUA, 0, "(a\\K(*:aa)){0}(?:b(?1)b|c)+c", "babbab cc" },
	{ MUA, 0, "(a\\K(*:aa)){0}(?:b(?1)b)+", "babba" },
	{ MUA, 0 | F_NOMATCH | F_STUDY, "(a\\K(*:aa)){0}(?:b(?1)b)+", "ba" },
	{ MUA, 0 | F_NOMATCH | F_STUDY, "(*:mark)m", "a" },

	/* (*COMMIT) verb. */
	{ MUA, 0 | F_NOMATCH, "a(*COMMIT)b", "ac" },
	{ MUA, 0, "aa(*COMMIT)b", "xaxaab" },
	{ MUA, 0 | F_NOMATCH, "a(*COMMIT)(*:msg)b|ac", "ac" },
	{ MUA, 0 | F_NOMATCH, "(a(*COMMIT)b)++", "abac" },
	{ MUA, 0 | F_NOMATCH, "((a)(*COMMIT)b)++", "abac" },
	{ MUA, 0 | F_NOMATCH, "(?=a(*COMMIT)b)ab|ad", "ad" },

	/* (*PRUNE) verb. */
	{ MUA, 0, "aa\\K(*PRUNE)b", "aaab" },
	{ MUA, 0, "aa(*PRUNE:bb)b|a", "aa" },
	{ MUA, 0, "(a)(a)(*PRUNE)b|(a)", "aa" },
	{ MUA, 0, "(a)(a)(a)(a)(a)(a)(a)(a)(*PRUNE)b|(a)", "aaaaaaaa" },
	{ MUA | PCRE_PARTIAL_SOFT, 0, "a(*PRUNE)a|", "a" },
	{ MUA | PCRE_PARTIAL_SOFT, 0, "a(*PRUNE)a|m", "a" },
	{ MUA, 0 | F_NOMATCH, "(?=a(*PRUNE)b)ab|ad", "ad" },
	{ MUA, 0, "a(*COMMIT)(*PRUNE)d|bc", "abc" },
	{ MUA, 0, "(?=a(*COMMIT)b)a(*PRUNE)c|bc", "abc" },
	{ MUA, 0 | F_NOMATCH, "(*COMMIT)(?=a(*COMMIT)b)a(*PRUNE)c|bc", "abc" },
	{ MUA, 0, "(?=(a)(*COMMIT)b)a(*PRUNE)c|bc", "abc" },
	{ MUA, 0 | F_NOMATCH, "(*COMMIT)(?=(a)(*COMMIT)b)a(*PRUNE)c|bc", "abc" },
	{ MUA, 0, "(a(*COMMIT)b){0}a(?1)(*PRUNE)c|bc", "abc" },
	{ MUA, 0 | F_NOMATCH, "(a(*COMMIT)b){0}a(*COMMIT)(?1)(*PRUNE)c|bc", "abc" },
	{ MUA, 0, "(a(*COMMIT)b)++(*PRUNE)d|c", "ababc" },
	{ MUA, 0 | F_NOMATCH, "(*COMMIT)(a(*COMMIT)b)++(*PRUNE)d|c", "ababc" },
	{ MUA, 0, "((a)(*COMMIT)b)++(*PRUNE)d|c", "ababc" },
	{ MUA, 0 | F_NOMATCH, "(*COMMIT)((a)(*COMMIT)b)++(*PRUNE)d|c", "ababc" },
	{ MUA, 0, "(?>a(*COMMIT)b)*abab(*PRUNE)d|ba", "ababab" },
	{ MUA, 0 | F_NOMATCH, "(*COMMIT)(?>a(*COMMIT)b)*abab(*PRUNE)d|ba", "ababab" },
	{ MUA, 0, "(?>a(*COMMIT)b)+abab(*PRUNE)d|ba", "ababab" },
	{ MUA, 0 | F_NOMATCH, "(*COMMIT)(?>a(*COMMIT)b)+abab(*PRUNE)d|ba", "ababab" },
	{ MUA, 0, "(?>a(*COMMIT)b)?ab(*PRUNE)d|ba", "aba" },
	{ MUA, 0 | F_NOMATCH, "(*COMMIT)(?>a(*COMMIT)b)?ab(*PRUNE)d|ba", "aba" },
	{ MUA, 0, "(?>a(*COMMIT)b)*?n(*PRUNE)d|ba", "abababn" },
	{ MUA, 0 | F_NOMATCH, "(*COMMIT)(?>a(*COMMIT)b)*?n(*PRUNE)d|ba", "abababn" },
	{ MUA, 0, "(?>a(*COMMIT)b)+?n(*PRUNE)d|ba", "abababn" },
	{ MUA, 0 | F_NOMATCH, "(*COMMIT)(?>a(*COMMIT)b)+?n(*PRUNE)d|ba", "abababn" },
	{ MUA, 0, "(?>a(*COMMIT)b)??n(*PRUNE)d|bn", "abn" },
	{ MUA, 0 | F_NOMATCH, "(*COMMIT)(?>a(*COMMIT)b)??n(*PRUNE)d|bn", "abn" },

	/* (*SKIP) verb. */
	{ MUA, 0 | F_NOMATCH, "(?=a(*SKIP)b)ab|ad", "ad" },

	/* (*THEN) verb. */
	{ MUA, 0, "((?:a(*THEN)|aab)(*THEN)c|a+)+m", "aabcaabcaabcaabcnacm" },
	{ MUA, 0 | F_NOMATCH, "((?:a(*THEN)|aab)(*THEN)c|a+)+m", "aabcm" },
	{ MUA, 0, "((?:a(*THEN)|aab)c|a+)+m", "aabcaabcnmaabcaabcm" },
	{ MUA, 0, "((?:a|aab)(*THEN)c|a+)+m", "aam" },
	{ MUA, 0, "((?:a(*COMMIT)|aab)(*THEN)c|a+)+m", "aam" },
	{ MUA, 0, "(?(?=a(*THEN)b)ab|ad)", "ad" },
	{ MUA, 0, "(?(?!a(*THEN)b)ad|add)", "add" },
	{ MUA, 0 | F_NOMATCH, "(?(?=a)a(*THEN)b|ad)", "ad" },
	{ MUA, 0, "(?!(?(?=a)ab|b(*THEN)d))bn|bnn", "bnn" },

	/* Deep recursion. */
	{ MUA, 0, "((((?:(?:(?:\\w)+)?)*|(?>\\w)+?)+|(?>\\w)?\?)*)?\\s", "aaaaa+ " },
	{ MUA, 0, "(?:((?:(?:(?:\\w*?)+)??|(?>\\w)?|\\w*+)*)+)+?\\s", "aa+ " },
	{ MUA, 0, "((a?)+)+b", "aaaaaaaaaaaa b" },

	/* Deep recursion: Stack limit reached. */
	{ MA, 0 | F_NOMATCH, "a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?aaaaaaaaaaaaaaaaaaaaaaa", "aaaaaaaaaaaaaaaaaaaaaaa" },
	{ MA, 0 | F_NOMATCH, "(?:a+)+b", "aaaaaaaaaaaaaaaaaaaaaaaa b" },
	{ MA, 0 | F_NOMATCH, "(?:a+?)+?b", "aaaaaaaaaaaaaaaaaaaaaaaa b" },
	{ MA, 0 | F_NOMATCH, "(?:a*)*b", "aaaaaaaaaaaaaaaaaaaaaaaa b" },
	{ MA, 0 | F_NOMATCH, "(?:a*?)*?b", "aaaaaaaaaaaaaaaaaaaaaaaa b" },

	{ 0, 0, NULL, NULL }
};

static const unsigned char *tables(int mode)
{
	/* The purpose of this function to allow valgrind
	for reporting invalid reads and writes. */
	static unsigned char *tables_copy;
	const char *errorptr;
	int erroroffset;
	unsigned char *default_tables;
#if defined SUPPORT_PCRE8
	pcre *regex;
	char null_str[1] = { 0 };
#elif defined SUPPORT_PCRE16
	pcre16 *regex;
	PCRE_UCHAR16 null_str[1] = { 0 };
#elif defined SUPPORT_PCRE32
	pcre32 *regex;
	PCRE_UCHAR32 null_str[1] = { 0 };
#endif

	if (mode) {
		if (tables_copy)
			free(tables_copy);
		tables_copy = NULL;
		return NULL;
	}

	if (tables_copy)
		return tables_copy;

	default_tables = NULL;
#if defined SUPPORT_PCRE8
	regex = pcre_compile(null_str, 0, &errorptr, &erroroffset, NULL);
	if (regex) {
		pcre_fullinfo(regex, NULL, PCRE_INFO_DEFAULT_TABLES, &default_tables);
		pcre_free(regex);
	}
#elif defined SUPPORT_PCRE16
	regex = pcre16_compile(null_str, 0, &errorptr, &erroroffset, NULL);
	if (regex) {
		pcre16_fullinfo(regex, NULL, PCRE_INFO_DEFAULT_TABLES, &default_tables);
		pcre16_free(regex);
	}
#elif defined SUPPORT_PCRE32
	regex = pcre32_compile(null_str, 0, &errorptr, &erroroffset, NULL);
	if (regex) {
		pcre32_fullinfo(regex, NULL, PCRE_INFO_DEFAULT_TABLES, &default_tables);
		pcre32_free(regex);
	}
#endif
	/* Shouldn't ever happen. */
	if (!default_tables)
		return NULL;

	/* Unfortunately this value cannot get from pcre_fullinfo.
	Since this is a test program, this is acceptable at the moment. */
	tables_copy = (unsigned char *)malloc(1088);
	if (!tables_copy)
		return NULL;

	memcpy(tables_copy, default_tables, 1088);
	return tables_copy;
}

#ifdef SUPPORT_PCRE8
static pcre_jit_stack* callback8(void *arg)
{
	return (pcre_jit_stack *)arg;
}
#endif

#ifdef SUPPORT_PCRE16
static pcre16_jit_stack* callback16(void *arg)
{
	return (pcre16_jit_stack *)arg;
}
#endif

#ifdef SUPPORT_PCRE32
static pcre32_jit_stack* callback32(void *arg)
{
	return (pcre32_jit_stack *)arg;
}
#endif

#ifdef SUPPORT_PCRE8
static pcre_jit_stack *stack8;

static pcre_jit_stack *getstack8(void)
{
	if (!stack8)
		stack8 = pcre_jit_stack_alloc(1, 1024 * 1024);
	return stack8;
}

static void setstack8(pcre_extra *extra)
{
	if (!extra) {
		if (stack8)
			pcre_jit_stack_free(stack8);
		stack8 = NULL;
		return;
	}

	pcre_assign_jit_stack(extra, callback8, getstack8());
}
#endif /* SUPPORT_PCRE8 */

#ifdef SUPPORT_PCRE16
static pcre16_jit_stack *stack16;

static pcre16_jit_stack *getstack16(void)
{
	if (!stack16)
		stack16 = pcre16_jit_stack_alloc(1, 1024 * 1024);
	return stack16;
}

static void setstack16(pcre16_extra *extra)
{
	if (!extra) {
		if (stack16)
			pcre16_jit_stack_free(stack16);
		stack16 = NULL;
		return;
	}

	pcre16_assign_jit_stack(extra, callback16, getstack16());
}
#endif /* SUPPORT_PCRE16 */

#ifdef SUPPORT_PCRE32
static pcre32_jit_stack *stack32;

static pcre32_jit_stack *getstack32(void)
{
	if (!stack32)
		stack32 = pcre32_jit_stack_alloc(1, 1024 * 1024);
	return stack32;
}

static void setstack32(pcre32_extra *extra)
{
	if (!extra) {
		if (stack32)
			pcre32_jit_stack_free(stack32);
		stack32 = NULL;
		return;
	}

	pcre32_assign_jit_stack(extra, callback32, getstack32());
}
#endif /* SUPPORT_PCRE32 */

#ifdef SUPPORT_PCRE16

static int convert_utf8_to_utf16(const char *input, PCRE_UCHAR16 *output, int *offsetmap, int max_length)
{
	unsigned char *iptr = (unsigned char*)input;
	PCRE_UCHAR16 *optr = output;
	unsigned int c;

	if (max_length == 0)
		return 0;

	while (*iptr && max_length > 1) {
		c = 0;
		if (offsetmap)
			*offsetmap++ = (int)(iptr - (unsigned char*)input);

		if (*iptr < 0xc0)
			c = *iptr++;
		else if (!(*iptr & 0x20)) {
			c = ((iptr[0] & 0x1f) << 6) | (iptr[1] & 0x3f);
			iptr += 2;
		} else if (!(*iptr & 0x10)) {
			c = ((iptr[0] & 0x0f) << 12) | ((iptr[1] & 0x3f) << 6) | (iptr[2] & 0x3f);
			iptr += 3;
		} else if (!(*iptr & 0x08)) {
			c = ((iptr[0] & 0x07) << 18) | ((iptr[1] & 0x3f) << 12) | ((iptr[2] & 0x3f) << 6) | (iptr[3] & 0x3f);
			iptr += 4;
		}

		if (c < 65536) {
			*optr++ = c;
			max_length--;
		} else if (max_length <= 2) {
			*optr = '\0';
			return (int)(optr - output);
		} else {
			c -= 0x10000;
			*optr++ = 0xd800 | ((c >> 10) & 0x3ff);
			*optr++ = 0xdc00 | (c & 0x3ff);
			max_length -= 2;
			if (offsetmap)
				offsetmap++;
		}
	}
	if (offsetmap)
		*offsetmap = (int)(iptr - (unsigned char*)input);
	*optr = '\0';
	return (int)(optr - output);
}

static int copy_char8_to_char16(const char *input, PCRE_UCHAR16 *output, int max_length)
{
	unsigned char *iptr = (unsigned char*)input;
	PCRE_UCHAR16 *optr = output;

	if (max_length == 0)
		return 0;

	while (*iptr && max_length > 1) {
		*optr++ = *iptr++;
		max_length--;
	}
	*optr = '\0';
	return (int)(optr - output);
}

#define REGTEST_MAX_LENGTH16 4096
static PCRE_UCHAR16 regtest_buf16[REGTEST_MAX_LENGTH16];
static int regtest_offsetmap16[REGTEST_MAX_LENGTH16];

#endif /* SUPPORT_PCRE16 */

#ifdef SUPPORT_PCRE32

static int convert_utf8_to_utf32(const char *input, PCRE_UCHAR32 *output, int *offsetmap, int max_length)
{
	unsigned char *iptr = (unsigned char*)input;
	PCRE_UCHAR32 *optr = output;
	unsigned int c;

	if (max_length == 0)
		return 0;

	while (*iptr && max_length > 1) {
		c = 0;
		if (offsetmap)
			*offsetmap++ = (int)(iptr - (unsigned char*)input);

		if (*iptr < 0xc0)
			c = *iptr++;
		else if (!(*iptr & 0x20)) {
			c = ((iptr[0] & 0x1f) << 6) | (iptr[1] & 0x3f);
			iptr += 2;
		} else if (!(*iptr & 0x10)) {
			c = ((iptr[0] & 0x0f) << 12) | ((iptr[1] & 0x3f) << 6) | (iptr[2] & 0x3f);
			iptr += 3;
		} else if (!(*iptr & 0x08)) {
			c = ((iptr[0] & 0x07) << 18) | ((iptr[1] & 0x3f) << 12) | ((iptr[2] & 0x3f) << 6) | (iptr[3] & 0x3f);
			iptr += 4;
		}

		*optr++ = c;
		max_length--;
	}
	if (offsetmap)
		*offsetmap = (int)(iptr - (unsigned char*)input);
	*optr = 0;
	return (int)(optr - output);
}

static int copy_char8_to_char32(const char *input, PCRE_UCHAR32 *output, int max_length)
{
	unsigned char *iptr = (unsigned char*)input;
	PCRE_UCHAR32 *optr = output;

	if (max_length == 0)
		return 0;

	while (*iptr && max_length > 1) {
		*optr++ = *iptr++;
		max_length--;
	}
	*optr = '\0';
	return (int)(optr - output);
}

#define REGTEST_MAX_LENGTH32 4096
static PCRE_UCHAR32 regtest_buf32[REGTEST_MAX_LENGTH32];
static int regtest_offsetmap32[REGTEST_MAX_LENGTH32];

#endif /* SUPPORT_PCRE32 */

static int check_ascii(const char *input)
{
	const unsigned char *ptr = (unsigned char *)input;
	while (*ptr) {
		if (*ptr > 127)
			return 0;
		ptr++;
	}
	return 1;
}

static int regression_tests(void)
{
	struct regression_test_case *current = regression_test_cases;
	const char *error;
	char *cpu_info;
	int i, err_offs;
	int is_successful, is_ascii;
	int total = 0;
	int successful = 0;
	int successful_row = 0;
	int counter = 0;
	int study_mode;
	int utf = 0, ucp = 0;
	int disabled_flags = 0;
#ifdef SUPPORT_PCRE8
	pcre *re8;
	pcre_extra *extra8;
	pcre_extra dummy_extra8;
	int ovector8_1[32];
	int ovector8_2[32];
	int return_value8[2];
	unsigned char *mark8_1, *mark8_2;
#endif
#ifdef SUPPORT_PCRE16
	pcre16 *re16;
	pcre16_extra *extra16;
	pcre16_extra dummy_extra16;
	int ovector16_1[32];
	int ovector16_2[32];
	int return_value16[2];
	PCRE_UCHAR16 *mark16_1, *mark16_2;
	int length16;
#endif
#ifdef SUPPORT_PCRE32
	pcre32 *re32;
	pcre32_extra *extra32;
	pcre32_extra dummy_extra32;
	int ovector32_1[32];
	int ovector32_2[32];
	int return_value32[2];
	PCRE_UCHAR32 *mark32_1, *mark32_2;
	int length32;
#endif

	/* This test compares the behaviour of interpreter and JIT. Although disabling
	utf or ucp may make tests fail, if the pcre_exec result is the SAME, it is
	still considered successful from pcre_jit_test point of view. */

#if defined SUPPORT_PCRE8
	pcre_config(PCRE_CONFIG_JITTARGET, &cpu_info);
#elif defined SUPPORT_PCRE16
	pcre16_config(PCRE_CONFIG_JITTARGET, &cpu_info);
#elif defined SUPPORT_PCRE32
	pcre32_config(PCRE_CONFIG_JITTARGET, &cpu_info);
#endif

	printf("Running JIT regression tests\n");
	printf("  target CPU of SLJIT compiler: %s\n", cpu_info);

#if defined SUPPORT_PCRE8
	pcre_config(PCRE_CONFIG_UTF8, &utf);
	pcre_config(PCRE_CONFIG_UNICODE_PROPERTIES, &ucp);
#elif defined SUPPORT_PCRE16
	pcre16_config(PCRE_CONFIG_UTF16, &utf);
	pcre16_config(PCRE_CONFIG_UNICODE_PROPERTIES, &ucp);
#elif defined SUPPORT_PCRE32
	pcre32_config(PCRE_CONFIG_UTF32, &utf);
	pcre32_config(PCRE_CONFIG_UNICODE_PROPERTIES, &ucp);
#endif

	if (!utf)
		disabled_flags |= PCRE_UTF8 | PCRE_UTF16 | PCRE_UTF32;
	if (!ucp)
		disabled_flags |= PCRE_UCP;
#ifdef SUPPORT_PCRE8
	printf("  in  8 bit mode with UTF-8  %s and ucp %s:\n", utf ? "enabled" : "disabled", ucp ? "enabled" : "disabled");
#endif
#ifdef SUPPORT_PCRE16
	printf("  in 16 bit mode with UTF-16 %s and ucp %s:\n", utf ? "enabled" : "disabled", ucp ? "enabled" : "disabled");
#endif
#ifdef SUPPORT_PCRE32
	printf("  in 32 bit mode with UTF-32 %s and ucp %s:\n", utf ? "enabled" : "disabled", ucp ? "enabled" : "disabled");
#endif

	while (current->pattern) {
		/* printf("\nPattern: %s :\n", current->pattern); */
		total++;
		is_ascii = 0;
		if (!(current->start_offset & F_PROPERTY))
			is_ascii = check_ascii(current->pattern) && check_ascii(current->input);

		if (current->flags & PCRE_PARTIAL_SOFT)
			study_mode = PCRE_STUDY_JIT_PARTIAL_SOFT_COMPILE;
		else if (current->flags & PCRE_PARTIAL_HARD)
			study_mode = PCRE_STUDY_JIT_PARTIAL_HARD_COMPILE;
		else
			study_mode = PCRE_STUDY_JIT_COMPILE;
		error = NULL;
#ifdef SUPPORT_PCRE8
		re8 = NULL;
		if (!(current->start_offset & F_NO8))
			re8 = pcre_compile(current->pattern,
				current->flags & ~(PCRE_NOTBOL | PCRE_NOTEOL | PCRE_NOTEMPTY | PCRE_NOTEMPTY_ATSTART | PCRE_PARTIAL_SOFT | PCRE_PARTIAL_HARD | disabled_flags),
				&error, &err_offs, tables(0));

		extra8 = NULL;
		if (re8) {
			error = NULL;
			extra8 = pcre_study(re8, study_mode, &error);
			if (!extra8) {
				printf("\n8 bit: Cannot study pattern: %s\n", current->pattern);
				pcre_free(re8);
				re8 = NULL;
			}
			else if (!(extra8->flags & PCRE_EXTRA_EXECUTABLE_JIT)) {
				printf("\n8 bit: JIT compiler does not support: %s\n", current->pattern);
				pcre_free_study(extra8);
				pcre_free(re8);
				re8 = NULL;
			}
			extra8->flags |= PCRE_EXTRA_MARK;
		} else if (((utf && ucp) || is_ascii) && !(current->start_offset & F_NO8))
			printf("\n8 bit: Cannot compile pattern \"%s\": %s\n", current->pattern, error);
#endif
#ifdef SUPPORT_PCRE16
		if ((current->flags & PCRE_UTF16) || (current->start_offset & F_FORCECONV))
			convert_utf8_to_utf16(current->pattern, regtest_buf16, NULL, REGTEST_MAX_LENGTH16);
		else
			copy_char8_to_char16(current->pattern, regtest_buf16, REGTEST_MAX_LENGTH16);

		re16 = NULL;
		if (!(current->start_offset & F_NO16))
			re16 = pcre16_compile(regtest_buf16,
				current->flags & ~(PCRE_NOTBOL | PCRE_NOTEOL | PCRE_NOTEMPTY | PCRE_NOTEMPTY_ATSTART | PCRE_PARTIAL_SOFT | PCRE_PARTIAL_HARD | disabled_flags),
				&error, &err_offs, tables(0));

		extra16 = NULL;
		if (re16) {
			error = NULL;
			extra16 = pcre16_study(re16, study_mode, &error);
			if (!extra16) {
				printf("\n16 bit: Cannot study pattern: %s\n", current->pattern);
				pcre16_free(re16);
				re16 = NULL;
			}
			else if (!(extra16->flags & PCRE_EXTRA_EXECUTABLE_JIT)) {
				printf("\n16 bit: JIT compiler does not support: %s\n", current->pattern);
				pcre16_free_study(extra16);
				pcre16_free(re16);
				re16 = NULL;
			}
			extra16->flags |= PCRE_EXTRA_MARK;
		} else if (((utf && ucp) || is_ascii) && !(current->start_offset & F_NO16))
			printf("\n16 bit: Cannot compile pattern \"%s\": %s\n", current->pattern, error);
#endif
#ifdef SUPPORT_PCRE32
		if ((current->flags & PCRE_UTF32) || (current->start_offset & F_FORCECONV))
			convert_utf8_to_utf32(current->pattern, regtest_buf32, NULL, REGTEST_MAX_LENGTH32);
		else
			copy_char8_to_char32(current->pattern, regtest_buf32, REGTEST_MAX_LENGTH32);

		re32 = NULL;
		if (!(current->start_offset & F_NO32))
			re32 = pcre32_compile(regtest_buf32,
				current->flags & ~(PCRE_NOTBOL | PCRE_NOTEOL | PCRE_NOTEMPTY | PCRE_NOTEMPTY_ATSTART | PCRE_PARTIAL_SOFT | PCRE_PARTIAL_HARD | disabled_flags),
				&error, &err_offs, tables(0));

		extra32 = NULL;
		if (re32) {
			error = NULL;
			extra32 = pcre32_study(re32, study_mode, &error);
			if (!extra32) {
				printf("\n32 bit: Cannot study pattern: %s\n", current->pattern);
				pcre32_free(re32);
				re32 = NULL;
			}
			if (!(extra32->flags & PCRE_EXTRA_EXECUTABLE_JIT)) {
				printf("\n32 bit: JIT compiler does not support: %s\n", current->pattern);
				pcre32_free_study(extra32);
				pcre32_free(re32);
				re32 = NULL;
			}
			extra32->flags |= PCRE_EXTRA_MARK;
		} else if (((utf && ucp) || is_ascii) && !(current->start_offset & F_NO32))
			printf("\n32 bit: Cannot compile pattern \"%s\": %s\n", current->pattern, error);
#endif

		counter++;
		if ((counter & 0x3) != 0) {
#ifdef SUPPORT_PCRE8
			setstack8(NULL);
#endif
#ifdef SUPPORT_PCRE16
			setstack16(NULL);
#endif
#ifdef SUPPORT_PCRE32
			setstack32(NULL);
#endif
		}

#ifdef SUPPORT_PCRE8
		return_value8[0] = -1000;
		return_value8[1] = -1000;
		for (i = 0; i < 32; ++i)
			ovector8_1[i] = -2;
		for (i = 0; i < 32; ++i)
			ovector8_2[i] = -2;
		if (re8) {
			mark8_1 = NULL;
			mark8_2 = NULL;
			extra8->mark = &mark8_1;

			if ((counter & 0x1) != 0) {
				setstack8(extra8);
				return_value8[0] = pcre_exec(re8, extra8, current->input, strlen(current->input), current->start_offset & OFFSET_MASK,
					current->flags & (PCRE_NOTBOL | PCRE_NOTEOL | PCRE_NOTEMPTY | PCRE_NOTEMPTY_ATSTART | PCRE_PARTIAL_SOFT | PCRE_PARTIAL_HARD | PCRE_NO_UTF8_CHECK), ovector8_1, 32);
			} else
				return_value8[0] = pcre_jit_exec(re8, extra8, current->input, strlen(current->input), current->start_offset & OFFSET_MASK,
					current->flags & (PCRE_NOTBOL | PCRE_NOTEOL | PCRE_NOTEMPTY | PCRE_NOTEMPTY_ATSTART | PCRE_PARTIAL_SOFT | PCRE_PARTIAL_HARD | PCRE_NO_UTF8_CHECK), ovector8_1, 32, getstack8());
			memset(&dummy_extra8, 0, sizeof(pcre_extra));
			dummy_extra8.flags = PCRE_EXTRA_MARK;
			if (current->start_offset & F_STUDY) {
				dummy_extra8.flags |= PCRE_EXTRA_STUDY_DATA;
				dummy_extra8.study_data = extra8->study_data;
			}
			dummy_extra8.mark = &mark8_2;
			return_value8[1] = pcre_exec(re8, &dummy_extra8, current->input, strlen(current->input), current->start_offset & OFFSET_MASK,
				current->flags & (PCRE_NOTBOL | PCRE_NOTEOL | PCRE_NOTEMPTY | PCRE_NOTEMPTY_ATSTART | PCRE_PARTIAL_SOFT | PCRE_PARTIAL_HARD | PCRE_NO_UTF8_CHECK), ovector8_2, 32);
		}
#endif

#ifdef SUPPORT_PCRE16
		return_value16[0] = -1000;
		return_value16[1] = -1000;
		for (i = 0; i < 32; ++i)
			ovector16_1[i] = -2;
		for (i = 0; i < 32; ++i)
			ovector16_2[i] = -2;
		if (re16) {
			mark16_1 = NULL;
			mark16_2 = NULL;
			if ((current->flags & PCRE_UTF16) || (current->start_offset & F_FORCECONV))
				length16 = convert_utf8_to_utf16(current->input, regtest_buf16, regtest_offsetmap16, REGTEST_MAX_LENGTH16);
			else
				length16 = copy_char8_to_char16(current->input, regtest_buf16, REGTEST_MAX_LENGTH16);
			extra16->mark = &mark16_1;
			if ((counter & 0x1) != 0) {
				setstack16(extra16);
				return_value16[0] = pcre16_exec(re16, extra16, regtest_buf16, length16, current->start_offset & OFFSET_MASK,
					current->flags & (PCRE_NOTBOL | PCRE_NOTEOL | PCRE_NOTEMPTY | PCRE_NOTEMPTY_ATSTART | PCRE_PARTIAL_SOFT | PCRE_PARTIAL_HARD | PCRE_NO_UTF8_CHECK), ovector16_1, 32);
			} else
				return_value16[0] = pcre16_jit_exec(re16, extra16, regtest_buf16, length16, current->start_offset & OFFSET_MASK,
					current->flags & (PCRE_NOTBOL | PCRE_NOTEOL | PCRE_NOTEMPTY | PCRE_NOTEMPTY_ATSTART | PCRE_PARTIAL_SOFT | PCRE_PARTIAL_HARD | PCRE_NO_UTF8_CHECK), ovector16_1, 32, getstack16());
			memset(&dummy_extra16, 0, sizeof(pcre16_extra));
			dummy_extra16.flags = PCRE_EXTRA_MARK;
			if (current->start_offset & F_STUDY) {
				dummy_extra16.flags |= PCRE_EXTRA_STUDY_DATA;
				dummy_extra16.study_data = extra16->study_data;
			}
			dummy_extra16.mark = &mark16_2;
			return_value16[1] = pcre16_exec(re16, &dummy_extra16, regtest_buf16, length16, current->start_offset & OFFSET_MASK,
				current->flags & (PCRE_NOTBOL | PCRE_NOTEOL | PCRE_NOTEMPTY | PCRE_NOTEMPTY_ATSTART | PCRE_PARTIAL_SOFT | PCRE_PARTIAL_HARD | PCRE_NO_UTF8_CHECK), ovector16_2, 32);
		}
#endif

#ifdef SUPPORT_PCRE32
		return_value32[0] = -1000;
		return_value32[1] = -1000;
		for (i = 0; i < 32; ++i)
			ovector32_1[i] = -2;
		for (i = 0; i < 32; ++i)
			ovector32_2[i] = -2;
		if (re32) {
			mark32_1 = NULL;
			mark32_2 = NULL;
			if ((current->flags & PCRE_UTF32) || (current->start_offset & F_FORCECONV))
				length32 = convert_utf8_to_utf32(current->input, regtest_buf32, regtest_offsetmap32, REGTEST_MAX_LENGTH32);
			else
				length32 = copy_char8_to_char32(current->input, regtest_buf32, REGTEST_MAX_LENGTH32);
			extra32->mark = &mark32_1;
			if ((counter & 0x1) != 0) {
				setstack32(extra32);
				return_value32[0] = pcre32_exec(re32, extra32, regtest_buf32, length32, current->start_offset & OFFSET_MASK,
					current->flags & (PCRE_NOTBOL | PCRE_NOTEOL | PCRE_NOTEMPTY | PCRE_NOTEMPTY_ATSTART | PCRE_PARTIAL_SOFT | PCRE_PARTIAL_HARD | PCRE_NO_UTF8_CHECK), ovector32_1, 32);
			} else
				return_value32[0] = pcre32_jit_exec(re32, extra32, regtest_buf32, length32, current->start_offset & OFFSET_MASK,
					current->flags & (PCRE_NOTBOL | PCRE_NOTEOL | PCRE_NOTEMPTY | PCRE_NOTEMPTY_ATSTART | PCRE_PARTIAL_SOFT | PCRE_PARTIAL_HARD | PCRE_NO_UTF8_CHECK), ovector32_1, 32, getstack32());
			memset(&dummy_extra32, 0, sizeof(pcre32_extra));
			dummy_extra32.flags = PCRE_EXTRA_MARK;
			if (current->start_offset & F_STUDY) {
				dummy_extra32.flags |= PCRE_EXTRA_STUDY_DATA;
				dummy_extra32.study_data = extra32->study_data;
			}
			dummy_extra32.mark = &mark32_2;
			return_value32[1] = pcre32_exec(re32, &dummy_extra32, regtest_buf32, length32, current->start_offset & OFFSET_MASK,
				current->flags & (PCRE_NOTBOL | PCRE_NOTEOL | PCRE_NOTEMPTY | PCRE_NOTEMPTY_ATSTART | PCRE_PARTIAL_SOFT | PCRE_PARTIAL_HARD | PCRE_NO_UTF8_CHECK), ovector32_2, 32);
		}
#endif

		/* printf("[%d-%d-%d|%d-%d|%d-%d|%d-%d]%s",
			return_value8[0], return_value16[0], return_value32[0],
			ovector8_1[0], ovector8_1[1],
			ovector16_1[0], ovector16_1[1],
			ovector32_1[0], ovector32_1[1],
			(current->flags & PCRE_CASELESS) ? "C" : ""); */

		/* If F_DIFF is set, just run the test, but do not compare the results.
		Segfaults can still be captured. */

		is_successful = 1;
		if (!(current->start_offset & F_DIFF)) {
#if defined SUPPORT_UTF && ((defined(SUPPORT_PCRE8) + defined(SUPPORT_PCRE16) + defined(SUPPORT_PCRE32)) >= 2)
			if (!(current->start_offset & F_FORCECONV)) {
				int return_value;

				/* All results must be the same. */
#ifdef SUPPORT_PCRE8
				if ((return_value = return_value8[0]) != return_value8[1]) {
					printf("\n8 bit: Return value differs(J8:%d,I8:%d): [%d] '%s' @ '%s'\n",
						return_value8[0], return_value8[1], total, current->pattern, current->input);
					is_successful = 0;
				} else
#endif
#ifdef SUPPORT_PCRE16
				if ((return_value = return_value16[0]) != return_value16[1]) {
					printf("\n16 bit: Return value differs(J16:%d,I16:%d): [%d] '%s' @ '%s'\n",
						return_value16[0], return_value16[1], total, current->pattern, current->input);
					is_successful = 0;
				} else
#endif
#ifdef SUPPORT_PCRE32
				if ((return_value = return_value32[0]) != return_value32[1]) {
					printf("\n32 bit: Return value differs(J32:%d,I32:%d): [%d] '%s' @ '%s'\n",
						return_value32[0], return_value32[1], total, current->pattern, current->input);
					is_successful = 0;
				} else
#endif
#if defined SUPPORT_PCRE8 && defined SUPPORT_PCRE16
				if (return_value8[0] != return_value16[0]) {
					printf("\n8 and 16 bit: Return value differs(J8:%d,J16:%d): [%d] '%s' @ '%s'\n",
						return_value8[0], return_value16[0],
						total, current->pattern, current->input);
					is_successful = 0;
				} else
#endif
#if defined SUPPORT_PCRE8 && defined SUPPORT_PCRE32
				if (return_value8[0] != return_value32[0]) {
					printf("\n8 and 32 bit: Return value differs(J8:%d,J32:%d): [%d] '%s' @ '%s'\n",
						return_value8[0], return_value32[0],
						total, current->pattern, current->input);
					is_successful = 0;
				} else
#endif
#if defined SUPPORT_PCRE16 && defined SUPPORT_PCRE32
				if (return_value16[0] != return_value32[0]) {
					printf("\n16 and 32 bit: Return value differs(J16:%d,J32:%d): [%d] '%s' @ '%s'\n",
						return_value16[0], return_value32[0],
						total, current->pattern, current->input);
					is_successful = 0;
				} else
#endif
				if (return_value >= 0 || return_value == PCRE_ERROR_PARTIAL) {
					if (return_value == PCRE_ERROR_PARTIAL) {
						return_value = 2;
					} else {
						return_value *= 2;
					}
#ifdef SUPPORT_PCRE8
					return_value8[0] = return_value;
#endif
#ifdef SUPPORT_PCRE16
					return_value16[0] = return_value;
#endif
#ifdef SUPPORT_PCRE32
					return_value32[0] = return_value;
#endif
					/* Transform back the results. */
					if (current->flags & PCRE_UTF8) {
#ifdef SUPPORT_PCRE16
						for (i = 0; i < return_value; ++i) {
							if (ovector16_1[i] >= 0)
								ovector16_1[i] = regtest_offsetmap16[ovector16_1[i]];
							if (ovector16_2[i] >= 0)
								ovector16_2[i] = regtest_offsetmap16[ovector16_2[i]];
						}
#endif
#ifdef SUPPORT_PCRE32
						for (i = 0; i < return_value; ++i) {
							if (ovector32_1[i] >= 0)
								ovector32_1[i] = regtest_offsetmap32[ovector32_1[i]];
							if (ovector32_2[i] >= 0)
								ovector32_2[i] = regtest_offsetmap32[ovector32_2[i]];
						}
#endif
					}

					for (i = 0; i < return_value; ++i) {
#if defined SUPPORT_PCRE8 && defined SUPPORT_PCRE16
						if (ovector8_1[i] != ovector8_2[i] || ovector8_1[i] != ovector16_1[i] || ovector8_1[i] != ovector16_2[i]) {
							printf("\n8 and 16 bit: Ovector[%d] value differs(J8:%d,I8:%d,J16:%d,I16:%d): [%d] '%s' @ '%s' \n",
								i, ovector8_1[i], ovector8_2[i], ovector16_1[i], ovector16_2[i],
								total, current->pattern, current->input);
							is_successful = 0;
						}
#endif
#if defined SUPPORT_PCRE8 && defined SUPPORT_PCRE32
						if (ovector8_1[i] != ovector8_2[i] || ovector8_1[i] != ovector32_1[i] || ovector8_1[i] != ovector32_2[i]) {
							printf("\n8 and 32 bit: Ovector[%d] value differs(J8:%d,I8:%d,J32:%d,I32:%d): [%d] '%s' @ '%s' \n",
								i, ovector8_1[i], ovector8_2[i], ovector32_1[i], ovector32_2[i],
								total, current->pattern, current->input);
							is_successful = 0;
						}
#endif
#if defined SUPPORT_PCRE16 && defined SUPPORT_PCRE16
						if (ovector16_1[i] != ovector16_2[i] || ovector16_1[i] != ovector16_1[i] || ovector16_1[i] != ovector16_2[i]) {
							printf("\n16 and 16 bit: Ovector[%d] value differs(J16:%d,I16:%d,J32:%d,I32:%d): [%d] '%s' @ '%s' \n",
								i, ovector16_1[i], ovector16_2[i], ovector16_1[i], ovector16_2[i],
								total, current->pattern, current->input);
							is_successful = 0;
						}
#endif
					}
				}
			} else
#endif /* more than one of SUPPORT_PCRE8, SUPPORT_PCRE16 and SUPPORT_PCRE32 */
			{
				/* Only the 8 bit and 16 bit results must be equal. */
#ifdef SUPPORT_PCRE8
				if (return_value8[0] != return_value8[1]) {
					printf("\n8 bit: Return value differs(%d:%d): [%d] '%s' @ '%s'\n",
						return_value8[0], return_value8[1], total, current->pattern, current->input);
					is_successful = 0;
				} else if (return_value8[0] >= 0 || return_value8[0] == PCRE_ERROR_PARTIAL) {
					if (return_value8[0] == PCRE_ERROR_PARTIAL)
						return_value8[0] = 2;
					else
						return_value8[0] *= 2;

					for (i = 0; i < return_value8[0]; ++i)
						if (ovector8_1[i] != ovector8_2[i]) {
							printf("\n8 bit: Ovector[%d] value differs(%d:%d): [%d] '%s' @ '%s'\n",
								i, ovector8_1[i], ovector8_2[i], total, current->pattern, current->input);
							is_successful = 0;
						}
				}
#endif

#ifdef SUPPORT_PCRE16
				if (return_value16[0] != return_value16[1]) {
					printf("\n16 bit: Return value differs(%d:%d): [%d] '%s' @ '%s'\n",
						return_value16[0], return_value16[1], total, current->pattern, current->input);
					is_successful = 0;
				} else if (return_value16[0] >= 0 || return_value16[0] == PCRE_ERROR_PARTIAL) {
					if (return_value16[0] == PCRE_ERROR_PARTIAL)
						return_value16[0] = 2;
					else
						return_value16[0] *= 2;

					for (i = 0; i < return_value16[0]; ++i)
						if (ovector16_1[i] != ovector16_2[i]) {
							printf("\n16 bit: Ovector[%d] value differs(%d:%d): [%d] '%s' @ '%s'\n",
								i, ovector16_1[i], ovector16_2[i], total, current->pattern, current->input);
							is_successful = 0;
						}
				}
#endif

#ifdef SUPPORT_PCRE32
				if (return_value32[0] != return_value32[1]) {
					printf("\n32 bit: Return value differs(%d:%d): [%d] '%s' @ '%s'\n",
						return_value32[0], return_value32[1], total, current->pattern, current->input);
					is_successful = 0;
				} else if (return_value32[0] >= 0 || return_value32[0] == PCRE_ERROR_PARTIAL) {
					if (return_value32[0] == PCRE_ERROR_PARTIAL)
						return_value32[0] = 2;
					else
						return_value32[0] *= 2;

					for (i = 0; i < return_value32[0]; ++i)
						if (ovector32_1[i] != ovector32_2[i]) {
							printf("\n32 bit: Ovector[%d] value differs(%d:%d): [%d] '%s' @ '%s'\n",
								i, ovector32_1[i], ovector32_2[i], total, current->pattern, current->input);
							is_successful = 0;
						}
				}
#endif
			}
		}

		if (is_successful) {
#ifdef SUPPORT_PCRE8
			if (!(current->start_offset & F_NO8) && ((utf && ucp) || is_ascii)) {
				if (return_value8[0] < 0 && !(current->start_offset & F_NOMATCH)) {
					printf("8 bit: Test should match: [%d] '%s' @ '%s'\n",
						total, current->pattern, current->input);
					is_successful = 0;
				}

				if (return_value8[0] >= 0 && (current->start_offset & F_NOMATCH)) {
					printf("8 bit: Test should not match: [%d] '%s' @ '%s'\n",
						total, current->pattern, current->input);
					is_successful = 0;
				}
			}
#endif
#ifdef SUPPORT_PCRE16
			if (!(current->start_offset & F_NO16) && ((utf && ucp) || is_ascii)) {
				if (return_value16[0] < 0 && !(current->start_offset & F_NOMATCH)) {
					printf("16 bit: Test should match: [%d] '%s' @ '%s'\n",
						total, current->pattern, current->input);
					is_successful = 0;
				}

				if (return_value16[0] >= 0 && (current->start_offset & F_NOMATCH)) {
					printf("16 bit: Test should not match: [%d] '%s' @ '%s'\n",
						total, current->pattern, current->input);
					is_successful = 0;
				}
			}
#endif
#ifdef SUPPORT_PCRE32
			if (!(current->start_offset & F_NO32) && ((utf && ucp) || is_ascii)) {
				if (return_value32[0] < 0 && !(current->start_offset & F_NOMATCH)) {
					printf("32 bit: Test should match: [%d] '%s' @ '%s'\n",
						total, current->pattern, current->input);
					is_successful = 0;
				}

				if (return_value32[0] >= 0 && (current->start_offset & F_NOMATCH)) {
					printf("32 bit: Test should not match: [%d] '%s' @ '%s'\n",
						total, current->pattern, current->input);
					is_successful = 0;
				}
			}
#endif
		}

		if (is_successful) {
#ifdef SUPPORT_PCRE8
			if (mark8_1 != mark8_2) {
				printf("8 bit: Mark value mismatch: [%d] '%s' @ '%s'\n",
					total, current->pattern, current->input);
				is_successful = 0;
			}
#endif
#ifdef SUPPORT_PCRE16
			if (mark16_1 != mark16_2) {
				printf("16 bit: Mark value mismatch: [%d] '%s' @ '%s'\n",
					total, current->pattern, current->input);
				is_successful = 0;
			}
#endif
#ifdef SUPPORT_PCRE32
			if (mark32_1 != mark32_2) {
				printf("32 bit: Mark value mismatch: [%d] '%s' @ '%s'\n",
					total, current->pattern, current->input);
				is_successful = 0;
			}
#endif
		}

#ifdef SUPPORT_PCRE8
		if (re8) {
			pcre_free_study(extra8);
			pcre_free(re8);
		}
#endif
#ifdef SUPPORT_PCRE16
		if (re16) {
			pcre16_free_study(extra16);
			pcre16_free(re16);
		}
#endif
#ifdef SUPPORT_PCRE32
		if (re32) {
			pcre32_free_study(extra32);
			pcre32_free(re32);
		}
#endif

		if (is_successful) {
			successful++;
			successful_row++;
			printf(".");
			if (successful_row >= 60) {
				successful_row = 0;
				printf("\n");
			}
		} else
			successful_row = 0;

		fflush(stdout);
		current++;
	}
	tables(1);
#ifdef SUPPORT_PCRE8
	setstack8(NULL);
#endif
#ifdef SUPPORT_PCRE16
	setstack16(NULL);
#endif
#ifdef SUPPORT_PCRE32
	setstack32(NULL);
#endif

	if (total == successful) {
		printf("\nAll JIT regression tests are successfully passed.\n");
		return 0;
	} else {
		printf("\nSuccessful test ratio: %d%% (%d failed)\n", successful * 100 / total, total - successful);
		return 1;
	}
}

/* End of pcre_jit_test.c */
