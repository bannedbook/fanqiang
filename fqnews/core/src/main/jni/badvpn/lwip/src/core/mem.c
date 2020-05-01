/**
 * @file
 * Dynamic memory manager
 *
 * This is a lightweight replacement for the standard C library malloc().
 *
 * If you want to use the standard C library malloc() instead, define
 * MEM_LIBC_MALLOC to 1 in your lwipopts.h
 *
 * To let mem_malloc() use pools (prevents fragmentation and is much faster than
 * a heap but might waste some memory), define MEM_USE_POOLS to 1, define
 * MEMP_USE_CUSTOM_POOLS to 1 and create a file "lwippools.h" that includes a list
 * of pools like this (more pools can be added between _START and _END):
 *
 * Define three pools with sizes 256, 512, and 1512 bytes
 * LWIP_MALLOC_MEMPOOL_START
 * LWIP_MALLOC_MEMPOOL(20, 256)
 * LWIP_MALLOC_MEMPOOL(10, 512)
 * LWIP_MALLOC_MEMPOOL(5, 1512)
 * LWIP_MALLOC_MEMPOOL_END
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *         Simon Goldschmidt
 *
 */

#include "lwip/opt.h"
#include "lwip/mem.h"
#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/stats.h"
#include "lwip/err.h"

#include <string.h>

#if MEM_LIBC_MALLOC
#include <stdlib.h> /* for malloc()/free() */
#endif

/* This is overridable for tests only... */
#ifndef LWIP_MEM_ILLEGAL_FREE
#define LWIP_MEM_ILLEGAL_FREE(msg)         LWIP_ASSERT(msg, 0)
#endif

#define MEM_STATS_INC_LOCKED(x)         SYS_ARCH_LOCKED(MEM_STATS_INC(x))
#define MEM_STATS_INC_USED_LOCKED(x, y) SYS_ARCH_LOCKED(MEM_STATS_INC_USED(x, y))
#define MEM_STATS_DEC_USED_LOCKED(x, y) SYS_ARCH_LOCKED(MEM_STATS_DEC_USED(x, y))

#if MEM_LIBC_MALLOC || MEM_USE_POOLS

/** mem_init is not used when using pools instead of a heap or using
 * C library malloc().
 */
void
mem_init(void)
{
}

/** mem_trim is not used when using pools instead of a heap or using
 * C library malloc(): we can't free part of a pool element and the stack
 * support mem_trim() to return a different pointer
 */
void *
mem_trim(void *mem, mem_size_t size)
{
  LWIP_UNUSED_ARG(size);
  return mem;
}
#endif /* MEM_LIBC_MALLOC || MEM_USE_POOLS */

#if MEM_LIBC_MALLOC
/* lwIP heap implemented using C library malloc() */

/* in case C library malloc() needs extra protection,
 * allow these defines to be overridden.
 */
#ifndef mem_clib_free
#define mem_clib_free free
#endif
#ifndef mem_clib_malloc
#define mem_clib_malloc malloc
#endif
#ifndef mem_clib_calloc
#define mem_clib_calloc calloc
#endif

#if LWIP_STATS && MEM_STATS
#define MEM_LIBC_STATSHELPER_SIZE LWIP_MEM_ALIGN_SIZE(sizeof(mem_size_t))
#else
#define MEM_LIBC_STATSHELPER_SIZE 0
#endif

/**
 * Allocate a block of memory with a minimum of 'size' bytes.
 *
 * @param size is the minimum size of the requested block in bytes.
 * @return pointer to allocated memory or NULL if no free memory was found.
 *
 * Note that the returned value must always be aligned (as defined by MEM_ALIGNMENT).
 */
void *
mem_malloc(mem_size_t size)
{
  void *ret = mem_clib_malloc(size + MEM_LIBC_STATSHELPER_SIZE);
  if (ret == NULL) {
    MEM_STATS_INC_LOCKED(err);
  } else {
    LWIP_ASSERT("malloc() must return aligned memory", LWIP_MEM_ALIGN(ret) == ret);
#if LWIP_STATS && MEM_STATS
    *(mem_size_t *)ret = size;
    ret = (u8_t *)ret + MEM_LIBC_STATSHELPER_SIZE;
    MEM_STATS_INC_USED_LOCKED(used, size);
#endif
  }
  return ret;
}

