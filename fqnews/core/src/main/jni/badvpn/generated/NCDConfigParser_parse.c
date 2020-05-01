/* Driver template for the LEMON parser generator.
** The author disclaims copyright to this source code.
*/
/* First off, code is included that follows the "include" declaration
** in the input grammar file. */
#include <stdio.h>
#line 30 "NCDConfigParser_parse.y"


#include <string.h>
#include <stddef.h>

#include <misc/debug.h>
#include <misc/concat_strings.h>
#include <ncd/NCDAst.h>

struct parser_out {
    int out_of_memory;
    int syntax_error;
    int have_ast;
    NCDProgram ast;
};

struct token {
    char *str;
    size_t len;
};

struct program {
    int have;
    NCDProgram v;
};

struct block {
    int have;
    NCDBlock v;
};

struct statement {
    int have;
    NCDStatement v;
};

struct ifblock {
    int have;
    NCDIfBlock v;
};

struct value {
    int have;
    NCDValue v;
};

static void free_token (struct token o) { free(o.str); }
static void free_program (struct program o) { if (o.have) NCDProgram_Free(&o.v); }
static void free_block (struct block o) { if (o.have) NCDBlock_Free(&o.v); }
static void free_statement (struct statement o) { if (o.have) NCDStatement_Free(&o.v); }
static void free_ifblock (struct ifblock o) { if (o.have) NCDIfBlock_Free(&o.v); }
static void free_value (struct value o) { if (o.have) NCDValue_Free(&o.v); }

