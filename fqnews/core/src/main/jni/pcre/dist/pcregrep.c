/*************************************************
*               pcregrep program                 *
*************************************************/

/* This is a grep program that uses the PCRE regular expression library to do
its pattern matching. On Unix-like, Windows, and native z/OS systems it can
recurse into directories, and in z/OS it can handle PDS files.

Note that for native z/OS, in addition to defining the NATIVE_ZOS macro, an
additional header is required. That header is not included in the main PCRE
distribution because other apparatus is needed to compile pcregrep for z/OS.
The header can be found in the special z/OS distribution, which is available
from www.zaconsultants.net or from www.cbttape.org.

           Copyright (c) 1997-2014 University of Cambridge

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

#include <ctype.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef SUPPORT_LIBZ
#include <zlib.h>
#endif

#ifdef SUPPORT_LIBBZ2
#include <bzlib.h>
#endif

#include "pcre.h"

#define FALSE 0
#define TRUE 1

typedef int BOOL;

#define OFFSET_SIZE 99

#if BUFSIZ > 8192
#define MAXPATLEN BUFSIZ
#else
#define MAXPATLEN 8192
#endif

#define PATBUFSIZE (MAXPATLEN + 10)   /* Allows for prefix+suffix */

/* Values for the "filenames" variable, which specifies options for file name
output. The order is important; it is assumed that a file name is wanted for
all values greater than FN_DEFAULT. */

enum { FN_NONE, FN_DEFAULT, FN_MATCH_ONLY, FN_NOMATCH_ONLY, FN_FORCE };

/* File reading styles */

enum { FR_PLAIN, FR_LIBZ, FR_LIBBZ2 };

/* Actions for the -d and -D options */

enum { dee_READ, dee_SKIP, dee_RECURSE };
enum { DEE_READ, DEE_SKIP };

/* Actions for special processing options (flag bits) */

#define PO_WORD_MATCH     0x0001
#define PO_LINE_MATCH     0x0002
#define PO_FIXED_STRINGS  0x0004

/* Line ending types */

enum { EL_LF, EL_CR, EL_CRLF, EL_ANY, EL_ANYCRLF };

/* Binary file options */

enum { BIN_BINARY, BIN_NOMATCH, BIN_TEXT };

/* In newer versions of gcc, with FORTIFY_SOURCE set (the default in some
environments), a warning is issued if the value of fwrite() is ignored.
Unfortunately, casting to (void) does not suppress the warning. To get round
this, we use a macro that compiles a fudge. Oddly, this does not also seem to
apply to fprintf(). */

#define FWRITE(a,b,c,d) if (fwrite(a,b,c,d)) {}



/*************************************************
*               Global variables                 *
*************************************************/

/* Jeffrey Friedl has some debugging requirements that are not part of the
regular code. */

#ifdef JFRIEDL_DEBUG
static int S_arg = -1;
static unsigned int jfriedl_XR = 0; /* repeat regex attempt this many times */
static unsigned int jfriedl_XT = 0; /* replicate text this many times */
static const char *jfriedl_prefix = "";
static const char *jfriedl_postfix = "";
#endif

static int  endlinetype;

static char *colour_string = (char *)"1;31";
static char *colour_option = NULL;
static char *dee_option = NULL;
static char *DEE_option = NULL;
static char *locale = NULL;
static char *main_buffer = NULL;
static char *newline = NULL;
static char *om_separator = (char *)"";
static char *stdin_name = (char *)"(standard input)";

static const unsigned char *pcretables = NULL;

static int after_context = 0;
static int before_context = 0;
static int binary_files = BIN_BINARY;
static int both_context = 0;
static int bufthird = PCREGREP_BUFSIZE;
static int bufsize = 3*PCREGREP_BUFSIZE;

#if defined HAVE_WINDOWS_H && HAVE_WINDOWS_H
static int dee_action = dee_SKIP;
#else
static int dee_action = dee_READ;
#endif

static int DEE_action = DEE_READ;
static int error_count = 0;
static int filenames = FN_DEFAULT;
static int pcre_options = 0;
static int process_options = 0;

#ifdef SUPPORT_PCREGREP_JIT
static int study_options = PCRE_STUDY_JIT_COMPILE;
#else
static int study_options = 0;
#endif

static unsigned long int match_limit = 0;
static unsigned long int match_limit_recursion = 0;

static BOOL count_only = FALSE;
static BOOL do_colour = FALSE;
static BOOL file_offsets = FALSE;
static BOOL hyphenpending = FALSE;
static BOOL invert = FALSE;
static BOOL line_buffered = FALSE;
static BOOL line_offsets = FALSE;
static BOOL multiline = FALSE;
static BOOL number = FALSE;
static BOOL omit_zero_count = FALSE;
static BOOL resource_error = FALSE;
static BOOL quiet = FALSE;
static BOOL show_only_matching = FALSE;
static BOOL silent = FALSE;
static BOOL utf8 = FALSE;

/* Structure for list of --only-matching capturing numbers. */

typedef struct omstr {
  struct omstr *next;
  int groupnum;
} omstr;

static omstr *only_matching = NULL;
static omstr *only_matching_last = NULL;

/* Structure for holding the two variables that describe a number chain. */

typedef struct omdatastr {
  omstr **anchor;
  omstr **lastptr;
} omdatastr;

static omdatastr only_matching_data = { &only_matching, &only_matching_last };

/* Structure for list of file names (for -f and --{in,ex}clude-from) */

typedef struct fnstr {
  struct fnstr *next;
  char *name;
} fnstr;

static fnstr *exclude_from = NULL;
static fnstr *exclude_from_last = NULL;
static fnstr *include_from = NULL;
static fnstr *include_from_last = NULL;

static fnstr *file_lists = NULL;
static fnstr *file_lists_last = NULL;
static fnstr *pattern_files = NULL;
static fnstr *pattern_files_last = NULL;

/* Structure for holding the two variables that describe a file name chain. */

typedef struct fndatastr {
  fnstr **anchor;
  fnstr **lastptr;
} fndatastr;

static fndatastr exclude_from_data = { &exclude_from, &exclude_from_last };
static fndatastr include_from_data = { &include_from, &include_from_last };
static fndatastr file_lists_data = { &file_lists, &file_lists_last };
static fndatastr pattern_files_data = { &pattern_files, &pattern_files_last };

/* Structure for pattern and its compiled form; used for matching patterns and
also for include/exclude patterns. */

typedef struct patstr {
  struct patstr *next;
  char *string;
  pcre *compiled;
  pcre_extra *hint;
} patstr;

static patstr *patterns = NULL;
static patstr *patterns_last = NULL;
static patstr *include_patterns = NULL;
static patstr *include_patterns_last = NULL;
static patstr *exclude_patterns = NULL;
static patstr *exclude_patterns_last = NULL;
static patstr *include_dir_patterns = NULL;
static patstr *include_dir_patterns_last = NULL;
static patstr *exclude_dir_patterns = NULL;
static patstr *exclude_dir_patterns_last = NULL;

/* Structure holding the two variables that describe a pattern chain. A pointer
to such structures is used for each appropriate option. */

typedef struct patdatastr {
  patstr **anchor;
  patstr **lastptr;
} patdatastr;

static patdatastr match_patdata = { &patterns, &patterns_last };
static patdatastr include_patdata = { &include_patterns, &include_patterns_last };
static patdatastr exclude_patdata = { &exclude_patterns, &exclude_patterns_last };
static patdatastr include_dir_patdata = { &include_dir_patterns, &include_dir_patterns_last };
static patdatastr exclude_dir_patdata = { &exclude_dir_patterns, &exclude_dir_patterns_last };

static patstr **incexlist[4] = { &include_patterns, &exclude_patterns,
                                 &include_dir_patterns, &exclude_dir_patterns };

static const char *incexname[4] = { "--include", "--exclude",
                                    "--include-dir", "--exclude-dir" };

/* Structure for options and list of them */

enum { OP_NODATA, OP_STRING, OP_OP_STRING, OP_NUMBER, OP_LONGNUMBER,
       OP_OP_NUMBER, OP_OP_NUMBERS, OP_PATLIST, OP_FILELIST, OP_BINFILES };

typedef struct option_item {
  int type;
  int one_char;
  void *dataptr;
  const char *long_name;
  const char *help_text;
} option_item;

/* Options without a single-letter equivalent get a negative value. This can be
used to identify them. */

#define N_COLOUR       (-1)
#define N_EXCLUDE      (-2)
#define N_EXCLUDE_DIR  (-3)
#define N_HELP         (-4)
#define N_INCLUDE      (-5)
#define N_INCLUDE_DIR  (-6)
#define N_LABEL        (-7)
#define N_LOCALE       (-8)
#define N_NULL         (-9)
#define N_LOFFSETS     (-10)
#define N_FOFFSETS     (-11)
#define N_LBUFFER      (-12)
#define N_M_LIMIT      (-13)
#define N_M_LIMIT_REC  (-14)
#define N_BUFSIZE      (-15)
#define N_NOJIT        (-16)
#define N_FILE_LIST    (-17)
#define N_BINARY_FILES (-18)
#define N_EXCLUDE_FROM (-19)
#define N_INCLUDE_FROM (-20)
#define N_OM_SEPARATOR (-21)

