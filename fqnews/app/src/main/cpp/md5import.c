#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdlib.h>

#if defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L
#define HAS_STDINT_H
#else
typedef unsigned int my_uint32_t;
#undef uint32_t
#define uint32_t my_uint32_t
#endif
#include "md5.c"
#undef uint32_t