/** Put memory back on the heap
 *
 * @param rmem is the pointer as returned by a previous call to mem_malloc()
 */
void
mem_free(void *rmem)
{
  LWIP_ASSERT("rmem != NULL", (rmem != NULL));
  LWIP_ASSERT("rmem == MEM_ALIGN(rmem)", (rmem == LWIP_MEM_ALIGN(rmem)));
#if LWIP_STATS && MEM_STATS
  rmem = (u8_t *)rmem - MEM_LIBC_STATSHELPER_SIZE;
  MEM_STATS_DEC_USED_LOCKED(used, *(mem_size_t *)rmem);
#endif
  mem_clib_free(rmem);
}

#elif MEM_USE_POOLS

/* lwIP heap implemented with different sized pools */

/**
 * Allocate memory: determine the smallest pool that is big enough
 * to contain an element of 'size' and get an element from that pool.
 *
 * @param size the size in bytes of the memory needed
 * @return a pointer to the allocated memory or NULL if the pool is empty
 */
void *
mem_malloc(mem_size_t size)
{
  void *ret;
  struct memp_malloc_helper *element = NULL;
  memp_t poolnr;
  mem_size_t required_size = size + LWIP_MEM_ALIGN_SIZE(sizeof(struct memp_malloc_helper));

  for (poolnr = MEMP_POOL_FIRST; poolnr <= MEMP_POOL_LAST; poolnr = (memp_t)(poolnr + 1)) {
    /* is this pool big enough to hold an element of the required size
       plus a struct memp_malloc_helper that saves the pool this element came from? */
    if (required_size <= memp_pools[poolnr]->size) {
      element = (struct memp_malloc_helper *)memp_malloc(poolnr);
      if (element == NULL) {
        /* No need to DEBUGF or ASSERT: This error is already taken care of in memp.c */
#if MEM_USE_POOLS_TRY_BIGGER_POOL
        /** Try a bigger pool if this one is empty! */
        if (poolnr < MEMP_POOL_LAST) {
          continue;
        }
#endif /* MEM_USE_POOLS_TRY_BIGGER_POOL */
        MEM_STATS_INC_LOCKED(err);
        return NULL;
      }
      break;
    }
  }
  if (poolnr > MEMP_POOL_LAST) {
    LWIP_ASSERT("mem_malloc(): no pool is that big!", 0);
    MEM_STATS_INC_LOCKED(err);
    return NULL;
  }

  /* save the pool number this element came from */
  element->poolnr = poolnr;
  /* and return a pointer to the memory directly after the struct memp_malloc_helper */
  ret = (u8_t *)element + LWIP_MEM_ALIGN_SIZE(sizeof(struct memp_malloc_helper));

#if MEMP_OVERFLOW_CHECK || (LWIP_STATS && MEM_STATS)
  /* truncating to u16_t is safe because struct memp_desc::size is u16_t */
  element->size = (u16_t)size;
  MEM_STATS_INC_USED_LOCKED(used, element->size);
#endif /* MEMP_OVERFLOW_CHECK || (LWIP_STATS && MEM_STATS) */
#if MEMP_OVERFLOW_CHECK
  /* initialize unused memory (diff between requested size and selected pool's size) */
  memset((u8_t *)ret + size, 0xcd, memp_pools[poolnr]->size - size);
#endif /* MEMP_OVERFLOW_CHECK */
  return ret;
}

/**
 * Free memory previously allocated by mem_malloc. Loads the pool number
 * and calls memp_free with that pool number to put the element back into
 * its pool
 *
 * @param rmem the memory element to free
 */