static option_item optionlist[] = {
  { OP_NODATA,     N_NULL,   NULL,              "",              "terminate options" },
  { OP_NODATA,     N_HELP,   NULL,              "help",          "display this help and exit" },
  { OP_NUMBER,     'A',      &after_context,    "after-context=number", "set number of following context lines" },
  { OP_NODATA,     'a',      NULL,              "text",          "treat binary files as text" },
  { OP_NUMBER,     'B',      &before_context,   "before-context=number", "set number of prior context lines" },
  { OP_BINFILES,   N_BINARY_FILES, NULL,        "binary-files=word", "set treatment of binary files" },
  { OP_NUMBER,     N_BUFSIZE,&bufthird,         "buffer-size=number", "set processing buffer size parameter" },
  { OP_OP_STRING,  N_COLOUR, &colour_option,    "color=option",  "matched text color option" },
  { OP_OP_STRING,  N_COLOUR, &colour_option,    "colour=option", "matched text colour option" },
  { OP_NUMBER,     'C',      &both_context,     "context=number", "set number of context lines, before & after" },
  { OP_NODATA,     'c',      NULL,              "count",         "print only a count of matching lines per FILE" },
  { OP_STRING,     'D',      &DEE_option,       "devices=action","how to handle devices, FIFOs, and sockets" },
  { OP_STRING,     'd',      &dee_option,       "directories=action", "how to handle directories" },
  { OP_PATLIST,    'e',      &match_patdata,    "regex(p)=pattern", "specify pattern (may be used more than once)" },
  { OP_NODATA,     'F',      NULL,              "fixed-strings", "patterns are sets of newline-separated strings" },
  { OP_FILELIST,   'f',      &pattern_files_data, "file=path",   "read patterns from file" },
  { OP_FILELIST,   N_FILE_LIST, &file_lists_data, "file-list=path","read files to search from file" },
  { OP_NODATA,     N_FOFFSETS, NULL,            "file-offsets",  "output file offsets, not text" },
  { OP_NODATA,     'H',      NULL,              "with-filename", "force the prefixing filename on output" },
  { OP_NODATA,     'h',      NULL,              "no-filename",   "suppress the prefixing filename on output" },
  { OP_NODATA,     'I',      NULL,              "",              "treat binary files as not matching (ignore)" },
  { OP_NODATA,     'i',      NULL,              "ignore-case",   "ignore case distinctions" },
#ifdef SUPPORT_PCREGREP_JIT
  { OP_NODATA,     N_NOJIT,  NULL,              "no-jit",        "do not use just-in-time compiler optimization" },
#else
  { OP_NODATA,     N_NOJIT,  NULL,              "no-jit",        "ignored: this pcregrep does not support JIT" },
#endif
  { OP_NODATA,     'l',      NULL,              "files-with-matches", "print only FILE names containing matches" },
  { OP_NODATA,     'L',      NULL,              "files-without-match","print only FILE names not containing matches" },
  { OP_STRING,     N_LABEL,  &stdin_name,       "label=name",    "set name for standard input" },
  { OP_NODATA,     N_LBUFFER, NULL,             "line-buffered", "use line buffering" },
  { OP_NODATA,     N_LOFFSETS, NULL,            "line-offsets",  "output line numbers and offsets, not text" },
  { OP_STRING,     N_LOCALE, &locale,           "locale=locale", "use the named locale" },
  { OP_LONGNUMBER, N_M_LIMIT, &match_limit,     "match-limit=number", "set PCRE match limit option" },
  { OP_LONGNUMBER, N_M_LIMIT_REC, &match_limit_recursion, "recursion-limit=number", "set PCRE match recursion limit option" },
  { OP_NODATA,     'M',      NULL,              "multiline",     "run in multiline mode" },
  { OP_STRING,     'N',      &newline,          "newline=type",  "set newline type (CR, LF, CRLF, ANYCRLF or ANY)" },
  { OP_NODATA,     'n',      NULL,              "line-number",   "print line number with output lines" },
  { OP_OP_NUMBERS, 'o',      &only_matching_data, "only-matching=n", "show only the part of the line that matched" },
  { OP_STRING,     N_OM_SEPARATOR, &om_separator, "om-separator=text", "set separator for multiple -o output" },
  { OP_NODATA,     'q',      NULL,              "quiet",         "suppress output, just set return code" },
  { OP_NODATA,     'r',      NULL,              "recursive",     "recursively scan sub-directories" },
  { OP_PATLIST,    N_EXCLUDE,&exclude_patdata,  "exclude=pattern","exclude matching files when recursing" },
  { OP_PATLIST,    N_INCLUDE,&include_patdata,  "include=pattern","include matching files when recursing" },
  { OP_PATLIST,    N_EXCLUDE_DIR,&exclude_dir_patdata, "exclude-dir=pattern","exclude matching directories when recursing" },
  { OP_PATLIST,    N_INCLUDE_DIR,&include_dir_patdata, "include-dir=pattern","include matching directories when recursing" },
  { OP_FILELIST,   N_EXCLUDE_FROM,&exclude_from_data, "exclude-from=path", "read exclude list from file" },
  { OP_FILELIST,   N_INCLUDE_FROM,&include_from_data, "include-from=path", "read include list from file" },

  /* These two were accidentally implemented with underscores instead of
  hyphens in the option names. As this was not discovered for several releases,
  the incorrect versions are left in the table for compatibility. However, the
  --help function misses out any option that has an underscore in its name. */

  { OP_PATLIST,   N_EXCLUDE_DIR,&exclude_dir_patdata, "exclude_dir=pattern","exclude matching directories when recursing" },
  { OP_PATLIST,   N_INCLUDE_DIR,&include_dir_patdata, "include_dir=pattern","include matching directories when recursing" },

#ifdef JFRIEDL_DEBUG
  { OP_OP_NUMBER, 'S',      &S_arg,            "jeffS",         "replace matched (sub)string with X" },
#endif
  { OP_NODATA,    's',      NULL,              "no-messages",   "suppress error messages" },
  { OP_NODATA,    'u',      NULL,              "utf-8",         "use UTF-8 mode" },
  { OP_NODATA,    'V',      NULL,              "version",       "print version information and exit" },
  { OP_NODATA,    'v',      NULL,              "invert-match",  "select non-matching lines" },
  { OP_NODATA,    'w',      NULL,              "word-regex(p)", "force patterns to match only as words"  },
  { OP_NODATA,    'x',      NULL,              "line-regex(p)", "force patterns to match only whole lines" },
  { OP_NODATA,    0,        NULL,               NULL,            NULL }
};

/* Tables for prefixing and suffixing patterns, according to the -w, -x, and -F
options. These set the 1, 2, and 4 bits in process_options, respectively. Note
that the combination of -w and -x has the same effect as -x on its own, so we
can treat them as the same. Note that the MAXPATLEN macro assumes the longest
prefix+suffix is 10 characters; if anything longer is added, it must be
adjusted. */

static const char *prefix[] = {
  "", "\\b", "^(?:", "^(?:", "\\Q", "\\b\\Q", "^(?:\\Q", "^(?:\\Q" };

static const char *suffix[] = {
  "", "\\b", ")$",   ")$",   "\\E", "\\E\\b", "\\E)$",   "\\E)$" };

/* UTF-8 tables - used only when the newline setting is "any". */

const int utf8_table3[] = { 0xff, 0x1f, 0x0f, 0x07, 0x03, 0x01};

const char utf8_table4[] = {
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5 };



/*************************************************
*         Exit from the program                  *
*************************************************/

/* If there has been a resource error, give a suitable message.

Argument:  the return code
Returns:   does not return
*/

static void
pcregrep_exit(int rc)
{
if (resource_error)
  {
  fprintf(stderr, "pcregrep: Error %d, %d or %d means that a resource limit "
    "was exceeded.\n", PCRE_ERROR_MATCHLIMIT, PCRE_ERROR_RECURSIONLIMIT,
    PCRE_ERROR_JIT_STACKLIMIT);
  fprintf(stderr, "pcregrep: Check your regex for nested unlimited loops.\n");
  }
exit(rc);
}


/*************************************************
*          Add item to chain of patterns         *
*************************************************/

/* Used to add an item onto a chain, or just return an unconnected item if the
"after" argument is NULL.

Arguments:
  s          pattern string to add
  after      if not NULL points to item to insert after

Returns:     new pattern block or NULL on error
*/

static patstr *
add_pattern(char *s, patstr *after)
{
patstr *p = (patstr *)malloc(sizeof(patstr));
if (p == NULL)
  {
  fprintf(stderr, "pcregrep: malloc failed\n");
  pcregrep_exit(2);
  }
if (strlen(s) > MAXPATLEN)
  {
  fprintf(stderr, "pcregrep: pattern is too long (limit is %d bytes)\n",
    MAXPATLEN);
  free(p);
  return NULL;
  }
p->next = NULL;
p->string = s;
p->compiled = NULL;
p->hint = NULL;

if (after != NULL)
  {
  p->next = after->next;
  after->next = p;
  }
return p;
}


/*************************************************
*           Free chain of patterns               *
*************************************************/

/* Used for several chains of patterns.

Argument: pointer to start of chain
Returns:  nothing
*/

static void
free_pattern_chain(patstr *pc)
{
while (pc != NULL)
  {
  patstr *p = pc;
  pc = p->next;
  if (p->hint != NULL) pcre_free_study(p->hint);
  if (p->compiled != NULL) pcre_free(p->compiled);
  free(p);
  }
}


/*************************************************
*           Free chain of file names             *
*************************************************/

/*
Argument: pointer to start of chain
Returns:  nothing
*/

static void
free_file_chain(fnstr *fn)
{
while (fn != NULL)
  {
  fnstr *f = fn;
  fn = f->next;
  free(f);
  }
}


/*************************************************
*            OS-specific functions               *
*************************************************/

/* These functions are defined so that they can be made system specific.
At present there are versions for Unix-style environments, Windows, native
z/OS, and "no support". */


/************* Directory scanning Unix-style and z/OS ***********/

#if (defined HAVE_SYS_STAT_H && defined HAVE_DIRENT_H && defined HAVE_SYS_TYPES_H) || defined NATIVE_ZOS
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#if defined NATIVE_ZOS
/************* Directory and PDS/E scanning for z/OS ***********/
/************* z/OS looks mostly like Unix with USS ************/
/* However, z/OS needs the #include statements in this header */
#include "pcrzosfs.h"
/* That header is not included in the main PCRE distribution because
   other apparatus is needed to compile pcregrep for z/OS. The header
   can be found in the special z/OS distribution, which is available
   from www.zaconsultants.net or from www.cbttape.org. */
#endif

typedef DIR directory_type;
#define FILESEP '/'

static int
isdirectory(char *filename)
{
struct stat statbuf;
if (stat(filename, &statbuf) < 0)
  return 0;        /* In the expectation that opening as a file will fail */
return (statbuf.st_mode & S_IFMT) == S_IFDIR;
}

static directory_type *
opendirectory(char *filename)
{
return opendir(filename);
}

static char *
readdirectory(directory_type *dir)
{
for (;;)
  {
  struct dirent *dent = readdir(dir);
  if (dent == NULL) return NULL;
  if (strcmp(dent->d_name, ".") != 0 && strcmp(dent->d_name, "..") != 0)
    return dent->d_name;
  }
/* Control never reaches here */
}

static void
closedirectory(directory_type *dir)
{
closedir(dir);
}


/************* Test for regular file, Unix-style **********/

static int
isregfile(char *filename)
{
struct stat statbuf;
if (stat(filename, &statbuf) < 0)
  return 1;        /* In the expectation that opening as a file will fail */
return (statbuf.st_mode & S_IFMT) == S_IFREG;
}


#if defined NATIVE_ZOS
/************* Test for a terminal in z/OS **********/
/* isatty() does not work in a TSO environment, so always give FALSE.*/

static BOOL
is_stdout_tty(void)
{
return FALSE;
}

static BOOL
is_file_tty(FILE *f)
{
return FALSE;
}


/************* Test for a terminal, Unix-style **********/

#else
static BOOL
is_stdout_tty(void)
{
return isatty(fileno(stdout));
}

static BOOL
is_file_tty(FILE *f)
{
return isatty(fileno(f));
}
#endif

/* End of Unix-style or native z/OS environment functions. */


/************* Directory scanning in Windows ***********/

/* I (Philip Hazel) have no means of testing this code. It was contributed by
Lionel Fourquaux. David Burgess added a patch to define INVALID_FILE_ATTRIBUTES
when it did not exist. David Byron added a patch that moved the #include of
<windows.h> to before the INVALID_FILE_ATTRIBUTES definition rather than after.
The double test below stops gcc 4.4.4 grumbling that HAVE_WINDOWS_H is
undefined when it is indeed undefined. */

#elif defined HAVE_WINDOWS_H && HAVE_WINDOWS_H

#ifndef STRICT
# define STRICT
#endif
#ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES 0xFFFFFFFF
#endif

typedef struct directory_type
{
HANDLE handle;
BOOL first;
WIN32_FIND_DATA data;
} directory_type;

#define FILESEP '/'