#line 62 "NCDConfigParser_parse.c"
/* Next is all token values, in a form suitable for use by makeheaders.
** This section will be null unless lemon is run with the -m switch.
*/
/* 
** These constants (all generated automatically by the parser generator)
** specify the various kinds of tokens (terminals) that the parser
** understands. 
**
** Each symbol here is a terminal symbol in the grammar.
*/
/* Make sure the INTERFACE macro is defined.
*/
#ifndef INTERFACE
# define INTERFACE 1
#endif
/* The next thing included is series of defines which control
** various aspects of the generated parser.
**    YYCODETYPE         is the data type used for storing terminal
**                       and nonterminal numbers.  "unsigned char" is
**                       used if there are fewer than 250 terminals
**                       and nonterminals.  "int" is used otherwise.
**    YYNOCODE           is a number of type YYCODETYPE which corresponds
**                       to no legal terminal or nonterminal number.  This
**                       number is used to fill in empty slots of the hash 
**                       table.
**    YYFALLBACK         If defined, this indicates that one or more tokens
**                       have fall-back values which should be used if the
**                       original value of the token will not parse.
**    YYACTIONTYPE       is the data type used for storing terminal
**                       and nonterminal numbers.  "unsigned char" is
**                       used if there are fewer than 250 rules and
**                       states combined.  "int" is used otherwise.
**    ParseTOKENTYPE     is the data type used for minor tokens given 
**                       directly to the parser from the tokenizer.
**    YYMINORTYPE        is the data type used for all minor tokens.
**                       This is typically a union of many types, one of
**                       which is ParseTOKENTYPE.  The entry in the union
**                       for base tokens is called "yy0".
**    YYSTACKDEPTH       is the maximum depth of the parser's stack.  If
**                       zero the stack is dynamically sized using realloc()
**    ParseARG_SDECL     A static variable declaration for the %extra_argument
**    ParseARG_PDECL     A parameter declaration for the %extra_argument
**    ParseARG_STORE     Code to store %extra_argument into yypParser
**    ParseARG_FETCH     Code to extract %extra_argument from yypParser
**    YYNSTATE           the combined number of states.
**    YYNRULE            the number of rules in the grammar
**    YYERRORSYMBOL      is the code number of the error symbol.  If not
**                       defined, then do no error processing.
*/
#define YYCODETYPE unsigned char
#define YYNOCODE 49
#define YYACTIONTYPE unsigned char
#define ParseTOKENTYPE  struct token 
typedef union {
  int yyinit;
  ParseTOKENTYPE yy0;
  struct program yy6;
  struct value yy19;
  int yy28;
  struct ifblock yy44;
  struct statement yy47;
  char * yy49;
  struct block yy69;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 0
#endif
#define ParseARG_SDECL  struct parser_out *parser_out ;
#define ParseARG_PDECL , struct parser_out *parser_out 
#define ParseARG_FETCH  struct parser_out *parser_out  = yypParser->parser_out 
#define ParseARG_STORE yypParser->parser_out  = parser_out 
#define YYNSTATE 130
#define YYNRULE 49
#define YY_NO_ACTION      (YYNSTATE+YYNRULE+2)
#define YY_ACCEPT_ACTION  (YYNSTATE+YYNRULE+1)
#define YY_ERROR_ACTION   (YYNSTATE+YYNRULE)

/* The yyzerominor constant is used to initialize instances of
** YYMINORTYPE objects to zero. */
static const YYMINORTYPE yyzerominor = { 0 };

/* Define the yytestcase() macro to be a no-op if is not already defined
** otherwise.
**
** Applications can choose to define yytestcase() in the %include section
** to a macro that can assist in verifying code coverage.  For production
** code the yytestcase() macro should be turned off.  But it is useful
** for testing.
*/
#ifndef yytestcase
# define yytestcase(X)
#endif


/* Next are the tables used to determine what action to take based on the
** current state and lookahead token.  These tables are used to implement
** functions that take a state number and lookahead value and return an
** action integer.  
**
** Suppose the action integer is N.  Then the action is determined as
** follows
**
**   0 <= N < YYNSTATE                  Shift N.  That is, push the lookahead
**                                      token onto the stack and goto state N.
**
**   YYNSTATE <= N < YYNSTATE+YYNRULE   Reduce by rule N-YYNSTATE.
**
**   N == YYNSTATE+YYNRULE              A syntax error has occurred.
**
**   N == YYNSTATE+YYNRULE+1            The parser accepts its input.
**
**   N == YYNSTATE+YYNRULE+2            No such action.  Denotes unused
**                                      slots in the yy_action[] table.
**
** The action table is constructed as a single large table named yy_action[].
** Given state S and lookahead X, the action is computed as
**
**      yy_action[ yy_shift_ofst[S] + X ]
**
** If the index value yy_shift_ofst[S]+X is out of range or if the value
** yy_lookahead[yy_shift_ofst[S]+X] is not equal to X or if yy_shift_ofst[S]
** is equal to YY_SHIFT_USE_DFLT, it means that the action is not in the table
** and that yy_default[S] should be used instead.  
**
** The formula above is for computing the action when the lookahead is
** a terminal symbol.  If the lookahead is a non-terminal (as occurs after
** a reduce action) then the yy_reduce_ofst[] array is used in place of
** the yy_shift_ofst[] array and YY_REDUCE_USE_DFLT is used in place of
** YY_SHIFT_USE_DFLT.
**
** The following are the tables generated in this section:
**
**  yy_action[]        A single table containing all actions.
**  yy_lookahead[]     A table containing the lookahead for each entry in
**                     yy_action.  Used to detect hash collisions.
**  yy_shift_ofst[]    For each state, the offset into yy_action for
**                     shifting terminals.
**  yy_reduce_ofst[]   For each state, the offset into yy_action for
**                     shifting non-terminals after a reduce.
**  yy_default[]       Default action for each state.
*/
static const YYACTIONTYPE yy_action[] = {
 /*     0 */   107,   51,   59,    4,  104,    9,  107,    2,   59,    4,
 /*    10 */    94,    9,  107,   58,   59,    4,   54,    9,   95,  180,
 /*    20 */     5,    6,   29,   30,  100,   54,    5,  117,   29,   30,
 /*    30 */    34,  128,    5,   54,   29,   30,  111,   57,  102,  112,
 /*    40 */    59,  113,  115,   43,  111,   62,  102,  112,   96,  113,
 /*    50 */   115,   43,    2,  111,   65,  102,  112,   91,  113,  115,
 /*    60 */    43,  101,  111,   54,   60,  112,    7,  113,  115,   43,
 /*    70 */   111,   61,    2,  112,   63,  113,  115,   44,  111,    8,
 /*    80 */   103,  112,  108,  113,  115,   43,  111,    2,  116,  112,
 /*    90 */   106,  113,  115,   44,  111,    2,   68,  112,    2,  113,
 /*   100 */   115,   45,    1,  111,   73,   32,  112,  110,  113,  115,
 /*   110 */    46,  111,   64,   52,  112,   53,  113,  115,   47,  111,
 /*   120 */   109,   66,  112,   71,  113,  115,   48,  111,   70,   59,
 /*   130 */   112,   35,  113,  115,   50,   39,   67,   72,   97,   98,
 /*   140 */    76,   20,   77,   79,   80,   56,   42,   20,   82,   20,
 /*   150 */   129,   69,   42,   90,   42,  130,   20,   84,   20,   89,
 /*   160 */    75,   42,   78,   42,   20,   23,   24,   20,   81,   42,
 /*   170 */    20,   83,   42,   20,  124,   42,   20,   88,   42,   74,
 /*   180 */    93,   42,    2,   92,   55,   85,   13,   27,   25,   28,
 /*   190 */   105,  118,   99,   31,  114,    3,  119,   33,   10,   14,
 /*   200 */    49,   26,  120,   15,   11,   16,   86,   36,  121,   17,
 /*   210 */   181,   37,  122,   18,   87,   38,  123,   19,  181,   21,
 /*   220 */   181,   40,  125,  127,   41,  126,  181,   12,   22,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */     2,   29,    4,    5,    6,    7,    2,    7,    4,    5,
 /*    10 */    29,    7,    2,   43,    4,    5,   44,    7,   29,   47,
 /*    20 */    22,   21,   24,   25,    4,   44,   22,   23,   24,   25,
 /*    30 */    31,   32,   22,   44,   24,   25,   35,   36,   37,   38,
 /*    40 */     4,   40,   41,   42,   35,   36,   37,   38,   29,   40,
 /*    50 */    41,   42,    7,   35,   36,   37,   38,   15,   40,   41,
 /*    60 */    42,   35,   35,   44,   37,   38,   21,   40,   41,   42,
 /*    70 */    35,    4,    7,   38,   39,   40,   41,   42,   35,   14,
 /*    80 */    37,   38,   35,   40,   41,   42,   35,    7,    8,   38,
 /*    90 */    39,   40,   41,   42,   35,    7,    8,   38,    7,   40,
 /*   100 */    41,   42,    7,   35,   13,   10,   38,   45,   40,   41,
 /*   110 */    42,   35,   35,    1,   38,    3,   40,   41,   42,   35,
 /*   120 */    45,   43,   38,   16,   40,   41,   42,   35,   43,    4,
 /*   130 */    38,   33,   40,   41,   42,   46,   11,   12,   26,   27,
 /*   140 */    43,   30,   17,   43,   19,   34,   35,   30,   18,   30,
 /*   150 */    32,   34,   35,   34,   35,    0,   30,   43,   30,   43,
 /*   160 */    34,   35,   34,   35,   30,    2,    2,   30,   34,   35,
 /*   170 */    30,   34,   35,   30,   34,   35,   30,   34,   35,    8,
 /*   180 */    34,   35,    7,    8,    4,   14,    5,    8,    6,   20,
 /*   190 */     6,   23,    9,   20,    8,    7,    9,    8,    7,    5,
 /*   200 */     4,    6,    9,    5,    7,    5,    4,    6,    9,    5,
 /*   210 */    48,    6,    9,    5,    8,    6,    6,    5,   48,    5,
 /*   220 */    48,    6,    9,    6,    6,    9,   48,    7,    5,
};
#define YY_SHIFT_USE_DFLT (-3)
#define YY_SHIFT_MAX 93
static const short yy_shift_ofst[] = {
 /*     0 */   112,   10,   10,   10,   -2,    4,   10,   10,   10,   10,
 /*    10 */    10,   10,   10,  125,  125,  125,  125,  125,  125,  125,
 /*    20 */   125,  125,  125,  112,  112,  112,   42,   20,   36,   36,
 /*    30 */    67,   67,   36,   20,  107,   20,   20,   20,  130,   20,
 /*    40 */    20,   42,   95,    0,   65,   45,   80,   88,   91,  171,
 /*    50 */   175,  155,  163,  164,  180,  181,  182,  179,  183,  169,
 /*    60 */   184,  173,  186,  168,  188,  189,  187,  191,  194,  195,
 /*    70 */   193,  198,  197,  196,  200,  201,  199,  204,  205,  203,
 /*    80 */   208,  209,  212,  210,  213,  202,  206,  214,  215,  216,
 /*    90 */   217,  220,  223,  218,
};
#define YY_REDUCE_USE_DFLT (-31)
#define YY_REDUCE_MAX 41
static const short yy_reduce_ofst[] = {
 /*     0 */   -28,    1,    9,   18,   27,   35,   43,   51,   59,   68,
 /*    10 */    76,   84,   92,  111,  117,  119,  126,  128,  134,  137,
 /*    20 */   140,  143,  146,  -19,  -11,   19,   -1,  -30,   26,   47,
 /*    30 */    62,   75,   77,   78,   98,   85,   97,  100,   89,  114,
 /*    40 */   116,  118,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   131,  156,  156,  156,  179,  179,  179,  179,  179,  179,
 /*    10 */   179,  179,  179,  179,  179,  179,  179,  179,  179,  179,
 /*    20 */   150,  179,  179,  131,  131,  131,  140,  175,  179,  179,
 /*    30 */   179,  179,  179,  175,  144,  175,  175,  175,  147,  175,
 /*    40 */   175,  142,  179,  158,  179,  162,  179,  179,  179,  179,
 /*    50 */   179,  179,  179,  179,  179,  179,  179,  179,  179,  152,
 /*    60 */   179,  154,  179,  179,  179,  179,  179,  179,  179,  179,
 /*    70 */   179,  179,  179,  179,  179,  179,  179,  179,  179,  179,
 /*    80 */   179,  179,  179,  179,  179,  179,  179,  179,  179,  179,
 /*    90 */   179,  179,  179,  179,  132,  133,  134,  177,  178,  135,
 /*   100 */   176,  153,  157,  159,  160,  161,  163,  167,  168,  155,
 /*   110 */   169,  170,  171,  172,  166,  174,  173,  164,  165,  136,
 /*   120 */   137,  138,  146,  148,  151,  149,  139,  145,  141,  143,
};
#define YY_SZ_ACTTAB (int)(sizeof(yy_action)/sizeof(yy_action[0]))

/* The next table maps tokens into fallback tokens.  If a construct
** like the following:
** 
**      %fallback ID X Y Z.
**
** appears in the grammar, then ID becomes a fallback token for X, Y,
** and Z.  Whenever one of the tokens X, Y, or Z is input to the parser
** but it does not parse, the type of the token is changed to ID and
** the parse is retried before an error is thrown.
*/
#ifdef YYFALLBACK
static const YYCODETYPE yyFallback[] = {
};
#endif /* YYFALLBACK */

/* The following structure represents a single element of the
** parser's stack.  Information stored includes:
**
**   +  The state number for the parser at this level of the stack.
**
**   +  The value of the token stored at this level of the stack.
**      (In other words, the "major" token.)
**
**   +  The semantic value stored at this level of the stack.  This is
**      the information used by the action routines in the grammar.
**      It is sometimes called the "minor" token.
*/
struct yyStackEntry {
  YYACTIONTYPE stateno;  /* The state-number */
  YYCODETYPE major;      /* The major token value.  This is the code
                         ** number for the token at this stack level */
  YYMINORTYPE minor;     /* The user-supplied minor token value.  This
                         ** is the value of the token  */
};
typedef struct yyStackEntry yyStackEntry;

/* The state of the parser is completely contained in an instance of
** the following structure */
struct yyParser {
  int yyidx;                    /* Index of top element in stack */
#ifdef YYTRACKMAXSTACKDEPTH
  int yyidxMax;                 /* Maximum value of yyidx */
#endif
  int yyerrcnt;                 /* Shifts left before out of the error */
  ParseARG_SDECL                /* A place to hold %extra_argument */
#if YYSTACKDEPTH<=0
  int yystksz;                  /* Current side of the stack */
  yyStackEntry *yystack;        /* The parser's stack */
#else
  yyStackEntry yystack[YYSTACKDEPTH];  /* The parser's stack */
#endif
};
typedef struct yyParser yyParser;

#ifndef NDEBUG
#include <stdio.h>
static FILE *yyTraceFILE = 0;
static char *yyTracePrompt = 0;
#endif /* NDEBUG */

#ifndef NDEBUG
/* 
** Turn parser tracing on by giving a stream to which to write the trace
** and a prompt to preface each trace message.  Tracing is turned off
** by making either argument NULL 
**
** Inputs:
** <ul>
** <li> A FILE* to which trace output should be written.
**      If NULL, then tracing is turned off.
** <li> A prefix string written at the beginning of every
**      line of trace output.  If NULL, then tracing is
**      turned off.
** </ul>
**
** Outputs:
** None.
*/
void ParseTrace(FILE *TraceFILE, char *zTracePrompt){
  yyTraceFILE = TraceFILE;
  yyTracePrompt = zTracePrompt;
  if( yyTraceFILE==0 ) yyTracePrompt = 0;
  else if( yyTracePrompt==0 ) yyTraceFILE = 0;
}
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing shifts, the names of all terminals and nonterminals
** are required.  The following table supplies these names */
static const char *const yyTokenName[] = { 
  "$",             "INCLUDE",       "STRING",        "INCLUDE_GUARD",
  "NAME",          "CURLY_OPEN",    "CURLY_CLOSE",   "ROUND_OPEN",  
  "ROUND_CLOSE",   "SEMICOLON",     "ARROW",         "IF",          
  "FOREACH",       "AS",            "COLON",         "ELIF",        
  "ELSE",          "BLOCK",         "TOKEN_INTERRUPT",  "TOKEN_DO",    
  "DOT",           "COMMA",         "BRACKET_OPEN",  "BRACKET_CLOSE",
  "AT_SIGN",       "CARET",         "PROCESS",       "TEMPLATE",    
  "error",         "processes",     "statement",     "elif_maybe",  
  "elif",          "else_maybe",    "statements",    "dotted_name", 
  "list_contents_maybe",  "list_contents",  "list",          "map_contents",
  "map",           "invoc",         "value",         "name_maybe",  
  "process_or_template",  "name_list",     "interrupt_maybe",  "input",       
};
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
*/
static const char *const yyRuleName[] = {
 /*   0 */ "input ::= processes",
 /*   1 */ "processes ::=",
 /*   2 */ "processes ::= INCLUDE STRING processes",
 /*   3 */ "processes ::= INCLUDE_GUARD STRING processes",
 /*   4 */ "processes ::= process_or_template NAME CURLY_OPEN statements CURLY_CLOSE processes",
 /*   5 */ "statement ::= dotted_name ROUND_OPEN list_contents_maybe ROUND_CLOSE name_maybe SEMICOLON",
 /*   6 */ "statement ::= dotted_name ARROW dotted_name ROUND_OPEN list_contents_maybe ROUND_CLOSE name_maybe SEMICOLON",
 /*   7 */ "statement ::= IF ROUND_OPEN value ROUND_CLOSE CURLY_OPEN statements CURLY_CLOSE elif_maybe else_maybe name_maybe SEMICOLON",
 /*   8 */ "statement ::= FOREACH ROUND_OPEN value AS NAME ROUND_CLOSE CURLY_OPEN statements CURLY_CLOSE name_maybe SEMICOLON",
 /*   9 */ "statement ::= FOREACH ROUND_OPEN value AS NAME COLON NAME ROUND_CLOSE CURLY_OPEN statements CURLY_CLOSE name_maybe SEMICOLON",
 /*  10 */ "elif_maybe ::=",
 /*  11 */ "elif_maybe ::= elif",
 /*  12 */ "elif ::= ELIF ROUND_OPEN value ROUND_CLOSE CURLY_OPEN statements CURLY_CLOSE",
 /*  13 */ "elif ::= ELIF ROUND_OPEN value ROUND_CLOSE CURLY_OPEN statements CURLY_CLOSE elif",
 /*  14 */ "else_maybe ::=",
 /*  15 */ "else_maybe ::= ELSE CURLY_OPEN statements CURLY_CLOSE",
 /*  16 */ "statement ::= BLOCK CURLY_OPEN statements CURLY_CLOSE name_maybe SEMICOLON",
 /*  17 */ "interrupt_maybe ::=",
 /*  18 */ "interrupt_maybe ::= TOKEN_INTERRUPT CURLY_OPEN statements CURLY_CLOSE",
 /*  19 */ "statement ::= TOKEN_DO CURLY_OPEN statements CURLY_CLOSE interrupt_maybe name_maybe SEMICOLON",
 /*  20 */ "statements ::= statement",
 /*  21 */ "statements ::= statement statements",
 /*  22 */ "dotted_name ::= NAME",
 /*  23 */ "dotted_name ::= NAME DOT dotted_name",
 /*  24 */ "name_list ::= NAME",
 /*  25 */ "name_list ::= NAME DOT name_list",
 /*  26 */ "list_contents_maybe ::=",
 /*  27 */ "list_contents_maybe ::= list_contents",
 /*  28 */ "list_contents ::= value",
 /*  29 */ "list_contents ::= value COMMA list_contents",
 /*  30 */ "list ::= CURLY_OPEN CURLY_CLOSE",
 /*  31 */ "list ::= CURLY_OPEN list_contents CURLY_CLOSE",
 /*  32 */ "map_contents ::= value COLON value",
 /*  33 */ "map_contents ::= value COLON value COMMA map_contents",
 /*  34 */ "map ::= BRACKET_OPEN BRACKET_CLOSE",
 /*  35 */ "map ::= BRACKET_OPEN map_contents BRACKET_CLOSE",
 /*  36 */ "invoc ::= value ROUND_OPEN list_contents_maybe ROUND_CLOSE",
 /*  37 */ "value ::= STRING",
 /*  38 */ "value ::= AT_SIGN dotted_name",
 /*  39 */ "value ::= CARET name_list",
 /*  40 */ "value ::= dotted_name",
 /*  41 */ "value ::= list",
 /*  42 */ "value ::= map",
 /*  43 */ "value ::= ROUND_OPEN value ROUND_CLOSE",
 /*  44 */ "value ::= invoc",
 /*  45 */ "name_maybe ::=",
 /*  46 */ "name_maybe ::= NAME",
 /*  47 */ "process_or_template ::= PROCESS",
 /*  48 */ "process_or_template ::= TEMPLATE",
};
#endif /* NDEBUG */


#if YYSTACKDEPTH<=0
/*
** Try to increase the size of the parser stack.
*/
static void yyGrowStack(yyParser *p){
  int newSize;
  yyStackEntry *pNew;

  newSize = p->yystksz*2 + 100;
  pNew = realloc(p->yystack, newSize*sizeof(pNew[0]));
  if( pNew ){
    p->yystack = pNew;
    p->yystksz = newSize;
#ifndef NDEBUG
    if( yyTraceFILE ){
      fprintf(yyTraceFILE,"%sStack grows to %d entries!\n",
              yyTracePrompt, p->yystksz);
    }
#endif
  }
}
#endif

/* 
** This function allocates a new parser.
** The only argument is a pointer to a function which works like
** malloc.
**
** Inputs:
** A pointer to the function used to allocate memory.
**
** Outputs:
** A pointer to a parser.  This pointer is used in subsequent calls
** to Parse and ParseFree.
*/
void *ParseAlloc(void *(*mallocProc)(size_t)){
  yyParser *pParser;
  pParser = (yyParser*)(*mallocProc)( (size_t)sizeof(yyParser) );
  if( pParser ){
    pParser->yyidx = -1;
#ifdef YYTRACKMAXSTACKDEPTH
    pParser->yyidxMax = 0;
#endif
#if YYSTACKDEPTH<=0
    pParser->yystack = NULL;
    pParser->yystksz = 0;
    yyGrowStack(pParser);
#endif
  }
  return pParser;
}

/* The following function deletes the value associated with a
** symbol.  The symbol can be either a terminal or nonterminal.
** "yymajor" is the symbol code, and "yypminor" is a pointer to
** the value.
*/
static void yy_destructor(
  yyParser *yypParser,    /* The parser */
  YYCODETYPE yymajor,     /* Type code for object to destroy */
  YYMINORTYPE *yypminor   /* The object to be destroyed */
){
  ParseARG_FETCH;
  switch( yymajor ){
    /* Here is inserted the actions which take place when a
    ** terminal or non-terminal is destroyed.  This can happen
    ** when the symbol is popped from the stack during a
    ** reduce or during error processing or when a parser is 
    ** being destroyed before it is finished parsing.
    **
    ** Note: during a reduce, the only symbols destroyed are those
    ** which appear on the RHS of the rule, but which are not used
    ** inside the C code.
    */
      /* TERMINAL Destructor */
    case 1: /* INCLUDE */
    case 2: /* STRING */
    case 3: /* INCLUDE_GUARD */
    case 4: /* NAME */
    case 5: /* CURLY_OPEN */
    case 6: /* CURLY_CLOSE */
    case 7: /* ROUND_OPEN */
    case 8: /* ROUND_CLOSE */
    case 9: /* SEMICOLON */
    case 10: /* ARROW */
    case 11: /* IF */
    case 12: /* FOREACH */
    case 13: /* AS */
    case 14: /* COLON */
    case 15: /* ELIF */
    case 16: /* ELSE */
    case 17: /* BLOCK */
    case 18: /* TOKEN_INTERRUPT */
    case 19: /* TOKEN_DO */
    case 20: /* DOT */
    case 21: /* COMMA */
    case 22: /* BRACKET_OPEN */
    case 23: /* BRACKET_CLOSE */
    case 24: /* AT_SIGN */
    case 25: /* CARET */
    case 26: /* PROCESS */
    case 27: /* TEMPLATE */
{
#line 89 "NCDConfigParser_parse.y"
 free_token((yypminor->yy0)); 
#line 561 "NCDConfigParser_parse.c"
}
      break;
    case 29: /* processes */
{
#line 111 "NCDConfigParser_parse.y"
 (void)parser_out; free_program((yypminor->yy6)); 
#line 568 "NCDConfigParser_parse.c"
}
      break;
    case 30: /* statement */
{
#line 112 "NCDConfigParser_parse.y"
 free_statement((yypminor->yy47)); 
#line 575 "NCDConfigParser_parse.c"
}
      break;
    case 31: /* elif_maybe */
    case 32: /* elif */
{
#line 113 "NCDConfigParser_parse.y"
 free_ifblock((yypminor->yy44)); 
#line 583 "NCDConfigParser_parse.c"
}
      break;
    case 33: /* else_maybe */
    case 34: /* statements */
    case 46: /* interrupt_maybe */
{
#line 115 "NCDConfigParser_parse.y"
 free_block((yypminor->yy69)); 
#line 592 "NCDConfigParser_parse.c"
}
      break;
    case 35: /* dotted_name */
    case 43: /* name_maybe */
{
#line 117 "NCDConfigParser_parse.y"
 free((yypminor->yy49)); 
#line 600 "NCDConfigParser_parse.c"
}
      break;
    case 36: /* list_contents_maybe */
    case 37: /* list_contents */
    case 38: /* list */
    case 39: /* map_contents */
    case 40: /* map */
    case 41: /* invoc */
    case 42: /* value */
    case 45: /* name_list */
{
#line 118 "NCDConfigParser_parse.y"
 free_value((yypminor->yy19)); 
#line 614 "NCDConfigParser_parse.c"
}
      break;
    default:  break;   /* If no destructor action specified: do nothing */
  }
}

/*
** Pop the parser's stack once.
**
** If there is a destructor routine associated with the token which
** is popped from the stack, then call it.
**
** Return the major token number for the symbol popped.
*/
static int yy_pop_parser_stack(yyParser *pParser){
  YYCODETYPE yymajor;
  yyStackEntry *yytos = &pParser->yystack[pParser->yyidx];

  if( pParser->yyidx<0 ) return 0;
#ifndef NDEBUG
  if( yyTraceFILE && pParser->yyidx>=0 ){
    fprintf(yyTraceFILE,"%sPopping %s\n",
      yyTracePrompt,
      yyTokenName[yytos->major]);
  }
#endif
  yymajor = yytos->major;
  yy_destructor(pParser, yymajor, &yytos->minor);
  pParser->yyidx--;
  return yymajor;
}

/* 
** Deallocate and destroy a parser.  Destructors are all called for
** all stack elements before shutting the parser down.
**
** Inputs:
** <ul>
** <li>  A pointer to the parser.  This should be a pointer
**       obtained from ParseAlloc.
** <li>  A pointer to a function used to reclaim memory obtained
**       from malloc.
** </ul>
*/
void ParseFree(
  void *p,                    /* The parser to be deleted */
  void (*freeProc)(void*)     /* Function used to reclaim memory */
){
  yyParser *pParser = (yyParser*)p;
  if( pParser==0 ) return;
  while( pParser->yyidx>=0 ) yy_pop_parser_stack(pParser);
#if YYSTACKDEPTH<=0
  free(pParser->yystack);
#endif
  (*freeProc)((void*)pParser);
}

/*
** Return the peak depth of the stack for a parser.
*/
#ifdef YYTRACKMAXSTACKDEPTH
int ParseStackPeak(void *p){
  yyParser *pParser = (yyParser*)p;
  return pParser->yyidxMax;
}
#endif

/*
** Find the appropriate action for a parser given the terminal
** look-ahead token iLookAhead.
**
** If the look-ahead token is YYNOCODE, then check to see if the action is
** independent of the look-ahead.  If it is, return the action, otherwise
** return YY_NO_ACTION.
*/
static int yy_find_shift_action(
  yyParser *pParser,        /* The parser */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
  int stateno = pParser->yystack[pParser->yyidx].stateno;
 
  if( stateno>YY_SHIFT_MAX || (i = yy_shift_ofst[stateno])==YY_SHIFT_USE_DFLT ){
    return yy_default[stateno];
  }
  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
  if( i<0 || i>=YY_SZ_ACTTAB || yy_lookahead[i]!=iLookAhead ){
    if( iLookAhead>0 ){
#ifdef YYFALLBACK
      YYCODETYPE iFallback;            /* Fallback token */
      if( iLookAhead<sizeof(yyFallback)/sizeof(yyFallback[0])
             && (iFallback = yyFallback[iLookAhead])!=0 ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE, "%sFALLBACK %s => %s\n",
             yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[iFallback]);
        }
#endif
        return yy_find_shift_action(pParser, iFallback);
      }
#endif
#ifdef YYWILDCARD
      {
        int j = i - iLookAhead + YYWILDCARD;
        if( j>=0 && j<YY_SZ_ACTTAB && yy_lookahead[j]==YYWILDCARD ){
#ifndef NDEBUG
          if( yyTraceFILE ){
            fprintf(yyTraceFILE, "%sWILDCARD %s => %s\n",
               yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[YYWILDCARD]);
          }
#endif /* NDEBUG */
          return yy_action[j];
        }
      }
#endif /* YYWILDCARD */
    }
    return yy_default[stateno];
  }else{
    return yy_action[i];
  }
}