void
mem_free(void *rmem)
{
  struct memp_malloc_helper *hmem;

  LWIP_ASSERT("rmem != NULL", (rmem != NULL));
  LWIP_ASSERT("rmem == MEM_ALIGN(rmem)", (rmem == LWIP_MEM_ALIGN(rmem)));

  /* get the original struct memp_malloc_helper */
  /* cast through void* to get rid of alignment warnings */
  hmem = (struct memp_malloc_helper *)(void *)((u8_t *)rmem - LWIP_MEM_ALIGN_SIZE(sizeof(struct memp_malloc_helper)));

  LWIP_ASSERT("hmem != NULL", (hmem != NULL));
  LWIP_ASSERT("hmem == MEM_ALIGN(hmem)", (hmem == LWIP_MEM_ALIGN(hmem)));
  LWIP_ASSERT("hmem->poolnr < MEMP_MAX", (hmem->poolnr < MEMP_MAX));

  MEM_STATS_DEC_USED_LOCKED(used, hmem->size);
#if MEMP_OVERFLOW_CHECK
  {
    u16_t i;
    LWIP_ASSERT("MEM_USE_POOLS: invalid chunk size",
                hmem->size <= memp_pools[hmem->poolnr]->size);
    /* check that unused memory remained untouched (diff between requested size and selected pool's size) */
    for (i = hmem->size; i < memp_pools[hmem->poolnr]->size; i++) {
      u8_t data = *((u8_t *)rmem + i);
      LWIP_ASSERT("MEM_USE_POOLS: mem overflow detected", data == 0xcd);
    }
  }
#endif /* MEMP_OVERFLOW_CHECK */

  /* and put it in the pool we saved earlier */
  memp_free(hmem->poolnr, hmem);
}

#else /* MEM_USE_POOLS */
/* lwIP replacement for your libc malloc() */

/**
 * The heap is made up as a list of structs of this type.
 * This does not have to be aligned since for getting its size,
 * we only use the macro SIZEOF_STRUCT_MEM, which automatically aligns.
 */
struct mem {
  /** index (-> ram[next]) of the next struct */
  mem_size_t next;
  /** index (-> ram[prev]) of the previous struct */
  mem_size_t prev;
  /** 1: this area is used; 0: this area is unused */
  u8_t used;
};

/** All allocated blocks will be MIN_SIZE bytes big, at least!
 * MIN_SIZE can be overridden to suit your needs. Smaller values save space,
 * larger values could prevent too small blocks to fragment the RAM too much. */
#ifndef MIN_SIZE
#define MIN_SIZE             12
#endif /* MIN_SIZE */
/* some alignment macros: we define them here for better source code layout */
#define MIN_SIZE_ALIGNED     LWIP_MEM_ALIGN_SIZE(MIN_SIZE)
#define SIZEOF_STRUCT_MEM    LWIP_MEM_ALIGN_SIZE(sizeof(struct mem))
#define MEM_SIZE_ALIGNED     LWIP_MEM_ALIGN_SIZE(MEM_SIZE)

/** If you want to relocate the heap to external memory, simply define
 * LWIP_RAM_HEAP_POINTER as a void-pointer to that location.
 * If so, make sure the memory at that location is big enough (see below on
 * how that space is calculated). */
#ifndef LWIP_RAM_HEAP_POINTER
/** the heap. we need one struct mem at the end and some room for alignment */
LWIP_DECLARE_MEMORY_ALIGNED(ram_heap, MEM_SIZE_ALIGNED + (2U * SIZEOF_STRUCT_MEM));
#define LWIP_RAM_HEAP_POINTER ram_heap
#endif /* LWIP_RAM_HEAP_POINTER */

/** pointer to the heap (ram_heap): for alignment, ram is now a pointer instead of an array */
static u8_t *ram;
/** the last entry, always unused! */
static struct mem *ram_end;
/** pointer to the lowest free block, this is used for faster search */
static struct mem *lfree;

/** concurrent access protection */
#if !NO_SYS
static sys_mutex_t mem_mutex;
#endif

#if LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT

static volatile u8_t mem_free_count;

/* Allow mem_free from other (e.g. interrupt) context */
#define LWIP_MEM_FREE_DECL_PROTECT()  SYS_ARCH_DECL_PROTECT(lev_free)
#define LWIP_MEM_FREE_PROTECT()       SYS_ARCH_PROTECT(lev_free)
#define LWIP_MEM_FREE_UNPROTECT()     SYS_ARCH_UNPROTECT(lev_free)
#define LWIP_MEM_ALLOC_DECL_PROTECT() SYS_ARCH_DECL_PROTECT(lev_alloc)
#define LWIP_MEM_ALLOC_PROTECT()      SYS_ARCH_PROTECT(lev_alloc)
#define LWIP_MEM_ALLOC_UNPROTECT()    SYS_ARCH_UNPROTECT(lev_alloc)

#else /* LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT */

/* Protect the heap only by using a mutex */
#define LWIP_MEM_FREE_DECL_PROTECT()
#define LWIP_MEM_FREE_PROTECT()    sys_mutex_lock(&mem_mutex)
#define LWIP_MEM_FREE_UNPROTECT()  sys_mutex_unlock(&mem_mutex)
/* mem_malloc is protected using mutex AND LWIP_MEM_ALLOC_PROTECT */
#define LWIP_MEM_ALLOC_DECL_PROTECT()
#define LWIP_MEM_ALLOC_PROTECT()
#define LWIP_MEM_ALLOC_UNPROTECT()

#endif /* LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT */


/**
 * "Plug holes" by combining adjacent empty struct mems.
 * After this function is through, there should not exist
 * one empty struct mem pointing to another empty struct mem.
 *
 * @param mem this points to a struct mem which just has been freed
 * @internal this function is only called by mem_free() and mem_trim()
 *
 * This assumes access to the heap is protected by the calling function
 * already.
 */
static void
plug_holes(struct mem *mem)
{
  struct mem *nmem;
  struct mem *pmem;

  LWIP_ASSERT("plug_holes: mem >= ram", (u8_t *)mem >= ram);
  LWIP_ASSERT("plug_holes: mem < ram_end", (u8_t *)mem < (u8_t *)ram_end);
  LWIP_ASSERT("plug_holes: mem->used == 0", mem->used == 0);

  /* plug hole forward */
  LWIP_ASSERT("plug_holes: mem->next <= MEM_SIZE_ALIGNED", mem->next <= MEM_SIZE_ALIGNED);

  nmem = (struct mem *)(void *)&ram[mem->next];
  if (mem != nmem && nmem->used == 0 && (u8_t *)nmem != (u8_t *)ram_end) {
    /* if mem->next is unused and not end of ram, combine mem and mem->next */
    if (lfree == nmem) {
      lfree = mem;
    }
    mem->next = nmem->next;
    ((struct mem *)(void *)&ram[nmem->next])->prev = (mem_size_t)((u8_t *)mem - ram);
  }

  /* plug hole backward */
  pmem = (struct mem *)(void *)&ram[mem->prev];
  if (pmem != mem && pmem->used == 0) {
    /* if mem->prev is unused, combine mem and mem->prev */
    if (lfree == mem) {
      lfree = pmem;
    }
    pmem->next = mem->next;
    ((struct mem *)(void *)&ram[mem->next])->prev = (mem_size_t)((u8_t *)pmem - ram);
  }
}

/**
 * Zero the heap and initialize start, end and lowest-free
 */
void
mem_init(void)
{
  struct mem *mem;

  LWIP_ASSERT("Sanity check alignment",
              (SIZEOF_STRUCT_MEM & (MEM_ALIGNMENT - 1)) == 0);

  /* align the heap */
  ram = (u8_t *)LWIP_MEM_ALIGN(LWIP_RAM_HEAP_POINTER);
  /* initialize the start of the heap */
  mem = (struct mem *)(void *)ram;
  mem->next = MEM_SIZE_ALIGNED;
  mem->prev = 0;
  mem->used = 0;
  /* initialize the end of the heap */
  ram_end = (struct mem *)(void *)&ram[MEM_SIZE_ALIGNED];
  ram_end->used = 1;
  ram_end->next = MEM_SIZE_ALIGNED;
  ram_end->prev = MEM_SIZE_ALIGNED;

  /* initialize the lowest-free pointer to the start of the heap */
  lfree = (struct mem *)(void *)ram;

  MEM_STATS_AVAIL(avail, MEM_SIZE_ALIGNED);

  if (sys_mutex_new(&mem_mutex) != ERR_OK) {
    LWIP_ASSERT("failed to create mem_mutex", 0);
  }
}

/* Check if a struct mem is correctly linked.
 * If not, double-free is a possible reason.
 */
static int
mem_link_valid(struct mem *mem)
{
  struct mem *nmem, *pmem;
  mem_size_t rmem_idx;
  rmem_idx = (mem_size_t)((u8_t *)mem - ram);
  nmem = (struct mem *)(void *)&ram[mem->next];
  pmem = (struct mem *)(void *)&ram[mem->prev];
  if ((mem->next > MEM_SIZE_ALIGNED) || (mem->prev > MEM_SIZE_ALIGNED) ||
      ((mem->prev != rmem_idx) && (pmem->next != rmem_idx)) ||
      ((nmem != ram_end) && (nmem->prev != rmem_idx))) {
    return 0;
  }
  return 1;
}

