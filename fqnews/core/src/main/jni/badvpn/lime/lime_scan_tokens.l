/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
%{
void out(char*t, char*v);
void lit();
void tok(char*t);
void php();
%}

%option stack
%option yylineno
%option main

%x code
%x dquote
%x squote

CHAR	\n|.

ALPHA [a-zA-Z]
DIGIT [0-9]
ALNUM {ALPHA}|{DIGIT}
WORD {ALNUM}|_
STOP "."

SYM {ALPHA}{WORD}*'*
LIT '.'

ESC "\"{CHAR}
SCHAR [^\']|ESC
DCHAR [^\"]|ESC
COM "//"|"#"

CC [^*\n]
CX "*"+{CC}+
CT "*"+"/"
BLOCKCMT "/*"({CC}|{CX})*{CT}

%x pragma


%%

[[:space:]]+ {}
#.* {}

{STOP} 			out("stop", ".");
{SYM} 			tok("sym");
{LIT}			tok("lit");
"/"{WORD}+		|
"/$"			out("lambda", yytext+1);
"%"{WORD}+ 		{
	out("pragma", yytext+1);
	yy_push_state(pragma);
}

<*>"{"		{
	lit();
	yy_push_state(code);
}

.	lit();


<pragma>{
\n {
	out("stop", ".");
	yy_pop_state();
}
[[:space:]] {}
{SYM} 			tok("sym");
{LIT}			tok("lit");
.	lit();
}

<code>{
"}" {
	lit();
	yy_pop_state();
}
'{SCHAR}*'		php();
\"{DCHAR}*\"	php();
{COM}.*			php();
{BLOCKCMT}		php();
[^{}'"#/]+		php();
.				php();
}

%%

void lit() {
	char lit[] = "'.'";
	lit[1] = *yytext;
	out(lit, yytext);
}

void tok(char*t) {
	out(t, yytext);
}

void php() {
	out("php", yytext);
}

void out(char*type, char*value) {
	printf("%d\001%s\001%s", yylineno, type, value);
	fputc(0, stdout);
}