/*
** Find the appropriate action for a parser given the non-terminal
** look-ahead token iLookAhead.
**
** If the look-ahead token is YYNOCODE, then check to see if the action is
** independent of the look-ahead.  If it is, return the action, otherwise
** return YY_NO_ACTION.
*/
static int yy_find_reduce_action(
  int stateno,              /* Current state number */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
#ifdef YYERRORSYMBOL
  if( stateno>YY_REDUCE_MAX ){
    return yy_default[stateno];
  }
#else
  assert( stateno<=YY_REDUCE_MAX );
#endif
  i = yy_reduce_ofst[stateno];
  assert( i!=YY_REDUCE_USE_DFLT );
  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
#ifdef YYERRORSYMBOL
  if( i<0 || i>=YY_SZ_ACTTAB || yy_lookahead[i]!=iLookAhead ){
    return yy_default[stateno];
  }
#else
  assert( i>=0 && i<YY_SZ_ACTTAB );
  assert( yy_lookahead[i]==iLookAhead );
#endif
  return yy_action[i];
}

/*
** The following routine is called if the stack overflows.
*/
static void yyStackOverflow(yyParser *yypParser, YYMINORTYPE *yypMinor){
   ParseARG_FETCH;
   yypParser->yyidx--;
#ifndef NDEBUG
   if( yyTraceFILE ){
     fprintf(yyTraceFILE,"%sStack Overflow!\n",yyTracePrompt);
   }
#endif
   while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
   /* Here code is inserted which will execute if the parser
   ** stack every overflows */
#line 136 "NCDConfigParser_parse.y"

    if (yypMinor) {
        free_token(yypMinor->yy0);
    }
#line 792 "NCDConfigParser_parse.c"
   ParseARG_STORE; /* Suppress warning about unused %extra_argument var */
}