/**
 * Put a struct mem back on the heap
 *
 * @param rmem is the data portion of a struct mem as returned by a previous
 *             call to mem_malloc()
 */
void
mem_free(void *rmem)
{
  struct mem *mem;
  LWIP_MEM_FREE_DECL_PROTECT();

  if (rmem == NULL) {
    LWIP_DEBUGF(MEM_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS, ("mem_free(p == NULL) was called.\n"));
    return;
  }
  if ((((mem_ptr_t)rmem) & (MEM_ALIGNMENT - 1)) != 0) {
    LWIP_MEM_ILLEGAL_FREE("mem_free: sanity check alignment");
    LWIP_DEBUGF(MEM_DEBUG | LWIP_DBG_LEVEL_SEVERE, ("mem_free: sanity check alignment\n"));
    /* protect mem stats from concurrent access */
    MEM_STATS_INC_LOCKED(illegal);
    return;
  }

  /* Get the corresponding struct mem: */
  /* cast through void* to get rid of alignment warnings */
  mem = (struct mem *)(void *)((u8_t *)rmem - SIZEOF_STRUCT_MEM);

  if ((u8_t *)mem < ram || (u8_t *)rmem + MIN_SIZE_ALIGNED > (u8_t *)ram_end) {
    LWIP_MEM_ILLEGAL_FREE("mem_free: illegal memory");
    LWIP_DEBUGF(MEM_DEBUG | LWIP_DBG_LEVEL_SEVERE, ("mem_free: illegal memory\n"));
    /* protect mem stats from concurrent access */
    MEM_STATS_INC_LOCKED(illegal);
    return;
  }
  /* protect the heap from concurrent access */
  LWIP_MEM_FREE_PROTECT();
  /* mem has to be in a used state */
  if (!mem->used) {
    LWIP_MEM_ILLEGAL_FREE("mem_free: illegal memory: double free");
    LWIP_MEM_FREE_UNPROTECT();
    LWIP_DEBUGF(MEM_DEBUG | LWIP_DBG_LEVEL_SEVERE, ("mem_free: illegal memory: double free?\n"));
    /* protect mem stats from concurrent access */
    MEM_STATS_INC_LOCKED(illegal);
    return;
  }

  if (!mem_link_valid(mem)) {
    LWIP_MEM_ILLEGAL_FREE("mem_free: illegal memory: non-linked: double free");
    LWIP_MEM_FREE_UNPROTECT();
    LWIP_DEBUGF(MEM_DEBUG | LWIP_DBG_LEVEL_SEVERE, ("mem_free: illegal memory: non-linked: double free?\n"));
    /* protect mem stats from concurrent access */
    MEM_STATS_INC_LOCKED(illegal);
    return;
  }

  /* mem is now unused. */
  mem->used = 0;

  if (mem < lfree) {
    /* the newly freed struct is now the lowest */
    lfree = mem;
  }

  MEM_STATS_DEC_USED(used, mem->next - (mem_size_t)(((u8_t *)mem - ram)));

  /* finally, see if prev or next are free also */
  plug_holes(mem);
#if LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT
  mem_free_count = 1;
#endif /* LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT */
  LWIP_MEM_FREE_UNPROTECT();
}

/**
 * Shrink memory returned by mem_malloc().
 *
 * @param rmem pointer to memory allocated by mem_malloc the is to be shrinked
 * @param new_size required size after shrinking (needs to be smaller than or
 *                equal to the previous size)
 * @return for compatibility reasons: is always == rmem, at the moment
 *         or NULL if newsize is > old size, in which case rmem is NOT touched
 *         or freed!
 */