int
isdirectory(char *filename)
{
DWORD attr = GetFileAttributes(filename);
if (attr == INVALID_FILE_ATTRIBUTES)
  return 0;
return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

directory_type *
opendirectory(char *filename)
{
size_t len;
char *pattern;
directory_type *dir;
DWORD err;
len = strlen(filename);
pattern = (char *)malloc(len + 3);
dir = (directory_type *)malloc(sizeof(*dir));
if ((pattern == NULL) || (dir == NULL))
  {
  fprintf(stderr, "pcregrep: malloc failed\n");
  pcregrep_exit(2);
  }
memcpy(pattern, filename, len);
memcpy(&(pattern[len]), "\\*", 3);
dir->handle = FindFirstFile(pattern, &(dir->data));
if (dir->handle != INVALID_HANDLE_VALUE)
  {
  free(pattern);
  dir->first = TRUE;
  return dir;
  }
err = GetLastError();
free(pattern);
free(dir);
errno = (err == ERROR_ACCESS_DENIED) ? EACCES : ENOENT;
return NULL;
}

char *
readdirectory(directory_type *dir)
{
for (;;)
  {
  if (!dir->first)
    {
    if (!FindNextFile(dir->handle, &(dir->data)))
      return NULL;
    }
  else
    {
    dir->first = FALSE;
    }
  if (strcmp(dir->data.cFileName, ".") != 0 && strcmp(dir->data.cFileName, "..") != 0)
    return dir->data.cFileName;
  }
#ifndef _MSC_VER
return NULL;   /* Keep compiler happy; never executed */
#endif
}

void
closedirectory(directory_type *dir)
{
FindClose(dir->handle);
free(dir);
}


/************* Test for regular file in Windows **********/

/* I don't know how to do this, or if it can be done; assume all paths are
regular if they are not directories. */

int isregfile(char *filename)
{
return !isdirectory(filename);
}


/************* Test for a terminal in Windows **********/

/* I don't know how to do this; assume never */

static BOOL
is_stdout_tty(void)
{
return FALSE;
}

static BOOL
is_file_tty(FILE *f)
{
return FALSE;
}

/* End of Windows functions */


/************* Directory scanning when we can't do it ***********/

/* The type is void, and apart from isdirectory(), the functions do nothing. */

#else

#define FILESEP 0
typedef void directory_type;

int isdirectory(char *filename) { return 0; }
directory_type * opendirectory(char *filename) { return (directory_type*)0;}
char *readdirectory(directory_type *dir) { return (char*)0;}
void closedirectory(directory_type *dir) {}


/************* Test for regular file when we can't do it **********/

/* Assume all files are regular. */

int isregfile(char *filename) { return 1; }


/************* Test for a terminal when we can't do it **********/

static BOOL
is_stdout_tty(void)
{
return FALSE;
}

static BOOL
is_file_tty(FILE *f)
{
return FALSE;
}

#endif  /* End of system-specific functions */



#ifndef HAVE_STRERROR
/*************************************************
*     Provide strerror() for non-ANSI libraries  *
*************************************************/

/* Some old-fashioned systems still around (e.g. SunOS4) don't have strerror()
in their libraries, but can provide the same facility by this simple
alternative function. */

extern int   sys_nerr;
extern char *sys_errlist[];

char *
strerror(int n)
{
if (n < 0 || n >= sys_nerr) return "unknown error number";
return sys_errlist[n];
}
#endif /* HAVE_STRERROR */



/*************************************************
*                Usage function                  *
*************************************************/

static int
usage(int rc)
{
option_item *op;
fprintf(stderr, "Usage: pcregrep [-");
for (op = optionlist; op->one_char != 0; op++)
  {
  if (op->one_char > 0) fprintf(stderr, "%c", op->one_char);
  }
fprintf(stderr, "] [long options] [pattern] [files]\n");
fprintf(stderr, "Type `pcregrep --help' for more information and the long "
  "options.\n");
return rc;
}



/*************************************************
*                Help function                   *
*************************************************/

static void
help(void)
{
option_item *op;

printf("Usage: pcregrep [OPTION]... [PATTERN] [FILE1 FILE2 ...]\n");
printf("Search for PATTERN in each FILE or standard input.\n");
printf("PATTERN must be present if neither -e nor -f is used.\n");
printf("\"-\" can be used as a file name to mean STDIN.\n");

#ifdef SUPPORT_LIBZ
printf("Files whose names end in .gz are read using zlib.\n");
#endif

#ifdef SUPPORT_LIBBZ2
printf("Files whose names end in .bz2 are read using bzlib2.\n");
#endif

#if defined SUPPORT_LIBZ || defined SUPPORT_LIBBZ2
printf("Other files and the standard input are read as plain files.\n\n");
#else
printf("All files are read as plain files, without any interpretation.\n\n");
#endif

printf("Example: pcregrep -i 'hello.*world' menu.h main.c\n\n");
printf("Options:\n");

for (op = optionlist; op->one_char != 0; op++)
  {
  int n;
  char s[4];

  /* Two options were accidentally implemented and documented with underscores
  instead of hyphens in their names, something that was not noticed for quite a
  few releases. When fixing this, I left the underscored versions in the list
  in case people were using them. However, we don't want to display them in the
  help data. There are no other options that contain underscores, and we do not
  expect ever to implement such options. Therefore, just omit any option that
  contains an underscore. */

  if (strchr(op->long_name, '_') != NULL) continue;

  if (op->one_char > 0 && (op->long_name)[0] == 0)
    n = 31 - printf("  -%c", op->one_char);
  else
    {
    if (op->one_char > 0) sprintf(s, "-%c,", op->one_char);
      else strcpy(s, "   ");
    n = 31 - printf("  %s --%s", s, op->long_name);
    }

  if (n < 1) n = 1;
  printf("%.*s%s\n", n, "                           ", op->help_text);
  }

printf("\nNumbers may be followed by K or M, e.g. --buffer-size=100K.\n");
printf("The default value for --buffer-size is %d.\n", PCREGREP_BUFSIZE);
printf("When reading patterns or file names from a file, trailing white\n");
printf("space is removed and blank lines are ignored.\n");
printf("The maximum size of any pattern is %d bytes.\n", MAXPATLEN);

printf("\nWith no FILEs, read standard input. If fewer than two FILEs given, assume -h.\n");
printf("Exit status is 0 if any matches, 1 if no matches, and 2 if trouble.\n");
}



/*************************************************
*            Test exclude/includes               *
*************************************************/

/* If any exclude pattern matches, the path is excluded. Otherwise, unless
there are no includes, the path must match an include pattern.

Arguments:
  path      the path to be matched
  ip        the chain of include patterns
  ep        the chain of exclude patterns

Returns:    TRUE if the path is not excluded
*/

static BOOL
test_incexc(char *path, patstr *ip, patstr *ep)
{
int plen = strlen(path);

for (; ep != NULL; ep = ep->next)
  {
  if (pcre_exec(ep->compiled, NULL, path, plen, 0, 0, NULL, 0) >= 0)
    return FALSE;
  }

if (ip == NULL) return TRUE;

for (; ip != NULL; ip = ip->next)
  {
  if (pcre_exec(ip->compiled, NULL, path, plen, 0, 0, NULL, 0) >= 0)
    return TRUE;
  }

return FALSE;
}



/*************************************************
*         Decode integer argument value          *
*************************************************/

/* Integer arguments can be followed by K or M. Avoid the use of strtoul()
because SunOS4 doesn't have it. This is used only for unpicking arguments, so
just keep it simple.

Arguments:
  option_data   the option data string
  op            the option item (for error messages)
  longop        TRUE if option given in long form

Returns:        a long integer
*/

static long int
decode_number(char *option_data, option_item *op, BOOL longop)
{
unsigned long int n = 0;
char *endptr = option_data;
while (*endptr != 0 && isspace((unsigned char)(*endptr))) endptr++;
while (isdigit((unsigned char)(*endptr)))
  n = n * 10 + (int)(*endptr++ - '0');
if (toupper(*endptr) == 'K')
  {
  n *= 1024;
  endptr++;
  }
else if (toupper(*endptr) == 'M')
  {
  n *= 1024*1024;
  endptr++;
  }

if (*endptr != 0)   /* Error */
  {
  if (longop)
    {
    char *equals = strchr(op->long_name, '=');
    int nlen = (equals == NULL)? (int)strlen(op->long_name) :
      (int)(equals - op->long_name);
    fprintf(stderr, "pcregrep: Malformed number \"%s\" after --%.*s\n",
      option_data, nlen, op->long_name);
    }
  else
    fprintf(stderr, "pcregrep: Malformed number \"%s\" after -%c\n",
      option_data, op->one_char);
  pcregrep_exit(usage(2));
  }

return n;
}



/*************************************************
*       Add item to a chain of numbers           *
*************************************************/

/* Used to add an item onto a chain, or just return an unconnected item if the
"after" argument is NULL.

Arguments:
  n          the number to add
  after      if not NULL points to item to insert after

Returns:     new number block
*/

static omstr *
add_number(int n, omstr *after)
{
omstr *om = (omstr *)malloc(sizeof(omstr));

if (om == NULL)
  {
  fprintf(stderr, "pcregrep: malloc failed\n");
  pcregrep_exit(2);
  }
om->next = NULL;
om->groupnum = n;

if (after != NULL)
  {
  om->next = after->next;
  after->next = om;
  }
return om;
}



/*************************************************
*            Read one line of input              *
*************************************************/

/* Normally, input is read using fread() into a large buffer, so many lines may
be read at once. However, doing this for tty input means that no output appears
until a lot of input has been typed. Instead, tty input is handled line by
line. We cannot use fgets() for this, because it does not stop at a binary
zero, and therefore there is no way of telling how many characters it has read,
because there may be binary zeros embedded in the data.

Arguments:
  buffer     the buffer to read into
  length     the maximum number of characters to read
  f          the file

Returns:     the number of characters read, zero at end of file
*/

static unsigned int
read_one_line(char *buffer, int length, FILE *f)
{
int c;
int yield = 0;
while ((c = fgetc(f)) != EOF)
  {
  buffer[yield++] = c;
  if (c == '\n' || yield >= length) break;
  }
return yield;
}



/*************************************************
*             Find end of line                   *
*************************************************/

/* The length of the endline sequence that is found is set via lenptr. This may
be zero at the very end of the file if there is no line-ending sequence there.

Arguments:
  p         current position in line
  endptr    end of available data
  lenptr    where to put the length of the eol sequence

Returns:    pointer after the last byte of the line,
            including the newline byte(s)
*/

static char *
end_of_line(char *p, char *endptr, int *lenptr)
{
switch(endlinetype)
  {
  default:      /* Just in case */
  case EL_LF:
  while (p < endptr && *p != '\n') p++;
  if (p < endptr)
    {
    *lenptr = 1;
    return p + 1;
    }
  *lenptr = 0;
  return endptr;

  case EL_CR:
  while (p < endptr && *p != '\r') p++;
  if (p < endptr)
    {
    *lenptr = 1;
    return p + 1;
    }
  *lenptr = 0;
  return endptr;

  case EL_CRLF:
  for (;;)
    {
    while (p < endptr && *p != '\r') p++;
    if (++p >= endptr)
      {
      *lenptr = 0;
      return endptr;
      }
    if (*p == '\n')
      {
      *lenptr = 2;
      return p + 1;
      }
    }
  break;

  case EL_ANYCRLF:
  while (p < endptr)
    {
    int extra = 0;
    register int c = *((unsigned char *)p);

    if (utf8 && c >= 0xc0)
      {
      int gcii, gcss;
      extra = utf8_table4[c & 0x3f];  /* Number of additional bytes */
      gcss = 6*extra;
      c = (c & utf8_table3[extra]) << gcss;
      for (gcii = 1; gcii <= extra; gcii++)
        {
        gcss -= 6;
        c |= (p[gcii] & 0x3f) << gcss;
        }
      }

    p += 1 + extra;

    switch (c)
      {
      case '\n':
      *lenptr = 1;
      return p;

      case '\r':
      if (p < endptr && *p == '\n')
        {
        *lenptr = 2;
        p++;
        }
      else *lenptr = 1;
      return p;

      default:
      break;
      }
    }   /* End of loop for ANYCRLF case */

  *lenptr = 0;  /* Must have hit the end */
  return endptr;

  case EL_ANY:
  while (p < endptr)
    {
    int extra = 0;
    register int c = *((unsigned char *)p);

    if (utf8 && c >= 0xc0)
      {
      int gcii, gcss;
      extra = utf8_table4[c & 0x3f];  /* Number of additional bytes */
      gcss = 6*extra;
      c = (c & utf8_table3[extra]) << gcss;
      for (gcii = 1; gcii <= extra; gcii++)
        {
        gcss -= 6;
        c |= (p[gcii] & 0x3f) << gcss;
        }
      }

    p += 1 + extra;

    switch (c)
      {
      case '\n':    /* LF */
      case '\v':    /* VT */
      case '\f':    /* FF */
      *lenptr = 1;
      return p;

      case '\r':    /* CR */
      if (p < endptr && *p == '\n')
        {
        *lenptr = 2;
        p++;
        }
      else *lenptr = 1;
      return p;

#ifndef EBCDIC
      case 0x85:    /* Unicode NEL */
      *lenptr = utf8? 2 : 1;
      return p;

      case 0x2028:  /* Unicode LS */
      case 0x2029:  /* Unicode PS */
      *lenptr = 3;
      return p;
#endif  /* Not EBCDIC */

      default:
      break;
      }
    }   /* End of loop for ANY case */

  *lenptr = 0;  /* Must have hit the end */
  return endptr;
  }     /* End of overall switch */
}



/*************************************************
*         Find start of previous line            *
*************************************************/

/* This is called when looking back for before lines to print.

Arguments:
  p         start of the subsequent line
  startptr  start of available data

Returns:    pointer to the start of the previous line
*/

static char *
previous_line(char *p, char *startptr)
{
switch(endlinetype)
  {
  default:      /* Just in case */
  case EL_LF:
  p--;
  while (p > startptr && p[-1] != '\n') p--;
  return p;

  case EL_CR:
  p--;
  while (p > startptr && p[-1] != '\n') p--;
  return p;

  case EL_CRLF:
  for (;;)
    {
    p -= 2;
    while (p > startptr && p[-1] != '\n') p--;
    if (p <= startptr + 1 || p[-2] == '\r') return p;
    }
  /* Control can never get here */

  case EL_ANY:
  case EL_ANYCRLF:
  if (*(--p) == '\n' && p > startptr && p[-1] == '\r') p--;
  if (utf8) while ((*p & 0xc0) == 0x80) p--;

  while (p > startptr)
    {
    register unsigned int c;
    char *pp = p - 1;

    if (utf8)
      {
      int extra = 0;
      while ((*pp & 0xc0) == 0x80) pp--;
      c = *((unsigned char *)pp);
      if (c >= 0xc0)
        {
        int gcii, gcss;
        extra = utf8_table4[c & 0x3f];  /* Number of additional bytes */
        gcss = 6*extra;
        c = (c & utf8_table3[extra]) << gcss;
        for (gcii = 1; gcii <= extra; gcii++)
          {
          gcss -= 6;
          c |= (pp[gcii] & 0x3f) << gcss;
          }
        }
      }
    else c = *((unsigned char *)pp);

    if (endlinetype == EL_ANYCRLF) switch (c)
      {
      case '\n':    /* LF */
      case '\r':    /* CR */
      return p;

      default:
      break;
      }

    else switch (c)
      {
      case '\n':    /* LF */
      case '\v':    /* VT */
      case '\f':    /* FF */
      case '\r':    /* CR */
#ifndef EBCDIE
      case 0x85:    /* Unicode NEL */
      case 0x2028:  /* Unicode LS */
      case 0x2029:  /* Unicode PS */
#endif  /* Not EBCDIC */
      return p;

      default:
      break;
      }

    p = pp;  /* Back one character */
    }        /* End of loop for ANY case */

  return startptr;  /* Hit start of data */
  }     /* End of overall switch */
}





/*************************************************
*       Print the previous "after" lines         *
*************************************************/

/* This is called if we are about to lose said lines because of buffer filling,
and at the end of the file. The data in the line is written using fwrite() so
that a binary zero does not terminate it.

Arguments:
  lastmatchnumber   the number of the last matching line, plus one
  lastmatchrestart  where we restarted after the last match
  endptr            end of available data
  printname         filename for printing

Returns:            nothing
*/

static void
do_after_lines(int lastmatchnumber, char *lastmatchrestart, char *endptr,
  char *printname)
{
if (after_context > 0 && lastmatchnumber > 0)
  {
  int count = 0;
  while (lastmatchrestart < endptr && count++ < after_context)
    {
    int ellength;
    char *pp = lastmatchrestart;
    if (printname != NULL) fprintf(stdout, "%s-", printname);
    if (number) fprintf(stdout, "%d-", lastmatchnumber++);
    pp = end_of_line(pp, endptr, &ellength);
    FWRITE(lastmatchrestart, 1, pp - lastmatchrestart, stdout);
    lastmatchrestart = pp;
    }
  hyphenpending = TRUE;
  }
}



/*************************************************
*   Apply patterns to subject till one matches   *
*************************************************/

/* This function is called to run through all patterns, looking for a match. It
is used multiple times for the same subject when colouring is enabled, in order
to find all possible matches.

Arguments:
  matchptr     the start of the subject
  length       the length of the subject to match
  options      options for pcre_exec
  startoffset  where to start matching
  offsets      the offets vector to fill in
  mrc          address of where to put the result of pcre_exec()

Returns:      TRUE if there was a match
              FALSE if there was no match
              invert if there was a non-fatal error
*/

static BOOL
match_patterns(char *matchptr, size_t length, unsigned int options,
  int startoffset, int *offsets, int *mrc)
{
int i;
size_t slen = length;
patstr *p = patterns;
const char *msg = "this text:\n\n";

if (slen > 200)
  {
  slen = 200;
  msg = "text that starts:\n\n";
  }
for (i = 1; p != NULL; p = p->next, i++)
  {
  *mrc = pcre_exec(p->compiled, p->hint, matchptr, (int)length,
    startoffset, options, offsets, OFFSET_SIZE);
  if (*mrc >= 0) return TRUE;
  if (*mrc == PCRE_ERROR_NOMATCH) continue;
  fprintf(stderr, "pcregrep: pcre_exec() gave error %d while matching ", *mrc);
  if (patterns->next != NULL) fprintf(stderr, "pattern number %d to ", i);
  fprintf(stderr, "%s", msg);
  FWRITE(matchptr, 1, slen, stderr);   /* In case binary zero included */
  fprintf(stderr, "\n\n");
  if (*mrc == PCRE_ERROR_MATCHLIMIT || *mrc == PCRE_ERROR_RECURSIONLIMIT ||
      *mrc == PCRE_ERROR_JIT_STACKLIMIT)
    resource_error = TRUE;
  if (error_count++ > 20)
    {
    fprintf(stderr, "pcregrep: Too many errors - abandoned.\n");
    pcregrep_exit(2);
    }
  return invert;    /* No more matching; don't show the line again */
  }

return FALSE;  /* No match, no errors */
}



/*************************************************
*            Grep an individual file             *
*************************************************/

/* This is called from grep_or_recurse() below. It uses a buffer that is three
times the value of bufthird. The matching point is never allowed to stray into
the top third of the buffer, thus keeping more of the file available for
context printing or for multiline scanning. For large files, the pointer will
be in the middle third most of the time, so the bottom third is available for
"before" context printing.

Arguments:
  handle       the fopened FILE stream for a normal file
               the gzFile pointer when reading is via libz
               the BZFILE pointer when reading is via libbz2
  frtype       FR_PLAIN, FR_LIBZ, or FR_LIBBZ2
  filename     the file name or NULL (for errors)
  printname    the file name if it is to be printed for each match
               or NULL if the file name is not to be printed
               it cannot be NULL if filenames[_nomatch]_only is set

Returns:       0 if there was at least one match
               1 otherwise (no matches)
               2 if an overlong line is encountered
               3 if there is a read error on a .bz2 file
*/

static int
pcregrep(void *handle, int frtype, char *filename, char *printname)
{
int rc = 1;
int linenumber = 1;
int lastmatchnumber = 0;
int count = 0;
int filepos = 0;
int offsets[OFFSET_SIZE];
char *lastmatchrestart = NULL;
char *ptr = main_buffer;
char *endptr;
size_t bufflength;
BOOL binary = FALSE;
BOOL endhyphenpending = FALSE;
BOOL input_line_buffered = line_buffered;
FILE *in = NULL;                    /* Ensure initialized */

#ifdef SUPPORT_LIBZ
gzFile ingz = NULL;
#endif

#ifdef SUPPORT_LIBBZ2
BZFILE *inbz2 = NULL;
#endif


/* Do the first read into the start of the buffer and set up the pointer to end
of what we have. In the case of libz, a non-zipped .gz file will be read as a
plain file. However, if a .bz2 file isn't actually bzipped, the first read will
fail. */

(void)frtype;

#ifdef SUPPORT_LIBZ
if (frtype == FR_LIBZ)
  {
  ingz = (gzFile)handle;
  bufflength = gzread (ingz, main_buffer, bufsize);
  }
else
#endif

#ifdef SUPPORT_LIBBZ2
if (frtype == FR_LIBBZ2)
  {
  inbz2 = (BZFILE *)handle;
  bufflength = BZ2_bzread(inbz2, main_buffer, bufsize);
  if ((int)bufflength < 0) return 2;   /* Gotcha: bufflength is size_t; */
  }                                    /* without the cast it is unsigned. */
else
#endif

  {
  in = (FILE *)handle;
  if (is_file_tty(in)) input_line_buffered = TRUE;
  bufflength = input_line_buffered?
    read_one_line(main_buffer, bufsize, in) :
    fread(main_buffer, 1, bufsize, in);
  }

endptr = main_buffer + bufflength;

/* Unless binary-files=text, see if we have a binary file. This uses the same
rule as GNU grep, namely, a search for a binary zero byte near the start of the
file. */

if (binary_files != BIN_TEXT)
  {
  binary =
    memchr(main_buffer, 0, (bufflength > 1024)? 1024 : bufflength) != NULL;
  if (binary && binary_files == BIN_NOMATCH) return 1;
  }

/* Loop while the current pointer is not at the end of the file. For large
files, endptr will be at the end of the buffer when we are in the middle of the
file, but ptr will never get there, because as soon as it gets over 2/3 of the
way, the buffer is shifted left and re-filled. */

while (ptr < endptr)
  {
  int endlinelength;
  int mrc = 0;
  int startoffset = 0;
  int prevoffsets[2];
  unsigned int options = 0;
  BOOL match;
  char *matchptr = ptr;
  char *t = ptr;
  size_t length, linelength;

  prevoffsets[0] = prevoffsets[1] = -1;

  /* At this point, ptr is at the start of a line. We need to find the length
  of the subject string to pass to pcre_exec(). In multiline mode, it is the
  length remainder of the data in the buffer. Otherwise, it is the length of
  the next line, excluding the terminating newline. After matching, we always
  advance by the length of the next line. In multiline mode the PCRE_FIRSTLINE
  option is used for compiling, so that any match is constrained to be in the
  first line. */

  t = end_of_line(t, endptr, &endlinelength);
  linelength = t - ptr - endlinelength;
  length = multiline? (size_t)(endptr - ptr) : linelength;

  /* Check to see if the line we are looking at extends right to the very end
  of the buffer without a line terminator. This means the line is too long to
  handle. */

  if (endlinelength == 0 && t == main_buffer + bufsize)
    {
    fprintf(stderr, "pcregrep: line %d%s%s is too long for the internal buffer\n"
                    "pcregrep: check the --buffer-size option\n",
                    linenumber,
                    (filename == NULL)? "" : " of file ",
                    (filename == NULL)? "" : filename);
    return 2;
    }

  /* Extra processing for Jeffrey Friedl's debugging. */

#ifdef JFRIEDL_DEBUG
  if (jfriedl_XT || jfriedl_XR)
  {
#     include <sys/time.h>
#     include <time.h>
      struct timeval start_time, end_time;
      struct timezone dummy;
      int i;

      if (jfriedl_XT)
      {
          unsigned long newlen = length * jfriedl_XT + strlen(jfriedl_prefix) + strlen(jfriedl_postfix);
          const char *orig = ptr;
          ptr = malloc(newlen + 1);
          if (!ptr) {
                  printf("out of memory");
                  pcregrep_exit(2);
          }
          endptr = ptr;
          strcpy(endptr, jfriedl_prefix); endptr += strlen(jfriedl_prefix);
          for (i = 0; i < jfriedl_XT; i++) {
                  strncpy(endptr, orig,  length);
                  endptr += length;
          }
          strcpy(endptr, jfriedl_postfix); endptr += strlen(jfriedl_postfix);
          length = newlen;
      }

      if (gettimeofday(&start_time, &dummy) != 0)
              perror("bad gettimeofday");


      for (i = 0; i < jfriedl_XR; i++)
          match = (pcre_exec(patterns->compiled, patterns->hint, ptr, length, 0,
              PCRE_NOTEMPTY, offsets, OFFSET_SIZE) >= 0);

      if (gettimeofday(&end_time, &dummy) != 0)
              perror("bad gettimeofday");

      double delta = ((end_time.tv_sec + (end_time.tv_usec / 1000000.0))
                      -
                      (start_time.tv_sec + (start_time.tv_usec / 1000000.0)));

      printf("%s TIMER[%.4f]\n", match ? "MATCH" : "FAIL", delta);
      return 0;
  }
#endif

  /* We come back here after a match when show_only_matching is set, in order
  to find any further matches in the same line. This applies to
  --only-matching, --file-offsets, and --line-offsets. */

  ONLY_MATCHING_RESTART:

  /* Run through all the patterns until one matches or there is an error other
  than NOMATCH. This code is in a subroutine so that it can be re-used for
  finding subsequent matches when colouring matched lines. After finding one
  match, set PCRE_NOTEMPTY to disable any further matches of null strings in
  this line. */

  match = match_patterns(matchptr, length, options, startoffset, offsets, &mrc);
  options = PCRE_NOTEMPTY;

  /* If it's a match or a not-match (as required), do what's wanted. */

  if (match != invert)
    {
    BOOL hyphenprinted = FALSE;

    /* We've failed if we want a file that doesn't have any matches. */

    if (filenames == FN_NOMATCH_ONLY) return 1;

    /* If all we want is a yes/no answer, stop now. */

    if (quiet) return 0;

    /* Just count if just counting is wanted. */

    else if (count_only) count++;

    /* When handling a binary file and binary-files==binary, the "binary"
    variable will be set true (it's false in all other cases). In this
    situation we just want to output the file name. No need to scan further. */

    else if (binary)
      {
      fprintf(stdout, "Binary file %s matches\n", filename);
      return 0;
      }

    /* If all we want is a file name, there is no need to scan any more lines
    in the file. */

    else if (filenames == FN_MATCH_ONLY)
      {
      fprintf(stdout, "%s\n", printname);
      return 0;
      }

    /* The --only-matching option prints just the substring that matched,
    and/or one or more captured portions of it, as long as these strings are
    not empty. The --file-offsets and --line-offsets options output offsets for
    the matching substring (all three set show_only_matching). None of these
    mutually exclusive options prints any context. Afterwards, adjust the start
    and then jump back to look for further matches in the same line. If we are
    in invert mode, however, nothing is printed and we do not restart - this
    could still be useful because the return code is set. */

    else if (show_only_matching)
      {
      if (!invert)
        {
        int oldstartoffset = startoffset;

        /* It is possible, when a lookbehind assertion contains \K, for the
        same string to be found again. The code below advances startoffset, but
        until it is past the "bumpalong" offset that gave the match, the same
        substring will be returned. The PCRE1 library does not return the
        bumpalong offset, so all we can do is ignore repeated strings. (PCRE2
        does this better.) */

        if (prevoffsets[0] != offsets[0] || prevoffsets[1] != offsets[1])
          {
          prevoffsets[0] = offsets[0];
          prevoffsets[1] = offsets[1];

          if (printname != NULL) fprintf(stdout, "%s:", printname);
          if (number) fprintf(stdout, "%d:", linenumber);

          /* Handle --line-offsets */

          if (line_offsets)
            fprintf(stdout, "%d,%d\n", (int)(matchptr + offsets[0] - ptr),
              offsets[1] - offsets[0]);

          /* Handle --file-offsets */

          else if (file_offsets)
            fprintf(stdout, "%d,%d\n",
              (int)(filepos + matchptr + offsets[0] - ptr),
              offsets[1] - offsets[0]);

          /* Handle --only-matching, which may occur many times */

          else
            {
            BOOL printed = FALSE;
            omstr *om;

            for (om = only_matching; om != NULL; om = om->next)
              {
              int n = om->groupnum;
              if (n < mrc)
                {
                int plen = offsets[2*n + 1] - offsets[2*n];
                if (plen > 0)
                  {
                  if (printed) fprintf(stdout, "%s", om_separator);
                  if (do_colour) fprintf(stdout, "%c[%sm", 0x1b, colour_string);
                  FWRITE(matchptr + offsets[n*2], 1, plen, stdout);
                  if (do_colour) fprintf(stdout, "%c[00m", 0x1b);
                  printed = TRUE;
                  }
                }
              }

            if (printed || printname != NULL || number) fprintf(stdout, "\n");
            }
          }

        /* Prepare to repeat to find the next match. If the patterned contained
        a lookbehind tht included \K, it is possible that the end of the match
        might be at or before the actual strting offset we have just used. We
        need to start one character further on. Unfortunately, for unanchored
        patterns, the actual start offset can be greater that the one that was
        set as a result of "bumpalong". PCRE1 does not return the actual start
        offset, so we have to check against the original start offset. This may
        lead to duplicates - we we need the fudge above to avoid printing them.
        (PCRE2 does this better.) */

        match = FALSE;
        if (line_buffered) fflush(stdout);
        rc = 0;                      /* Had some success */
        startoffset = offsets[1];    /* Restart after the match */
        if (startoffset <= oldstartoffset)
          {
          if ((size_t)startoffset >= length)
            goto END_ONE_MATCH;              /* We were at the end */
          startoffset = oldstartoffset + 1;
          if (utf8)
            while ((matchptr[startoffset] & 0xc0) == 0x80) startoffset++;
          }
        goto ONLY_MATCHING_RESTART;
        }
      }

    /* This is the default case when none of the above options is set. We print
    the matching lines(s), possibly preceded and/or followed by other lines of
    context. */

    else
      {
      /* See if there is a requirement to print some "after" lines from a
      previous match. We never print any overlaps. */

      if (after_context > 0 && lastmatchnumber > 0)
        {
        int ellength;
        int linecount = 0;
        char *p = lastmatchrestart;

        while (p < ptr && linecount < after_context)
          {
          p = end_of_line(p, ptr, &ellength);
          linecount++;
          }

        /* It is important to advance lastmatchrestart during this printing so
        that it interacts correctly with any "before" printing below. Print
        each line's data using fwrite() in case there are binary zeroes. */

        while (lastmatchrestart < p)
          {
          char *pp = lastmatchrestart;
          if (printname != NULL) fprintf(stdout, "%s-", printname);
          if (number) fprintf(stdout, "%d-", lastmatchnumber++);
          pp = end_of_line(pp, endptr, &ellength);
          FWRITE(lastmatchrestart, 1, pp - lastmatchrestart, stdout);
          lastmatchrestart = pp;
          }
        if (lastmatchrestart != ptr) hyphenpending = TRUE;
        }

      /* If there were non-contiguous lines printed above, insert hyphens. */

      if (hyphenpending)
        {
        fprintf(stdout, "--\n");
        hyphenpending = FALSE;
        hyphenprinted = TRUE;
        }

      /* See if there is a requirement to print some "before" lines for this
      match. Again, don't print overlaps. */

      if (before_context > 0)
        {
        int linecount = 0;
        char *p = ptr;

        while (p > main_buffer && (lastmatchnumber == 0 || p > lastmatchrestart) &&
               linecount < before_context)
          {
          linecount++;
          p = previous_line(p, main_buffer);
          }

        if (lastmatchnumber > 0 && p > lastmatchrestart && !hyphenprinted)
          fprintf(stdout, "--\n");

        while (p < ptr)
          {
          int ellength;
          char *pp = p;
          if (printname != NULL) fprintf(stdout, "%s-", printname);
          if (number) fprintf(stdout, "%d-", linenumber - linecount--);
          pp = end_of_line(pp, endptr, &ellength);
          FWRITE(p, 1, pp - p, stdout);
          p = pp;
          }
        }

      /* Now print the matching line(s); ensure we set hyphenpending at the end
      of the file if any context lines are being output. */

      if (after_context > 0 || before_context > 0)
        endhyphenpending = TRUE;

      if (printname != NULL) fprintf(stdout, "%s:", printname);
      if (number) fprintf(stdout, "%d:", linenumber);

      /* In multiline mode, we want to print to the end of the line in which
      the end of the matched string is found, so we adjust linelength and the
      line number appropriately, but only when there actually was a match
      (invert not set). Because the PCRE_FIRSTLINE option is set, the start of
      the match will always be before the first newline sequence. */

      if (multiline & !invert)
        {
        char *endmatch = ptr + offsets[1];
        t = ptr;
        while (t <= endmatch)
          {
          t = end_of_line(t, endptr, &endlinelength);
          if (t < endmatch) linenumber++; else break;
          }
        linelength = t - ptr - endlinelength;
        }

      /*** NOTE: Use only fwrite() to output the data line, so that binary
      zeroes are treated as just another data character. */

      /* This extra option, for Jeffrey Friedl's debugging requirements,
      replaces the matched string, or a specific captured string if it exists,
      with X. When this happens, colouring is ignored. */

#ifdef JFRIEDL_DEBUG
      if (S_arg >= 0 && S_arg < mrc)
        {
        int first = S_arg * 2;
        int last  = first + 1;
        FWRITE(ptr, 1, offsets[first], stdout);
        fprintf(stdout, "X");
        FWRITE(ptr + offsets[last], 1, linelength - offsets[last], stdout);
        }
      else
#endif

      /* We have to split the line(s) up if colouring, and search for further
      matches, but not of course if the line is a non-match. */

      if (do_colour && !invert)
        {
        int plength;
        FWRITE(ptr, 1, offsets[0], stdout);
        fprintf(stdout, "%c[%sm", 0x1b, colour_string);
        FWRITE(ptr + offsets[0], 1, offsets[1] - offsets[0], stdout);
        fprintf(stdout, "%c[00m", 0x1b);
        for (;;)
          {
          startoffset = offsets[1];
          if (startoffset >= (int)linelength + endlinelength ||
              !match_patterns(matchptr, length, options, startoffset, offsets,
                &mrc))
            break;
          FWRITE(matchptr + startoffset, 1, offsets[0] - startoffset, stdout);
          fprintf(stdout, "%c[%sm", 0x1b, colour_string);
          FWRITE(matchptr + offsets[0], 1, offsets[1] - offsets[0], stdout);
          fprintf(stdout, "%c[00m", 0x1b);
          }

        /* In multiline mode, we may have already printed the complete line
        and its line-ending characters (if they matched the pattern), so there
        may be no more to print. */

        plength = (int)((linelength + endlinelength) - startoffset);
        if (plength > 0) FWRITE(ptr + startoffset, 1, plength, stdout);
        }

      /* Not colouring; no need to search for further matches */

      else FWRITE(ptr, 1, linelength + endlinelength, stdout);
      }

    /* End of doing what has to be done for a match. If --line-buffered was
    given, flush the output. */

    if (line_buffered) fflush(stdout);
    rc = 0;    /* Had some success */

    /* Remember where the last match happened for after_context. We remember
    where we are about to restart, and that line's number. */

    lastmatchrestart = ptr + linelength + endlinelength;
    lastmatchnumber = linenumber + 1;
    }

  /* For a match in multiline inverted mode (which of course did not cause
  anything to be printed), we have to move on to the end of the match before
  proceeding. */

  if (multiline && invert && match)
    {
    int ellength;
    char *endmatch = ptr + offsets[1];
    t = ptr;
    while (t < endmatch)
      {
      t = end_of_line(t, endptr, &ellength);
      if (t <= endmatch) linenumber++; else break;
      }
    endmatch = end_of_line(endmatch, endptr, &ellength);
    linelength = endmatch - ptr - ellength;
    }

  /* Advance to after the newline and increment the line number. The file
  offset to the current line is maintained in filepos. */

  END_ONE_MATCH:
  ptr += linelength + endlinelength;
  filepos += (int)(linelength + endlinelength);
  linenumber++;

  /* If input is line buffered, and the buffer is not yet full, read another
  line and add it into the buffer. */

  if (input_line_buffered && bufflength < (size_t)bufsize)
    {
    int add = read_one_line(ptr, bufsize - (int)(ptr - main_buffer), in);
    bufflength += add;
    endptr += add;
    }

  /* If we haven't yet reached the end of the file (the buffer is full), and
  the current point is in the top 1/3 of the buffer, slide the buffer down by
  1/3 and refill it. Before we do this, if some unprinted "after" lines are
  about to be lost, print them. */

  if (bufflength >= (size_t)bufsize && ptr > main_buffer + 2*bufthird)
    {
    if (after_context > 0 &&
        lastmatchnumber > 0 &&
        lastmatchrestart < main_buffer + bufthird)
      {
      do_after_lines(lastmatchnumber, lastmatchrestart, endptr, printname);
      lastmatchnumber = 0;
      }

    /* Now do the shuffle */

    memmove(main_buffer, main_buffer + bufthird, 2*bufthird);
    ptr -= bufthird;

#ifdef SUPPORT_LIBZ
    if (frtype == FR_LIBZ)
      bufflength = 2*bufthird +
        gzread (ingz, main_buffer + 2*bufthird, bufthird);
    else
#endif

#ifdef SUPPORT_LIBBZ2
    if (frtype == FR_LIBBZ2)
      bufflength = 2*bufthird +
        BZ2_bzread(inbz2, main_buffer + 2*bufthird, bufthird);
    else
#endif

    bufflength = 2*bufthird +
      (input_line_buffered?
       read_one_line(main_buffer + 2*bufthird, bufthird, in) :
       fread(main_buffer + 2*bufthird, 1, bufthird, in));
    endptr = main_buffer + bufflength;

    /* Adjust any last match point */

    if (lastmatchnumber > 0) lastmatchrestart -= bufthird;
    }
  }     /* Loop through the whole file */

/* End of file; print final "after" lines if wanted; do_after_lines sets
hyphenpending if it prints something. */

if (!show_only_matching && !count_only)
  {
  do_after_lines(lastmatchnumber, lastmatchrestart, endptr, printname);
  hyphenpending |= endhyphenpending;
  }

/* Print the file name if we are looking for those without matches and there
were none. If we found a match, we won't have got this far. */

if (filenames == FN_NOMATCH_ONLY)
  {
  fprintf(stdout, "%s\n", printname);
  return 0;
  }

/* Print the match count if wanted */

if (count_only && !quiet)
  {
  if (count > 0 || !omit_zero_count)
    {
    if (printname != NULL && filenames != FN_NONE)
      fprintf(stdout, "%s:", printname);
    fprintf(stdout, "%d\n", count);
    }
  }

return rc;
}



/*************************************************
*     Grep a file or recurse into a directory    *
*************************************************/

/* Given a path name, if it's a directory, scan all the files if we are
recursing; if it's a file, grep it.

Arguments:
  pathname          the path to investigate
  dir_recurse       TRUE if recursing is wanted (-r or -drecurse)
  only_one_at_top   TRUE if the path is the only one at toplevel

Returns:  -1 the file/directory was skipped
           0 if there was at least one match
           1 if there were no matches
           2 there was some kind of error

However, file opening failures are suppressed if "silent" is set.
*/

static int
grep_or_recurse(char *pathname, BOOL dir_recurse, BOOL only_one_at_top)
{
int rc = 1;
int frtype;
void *handle;
char *lastcomp;
FILE *in = NULL;           /* Ensure initialized */

#ifdef SUPPORT_LIBZ
gzFile ingz = NULL;
#endif

#ifdef SUPPORT_LIBBZ2
BZFILE *inbz2 = NULL;
#endif

#if defined SUPPORT_LIBZ || defined SUPPORT_LIBBZ2
int pathlen;
#endif

#if defined NATIVE_ZOS
int zos_type;
FILE *zos_test_file;
#endif

/* If the file name is "-" we scan stdin */

if (strcmp(pathname, "-") == 0)
  {
  return pcregrep(stdin, FR_PLAIN, stdin_name,
    (filenames > FN_DEFAULT || (filenames == FN_DEFAULT && !only_one_at_top))?
      stdin_name : NULL);
  }

/* Inclusion and exclusion: --include-dir and --exclude-dir apply only to
directories, whereas --include and --exclude apply to everything else. The test
is against the final component of the path. */

lastcomp = strrchr(pathname, FILESEP);
lastcomp = (lastcomp == NULL)? pathname : lastcomp + 1;

/* If the file is a directory, skip if not recursing or if explicitly excluded.
Otherwise, scan the directory and recurse for each path within it. The scanning
code is localized so it can be made system-specific. */


/* For z/OS, determine the file type. */

#if defined NATIVE_ZOS
zos_test_file =  fopen(pathname,"rb");

if (zos_test_file == NULL)
   {
   if (!silent) fprintf(stderr, "pcregrep: failed to test next file %s\n",
     pathname, strerror(errno));
   return -1;
   }
zos_type = identifyzosfiletype (zos_test_file);
fclose (zos_test_file);

/* Handle a PDS in separate code */

if (zos_type == __ZOS_PDS || zos_type == __ZOS_PDSE)
   {
   return travelonpdsdir (pathname, only_one_at_top);
   }

/* Deal with regular files in the normal way below. These types are:
   zos_type == __ZOS_PDS_MEMBER
   zos_type == __ZOS_PS
   zos_type == __ZOS_VSAM_KSDS
   zos_type == __ZOS_VSAM_ESDS
   zos_type == __ZOS_VSAM_RRDS
*/

/* Handle a z/OS directory using common code. */

else if (zos_type == __ZOS_HFS)
 {
#endif  /* NATIVE_ZOS */


/* Handle directories: common code for all OS */

if (isdirectory(pathname))
  {
  if (dee_action == dee_SKIP ||
      !test_incexc(lastcomp, include_dir_patterns, exclude_dir_patterns))
    return -1;

  if (dee_action == dee_RECURSE)
    {
    char buffer[1024];
    char *nextfile;
    directory_type *dir = opendirectory(pathname);

    if (dir == NULL)
      {
      if (!silent)
        fprintf(stderr, "pcregrep: Failed to open directory %s: %s\n", pathname,
          strerror(errno));
      return 2;
      }

    while ((nextfile = readdirectory(dir)) != NULL)
      {
      int frc;
      sprintf(buffer, "%.512s%c%.128s", pathname, FILESEP, nextfile);
      frc = grep_or_recurse(buffer, dir_recurse, FALSE);
      if (frc > 1) rc = frc;
       else if (frc == 0 && rc == 1) rc = 0;
      }

    closedirectory(dir);
    return rc;
    }
  }

#if defined NATIVE_ZOS
 }
#endif

/* If the file is not a directory, check for a regular file, and if it is not,
skip it if that's been requested. Otherwise, check for an explicit inclusion or
exclusion. */

else if (
#if defined NATIVE_ZOS
        (zos_type == __ZOS_NOFILE && DEE_action == DEE_SKIP) ||
#else  /* all other OS */
        (!isregfile(pathname) && DEE_action == DEE_SKIP) ||
#endif
        !test_incexc(lastcomp, include_patterns, exclude_patterns))
  return -1;  /* File skipped */

/* Control reaches here if we have a regular file, or if we have a directory
and recursion or skipping was not requested, or if we have anything else and
skipping was not requested. The scan proceeds. If this is the first and only
argument at top level, we don't show the file name, unless we are only showing
the file name, or the filename was forced (-H). */

#if defined SUPPORT_LIBZ || defined SUPPORT_LIBBZ2
pathlen = (int)(strlen(pathname));
#endif

/* Open using zlib if it is supported and the file name ends with .gz. */

#ifdef SUPPORT_LIBZ
if (pathlen > 3 && strcmp(pathname + pathlen - 3, ".gz") == 0)
  {
  ingz = gzopen(pathname, "rb");
  if (ingz == NULL)
    {
    if (!silent)
      fprintf(stderr, "pcregrep: Failed to open %s: %s\n", pathname,
        strerror(errno));
    return 2;
    }
  handle = (void *)ingz;
  frtype = FR_LIBZ;
  }
else
#endif

/* Otherwise open with bz2lib if it is supported and the name ends with .bz2. */

#ifdef SUPPORT_LIBBZ2
if (pathlen > 4 && strcmp(pathname + pathlen - 4, ".bz2") == 0)
  {
  inbz2 = BZ2_bzopen(pathname, "rb");
  handle = (void *)inbz2;
  frtype = FR_LIBBZ2;
  }
else
#endif

/* Otherwise use plain fopen(). The label is so that we can come back here if
an attempt to read a .bz2 file indicates that it really is a plain file. */

#ifdef SUPPORT_LIBBZ2
PLAIN_FILE:
#endif
  {
  in = fopen(pathname, "rb");
  handle = (void *)in;
  frtype = FR_PLAIN;
  }

/* All the opening methods return errno when they fail. */

if (handle == NULL)
  {
  if (!silent)
    fprintf(stderr, "pcregrep: Failed to open %s: %s\n", pathname,
      strerror(errno));
  return 2;
  }

/* Now grep the file */

rc = pcregrep(handle, frtype, pathname, (filenames > FN_DEFAULT ||
  (filenames == FN_DEFAULT && !only_one_at_top))? pathname : NULL);

/* Close in an appropriate manner. */

#ifdef SUPPORT_LIBZ
if (frtype == FR_LIBZ)
  gzclose(ingz);
else
#endif

/* If it is a .bz2 file and the result is 3, it means that the first attempt to
read failed. If the error indicates that the file isn't in fact bzipped, try
again as a normal file. */

#ifdef SUPPORT_LIBBZ2
if (frtype == FR_LIBBZ2)
  {
  if (rc == 3)
    {
    int errnum;
    const char *err = BZ2_bzerror(inbz2, &errnum);
    if (errnum == BZ_DATA_ERROR_MAGIC)
      {
      BZ2_bzclose(inbz2);
      goto PLAIN_FILE;
      }
    else if (!silent)
      fprintf(stderr, "pcregrep: Failed to read %s using bzlib: %s\n",
        pathname, err);
    rc = 2;    /* The normal "something went wrong" code */
    }
  BZ2_bzclose(inbz2);
  }
else
#endif

/* Normal file close */

fclose(in);

/* Pass back the yield from pcregrep(). */

return rc;
}



/*************************************************
*    Handle a single-letter, no data option      *
*************************************************/

static int
handle_option(int letter, int options)
{
switch(letter)
  {
  case N_FOFFSETS: file_offsets = TRUE; break;
  case N_HELP: help(); pcregrep_exit(0);
  case N_LBUFFER: line_buffered = TRUE; break;
  case N_LOFFSETS: line_offsets = number = TRUE; break;
  case N_NOJIT: study_options &= ~PCRE_STUDY_JIT_COMPILE; break;
  case 'a': binary_files = BIN_TEXT; break;
  case 'c': count_only = TRUE; break;
  case 'F': process_options |= PO_FIXED_STRINGS; break;
  case 'H': filenames = FN_FORCE; break;
  case 'I': binary_files = BIN_NOMATCH; break;
  case 'h': filenames = FN_NONE; break;
  case 'i': options |= PCRE_CASELESS; break;
  case 'l': omit_zero_count = TRUE; filenames = FN_MATCH_ONLY; break;
  case 'L': filenames = FN_NOMATCH_ONLY; break;
  case 'M': multiline = TRUE; options |= PCRE_MULTILINE|PCRE_FIRSTLINE; break;
  case 'n': number = TRUE; break;

  case 'o':
  only_matching_last = add_number(0, only_matching_last);
  if (only_matching == NULL) only_matching = only_matching_last;
  break;

  case 'q': quiet = TRUE; break;
  case 'r': dee_action = dee_RECURSE; break;
  case 's': silent = TRUE; break;
  case 'u': options |= PCRE_UTF8; utf8 = TRUE; break;
  case 'v': invert = TRUE; break;
  case 'w': process_options |= PO_WORD_MATCH; break;
  case 'x': process_options |= PO_LINE_MATCH; break;

  case 'V':
  fprintf(stdout, "pcregrep version %s\n", pcre_version());
  pcregrep_exit(0);
  break;

  default:
  fprintf(stderr, "pcregrep: Unknown option -%c\n", letter);
  pcregrep_exit(usage(2));
  }

return options;
}




/*************************************************
*          Construct printed ordinal             *
*************************************************/

/* This turns a number into "1st", "3rd", etc. */

static char *
ordin(int n)
{
static char buffer[8];
char *p = buffer;
sprintf(p, "%d", n);
while (*p != 0) p++;
switch (n%10)
  {
  case 1: strcpy(p, "st"); break;
  case 2: strcpy(p, "nd"); break;
  case 3: strcpy(p, "rd"); break;
  default: strcpy(p, "th"); break;
  }
return buffer;
}



/*************************************************
*          Compile a single pattern              *
*************************************************/

/* Do nothing if the pattern has already been compiled. This is the case for
include/exclude patterns read from a file.

When the -F option has been used, each "pattern" may be a list of strings,
separated by line breaks. They will be matched literally. We split such a
string and compile the first substring, inserting an additional block into the
pattern chain.

Arguments:
  p              points to the pattern block
  options        the PCRE options
  popts          the processing options
  fromfile       TRUE if the pattern was read from a file
  fromtext       file name or identifying text (e.g. "include")
  count          0 if this is the only command line pattern, or
                 number of the command line pattern, or
                 linenumber for a pattern from a file

Returns:         TRUE on success, FALSE after an error
*/

static BOOL
compile_pattern(patstr *p, int options, int popts, int fromfile,
  const char *fromtext, int count)
{
char buffer[PATBUFSIZE];
const char *error;
char *ps = p->string;
int patlen = strlen(ps);
int errptr;

if (p->compiled != NULL) return TRUE;

if ((popts & PO_FIXED_STRINGS) != 0)
  {
  int ellength;
  char *eop = ps + patlen;
  char *pe = end_of_line(ps, eop, &ellength);

  if (ellength != 0)
    {
    if (add_pattern(pe, p) == NULL) return FALSE;
    patlen = (int)(pe - ps - ellength);
    }
  }

sprintf(buffer, "%s%.*s%s", prefix[popts], patlen, ps, suffix[popts]);
p->compiled = pcre_compile(buffer, options, &error, &errptr, pcretables);
if (p->compiled != NULL) return TRUE;

/* Handle compile errors */

errptr -= (int)strlen(prefix[popts]);
if (errptr > patlen) errptr = patlen;

if (fromfile)
  {
  fprintf(stderr, "pcregrep: Error in regex in line %d of %s "
    "at offset %d: %s\n", count, fromtext, errptr, error);
  }
else
  {
  if (count == 0)
    fprintf(stderr, "pcregrep: Error in %s regex at offset %d: %s\n",
      fromtext, errptr, error);
  else
    fprintf(stderr, "pcregrep: Error in %s %s regex at offset %d: %s\n",
      ordin(count), fromtext, errptr, error);
  }

return FALSE;
}



/*************************************************
*     Read and compile a file of patterns        *
*************************************************/

/* This is used for --filelist, --include-from, and --exclude-from.

Arguments:
  name         the name of the file; "-" is stdin
  patptr       pointer to the pattern chain anchor
  patlastptr   pointer to the last pattern pointer
  popts        the process options to pass to pattern_compile()

Returns:       TRUE if all went well
*/

static BOOL
read_pattern_file(char *name, patstr **patptr, patstr **patlastptr, int popts)
{
int linenumber = 0;
FILE *f;
char *filename;
char buffer[PATBUFSIZE];

if (strcmp(name, "-") == 0)
  {
  f = stdin;
  filename = stdin_name;
  }
else
  {
  f = fopen(name, "r");
  if (f == NULL)
    {
    fprintf(stderr, "pcregrep: Failed to open %s: %s\n", name, strerror(errno));
    return FALSE;
    }
  filename = name;
  }

while (fgets(buffer, PATBUFSIZE, f) != NULL)
  {
  char *s = buffer + (int)strlen(buffer);
  while (s > buffer && isspace((unsigned char)(s[-1]))) s--;
  *s = 0;
  linenumber++;
  if (buffer[0] == 0) continue;   /* Skip blank lines */

  /* Note: this call to add_pattern() puts a pointer to the local variable
  "buffer" into the pattern chain. However, that pointer is used only when
  compiling the pattern, which happens immediately below, so we flatten it
  afterwards, as a precaution against any later code trying to use it. */

  *patlastptr = add_pattern(buffer, *patlastptr);
  if (*patlastptr == NULL)
    {
    if (f != stdin) fclose(f);
    return FALSE;
    }
  if (*patptr == NULL) *patptr = *patlastptr;

  /* This loop is needed because compiling a "pattern" when -F is set may add
  on additional literal patterns if the original contains a newline. In the
  common case, it never will, because fgets() stops at a newline. However,
  the -N option can be used to give pcregrep a different newline setting. */

  for(;;)
    {
    if (!compile_pattern(*patlastptr, pcre_options, popts, TRUE, filename,
        linenumber))
      {
      if (f != stdin) fclose(f);
      return FALSE;
      }
    (*patlastptr)->string = NULL;            /* Insurance */
    if ((*patlastptr)->next == NULL) break;
    *patlastptr = (*patlastptr)->next;
    }
  }

if (f != stdin) fclose(f);
return TRUE;
}



/*************************************************
*                Main program                    *
*************************************************/

/* Returns 0 if something matched, 1 if nothing matched, 2 after an error. */

int
main(int argc, char **argv)
{
int i, j;
int rc = 1;
BOOL only_one_at_top;
patstr *cp;
fnstr *fn;
const char *locale_from = "--locale";
const char *error;

#ifdef SUPPORT_PCREGREP_JIT
pcre_jit_stack *jit_stack = NULL;
#endif

/* Set the default line ending value from the default in the PCRE library;
"lf", "cr", "crlf", and "any" are supported. Anything else is treated as "lf".
Note that the return values from pcre_config(), though derived from the ASCII
codes, are the same in EBCDIC environments, so we must use the actual values
rather than escapes such as as '\r'. */

(void)pcre_config(PCRE_CONFIG_NEWLINE, &i);
switch(i)
  {
  default:               newline = (char *)"lf"; break;
  case 13:               newline = (char *)"cr"; break;
  case (13 << 8) | 10:   newline = (char *)"crlf"; break;
  case -1:               newline = (char *)"any"; break;
  case -2:               newline = (char *)"anycrlf"; break;
  }

/* Process the options */

for (i = 1; i < argc; i++)
  {
  option_item *op = NULL;
  char *option_data = (char *)"";    /* default to keep compiler happy */
  BOOL longop;
  BOOL longopwasequals = FALSE;

  if (argv[i][0] != '-') break;

  /* If we hit an argument that is just "-", it may be a reference to STDIN,
  but only if we have previously had -e or -f to define the patterns. */

  if (argv[i][1] == 0)
    {
    if (pattern_files != NULL || patterns != NULL) break;
      else pcregrep_exit(usage(2));
    }

  /* Handle a long name option, or -- to terminate the options */

  if (argv[i][1] == '-')
    {
    char *arg = argv[i] + 2;
    char *argequals = strchr(arg, '=');

    if (*arg == 0)    /* -- terminates options */
      {
      i++;
      break;                /* out of the options-handling loop */
      }

    longop = TRUE;

    /* Some long options have data that follows after =, for example file=name.
    Some options have variations in the long name spelling: specifically, we
    allow "regexp" because GNU grep allows it, though I personally go along
    with Jeffrey Friedl and Larry Wall in preferring "regex" without the "p".
    These options are entered in the table as "regex(p)". Options can be in
    both these categories. */

    for (op = optionlist; op->one_char != 0; op++)
      {
      char *opbra = strchr(op->long_name, '(');
      char *equals = strchr(op->long_name, '=');

      /* Handle options with only one spelling of the name */

      if (opbra == NULL)     /* Does not contain '(' */
        {
        if (equals == NULL)  /* Not thing=data case */
          {
          if (strcmp(arg, op->long_name) == 0) break;
          }
        else                 /* Special case xxx=data */
          {
          int oplen = (int)(equals - op->long_name);
          int arglen = (argequals == NULL)?
            (int)strlen(arg) : (int)(argequals - arg);
          if (oplen == arglen && strncmp(arg, op->long_name, oplen) == 0)
            {
            option_data = arg + arglen;
            if (*option_data == '=')
              {
              option_data++;
              longopwasequals = TRUE;
              }
            break;
            }
          }
        }

      /* Handle options with an alternate spelling of the name */

      else
        {
        char buff1[24];
        char buff2[24];

        int baselen = (int)(opbra - op->long_name);
        int fulllen = (int)(strchr(op->long_name, ')') - op->long_name + 1);
        int arglen = (argequals == NULL || equals == NULL)?
          (int)strlen(arg) : (int)(argequals - arg);

        sprintf(buff1, "%.*s", baselen, op->long_name);
        sprintf(buff2, "%s%.*s", buff1, fulllen - baselen - 2, opbra + 1);

        if (strncmp(arg, buff1, arglen) == 0 ||
           strncmp(arg, buff2, arglen) == 0)
          {
          if (equals != NULL && argequals != NULL)
            {
            option_data = argequals;
            if (*option_data == '=')
              {
              option_data++;
              longopwasequals = TRUE;
              }
            }
          break;
          }
        }
      }

    if (op->one_char == 0)
      {
      fprintf(stderr, "pcregrep: Unknown option %s\n", argv[i]);
      pcregrep_exit(usage(2));
      }
    }

  /* Jeffrey Friedl's debugging harness uses these additional options which
  are not in the right form for putting in the option table because they use
  only one hyphen, yet are more than one character long. By putting them
  separately here, they will not get displayed as part of the help() output,
  but I don't think Jeffrey will care about that. */

#ifdef JFRIEDL_DEBUG
  else if (strcmp(argv[i], "-pre") == 0) {
          jfriedl_prefix = argv[++i];
          continue;
  } else if (strcmp(argv[i], "-post") == 0) {
          jfriedl_postfix = argv[++i];
          continue;
  } else if (strcmp(argv[i], "-XT") == 0) {
          sscanf(argv[++i], "%d", &jfriedl_XT);
          continue;
  } else if (strcmp(argv[i], "-XR") == 0) {
          sscanf(argv[++i], "%d", &jfriedl_XR);
          continue;
  }
#endif


  /* One-char options; many that have no data may be in a single argument; we
  continue till we hit the last one or one that needs data. */

  else
    {
    char *s = argv[i] + 1;
    longop = FALSE;

    while (*s != 0)
      {
      for (op = optionlist; op->one_char != 0; op++)
        {
        if (*s == op->one_char) break;
        }
      if (op->one_char == 0)
        {
        fprintf(stderr, "pcregrep: Unknown option letter '%c' in \"%s\"\n",
          *s, argv[i]);
        pcregrep_exit(usage(2));
        }

      option_data = s+1;

      /* Break out if this is the last character in the string; it's handled
      below like a single multi-char option. */

      if (*option_data == 0) break;

      /* Check for a single-character option that has data: OP_OP_NUMBER(S)
      are used for ones that either have a numerical number or defaults, i.e.
      the data is optional. If a digit follows, there is data; if not, carry on
      with other single-character options in the same string. */

      if (op->type == OP_OP_NUMBER || op->type == OP_OP_NUMBERS)
        {
        if (isdigit((unsigned char)s[1])) break;
        }
      else   /* Check for an option with data */
        {
        if (op->type != OP_NODATA) break;
        }

      /* Handle a single-character option with no data, then loop for the
      next character in the string. */

      pcre_options = handle_option(*s++, pcre_options);
      }
    }

  /* At this point we should have op pointing to a matched option. If the type
  is NO_DATA, it means that there is no data, and the option might set
  something in the PCRE options. */

  if (op->type == OP_NODATA)
    {
    pcre_options = handle_option(op->one_char, pcre_options);
    continue;
    }

  /* If the option type is OP_OP_STRING or OP_OP_NUMBER(S), it's an option that
  either has a value or defaults to something. It cannot have data in a
  separate item. At the moment, the only such options are "colo(u)r",
  "only-matching", and Jeffrey Friedl's special -S debugging option. */

  if (*option_data == 0 &&
      (op->type == OP_OP_STRING || op->type == OP_OP_NUMBER ||
       op->type == OP_OP_NUMBERS))
    {
    switch (op->one_char)
      {
      case N_COLOUR:
      colour_option = (char *)"auto";
      break;

      case 'o':
      only_matching_last = add_number(0, only_matching_last);
      if (only_matching == NULL) only_matching = only_matching_last;
      break;

#ifdef JFRIEDL_DEBUG
      case 'S':
      S_arg = 0;
      break;
#endif
      }
    continue;
    }

  /* Otherwise, find the data string for the option. */

  if (*option_data == 0)
    {
    if (i >= argc - 1 || longopwasequals)
      {
      fprintf(stderr, "pcregrep: Data missing after %s\n", argv[i]);
      pcregrep_exit(usage(2));
      }
    option_data = argv[++i];
    }

  /* If the option type is OP_OP_NUMBERS, the value is a number that is to be
  added to a chain of numbers. */

  if (op->type == OP_OP_NUMBERS)
    {
    unsigned long int n = decode_number(option_data, op, longop);
    omdatastr *omd = (omdatastr *)op->dataptr;
    *(omd->lastptr) = add_number((int)n, *(omd->lastptr));
    if (*(omd->anchor) == NULL) *(omd->anchor) = *(omd->lastptr);
    }

  /* If the option type is OP_PATLIST, it's the -e option, or one of the
  include/exclude options, which can be called multiple times to create lists
  of patterns. */

  else if (op->type == OP_PATLIST)
    {
    patdatastr *pd = (patdatastr *)op->dataptr;
    *(pd->lastptr) = add_pattern(option_data, *(pd->lastptr));
    if (*(pd->lastptr) == NULL) goto EXIT2;
    if (*(pd->anchor) == NULL) *(pd->anchor) = *(pd->lastptr);
    }

  /* If the option type is OP_FILELIST, it's one of the options that names a
  file. */

  else if (op->type == OP_FILELIST)
    {
    fndatastr *fd = (fndatastr *)op->dataptr;
    fn = (fnstr *)malloc(sizeof(fnstr));
    if (fn == NULL)
      {
      fprintf(stderr, "pcregrep: malloc failed\n");
      goto EXIT2;
      }
    fn->next = NULL;
    fn->name = option_data;
    if (*(fd->anchor) == NULL)
      *(fd->anchor) = fn;
    else
      (*(fd->lastptr))->next = fn;
    *(fd->lastptr) = fn;
    }

  /* Handle OP_BINARY_FILES */

  else if (op->type == OP_BINFILES)
    {
    if (strcmp(option_data, "binary") == 0)
      binary_files = BIN_BINARY;
    else if (strcmp(option_data, "without-match") == 0)
      binary_files = BIN_NOMATCH;
    else if (strcmp(option_data, "text") == 0)
      binary_files = BIN_TEXT;
    else
      {
      fprintf(stderr, "pcregrep: unknown value \"%s\" for binary-files\n",
        option_data);
      pcregrep_exit(usage(2));
      }
    }

  /* Otherwise, deal with a single string or numeric data value. */

  else if (op->type != OP_NUMBER && op->type != OP_LONGNUMBER &&
           op->type != OP_OP_NUMBER)
    {
    *((char **)op->dataptr) = option_data;
    }
  else
    {
    unsigned long int n = decode_number(option_data, op, longop);
    if (op->type == OP_LONGNUMBER) *((unsigned long int *)op->dataptr) = n;
      else *((int *)op->dataptr) = n;
    }
  }

/* Options have been decoded. If -C was used, its value is used as a default
for -A and -B. */

if (both_context > 0)
  {
  if (after_context == 0) after_context = both_context;
  if (before_context == 0) before_context = both_context;
  }

/* Only one of --only-matching, --file-offsets, or --line-offsets is permitted.
However, all three set show_only_matching because they display, each in their
own way, only the data that has matched. */

if ((only_matching != NULL && (file_offsets || line_offsets)) ||
    (file_offsets && line_offsets))
  {
  fprintf(stderr, "pcregrep: Cannot mix --only-matching, --file-offsets "
    "and/or --line-offsets\n");
  pcregrep_exit(usage(2));
  }

if (only_matching != NULL || file_offsets || line_offsets)
  show_only_matching = TRUE;

/* If a locale has not been provided as an option, see if the LC_CTYPE or
LC_ALL environment variable is set, and if so, use it. */

if (locale == NULL)
  {
  locale = getenv("LC_ALL");
  locale_from = "LCC_ALL";
  }

if (locale == NULL)
  {
  locale = getenv("LC_CTYPE");
  locale_from = "LC_CTYPE";
  }

/* If a locale is set, use it to generate the tables the PCRE needs. Otherwise,
pcretables==NULL, which causes the use of default tables. */

if (locale != NULL)
  {
  if (setlocale(LC_CTYPE, locale) == NULL)
    {
    fprintf(stderr, "pcregrep: Failed to set locale %s (obtained from %s)\n",
      locale, locale_from);
    goto EXIT2;
    }
  pcretables = pcre_maketables();
  }

/* Sort out colouring */

if (colour_option != NULL && strcmp(colour_option, "never") != 0)
  {
  if (strcmp(colour_option, "always") == 0) do_colour = TRUE;
  else if (strcmp(colour_option, "auto") == 0) do_colour = is_stdout_tty();
  else
    {
    fprintf(stderr, "pcregrep: Unknown colour setting \"%s\"\n",
      colour_option);
    goto EXIT2;
    }
  if (do_colour)
    {
    char *cs = getenv("PCREGREP_COLOUR");
    if (cs == NULL) cs = getenv("PCREGREP_COLOR");
    if (cs != NULL) colour_string = cs;
    }
  }

/* Interpret the newline type; the default settings are Unix-like. */

if (strcmp(newline, "cr") == 0 || strcmp(newline, "CR") == 0)
  {
  pcre_options |= PCRE_NEWLINE_CR;
  endlinetype = EL_CR;
  }
else if (strcmp(newline, "lf") == 0 || strcmp(newline, "LF") == 0)
  {
  pcre_options |= PCRE_NEWLINE_LF;
  endlinetype = EL_LF;
  }
else if (strcmp(newline, "crlf") == 0 || strcmp(newline, "CRLF") == 0)
  {
  pcre_options |= PCRE_NEWLINE_CRLF;
  endlinetype = EL_CRLF;
  }
else if (strcmp(newline, "any") == 0 || strcmp(newline, "ANY") == 0)
  {
  pcre_options |= PCRE_NEWLINE_ANY;
  endlinetype = EL_ANY;
  }
else if (strcmp(newline, "anycrlf") == 0 || strcmp(newline, "ANYCRLF") == 0)
  {
  pcre_options |= PCRE_NEWLINE_ANYCRLF;
  endlinetype = EL_ANYCRLF;
  }
else
  {
  fprintf(stderr, "pcregrep: Invalid newline specifier \"%s\"\n", newline);
  goto EXIT2;
  }

/* Interpret the text values for -d and -D */

if (dee_option != NULL)
  {
  if (strcmp(dee_option, "read") == 0) dee_action = dee_READ;
  else if (strcmp(dee_option, "recurse") == 0) dee_action = dee_RECURSE;
  else if (strcmp(dee_option, "skip") == 0) dee_action = dee_SKIP;
  else
    {
    fprintf(stderr, "pcregrep: Invalid value \"%s\" for -d\n", dee_option);
    goto EXIT2;
    }
  }

if (DEE_option != NULL)
  {
  if (strcmp(DEE_option, "read") == 0) DEE_action = DEE_READ;
  else if (strcmp(DEE_option, "skip") == 0) DEE_action = DEE_SKIP;
  else
    {
    fprintf(stderr, "pcregrep: Invalid value \"%s\" for -D\n", DEE_option);
    goto EXIT2;
    }
  }

/* Check the values for Jeffrey Friedl's debugging options. */

#ifdef JFRIEDL_DEBUG
if (S_arg > 9)
  {
  fprintf(stderr, "pcregrep: bad value for -S option\n");
  return 2;
  }
if (jfriedl_XT != 0 || jfriedl_XR != 0)
  {
  if (jfriedl_XT == 0) jfriedl_XT = 1;
  if (jfriedl_XR == 0) jfriedl_XR = 1;
  }
#endif

/* Get memory for the main buffer. */

bufsize = 3*bufthird;
main_buffer = (char *)malloc(bufsize);

if (main_buffer == NULL)
  {
  fprintf(stderr, "pcregrep: malloc failed\n");
  goto EXIT2;
  }

/* If no patterns were provided by -e, and there are no files provided by -f,
the first argument is the one and only pattern, and it must exist. */

if (patterns == NULL && pattern_files == NULL)
  {
  if (i >= argc) return usage(2);
  patterns = patterns_last = add_pattern(argv[i++], NULL);
  if (patterns == NULL) goto EXIT2;
  }

/* Compile the patterns that were provided on the command line, either by
multiple uses of -e or as a single unkeyed pattern. We cannot do this until
after all the command-line options are read so that we know which PCRE options
to use. When -F is used, compile_pattern() may add another block into the
chain, so we must not access the next pointer till after the compile. */

for (j = 1, cp = patterns; cp != NULL; j++, cp = cp->next)
  {
  if (!compile_pattern(cp, pcre_options, process_options, FALSE, "command-line",
       (j == 1 && patterns->next == NULL)? 0 : j))
    goto EXIT2;
  }

/* Read and compile the regular expressions that are provided in files. */

for (fn = pattern_files; fn != NULL; fn = fn->next)
  {
  if (!read_pattern_file(fn->name, &patterns, &patterns_last, process_options))
    goto EXIT2;
  }

/* Study the regular expressions, as we will be running them many times. If an
extra block is needed for a limit, set PCRE_STUDY_EXTRA_NEEDED so that one is
returned, even if studying produces no data. */

if (match_limit > 0 || match_limit_recursion > 0)
  study_options |= PCRE_STUDY_EXTRA_NEEDED;

/* Unless JIT has been explicitly disabled, arrange a stack for it to use. */

#ifdef SUPPORT_PCREGREP_JIT
if ((study_options & PCRE_STUDY_JIT_COMPILE) != 0)
  jit_stack = pcre_jit_stack_alloc(32*1024, 1024*1024);
#endif

for (j = 1, cp = patterns; cp != NULL; j++, cp = cp->next)
  {
  cp->hint = pcre_study(cp->compiled, study_options, &error);
  if (error != NULL)
    {
    char s[16];
    if (patterns->next == NULL) s[0] = 0; else sprintf(s, " number %d", j);
    fprintf(stderr, "pcregrep: Error while studying regex%s: %s\n", s, error);
    goto EXIT2;
    }
#ifdef SUPPORT_PCREGREP_JIT
  if (jit_stack != NULL && cp->hint != NULL)
    pcre_assign_jit_stack(cp->hint, NULL, jit_stack);
#endif
  }

/* If --match-limit or --recursion-limit was set, put the value(s) into the
pcre_extra block for each pattern. There will always be an extra block because
of the use of PCRE_STUDY_EXTRA_NEEDED above. */

for (cp = patterns; cp != NULL; cp = cp->next)
  {
  if (match_limit > 0)
    {
    cp->hint->flags |= PCRE_EXTRA_MATCH_LIMIT;
    cp->hint->match_limit = match_limit;
    }

  if (match_limit_recursion > 0)
    {
    cp->hint->flags |= PCRE_EXTRA_MATCH_LIMIT_RECURSION;
    cp->hint->match_limit_recursion = match_limit_recursion;
    }
  }

/* If there are include or exclude patterns read from the command line, compile
them. -F, -w, and -x do not apply, so the third argument of compile_pattern is
0. */

for (j = 0; j < 4; j++)
  {
  int k;
  for (k = 1, cp = *(incexlist[j]); cp != NULL; k++, cp = cp->next)
    {
    if (!compile_pattern(cp, pcre_options, 0, FALSE, incexname[j],
         (k == 1 && cp->next == NULL)? 0 : k))
      goto EXIT2;
    }
  }

/* Read and compile include/exclude patterns from files. */

for (fn = include_from; fn != NULL; fn = fn->next)
  {
  if (!read_pattern_file(fn->name, &include_patterns, &include_patterns_last, 0))
    goto EXIT2;
  }

for (fn = exclude_from; fn != NULL; fn = fn->next)
  {
  if (!read_pattern_file(fn->name, &exclude_patterns, &exclude_patterns_last, 0))
    goto EXIT2;
  }

/* If there are no files that contain lists of files to search, and there are
no file arguments, search stdin, and then exit. */

if (file_lists == NULL && i >= argc)
  {
  rc = pcregrep(stdin, FR_PLAIN, stdin_name,
    (filenames > FN_DEFAULT)? stdin_name : NULL);
  goto EXIT;
  }

/* If any files that contains a list of files to search have been specified,
read them line by line and search the given files. */

for (fn = file_lists; fn != NULL; fn = fn->next)
  {
  char buffer[PATBUFSIZE];
  FILE *fl;
  if (strcmp(fn->name, "-") == 0) fl = stdin; else
    {
    fl = fopen(fn->name, "rb");
    if (fl == NULL)
      {
      fprintf(stderr, "pcregrep: Failed to open %s: %s\n", fn->name,
        strerror(errno));
      goto EXIT2;
      }
    }
  while (fgets(buffer, PATBUFSIZE, fl) != NULL)
    {
    int frc;
    char *end = buffer + (int)strlen(buffer);
    while (end > buffer && isspace(end[-1])) end--;
    *end = 0;
    if (*buffer != 0)
      {
      frc = grep_or_recurse(buffer, dee_action == dee_RECURSE, FALSE);
      if (frc > 1) rc = frc;
        else if (frc == 0 && rc == 1) rc = 0;
      }
    }
  if (fl != stdin) fclose(fl);
  }

/* After handling file-list, work through remaining arguments. Pass in the fact
that there is only one argument at top level - this suppresses the file name if
the argument is not a directory and filenames are not otherwise forced. */

only_one_at_top = i == argc - 1 && file_lists == NULL;

for (; i < argc; i++)
  {
  int frc = grep_or_recurse(argv[i], dee_action == dee_RECURSE,
    only_one_at_top);
  if (frc > 1) rc = frc;
    else if (frc == 0 && rc == 1) rc = 0;
  }

EXIT:
#ifdef SUPPORT_PCREGREP_JIT
if (jit_stack != NULL) pcre_jit_stack_free(jit_stack);
#endif

free(main_buffer);
free((void *)pcretables);

free_pattern_chain(patterns);
free_pattern_chain(include_patterns);
free_pattern_chain(include_dir_patterns);
free_pattern_chain(exclude_patterns);
free_pattern_chain(exclude_dir_patterns);

free_file_chain(exclude_from);
free_file_chain(include_from);
free_file_chain(pattern_files);
free_file_chain(file_lists);

while (only_matching != NULL)
  {
  omstr *this = only_matching;
  only_matching = this->next;
  free(this);
  }

pcregrep_exit(rc);

EXIT2:
rc = 2;
goto EXIT;
}

/* End of pcregrep */