/*
** Perform a shift action.
*/
static void yy_shift(
  yyParser *yypParser,          /* The parser to be shifted */
  int yyNewState,               /* The new state to shift in */
  int yyMajor,                  /* The major token to shift in */
  YYMINORTYPE *yypMinor         /* Pointer to the minor token to shift in */
){
  yyStackEntry *yytos;
  yypParser->yyidx++;
#ifdef YYTRACKMAXSTACKDEPTH
  if( yypParser->yyidx>yypParser->yyidxMax ){
    yypParser->yyidxMax = yypParser->yyidx;
  }
#endif
#if YYSTACKDEPTH>0 
  if( yypParser->yyidx>=YYSTACKDEPTH ){
    yyStackOverflow(yypParser, yypMinor);
    return;
  }
#else
  if( yypParser->yyidx>=yypParser->yystksz ){
    yyGrowStack(yypParser);
    if( yypParser->yyidx>=yypParser->yystksz ){
      yyStackOverflow(yypParser, yypMinor);
      return;
    }
  }
#endif
  yytos = &yypParser->yystack[yypParser->yyidx];
  yytos->stateno = (YYACTIONTYPE)yyNewState;
  yytos->major = (YYCODETYPE)yyMajor;
  yytos->minor = *yypMinor;
#ifndef NDEBUG
  if( yyTraceFILE && yypParser->yyidx>0 ){
    int i;
    fprintf(yyTraceFILE,"%sShift %d\n",yyTracePrompt,yyNewState);
    fprintf(yyTraceFILE,"%sStack:",yyTracePrompt);
    for(i=1; i<=yypParser->yyidx; i++)
      fprintf(yyTraceFILE," %s",yyTokenName[yypParser->yystack[i].major]);
    fprintf(yyTraceFILE,"\n");
  }
#endif
}

/* The following table contains information about every rule that
** is used during the reduce.
*/
static const struct {
  YYCODETYPE lhs;         /* Symbol on the left-hand side of the rule */
  unsigned char nrhs;     /* Number of right-hand side symbols in the rule */
} yyRuleInfo[] = {
  { 47, 1 },
  { 29, 0 },
  { 29, 3 },
  { 29, 3 },
  { 29, 6 },
  { 30, 6 },
  { 30, 8 },
  { 30, 11 },
  { 30, 11 },
  { 30, 13 },
  { 31, 0 },
  { 31, 1 },
  { 32, 7 },
  { 32, 8 },
  { 33, 0 },
  { 33, 4 },
  { 30, 6 },
  { 46, 0 },
  { 46, 4 },
  { 30, 7 },
  { 34, 1 },
  { 34, 2 },
  { 35, 1 },
  { 35, 3 },
  { 45, 1 },
  { 45, 3 },
  { 36, 0 },
  { 36, 1 },
  { 37, 1 },
  { 37, 3 },
  { 38, 2 },
  { 38, 3 },
  { 39, 3 },
  { 39, 5 },
  { 40, 2 },
  { 40, 3 },
  { 41, 4 },
  { 42, 1 },
  { 42, 2 },
  { 42, 2 },
  { 42, 1 },
  { 42, 1 },
  { 42, 1 },
  { 42, 3 },
  { 42, 1 },
  { 43, 0 },
  { 43, 1 },
  { 44, 1 },
  { 44, 1 },
};

static void yy_accept(yyParser*);  /* Forward Declaration */