void *
mem_trim(void *rmem, mem_size_t new_size)
{
  mem_size_t size, newsize;
  mem_size_t ptr, ptr2;
  struct mem *mem, *mem2;
  /* use the FREE_PROTECT here: it protects with sem OR SYS_ARCH_PROTECT */
  LWIP_MEM_FREE_DECL_PROTECT();

  /* Expand the size of the allocated memory region so that we can
     adjust for alignment. */
  newsize = (mem_size_t)LWIP_MEM_ALIGN_SIZE(new_size);
  if ((newsize > MEM_SIZE_ALIGNED) || (newsize < new_size)) {
    return NULL;
  }

  if (newsize < MIN_SIZE_ALIGNED) {
    /* every data block must be at least MIN_SIZE_ALIGNED long */
    newsize = MIN_SIZE_ALIGNED;
  }

  LWIP_ASSERT("mem_trim: legal memory", (u8_t *)rmem >= (u8_t *)ram &&
              (u8_t *)rmem < (u8_t *)ram_end);

  if ((u8_t *)rmem < (u8_t *)ram || (u8_t *)rmem >= (u8_t *)ram_end) {
    LWIP_DEBUGF(MEM_DEBUG | LWIP_DBG_LEVEL_SEVERE, ("mem_trim: illegal memory\n"));
    /* protect mem stats from concurrent access */
    MEM_STATS_INC_LOCKED(illegal);
    return rmem;
  }
  /* Get the corresponding struct mem ... */
  /* cast through void* to get rid of alignment warnings */
  mem = (struct mem *)(void *)((u8_t *)rmem - SIZEOF_STRUCT_MEM);
  /* ... and its offset pointer */
  ptr = (mem_size_t)((u8_t *)mem - ram);

  size = (mem_size_t)((mem_size_t)(mem->next - ptr) - SIZEOF_STRUCT_MEM);
  LWIP_ASSERT("mem_trim can only shrink memory", newsize <= size);
  if (newsize > size) {
    /* not supported */
    return NULL;
  }
  if (newsize == size) {
    /* No change in size, simply return */
    return rmem;
  }

  /* protect the heap from concurrent access */
  LWIP_MEM_FREE_PROTECT();

  mem2 = (struct mem *)(void *)&ram[mem->next];
  if (mem2->used == 0) {
    /* The next struct is unused, we can simply move it at little */
    mem_size_t next;
    /* remember the old next pointer */
    next = mem2->next;
    /* create new struct mem which is moved directly after the shrinked mem */
    ptr2 = (mem_size_t)(ptr + SIZEOF_STRUCT_MEM + newsize);
    if (lfree == mem2) {
      lfree = (struct mem *)(void *)&ram[ptr2];
    }
    mem2 = (struct mem *)(void *)&ram[ptr2];
    mem2->used = 0;
    /* restore the next pointer */
    mem2->next = next;
    /* link it back to mem */
    mem2->prev = ptr;
    /* link mem to it */
    mem->next = ptr2;
    /* last thing to restore linked list: as we have moved mem2,
     * let 'mem2->next->prev' point to mem2 again. but only if mem2->next is not
     * the end of the heap */
    if (mem2->next != MEM_SIZE_ALIGNED) {
      ((struct mem *)(void *)&ram[mem2->next])->prev = ptr2;
    }
    MEM_STATS_DEC_USED(used, (size - newsize));
    /* no need to plug holes, we've already done that */
  } else if (newsize + SIZEOF_STRUCT_MEM + MIN_SIZE_ALIGNED <= size) {
    /* Next struct is used but there's room for another struct mem with
     * at least MIN_SIZE_ALIGNED of data.
     * Old size ('size') must be big enough to contain at least 'newsize' plus a struct mem
     * ('SIZEOF_STRUCT_MEM') with some data ('MIN_SIZE_ALIGNED').
     * @todo we could leave out MIN_SIZE_ALIGNED. We would create an empty
     *       region that couldn't hold data, but when mem->next gets freed,
     *       the 2 regions would be combined, resulting in more free memory */
    ptr2 = (mem_size_t)(ptr + SIZEOF_STRUCT_MEM + newsize);
    mem2 = (struct mem *)(void *)&ram[ptr2];
    if (mem2 < lfree) {
      lfree = mem2;
    }
    mem2->used = 0;
    mem2->next = mem->next;
    mem2->prev = ptr;
    mem->next = ptr2;
    if (mem2->next != MEM_SIZE_ALIGNED) {
      ((struct mem *)(void *)&ram[mem2->next])->prev = ptr2;
    }
    MEM_STATS_DEC_USED(used, (size - newsize));
    /* the original mem->next is used, so no need to plug holes! */
  }
  /* else {
    next struct mem is used but size between mem and mem2 is not big enough
    to create another struct mem
    -> don't do anyhting.
    -> the remaining space stays unused since it is too small
  } */
#if LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT
  mem_free_count = 1;
#endif /* LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT */
  LWIP_MEM_FREE_UNPROTECT();
  return rmem;
}

