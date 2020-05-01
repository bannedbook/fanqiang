#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "polipo.h"

#ifndef HAVE_FTS
#include "fts_compat.c"
#else
static int dummy ATTRIBUTE((unused));
#endif