/*
** Perform a reduce action and the shift that must immediately
** follow the reduce.
*/
static void yy_reduce(
  yyParser *yypParser,         /* The parser */
  int yyruleno                 /* Number of the rule by which to reduce */
){
  int yygoto;                     /* The next state */
  int yyact;                      /* The next action */
  YYMINORTYPE yygotominor;        /* The LHS of the rule reduced */
  yyStackEntry *yymsp;            /* The top of the parser's stack */
  int yysize;                     /* Amount to pop the stack */
  ParseARG_FETCH;
  yymsp = &yypParser->yystack[yypParser->yyidx];
#ifndef NDEBUG
  if( yyTraceFILE && yyruleno>=0 
        && yyruleno<(int)(sizeof(yyRuleName)/sizeof(yyRuleName[0])) ){
    fprintf(yyTraceFILE, "%sReduce [%s].\n", yyTracePrompt,
      yyRuleName[yyruleno]);
  }
#endif /* NDEBUG */

  /* Silence complaints from purify about yygotominor being uninitialized
  ** in some cases when it is copied into the stack after the following
  ** switch.  yygotominor is uninitialized when a rule reduces that does
  ** not set the value of its left-hand side nonterminal.  Leaving the
  ** value of the nonterminal uninitialized is utterly harmless as long
  ** as the value is never used.  So really the only thing this code
  ** accomplishes is to quieten purify.  
  **
  ** 2007-01-16:  The wireshark project (www.wireshark.org) reports that
  ** without this code, their parser segfaults.  I'm not sure what there
  ** parser is doing to make this happen.  This is the second bug report
  ** from wireshark this week.  Clearly they are stressing Lemon in ways
  ** that it has not been previously stressed...  (SQLite ticket #2172)
  */
  /*memset(&yygotominor, 0, sizeof(yygotominor));*/
  yygotominor = yyzerominor;


  switch( yyruleno ){
  /* Beginning here are the reduction cases.  A typical example
  ** follows:
  **   case 0:
  **  #line <lineno> <grammarfile>
  **     { ... }           // User supplied code
  **  #line <lineno> <thisfile>
  **     break;
  */
      case 0: /* input ::= processes */
#line 142 "NCDConfigParser_parse.y"
{
    ASSERT(!parser_out->have_ast)

    if (yymsp[0].minor.yy6.have) {
        parser_out->have_ast = 1;
        parser_out->ast = yymsp[0].minor.yy6.v;
    }
}
#line 962 "NCDConfigParser_parse.c"
        break;
      case 1: /* processes ::= */
#line 151 "NCDConfigParser_parse.y"
{
    NCDProgram prog;
    NCDProgram_Init(&prog);
    
    yygotominor.yy6.have = 1;
    yygotominor.yy6.v = prog;
}
#line 973 "NCDConfigParser_parse.c"
        break;
      case 2: /* processes ::= INCLUDE STRING processes */
#line 159 "NCDConfigParser_parse.y"
{
    ASSERT(yymsp[-1].minor.yy0.str)
    if (!yymsp[0].minor.yy6.have) {
        goto failA0;
    }
    
    NCDProgramElem elem;
    if (!NCDProgramElem_InitInclude(&elem, yymsp[-1].minor.yy0.str, yymsp[-1].minor.yy0.len)) {
        goto failA0;
    }
    
    if (!NCDProgram_PrependElem(&yymsp[0].minor.yy6.v, elem)) {
        goto failA1;
    }
    
    yygotominor.yy6.have = 1;
    yygotominor.yy6.v = yymsp[0].minor.yy6.v;
    yymsp[0].minor.yy6.have = 0;
    goto doneA;

failA1:
    NCDProgramElem_Free(&elem);
failA0:
    yygotominor.yy6.have = 0;
    parser_out->out_of_memory = 1;
doneA:
    free_token(yymsp[-1].minor.yy0);
    free_program(yymsp[0].minor.yy6);
  yy_destructor(yypParser,1,&yymsp[-2].minor);
}
#line 1007 "NCDConfigParser_parse.c"
        break;
      case 3: /* processes ::= INCLUDE_GUARD STRING processes */
#line 189 "NCDConfigParser_parse.y"
{
    ASSERT(yymsp[-1].minor.yy0.str)
    if (!yymsp[0].minor.yy6.have) {
        goto failZ0;
    }
    
    NCDProgramElem elem;
    if (!NCDProgramElem_InitIncludeGuard(&elem, yymsp[-1].minor.yy0.str, yymsp[-1].minor.yy0.len)) {
        goto failZ0;
    }
    
    if (!NCDProgram_PrependElem(&yymsp[0].minor.yy6.v, elem)) {
        goto failZ1;
    }
    
    yygotominor.yy6.have = 1;
    yygotominor.yy6.v = yymsp[0].minor.yy6.v;
    yymsp[0].minor.yy6.have = 0;
    goto doneZ;

failZ1:
    NCDProgramElem_Free(&elem);
failZ0:
    yygotominor.yy6.have = 0;
    parser_out->out_of_memory = 1;
doneZ:
    free_token(yymsp[-1].minor.yy0);
    free_program(yymsp[0].minor.yy6);
  yy_destructor(yypParser,3,&yymsp[-2].minor);
}
#line 1041 "NCDConfigParser_parse.c"
        break;
      case 4: /* processes ::= process_or_template NAME CURLY_OPEN statements CURLY_CLOSE processes */
#line 219 "NCDConfigParser_parse.y"
{
    ASSERT(yymsp[-4].minor.yy0.str)
    if (!yymsp[-2].minor.yy69.have || !yymsp[0].minor.yy6.have) {
        goto failB0;
    }

    NCDProcess proc;
    if (!NCDProcess_Init(&proc, yymsp[-5].minor.yy28, yymsp[-4].minor.yy0.str, yymsp[-2].minor.yy69.v)) {
        goto failB0;
    }
    yymsp[-2].minor.yy69.have = 0;
    
    NCDProgramElem elem;
    NCDProgramElem_InitProcess(&elem, proc);

    if (!NCDProgram_PrependElem(&yymsp[0].minor.yy6.v, elem)) {
        goto failB1;
    }

    yygotominor.yy6.have = 1;
    yygotominor.yy6.v = yymsp[0].minor.yy6.v;
    yymsp[0].minor.yy6.have = 0;
    goto doneB;

failB1:
    NCDProgramElem_Free(&elem);
failB0:
    yygotominor.yy6.have = 0;
    parser_out->out_of_memory = 1;
doneB:
    free_token(yymsp[-4].minor.yy0);
    free_block(yymsp[-2].minor.yy69);
    free_program(yymsp[0].minor.yy6);
  yy_destructor(yypParser,5,&yymsp[-3].minor);
  yy_destructor(yypParser,6,&yymsp[-1].minor);
}
#line 1081 "NCDConfigParser_parse.c"
        break;
      case 5: /* statement ::= dotted_name ROUND_OPEN list_contents_maybe ROUND_CLOSE name_maybe SEMICOLON */
#line 254 "NCDConfigParser_parse.y"
{
    if (!yymsp[-5].minor.yy49 || !yymsp[-3].minor.yy19.have) {
        goto failC0;
    }

    if (!NCDStatement_InitReg(&yygotominor.yy47.v, yymsp[-1].minor.yy49, NULL, yymsp[-5].minor.yy49, yymsp[-3].minor.yy19.v)) {
        goto failC0;
    }
    yymsp[-3].minor.yy19.have = 0;

    yygotominor.yy47.have = 1;
    goto doneC;

failC0:
    yygotominor.yy47.have = 0;
    parser_out->out_of_memory = 1;
doneC:
    free(yymsp[-5].minor.yy49);
    free_value(yymsp[-3].minor.yy19);
    free(yymsp[-1].minor.yy49);
  yy_destructor(yypParser,7,&yymsp[-4].minor);
  yy_destructor(yypParser,8,&yymsp[-2].minor);
  yy_destructor(yypParser,9,&yymsp[0].minor);
}
#line 1109 "NCDConfigParser_parse.c"
        break;
      case 6: /* statement ::= dotted_name ARROW dotted_name ROUND_OPEN list_contents_maybe ROUND_CLOSE name_maybe SEMICOLON */
#line 276 "NCDConfigParser_parse.y"
{
    if (!yymsp[-7].minor.yy49 || !yymsp[-5].minor.yy49 || !yymsp[-3].minor.yy19.have) {
        goto failD0;
    }

    if (!NCDStatement_InitReg(&yygotominor.yy47.v, yymsp[-1].minor.yy49, yymsp[-7].minor.yy49, yymsp[-5].minor.yy49, yymsp[-3].minor.yy19.v)) {
        goto failD0;
    }
    yymsp[-3].minor.yy19.have = 0;

    yygotominor.yy47.have = 1;
    goto doneD;

failD0:
    yygotominor.yy47.have = 0;
    parser_out->out_of_memory = 1;
doneD:
    free(yymsp[-7].minor.yy49);
    free(yymsp[-5].minor.yy49);
    free_value(yymsp[-3].minor.yy19);
    free(yymsp[-1].minor.yy49);
  yy_destructor(yypParser,10,&yymsp[-6].minor);
  yy_destructor(yypParser,7,&yymsp[-4].minor);
  yy_destructor(yypParser,8,&yymsp[-2].minor);
  yy_destructor(yypParser,9,&yymsp[0].minor);
}
#line 1139 "NCDConfigParser_parse.c"
        break;
      case 7: /* statement ::= IF ROUND_OPEN value ROUND_CLOSE CURLY_OPEN statements CURLY_CLOSE elif_maybe else_maybe name_maybe SEMICOLON */
#line 299 "NCDConfigParser_parse.y"
{
    if (!yymsp[-8].minor.yy19.have || !yymsp[-5].minor.yy69.have || !yymsp[-3].minor.yy44.have) {
        goto failE0;
    }

    NCDIf ifc;
    NCDIf_Init(&ifc, yymsp[-8].minor.yy19.v, yymsp[-5].minor.yy69.v);
    yymsp[-8].minor.yy19.have = 0;
    yymsp[-5].minor.yy69.have = 0;

    if (!NCDIfBlock_PrependIf(&yymsp[-3].minor.yy44.v, ifc)) {
        NCDIf_Free(&ifc);
        goto failE0;
    }

    if (!NCDStatement_InitIf(&yygotominor.yy47.v, yymsp[-1].minor.yy49, yymsp[-3].minor.yy44.v, NCDIFTYPE_IF)) {
        goto failE0;
    }
    yymsp[-3].minor.yy44.have = 0;

    if (yymsp[-2].minor.yy69.have) {
        NCDStatement_IfAddElse(&yygotominor.yy47.v, yymsp[-2].minor.yy69.v);
        yymsp[-2].minor.yy69.have = 0;
    }

    yygotominor.yy47.have = 1;
    goto doneE;

failE0:
    yygotominor.yy47.have = 0;
    parser_out->out_of_memory = 1;
doneE:
    free_value(yymsp[-8].minor.yy19);
    free_block(yymsp[-5].minor.yy69);
    free_ifblock(yymsp[-3].minor.yy44);
    free_block(yymsp[-2].minor.yy69);
    free(yymsp[-1].minor.yy49);
  yy_destructor(yypParser,11,&yymsp[-10].minor);
  yy_destructor(yypParser,7,&yymsp[-9].minor);
  yy_destructor(yypParser,8,&yymsp[-7].minor);
  yy_destructor(yypParser,5,&yymsp[-6].minor);
  yy_destructor(yypParser,6,&yymsp[-4].minor);
  yy_destructor(yypParser,9,&yymsp[0].minor);
}
#line 1187 "NCDConfigParser_parse.c"
        break;
      case 8: /* statement ::= FOREACH ROUND_OPEN value AS NAME ROUND_CLOSE CURLY_OPEN statements CURLY_CLOSE name_maybe SEMICOLON */
#line 338 "NCDConfigParser_parse.y"
{
    if (!yymsp[-8].minor.yy19.have || !yymsp[-6].minor.yy0.str || !yymsp[-3].minor.yy69.have) {
        goto failEA0;
    }
    
    if (!NCDStatement_InitForeach(&yygotominor.yy47.v, yymsp[-1].minor.yy49, yymsp[-8].minor.yy19.v, yymsp[-6].minor.yy0.str, NULL, yymsp[-3].minor.yy69.v)) {
        goto failEA0;
    }
    yymsp[-8].minor.yy19.have = 0;
    yymsp[-3].minor.yy69.have = 0;
    
    yygotominor.yy47.have = 1;
    goto doneEA0;
    
failEA0:
    yygotominor.yy47.have = 0;
    parser_out->out_of_memory = 1;
doneEA0:
    free_value(yymsp[-8].minor.yy19);
    free_token(yymsp[-6].minor.yy0);
    free_block(yymsp[-3].minor.yy69);
    free(yymsp[-1].minor.yy49);
  yy_destructor(yypParser,12,&yymsp[-10].minor);
  yy_destructor(yypParser,7,&yymsp[-9].minor);
  yy_destructor(yypParser,13,&yymsp[-7].minor);
  yy_destructor(yypParser,8,&yymsp[-5].minor);
  yy_destructor(yypParser,5,&yymsp[-4].minor);
  yy_destructor(yypParser,6,&yymsp[-2].minor);
  yy_destructor(yypParser,9,&yymsp[0].minor);
}
#line 1221 "NCDConfigParser_parse.c"
        break;
      case 9: /* statement ::= FOREACH ROUND_OPEN value AS NAME COLON NAME ROUND_CLOSE CURLY_OPEN statements CURLY_CLOSE name_maybe SEMICOLON */
#line 362 "NCDConfigParser_parse.y"
{
    if (!yymsp[-10].minor.yy19.have || !yymsp[-8].minor.yy0.str || !yymsp[-6].minor.yy0.str || !yymsp[-3].minor.yy69.have) {
        goto failEB0;
    }
    
    if (!NCDStatement_InitForeach(&yygotominor.yy47.v, yymsp[-1].minor.yy49, yymsp[-10].minor.yy19.v, yymsp[-8].minor.yy0.str, yymsp[-6].minor.yy0.str, yymsp[-3].minor.yy69.v)) {
        goto failEB0;
    }
    yymsp[-10].minor.yy19.have = 0;
    yymsp[-3].minor.yy69.have = 0;
    
    yygotominor.yy47.have = 1;
    goto doneEB0;
    
failEB0:
    yygotominor.yy47.have = 0;
    parser_out->out_of_memory = 1;
doneEB0:
    free_value(yymsp[-10].minor.yy19);
    free_token(yymsp[-8].minor.yy0);
    free_token(yymsp[-6].minor.yy0);
    free_block(yymsp[-3].minor.yy69);
    free(yymsp[-1].minor.yy49);
  yy_destructor(yypParser,12,&yymsp[-12].minor);
  yy_destructor(yypParser,7,&yymsp[-11].minor);
  yy_destructor(yypParser,13,&yymsp[-9].minor);
  yy_destructor(yypParser,14,&yymsp[-7].minor);
  yy_destructor(yypParser,8,&yymsp[-5].minor);
  yy_destructor(yypParser,5,&yymsp[-4].minor);
  yy_destructor(yypParser,6,&yymsp[-2].minor);
  yy_destructor(yypParser,9,&yymsp[0].minor);
}
#line 1257 "NCDConfigParser_parse.c"
        break;
      case 10: /* elif_maybe ::= */
#line 387 "NCDConfigParser_parse.y"
{
    NCDIfBlock_Init(&yygotominor.yy44.v);
    yygotominor.yy44.have = 1;
}
#line 1265 "NCDConfigParser_parse.c"
        break;
      case 11: /* elif_maybe ::= elif */
#line 392 "NCDConfigParser_parse.y"
{
    yygotominor.yy44 = yymsp[0].minor.yy44;
}
#line 1272 "NCDConfigParser_parse.c"
        break;
      case 12: /* elif ::= ELIF ROUND_OPEN value ROUND_CLOSE CURLY_OPEN statements CURLY_CLOSE */
#line 396 "NCDConfigParser_parse.y"
{
    if (!yymsp[-4].minor.yy19.have || !yymsp[-1].minor.yy69.have) {
        goto failF0;
    }

    NCDIfBlock_Init(&yygotominor.yy44.v);

    NCDIf ifc;
    NCDIf_Init(&ifc, yymsp[-4].minor.yy19.v, yymsp[-1].minor.yy69.v);
    yymsp[-4].minor.yy19.have = 0;
    yymsp[-1].minor.yy69.have = 0;

    if (!NCDIfBlock_PrependIf(&yygotominor.yy44.v, ifc)) {
        goto failF1;
    }

    yygotominor.yy44.have = 1;
    goto doneF0;

failF1:
    NCDIf_Free(&ifc);
    NCDIfBlock_Free(&yygotominor.yy44.v);
failF0:
    yygotominor.yy44.have = 0;
    parser_out->out_of_memory = 1;
doneF0:
    free_value(yymsp[-4].minor.yy19);
    free_block(yymsp[-1].minor.yy69);
  yy_destructor(yypParser,15,&yymsp[-6].minor);
  yy_destructor(yypParser,7,&yymsp[-5].minor);
  yy_destructor(yypParser,8,&yymsp[-3].minor);
  yy_destructor(yypParser,5,&yymsp[-2].minor);
  yy_destructor(yypParser,6,&yymsp[0].minor);
}
#line 1310 "NCDConfigParser_parse.c"
        break;
      case 13: /* elif ::= ELIF ROUND_OPEN value ROUND_CLOSE CURLY_OPEN statements CURLY_CLOSE elif */
#line 426 "NCDConfigParser_parse.y"
{
    if (!yymsp[-5].minor.yy19.have || !yymsp[-2].minor.yy69.have || !yymsp[0].minor.yy44.have) {
        goto failG0;
    }

    NCDIf ifc;
    NCDIf_Init(&ifc, yymsp[-5].minor.yy19.v, yymsp[-2].minor.yy69.v);
    yymsp[-5].minor.yy19.have = 0;
    yymsp[-2].minor.yy69.have = 0;

    if (!NCDIfBlock_PrependIf(&yymsp[0].minor.yy44.v, ifc)) {
        goto failG1;
    }

    yygotominor.yy44.have = 1;
    yygotominor.yy44.v = yymsp[0].minor.yy44.v;
    yymsp[0].minor.yy44.have = 0;
    goto doneG0;

failG1:
    NCDIf_Free(&ifc);
failG0:
    yygotominor.yy44.have = 0;
    parser_out->out_of_memory = 1;
doneG0:
    free_value(yymsp[-5].minor.yy19);
    free_block(yymsp[-2].minor.yy69);
    free_ifblock(yymsp[0].minor.yy44);
  yy_destructor(yypParser,15,&yymsp[-7].minor);
  yy_destructor(yypParser,7,&yymsp[-6].minor);
  yy_destructor(yypParser,8,&yymsp[-4].minor);
  yy_destructor(yypParser,5,&yymsp[-3].minor);
  yy_destructor(yypParser,6,&yymsp[-1].minor);
}
#line 1348 "NCDConfigParser_parse.c"
        break;
      case 14: /* else_maybe ::= */
      case 17: /* interrupt_maybe ::= */ yytestcase(yyruleno==17);
#line 456 "NCDConfigParser_parse.y"
{
    yygotominor.yy69.have = 0;
}
#line 1356 "NCDConfigParser_parse.c"
        break;
      case 15: /* else_maybe ::= ELSE CURLY_OPEN statements CURLY_CLOSE */
#line 460 "NCDConfigParser_parse.y"
{
    yygotominor.yy69 = yymsp[-1].minor.yy69;
  yy_destructor(yypParser,16,&yymsp[-3].minor);
  yy_destructor(yypParser,5,&yymsp[-2].minor);
  yy_destructor(yypParser,6,&yymsp[0].minor);
}
#line 1366 "NCDConfigParser_parse.c"
        break;
      case 16: /* statement ::= BLOCK CURLY_OPEN statements CURLY_CLOSE name_maybe SEMICOLON */
#line 464 "NCDConfigParser_parse.y"
{
    if (!yymsp[-3].minor.yy69.have) {
        goto failGA0;
    }
    
    if (!NCDStatement_InitBlock(&yygotominor.yy47.v, yymsp[-1].minor.yy49, yymsp[-3].minor.yy69.v)) {
        goto failGA0;
    }
    yymsp[-3].minor.yy69.have = 0;
    
    yygotominor.yy47.have = 1;
    goto doneGA0;
    
failGA0:
    yygotominor.yy47.have = 0;
    parser_out->out_of_memory = 1;
doneGA0:
    free_block(yymsp[-3].minor.yy69);
    free(yymsp[-1].minor.yy49);
  yy_destructor(yypParser,17,&yymsp[-5].minor);
  yy_destructor(yypParser,5,&yymsp[-4].minor);
  yy_destructor(yypParser,6,&yymsp[-2].minor);
  yy_destructor(yypParser,9,&yymsp[0].minor);
}
#line 1394 "NCDConfigParser_parse.c"
        break;
      case 18: /* interrupt_maybe ::= TOKEN_INTERRUPT CURLY_OPEN statements CURLY_CLOSE */
#line 489 "NCDConfigParser_parse.y"
{
    yygotominor.yy69 = yymsp[-1].minor.yy69;
  yy_destructor(yypParser,18,&yymsp[-3].minor);
  yy_destructor(yypParser,5,&yymsp[-2].minor);
  yy_destructor(yypParser,6,&yymsp[0].minor);
}
#line 1404 "NCDConfigParser_parse.c"
        break;
      case 19: /* statement ::= TOKEN_DO CURLY_OPEN statements CURLY_CLOSE interrupt_maybe name_maybe SEMICOLON */
#line 493 "NCDConfigParser_parse.y"
{
    if (!yymsp[-4].minor.yy69.have) {
        goto failGB0;
    }
    
    NCDIfBlock if_block;
    NCDIfBlock_Init(&if_block);
    
    if (yymsp[-2].minor.yy69.have) {
        NCDIf int_if;
        NCDIf_InitBlock(&int_if, yymsp[-2].minor.yy69.v);
        yymsp[-2].minor.yy69.have = 0;
        
        if (!NCDIfBlock_PrependIf(&if_block, int_if)) {
            NCDIf_Free(&int_if);
            goto failGB1;
        }
    }
    
    NCDIf the_if;
    NCDIf_InitBlock(&the_if, yymsp[-4].minor.yy69.v);
    yymsp[-4].minor.yy69.have = 0;
    
    if (!NCDIfBlock_PrependIf(&if_block, the_if)) {
        NCDIf_Free(&the_if);
        goto failGB1;
    }
    
    if (!NCDStatement_InitIf(&yygotominor.yy47.v, yymsp[-1].minor.yy49, if_block, NCDIFTYPE_DO)) {
        goto failGB1;
    }
    
    yygotominor.yy47.have = 1;
    goto doneGB0;
    
failGB1:
    NCDIfBlock_Free(&if_block);
failGB0:
    yygotominor.yy47.have = 0;
    parser_out->out_of_memory = 1;
doneGB0:
    free_block(yymsp[-4].minor.yy69);
    free_block(yymsp[-2].minor.yy69);
    free(yymsp[-1].minor.yy49);
  yy_destructor(yypParser,19,&yymsp[-6].minor);
  yy_destructor(yypParser,5,&yymsp[-5].minor);
  yy_destructor(yypParser,6,&yymsp[-3].minor);
  yy_destructor(yypParser,9,&yymsp[0].minor);
}
#line 1457 "NCDConfigParser_parse.c"
        break;
      case 20: /* statements ::= statement */
#line 539 "NCDConfigParser_parse.y"
{
    if (!yymsp[0].minor.yy47.have) {
        goto failH0;
    }

    NCDBlock_Init(&yygotominor.yy69.v);

    if (!NCDBlock_PrependStatement(&yygotominor.yy69.v, yymsp[0].minor.yy47.v)) {
        goto failH1;
    }
    yymsp[0].minor.yy47.have = 0;

    yygotominor.yy69.have = 1;
    goto doneH;

failH1:
    NCDBlock_Free(&yygotominor.yy69.v);
failH0:
    yygotominor.yy69.have = 0;
    parser_out->out_of_memory = 1;
doneH:
    free_statement(yymsp[0].minor.yy47);
}
#line 1484 "NCDConfigParser_parse.c"
        break;
      case 21: /* statements ::= statement statements */
#line 563 "NCDConfigParser_parse.y"
{
    if (!yymsp[-1].minor.yy47.have || !yymsp[0].minor.yy69.have) {
        goto failI0;
    }

    if (!NCDBlock_PrependStatement(&yymsp[0].minor.yy69.v, yymsp[-1].minor.yy47.v)) {
        goto failI1;
    }
    yymsp[-1].minor.yy47.have = 0;

    yygotominor.yy69.have = 1;
    yygotominor.yy69.v = yymsp[0].minor.yy69.v;
    yymsp[0].minor.yy69.have = 0;
    goto doneI;

failI1:
    NCDBlock_Free(&yygotominor.yy69.v);
failI0:
    yygotominor.yy69.have = 0;
    parser_out->out_of_memory = 1;
doneI:
    free_statement(yymsp[-1].minor.yy47);
    free_block(yymsp[0].minor.yy69);
}
#line 1512 "NCDConfigParser_parse.c"
        break;
      case 22: /* dotted_name ::= NAME */
      case 46: /* name_maybe ::= NAME */ yytestcase(yyruleno==46);
#line 588 "NCDConfigParser_parse.y"
{
    ASSERT(yymsp[0].minor.yy0.str)

    yygotominor.yy49 = yymsp[0].minor.yy0.str;
}
#line 1522 "NCDConfigParser_parse.c"
        break;
      case 23: /* dotted_name ::= NAME DOT dotted_name */
#line 594 "NCDConfigParser_parse.y"
{
    ASSERT(yymsp[-2].minor.yy0.str)
    if (!yymsp[0].minor.yy49) {
        goto failJ0;
    }

    if (!(yygotominor.yy49 = concat_strings(3, yymsp[-2].minor.yy0.str, ".", yymsp[0].minor.yy49))) {
        goto failJ0;
    }

    goto doneJ;

failJ0:
    yygotominor.yy49 = NULL;
    parser_out->out_of_memory = 1;
doneJ:
    free_token(yymsp[-2].minor.yy0);
    free(yymsp[0].minor.yy49);
  yy_destructor(yypParser,20,&yymsp[-1].minor);
}
#line 1546 "NCDConfigParser_parse.c"
        break;
      case 24: /* name_list ::= NAME */
#line 614 "NCDConfigParser_parse.y"
{
    if (!yymsp[0].minor.yy0.str) {
        goto failK0;
    }

    NCDValue_InitList(&yygotominor.yy19.v);
    
    NCDValue this_string;
    if (!NCDValue_InitString(&this_string, yymsp[0].minor.yy0.str)) {
        goto failK1;
    }
    
    if (!NCDValue_ListPrepend(&yygotominor.yy19.v, this_string)) {
        goto failK2;
    }

    yygotominor.yy19.have = 1;
    goto doneK;

failK2:
    NCDValue_Free(&this_string);
failK1:
    NCDValue_Free(&yygotominor.yy19.v);
failK0:
    yygotominor.yy19.have = 0;
    parser_out->out_of_memory = 1;
doneK:
    free_token(yymsp[0].minor.yy0);
}
#line 1579 "NCDConfigParser_parse.c"
        break;
      case 25: /* name_list ::= NAME DOT name_list */
#line 644 "NCDConfigParser_parse.y"
{
    if (!yymsp[-2].minor.yy0.str || !yymsp[0].minor.yy19.have) {
        goto failKA0;
    }
    
    NCDValue this_string;
    if (!NCDValue_InitString(&this_string, yymsp[-2].minor.yy0.str)) {
        goto failKA0;
    }

    if (!NCDValue_ListPrepend(&yymsp[0].minor.yy19.v, this_string)) {
        goto failKA1;
    }

    yygotominor.yy19.have = 1;
    yygotominor.yy19.v = yymsp[0].minor.yy19.v;
    yymsp[0].minor.yy19.have = 0;
    goto doneKA;

failKA1:
    NCDValue_Free(&this_string);
failKA0:
    yygotominor.yy19.have = 0;
    parser_out->out_of_memory = 1;
doneKA:
    free_token(yymsp[-2].minor.yy0);
    free_value(yymsp[0].minor.yy19);
  yy_destructor(yypParser,20,&yymsp[-1].minor);
}
#line 1612 "NCDConfigParser_parse.c"
        break;
      case 26: /* list_contents_maybe ::= */
#line 673 "NCDConfigParser_parse.y"
{
    yygotominor.yy19.have = 1;
    NCDValue_InitList(&yygotominor.yy19.v);
}
#line 1620 "NCDConfigParser_parse.c"
        break;
      case 27: /* list_contents_maybe ::= list_contents */
      case 41: /* value ::= list */ yytestcase(yyruleno==41);
      case 42: /* value ::= map */ yytestcase(yyruleno==42);
      case 44: /* value ::= invoc */ yytestcase(yyruleno==44);
#line 678 "NCDConfigParser_parse.y"
{
    yygotominor.yy19 = yymsp[0].minor.yy19;
}
#line 1630 "NCDConfigParser_parse.c"
        break;
      case 28: /* list_contents ::= value */
#line 682 "NCDConfigParser_parse.y"
{
    if (!yymsp[0].minor.yy19.have) {
        goto failL0;
    }

    NCDValue_InitList(&yygotominor.yy19.v);

    if (!NCDValue_ListPrepend(&yygotominor.yy19.v, yymsp[0].minor.yy19.v)) {
        goto failL1;
    }
    yymsp[0].minor.yy19.have = 0;

    yygotominor.yy19.have = 1;
    goto doneL;

failL1:
    NCDValue_Free(&yygotominor.yy19.v);
failL0:
    yygotominor.yy19.have = 0;
    parser_out->out_of_memory = 1;
doneL:
    free_value(yymsp[0].minor.yy19);
}
#line 1657 "NCDConfigParser_parse.c"
        break;
      case 29: /* list_contents ::= value COMMA list_contents */
#line 706 "NCDConfigParser_parse.y"
{
    if (!yymsp[-2].minor.yy19.have || !yymsp[0].minor.yy19.have) {
        goto failM0;
    }

    if (!NCDValue_ListPrepend(&yymsp[0].minor.yy19.v, yymsp[-2].minor.yy19.v)) {
        goto failM0;
    }
    yymsp[-2].minor.yy19.have = 0;

    yygotominor.yy19.have = 1;
    yygotominor.yy19.v = yymsp[0].minor.yy19.v;
    yymsp[0].minor.yy19.have = 0;
    goto doneM;

failM0:
    yygotominor.yy19.have = 0;
    parser_out->out_of_memory = 1;
doneM:
    free_value(yymsp[-2].minor.yy19);
    free_value(yymsp[0].minor.yy19);
  yy_destructor(yypParser,21,&yymsp[-1].minor);
}
#line 1684 "NCDConfigParser_parse.c"
        break;
      case 30: /* list ::= CURLY_OPEN CURLY_CLOSE */
#line 729 "NCDConfigParser_parse.y"
{
    yygotominor.yy19.have = 1;
    NCDValue_InitList(&yygotominor.yy19.v);
  yy_destructor(yypParser,5,&yymsp[-1].minor);
  yy_destructor(yypParser,6,&yymsp[0].minor);
}
#line 1694 "NCDConfigParser_parse.c"
        break;
      case 31: /* list ::= CURLY_OPEN list_contents CURLY_CLOSE */
#line 734 "NCDConfigParser_parse.y"
{
    yygotominor.yy19 = yymsp[-1].minor.yy19;
  yy_destructor(yypParser,5,&yymsp[-2].minor);
  yy_destructor(yypParser,6,&yymsp[0].minor);
}
#line 1703 "NCDConfigParser_parse.c"
        break;
      case 32: /* map_contents ::= value COLON value */
#line 738 "NCDConfigParser_parse.y"
{
    if (!yymsp[-2].minor.yy19.have || !yymsp[0].minor.yy19.have) {
        goto failS0;
    }

    NCDValue_InitMap(&yygotominor.yy19.v);

    if (!NCDValue_MapPrepend(&yygotominor.yy19.v, yymsp[-2].minor.yy19.v, yymsp[0].minor.yy19.v)) {
        goto failS1;
    }
    yymsp[-2].minor.yy19.have = 0;
    yymsp[0].minor.yy19.have = 0;

    yygotominor.yy19.have = 1;
    goto doneS;

failS1:
    NCDValue_Free(&yygotominor.yy19.v);
failS0:
    yygotominor.yy19.have = 0;
    parser_out->out_of_memory = 1;
doneS:
    free_value(yymsp[-2].minor.yy19);
    free_value(yymsp[0].minor.yy19);
  yy_destructor(yypParser,14,&yymsp[-1].minor);
}
#line 1733 "NCDConfigParser_parse.c"
        break;
      case 33: /* map_contents ::= value COLON value COMMA map_contents */
#line 764 "NCDConfigParser_parse.y"
{
    if (!yymsp[-4].minor.yy19.have || !yymsp[-2].minor.yy19.have || !yymsp[0].minor.yy19.have) {
        goto failT0;
    }

    if (!NCDValue_MapPrepend(&yymsp[0].minor.yy19.v, yymsp[-4].minor.yy19.v, yymsp[-2].minor.yy19.v)) {
        goto failT0;
    }
    yymsp[-4].minor.yy19.have = 0;
    yymsp[-2].minor.yy19.have = 0;

    yygotominor.yy19.have = 1;
    yygotominor.yy19.v = yymsp[0].minor.yy19.v;
    yymsp[0].minor.yy19.have = 0;
    goto doneT;

failT0:
    yygotominor.yy19.have = 0;
    parser_out->out_of_memory = 1;
doneT:
    free_value(yymsp[-4].minor.yy19);
    free_value(yymsp[-2].minor.yy19);
    free_value(yymsp[0].minor.yy19);
  yy_destructor(yypParser,14,&yymsp[-3].minor);
  yy_destructor(yypParser,21,&yymsp[-1].minor);
}
#line 1763 "NCDConfigParser_parse.c"
        break;
      case 34: /* map ::= BRACKET_OPEN BRACKET_CLOSE */
#line 789 "NCDConfigParser_parse.y"
{
    yygotominor.yy19.have = 1;
    NCDValue_InitMap(&yygotominor.yy19.v);
  yy_destructor(yypParser,22,&yymsp[-1].minor);
  yy_destructor(yypParser,23,&yymsp[0].minor);
}
#line 1773 "NCDConfigParser_parse.c"
        break;
      case 35: /* map ::= BRACKET_OPEN map_contents BRACKET_CLOSE */
#line 794 "NCDConfigParser_parse.y"
{
    yygotominor.yy19 = yymsp[-1].minor.yy19;
  yy_destructor(yypParser,22,&yymsp[-2].minor);
  yy_destructor(yypParser,23,&yymsp[0].minor);
}
#line 1782 "NCDConfigParser_parse.c"
        break;
      case 36: /* invoc ::= value ROUND_OPEN list_contents_maybe ROUND_CLOSE */
#line 798 "NCDConfigParser_parse.y"
{
    if (!yymsp[-3].minor.yy19.have || !yymsp[-1].minor.yy19.have) {
        goto failQ0;
    }
    
    if (!NCDValue_InitInvoc(&yygotominor.yy19.v, yymsp[-3].minor.yy19.v, yymsp[-1].minor.yy19.v)) {
        goto failQ0;
    }
    yymsp[-3].minor.yy19.have = 0;
    yymsp[-1].minor.yy19.have = 0;
    yygotominor.yy19.have = 1;
    goto doneQ;
    
failQ0:
    yygotominor.yy19.have = 0;
    parser_out->out_of_memory = 1;
doneQ:
    free_value(yymsp[-3].minor.yy19);
    free_value(yymsp[-1].minor.yy19);
  yy_destructor(yypParser,7,&yymsp[-2].minor);
  yy_destructor(yypParser,8,&yymsp[0].minor);
}
#line 1808 "NCDConfigParser_parse.c"
        break;
      case 37: /* value ::= STRING */
#line 819 "NCDConfigParser_parse.y"
{
    ASSERT(yymsp[0].minor.yy0.str)

    if (!NCDValue_InitStringBin(&yygotominor.yy19.v, (uint8_t *)yymsp[0].minor.yy0.str, yymsp[0].minor.yy0.len)) {
        goto failU0;
    }

    yygotominor.yy19.have = 1;
    goto doneU;

failU0:
    yygotominor.yy19.have = 0;
    parser_out->out_of_memory = 1;
doneU:
    free_token(yymsp[0].minor.yy0);
}
#line 1828 "NCDConfigParser_parse.c"
        break;
      case 38: /* value ::= AT_SIGN dotted_name */
#line 836 "NCDConfigParser_parse.y"
{
    if (!yymsp[0].minor.yy49) {
        goto failUA0;
    }
    
    if (!NCDValue_InitString(&yygotominor.yy19.v, yymsp[0].minor.yy49)) {
        goto failUA0;
    }
    
    yygotominor.yy19.have = 1;
    goto doneUA0;
    
failUA0:
    yygotominor.yy19.have = 0;
    parser_out->out_of_memory = 1;
doneUA0:
    free(yymsp[0].minor.yy49);
  yy_destructor(yypParser,24,&yymsp[-1].minor);
}
#line 1851 "NCDConfigParser_parse.c"
        break;
      case 39: /* value ::= CARET name_list */
#line 855 "NCDConfigParser_parse.y"
{
    yygotominor.yy19 = yymsp[0].minor.yy19;
  yy_destructor(yypParser,25,&yymsp[-1].minor);
}
#line 1859 "NCDConfigParser_parse.c"
        break;
      case 40: /* value ::= dotted_name */
#line 859 "NCDConfigParser_parse.y"
{
    if (!yymsp[0].minor.yy49) {
        goto failV0;
    }

    if (!NCDValue_InitVar(&yygotominor.yy19.v, yymsp[0].minor.yy49)) {
        goto failV0;
    }

    yygotominor.yy19.have = 1;
    goto doneV;

failV0:
    yygotominor.yy19.have = 0;
    parser_out->out_of_memory = 1;
doneV:
    free(yymsp[0].minor.yy49);
}
#line 1881 "NCDConfigParser_parse.c"
        break;
      case 43: /* value ::= ROUND_OPEN value ROUND_CLOSE */
#line 886 "NCDConfigParser_parse.y"
{
    yygotominor.yy19 = yymsp[-1].minor.yy19;
  yy_destructor(yypParser,7,&yymsp[-2].minor);
  yy_destructor(yypParser,8,&yymsp[0].minor);
}
#line 1890 "NCDConfigParser_parse.c"
        break;
      case 45: /* name_maybe ::= */
#line 894 "NCDConfigParser_parse.y"
{
    yygotominor.yy49 = NULL;
}
#line 1897 "NCDConfigParser_parse.c"
        break;
      case 47: /* process_or_template ::= PROCESS */
#line 904 "NCDConfigParser_parse.y"
{
    yygotominor.yy28 = 0;
  yy_destructor(yypParser,26,&yymsp[0].minor);
}
#line 1905 "NCDConfigParser_parse.c"
        break;
      case 48: /* process_or_template ::= TEMPLATE */
#line 908 "NCDConfigParser_parse.y"
{
    yygotominor.yy28 = 1;
  yy_destructor(yypParser,27,&yymsp[0].minor);
}
#line 1913 "NCDConfigParser_parse.c"
        break;
      default:
        break;
  };
  yygoto = yyRuleInfo[yyruleno].lhs;
  yysize = yyRuleInfo[yyruleno].nrhs;
  yypParser->yyidx -= yysize;
  yyact = yy_find_reduce_action(yymsp[-yysize].stateno,(YYCODETYPE)yygoto);
  if( yyact < YYNSTATE ){
#ifdef NDEBUG
    /* If we are not debugging and the reduce action popped at least
    ** one element off the stack, then we can push the new element back
    ** onto the stack here, and skip the stack overflow test in yy_shift().
    ** That gives a significant speed improvement. */
    if( yysize ){
      yypParser->yyidx++;
      yymsp -= yysize-1;
      yymsp->stateno = (YYACTIONTYPE)yyact;
      yymsp->major = (YYCODETYPE)yygoto;
      yymsp->minor = yygotominor;
    }else
#endif
    {
      yy_shift(yypParser,yyact,yygoto,&yygotominor);
    }
  }else{
    assert( yyact == YYNSTATE + YYNRULE + 1 );
    yy_accept(yypParser);
  }
}