/**
 * Allocate a block of memory with a minimum of 'size' bytes.
 *
 * @param size_in is the minimum size of the requested block in bytes.
 * @return pointer to allocated memory or NULL if no free memory was found.
 *
 * Note that the returned value will always be aligned (as defined by MEM_ALIGNMENT).
 */
void *
mem_malloc(mem_size_t size_in)
{
  mem_size_t ptr, ptr2, size;
  struct mem *mem, *mem2;
#if LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT
  u8_t local_mem_free_count = 0;
#endif /* LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT */
  LWIP_MEM_ALLOC_DECL_PROTECT();

  if (size_in == 0) {
    return NULL;
  }

  /* Expand the size of the allocated memory region so that we can
     adjust for alignment. */
  size = (mem_size_t)LWIP_MEM_ALIGN_SIZE(size_in);
  if ((size > MEM_SIZE_ALIGNED) ||
      (size < size_in)) {
    return NULL;
  }

  if (size < MIN_SIZE_ALIGNED) {
    /* every data block must be at least MIN_SIZE_ALIGNED long */
    size = MIN_SIZE_ALIGNED;
  }

  /* protect the heap from concurrent access */
  sys_mutex_lock(&mem_mutex);
  LWIP_MEM_ALLOC_PROTECT();
#if LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT
  /* run as long as a mem_free disturbed mem_malloc or mem_trim */
  do {
    local_mem_free_count = 0;
#endif /* LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT */

    /* Scan through the heap searching for a free block that is big enough,
     * beginning with the lowest free block.
     */
    for (ptr = (mem_size_t)((u8_t *)lfree - ram); ptr < MEM_SIZE_ALIGNED - size;
         ptr = ((struct mem *)(void *)&ram[ptr])->next) {
      mem = (struct mem *)(void *)&ram[ptr];
#if LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT
      mem_free_count = 0;
      LWIP_MEM_ALLOC_UNPROTECT();
      /* allow mem_free or mem_trim to run */
      LWIP_MEM_ALLOC_PROTECT();
      if (mem_free_count != 0) {
        /* If mem_free or mem_trim have run, we have to restart since they
           could have altered our current struct mem. */
        local_mem_free_count = 1;
        break;
      }
#endif /* LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT */

      if ((!mem->used) &&
          (mem->next - (ptr + SIZEOF_STRUCT_MEM)) >= size) {
        /* mem is not used and at least perfect fit is possible:
         * mem->next - (ptr + SIZEOF_STRUCT_MEM) gives us the 'user data size' of mem */

        if (mem->next - (ptr + SIZEOF_STRUCT_MEM) >= (size + SIZEOF_STRUCT_MEM + MIN_SIZE_ALIGNED)) {
          /* (in addition to the above, we test if another struct mem (SIZEOF_STRUCT_MEM) containing
           * at least MIN_SIZE_ALIGNED of data also fits in the 'user data space' of 'mem')
           * -> split large block, create empty remainder,
           * remainder must be large enough to contain MIN_SIZE_ALIGNED data: if
           * mem->next - (ptr + (2*SIZEOF_STRUCT_MEM)) == size,
           * struct mem would fit in but no data between mem2 and mem2->next
           * @todo we could leave out MIN_SIZE_ALIGNED. We would create an empty
           *       region that couldn't hold data, but when mem->next gets freed,
           *       the 2 regions would be combined, resulting in more free memory
           */
          ptr2 = (mem_size_t)(ptr + SIZEOF_STRUCT_MEM + size);
          /* create mem2 struct */
          mem2 = (struct mem *)(void *)&ram[ptr2];
          mem2->used = 0;
          mem2->next = mem->next;
          mem2->prev = ptr;
          /* and insert it between mem and mem->next */
          mem->next = ptr2;
          mem->used = 1;

          if (mem2->next != MEM_SIZE_ALIGNED) {
            ((struct mem *)(void *)&ram[mem2->next])->prev = ptr2;
          }
          MEM_STATS_INC_USED(used, (size + SIZEOF_STRUCT_MEM));
        } else {
          /* (a mem2 struct does no fit into the user data space of mem and mem->next will always
           * be used at this point: if not we have 2 unused structs in a row, plug_holes should have
           * take care of this).
           * -> near fit or exact fit: do not split, no mem2 creation
           * also can't move mem->next directly behind mem, since mem->next
           * will always be used at this point!
           */
          mem->used = 1;
          MEM_STATS_INC_USED(used, mem->next - (mem_size_t)((u8_t *)mem - ram));
        }
#if LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT
mem_malloc_adjust_lfree:
#endif /* LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT */
        if (mem == lfree) {
          struct mem *cur = lfree;
          /* Find next free block after mem and update lowest free pointer */
          while (cur->used && cur != ram_end) {
#if LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT
            mem_free_count = 0;
            LWIP_MEM_ALLOC_UNPROTECT();
            /* prevent high interrupt latency... */
            LWIP_MEM_ALLOC_PROTECT();
            if (mem_free_count != 0) {
              /* If mem_free or mem_trim have run, we have to restart since they
                 could have altered our current struct mem or lfree. */
              goto mem_malloc_adjust_lfree;
            }
#endif /* LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT */
            cur = (struct mem *)(void *)&ram[cur->next];
          }
          lfree = cur;
          LWIP_ASSERT("mem_malloc: !lfree->used", ((lfree == ram_end) || (!lfree->used)));
        }
        LWIP_MEM_ALLOC_UNPROTECT();
        sys_mutex_unlock(&mem_mutex);
        LWIP_ASSERT("mem_malloc: allocated memory not above ram_end.",
                    (mem_ptr_t)mem + SIZEOF_STRUCT_MEM + size <= (mem_ptr_t)ram_end);
        LWIP_ASSERT("mem_malloc: allocated memory properly aligned.",
                    ((mem_ptr_t)mem + SIZEOF_STRUCT_MEM) % MEM_ALIGNMENT == 0);
        LWIP_ASSERT("mem_malloc: sanity check alignment",
                    (((mem_ptr_t)mem) & (MEM_ALIGNMENT - 1)) == 0);

        return (u8_t *)mem + SIZEOF_STRUCT_MEM;
      }
    }
#if LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT
    /* if we got interrupted by a mem_free, try again */
  } while (local_mem_free_count != 0);
#endif /* LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT */
  MEM_STATS_INC(err);
  LWIP_MEM_ALLOC_UNPROTECT();
  sys_mutex_unlock(&mem_mutex);
  LWIP_DEBUGF(MEM_DEBUG | LWIP_DBG_LEVEL_SERIOUS, ("mem_malloc: could not allocate %"S16_F" bytes\n", (s16_t)size));
  return NULL;
}

#endif /* MEM_USE_POOLS */

#if MEM_LIBC_MALLOC && (!LWIP_STATS || !MEM_STATS)
void *
mem_calloc(mem_size_t count, mem_size_t size)
{
  return mem_clib_calloc(count, size);
}

#else /* MEM_LIBC_MALLOC && (!LWIP_STATS || !MEM_STATS) */
/**
 * Contiguously allocates enough space for count objects that are size bytes
 * of memory each and returns a pointer to the allocated memory.
 *
 * The allocated memory is filled with bytes of value zero.
 *
 * @param count number of objects to allocate
 * @param size size of the objects to allocate
 * @return pointer to allocated memory / NULL pointer if there is an error
 */
void *
mem_calloc(mem_size_t count, mem_size_t size)
{
  void *p;
  size_t alloc_size = (size_t)count * (size_t)size;

  if ((size_t)(mem_size_t)alloc_size != alloc_size) {
    LWIP_DEBUGF(MEM_DEBUG | LWIP_DBG_LEVEL_SERIOUS, ("mem_calloc: could not allocate %"SZT_F" bytes\n", alloc_size));
    return NULL;
  }

  /* allocate 'count' objects of size 'size' */
  p = mem_malloc((mem_size_t)alloc_size);
  if (p) {
    /* zero the memory */
    memset(p, 0, alloc_size);
  }
  return p;
}
#endif /* MEM_LIBC_MALLOC && (!LWIP_STATS || !MEM_STATS) */