/*
** The following code executes when the parse fails
*/
#ifndef YYNOERRORRECOVERY
static void yy_parse_failed(
  yyParser *yypParser           /* The parser */
){
  ParseARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sFail!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser fails */
  ParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}
#endif /* YYNOERRORRECOVERY */

/*
** The following code executes when a syntax error first occurs.
*/
static void yy_syntax_error(
  yyParser *yypParser,           /* The parser */
  int yymajor,                   /* The major type of the error token */
  YYMINORTYPE yyminor            /* The minor type of the error token */
){
  ParseARG_FETCH;
#define TOKEN (yyminor.yy0)
#line 131 "NCDConfigParser_parse.y"

    parser_out->syntax_error = 1;
#line 1978 "NCDConfigParser_parse.c"
  ParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/*
** The following is executed when the parser accepts
*/
static void yy_accept(
  yyParser *yypParser           /* The parser */
){
  ParseARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sAccept!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser accepts */
  ParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/* The main parser program.
** The first argument is a pointer to a structure obtained from
** "ParseAlloc" which describes the current state of the parser.
** The second argument is the major token number.  The third is
** the minor token.  The fourth optional argument is whatever the
** user wants (and specified in the grammar) and is available for
** use by the action routines.
**
** Inputs:
** <ul>
** <li> A pointer to the parser (an opaque structure.)
** <li> The major token number.
** <li> The minor token number.
** <li> An option argument of a grammar-specified type.
** </ul>
**
** Outputs:
** None.
*/
void Parse(
  void *yyp,                   /* The parser */
  int yymajor,                 /* The major token code number */
  ParseTOKENTYPE yyminor       /* The value for the token */
  ParseARG_PDECL               /* Optional %extra_argument parameter */
){
  YYMINORTYPE yyminorunion;
  int yyact;            /* The parser action. */
  int yyendofinput;     /* True if we are at the end of input */
#ifdef YYERRORSYMBOL
  int yyerrorhit = 0;   /* True if yymajor has invoked an error */
#endif
  yyParser *yypParser;  /* The parser */

  /* (re)initialize the parser, if necessary */
  yypParser = (yyParser*)yyp;
  if( yypParser->yyidx<0 ){
#if YYSTACKDEPTH<=0
    if( yypParser->yystksz <=0 ){
      /*memset(&yyminorunion, 0, sizeof(yyminorunion));*/
      yyminorunion = yyzerominor;
      yyStackOverflow(yypParser, &yyminorunion);
      return;
    }
#endif
    yypParser->yyidx = 0;
    yypParser->yyerrcnt = -1;
    yypParser->yystack[0].stateno = 0;
    yypParser->yystack[0].major = 0;
  }
  yyminorunion.yy0 = yyminor;
  yyendofinput = (yymajor==0);
  ParseARG_STORE;

#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sInput %s\n",yyTracePrompt,yyTokenName[yymajor]);
  }
#endif

  do{
    yyact = yy_find_shift_action(yypParser,(YYCODETYPE)yymajor);
    if( yyact<YYNSTATE ){
      assert( !yyendofinput );  /* Impossible to shift the $ token */
      yy_shift(yypParser,yyact,yymajor,&yyminorunion);
      yypParser->yyerrcnt--;
      yymajor = YYNOCODE;
    }else if( yyact < YYNSTATE + YYNRULE ){
      yy_reduce(yypParser,yyact-YYNSTATE);
    }else{
      assert( yyact == YY_ERROR_ACTION );
#ifdef YYERRORSYMBOL
      int yymx;
#endif
#ifndef NDEBUG
      if( yyTraceFILE ){
        fprintf(yyTraceFILE,"%sSyntax Error!\n",yyTracePrompt);
      }
#endif
#ifdef YYERRORSYMBOL
      /* A syntax error has occurred.
      ** The response to an error depends upon whether or not the
      ** grammar defines an error token "ERROR".  
      **
      ** This is what we do if the grammar does define ERROR:
      **
      **  * Call the %syntax_error function.
      **
      **  * Begin popping the stack until we enter a state where
      **    it is legal to shift the error symbol, then shift
      **    the error symbol.
      **
      **  * Set the error count to three.
      **
      **  * Begin accepting and shifting new tokens.  No new error
      **    processing will occur until three tokens have been
      **    shifted successfully.
      **
      */
      if( yypParser->yyerrcnt<0 ){
        yy_syntax_error(yypParser,yymajor,yyminorunion);
      }
      yymx = yypParser->yystack[yypParser->yyidx].major;
      if( yymx==YYERRORSYMBOL || yyerrorhit ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE,"%sDiscard input token %s\n",
             yyTracePrompt,yyTokenName[yymajor]);
        }
#endif
        yy_destructor(yypParser, (YYCODETYPE)yymajor,&yyminorunion);
        yymajor = YYNOCODE;
      }else{
         while(
          yypParser->yyidx >= 0 &&
          yymx != YYERRORSYMBOL &&
          (yyact = yy_find_reduce_action(
                        yypParser->yystack[yypParser->yyidx].stateno,
                        YYERRORSYMBOL)) >= YYNSTATE
        ){
          yy_pop_parser_stack(yypParser);
        }
        if( yypParser->yyidx < 0 || yymajor==0 ){
          yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
          yy_parse_failed(yypParser);
          yymajor = YYNOCODE;
        }else if( yymx!=YYERRORSYMBOL ){
          YYMINORTYPE u2;
          u2.YYERRSYMDT = 0;
          yy_shift(yypParser,yyact,YYERRORSYMBOL,&u2);
        }
      }
      yypParser->yyerrcnt = 3;
      yyerrorhit = 1;
#elif defined(YYNOERRORRECOVERY)
      /* If the YYNOERRORRECOVERY macro is defined, then do not attempt to
      ** do any kind of error recovery.  Instead, simply invoke the syntax
      ** error routine and continue going as if nothing had happened.
      **
      ** Applications can set this macro (for example inside %include) if
      ** they intend to abandon the parse upon the first syntax error seen.
      */
      yy_syntax_error(yypParser,yymajor,yyminorunion);
      yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
      yymajor = YYNOCODE;
      
#else  /* YYERRORSYMBOL is not defined */
      /* This is what we do if the grammar does not define ERROR:
      **
      **  * Report an error message, and throw away the input token.
      **
      **  * If the input token is $, then fail the parse.
      **
      ** As before, subsequent error messages are suppressed until
      ** three input tokens have been successfully shifted.
      */
      if( yypParser->yyerrcnt<=0 ){
        yy_syntax_error(yypParser,yymajor,yyminorunion);
      }
      yypParser->yyerrcnt = 3;
      yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
      if( yyendofinput ){
        yy_parse_failed(yypParser);
      }
      yymajor = YYNOCODE;
#endif
    }
  }while( yymajor!=YYNOCODE && yypParser->yyidx>=0 );
  return;
}
