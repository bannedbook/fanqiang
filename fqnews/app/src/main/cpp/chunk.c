/*
Copyright (c) 2003-2006 by Juliusz Chroboczek

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "polipo.h"

#define MB (1024 * 1024)
int chunkLowMark = 0, 
    chunkCriticalMark = 0,
    chunkHighMark = 0;

void
preinitChunks()
{
    CONFIG_VARIABLE(chunkLowMark, CONFIG_INT,
                    "Low mark for chunk memory (0 = auto).");
    CONFIG_VARIABLE(chunkCriticalMark, CONFIG_INT,
                    "Critical mark for chunk memory (0 = auto).");
    CONFIG_VARIABLE(chunkHighMark, CONFIG_INT,
                    "High mark for chunk memory.");
}

static void
initChunksCommon()
{
#define ROUND_CHUNKS(a) a = (((unsigned long)(a) + CHUNK_SIZE - 1) / CHUNK_SIZE) * CHUNK_SIZE;
    int q;

    if(CHUNK_SIZE != 1 << log2_ceil(CHUNK_SIZE)) {
        do_log(L_ERROR, "CHUNK SIZE %d is not a power of two.\n", CHUNK_SIZE);
        exit(1);
    }

    ROUND_CHUNKS(chunkHighMark);
    ROUND_CHUNKS(chunkCriticalMark);
    ROUND_CHUNKS(chunkLowMark);

    if(chunkHighMark < 8 * CHUNK_SIZE) {
        int mem = physicalMemory();
        if(mem > 0)
            chunkHighMark = mem / 4;
        else
            chunkHighMark = 24 * MB;
        chunkHighMark = MIN(chunkHighMark, 24 * MB);
        chunkHighMark = MAX(chunkHighMark, 8 * CHUNK_SIZE);
    }

    if(chunkHighMark < MB / 2)
        fprintf(stderr,
                "Warning: little chunk memory (%d bytes)\n", chunkHighMark);

    q = 0;
    if(chunkLowMark <= 0) q = 1;
    if(chunkLowMark < 4 * CHUNK_SIZE ||
       chunkLowMark > chunkHighMark - 4 * CHUNK_SIZE) {
        chunkLowMark = MIN(chunkHighMark - 4 * CHUNK_SIZE,
                           chunkHighMark * 3 / 4);
        ROUND_CHUNKS(chunkLowMark);
        if(!q) do_log(L_WARN, "Inconsistent chunkLowMark -- setting to %d.\n",
                      chunkLowMark);
    }

    q = 0;
    if(chunkCriticalMark <= 0) q = 1;
    if(chunkCriticalMark >= chunkHighMark - 2 * CHUNK_SIZE ||
       chunkCriticalMark <= chunkLowMark + 2 * CHUNK_SIZE) {
        chunkCriticalMark =
            MIN(chunkHighMark - 2 * CHUNK_SIZE,
                chunkLowMark + (chunkHighMark - chunkLowMark) * 15 / 16);
        ROUND_CHUNKS(chunkCriticalMark);
        if(!q) do_log(L_WARN, "Inconsistent chunkCriticalMark -- "
                      "setting to %d.\n", chunkCriticalMark);
    }
#undef ROUND_CHUNKS
}


int used_chunks = 0;

static void
maybe_free_chunks(int arenas, int force)
{
    if(force || used_chunks >= CHUNKS(chunkHighMark)) {
        discardObjects(force, force);
    }

    if(arenas)
        free_chunk_arenas();

    if(used_chunks >= CHUNKS(chunkLowMark) && !objectExpiryScheduled) {
        TimeEventHandlerPtr event;
        event = scheduleTimeEvent(1, discardObjectsHandler, 0, NULL);
        if(event)
            objectExpiryScheduled = 1;
    }
}
    


#ifdef MALLOC_CHUNKS

void
initChunks(void)
{
    do_log(L_WARN, "Warning: using malloc(3) for chunk allocation.\n");
    used_chunks = 0;
    initChunksCommon();
}

void
free_chunk_arenas()
{
    return;
}

void *
get_chunk()
{
    void *chunk;

    if(used_chunks > CHUNKS(chunkHighMark))
        maybe_free_chunks(0, 0);
    if(used_chunks > CHUNKS(chunkHighMark))
        return NULL;
    chunk = malloc(CHUNK_SIZE);
    if(!chunk) {
        maybe_free_chunks(1, 1);
        chunk = malloc(CHUNK_SIZE);
        if(!chunk)
            return NULL;
    }
    used_chunks++;
    return chunk;
}

void *
maybe_get_chunk()
{
    void *chunk;
    if(used_chunks > CHUNKS(chunkHighMark))
        return NULL;
    chunk = malloc(CHUNK_SIZE);
    if(chunk)
        used_chunks++;
    return chunk;
}

void
dispose_chunk(void *chunk)
{
    assert(chunk != NULL);
    free(chunk);
    used_chunks--;
}

void
free_chunks()
{
    return;
}

int
totalChunkArenaSize()
{
    return used_chunks * CHUNK_SIZE;
}
#else

#ifdef WIN32 /*MINGW*/
#define MAP_FAILED NULL
#define getpagesize() (64 * 1024)
static void *
alloc_arena(size_t size)
{
    return VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}
static int
free_arena(void *addr, size_t size)
{
    int rc;
    rc = VirtualFree(addr, size, MEM_RELEASE);
    if(!rc)
        rc = -1;
    return rc;
}
#else
#ifndef MAP_FAILED
#define MAP_FAILED ((void*)((long int)-1))
#endif
static void *
alloc_arena(size_t size)
{
    return mmap(NULL, size, PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}
static int
free_arena(void *addr, size_t size)
{
    return munmap(addr, size);
}
#endif

/* Memory is organised into a number of chunks of ARENA_CHUNKS chunks
   each.  Every arena is pointed at by a struct _ChunkArena. */
/* If currentArena is not NULL, it points at the last arena used,
   which gives very fast dispose/get sequences. */

#define DEFINE_FFS(type, ffs_name) \
int                           \
ffs_name(type i)              \
{                             \
    int n;                    \
    if(i == 0) return 0;      \
    n = 1;                    \
    while((i & 1) == 0) {     \
        i >>= 1;              \
        n++;                  \
    }                         \
    return n;                 \
}

#if defined(DEFAULT_ARENA_BITMAPS) + defined(LONG_ARENA_BITMAPS) + defined(LONG_LONG_ARENA_BITMAPS) > 1
#error "Multiple sizes of arenas defined"
#endif

#if defined(DEFAULT_ARENA_BITMAPS) + defined(LONG_ARENA_BITMAPS) + defined(LONG_LONG_ARENA_BITMAPS) == 0
#ifdef HAVE_FFSL
/* This gives us 32-bit arena bitmaps on LP32, and 64-bit ones on LP64 */
#define LONG_ARENA_BITMAPS
#else
#define DEFAULT_ARENA_BITMAPS
#endif
#endif

#if defined(DEFAULT_ARENA_BITMAPS)

#ifndef HAVE_FFS
DEFINE_FFS(int, ffs)
#endif
typedef unsigned int ChunkBitmap;
#define BITMAP_FFS(bitmap) (ffs(bitmap))

#elif defined(LONG_ARENA_BITMAPS)

#ifndef HAVE_FFSL
DEFINE_FFS(long, ffsl)
#endif
typedef unsigned long ChunkBitmap;
#define BITMAP_FFS(bitmap) (ffsl(bitmap))

#elif defined(LONG_LONG_ARENA_BITMAPS)

#ifndef HAVE_FFSLL
DEFINE_FFS(long long, ffsll)
#endif
typedef unsigned long long ChunkBitmap;
#define BITMAP_FFS(bitmap) (ffsll(bitmap))

#else

#error "You lose"

#endif

#define ARENA_CHUNKS ((unsigned)sizeof(ChunkBitmap) * 8)
#define EMPTY_BITMAP (~(ChunkBitmap)0)
#define BITMAP_BIT(i) (((ChunkBitmap)1) << (i))

static int pagesize;
typedef struct _ChunkArena {
    ChunkBitmap bitmap;
    char *chunks;
} ChunkArenaRec, *ChunkArenaPtr;

static ChunkArenaPtr chunkArenas, currentArena;
static int numArenas;
#define CHUNK_IN_ARENA(chunk, arena)                                    \
    ((arena)->chunks &&                                                 \
     (char*)(chunk) >= (arena)->chunks &&                               \
     (char*)(chunk) < (arena)->chunks + (ARENA_CHUNKS * CHUNK_SIZE))

#define CHUNK_ARENA_INDEX(chunk, arena)                                 \
    ((unsigned)((unsigned long)(((char*)(chunk) - (arena)->chunks)) /   \
                CHUNK_SIZE))

void
initChunks(void)
{
    int i;
    used_chunks = 0;
    initChunksCommon();
    pagesize = getpagesize();
    if((CHUNK_SIZE * ARENA_CHUNKS) % pagesize != 0) {
        do_log(L_ERROR,
               "The arena size %d (%d x %d) "
               "is not a multiple of the page size %d.\n",
                ARENA_CHUNKS * CHUNK_SIZE, ARENA_CHUNKS, CHUNK_SIZE, pagesize);
        abort();
    }
    numArenas = 
        (CHUNKS(chunkHighMark) + (ARENA_CHUNKS - 1)) / ARENA_CHUNKS;
    chunkArenas = malloc(numArenas * sizeof(ChunkArenaRec));
    if(chunkArenas == NULL) {
        do_log(L_ERROR, "Couldn't allocate chunk arenas.\n");
        exit (1);
    }
    for(i = 0; i < numArenas; i++) {
        chunkArenas[i].bitmap = EMPTY_BITMAP;
        chunkArenas[i].chunks = NULL;
    }
    currentArena = NULL;
}

static ChunkArenaPtr
findArena()
{
    ChunkArenaPtr arena = NULL;
    int i;

    for(i = 0; i < numArenas; i++) {
        arena = &(chunkArenas[i]);
        if(arena->bitmap != 0)
            break;
        else
            arena = NULL;
    }

    assert(arena != NULL);

    if(!arena->chunks) {
        void *p;
        p = alloc_arena(CHUNK_SIZE * ARENA_CHUNKS);
        if(p == MAP_FAILED) {
            do_log_error(L_ERROR, errno, "Couldn't allocate chunk");
            maybe_free_chunks(1, 1);
            return NULL;
        }
        arena->chunks = p;
    }
    return arena;
}

void *
get_chunk()
{
    unsigned i;
    ChunkArenaPtr arena = NULL;

    if(currentArena && currentArena->bitmap != 0) {
        arena = currentArena;
    } else {
        if(used_chunks >= CHUNKS(chunkHighMark))
            maybe_free_chunks(0, 0);

        if(used_chunks >= CHUNKS(chunkHighMark))
            return NULL;
        
        arena = findArena();
        if(!arena)
            return NULL;
        currentArena = arena;
    }
    i = BITMAP_FFS(arena->bitmap) - 1;
    arena->bitmap &= ~BITMAP_BIT(i);
    used_chunks++;
    return arena->chunks + CHUNK_SIZE * i;
}

void *
maybe_get_chunk()
{
    unsigned i;
    ChunkArenaPtr arena = NULL;

    if(currentArena && currentArena->bitmap != 0) {
        arena = currentArena;
    } else {
        if(used_chunks >= CHUNKS(chunkHighMark))
            return NULL;

        arena = findArena();
        if(!arena)
            return NULL;
        currentArena = arena;
    }
    i = BITMAP_FFS(arena->bitmap) - 1;
    arena->bitmap &= ~BITMAP_BIT(i);
    used_chunks++;
    return arena->chunks + CHUNK_SIZE * i;
}

void
dispose_chunk(void *chunk)
{
    ChunkArenaPtr arena = NULL;
    unsigned i;

    assert(chunk != NULL);

    if(currentArena && CHUNK_IN_ARENA(chunk, currentArena)) {
        arena = currentArena;
    } else {
        for(i = 0; i < numArenas; i++) {
            arena = &(chunkArenas[i]);
            if(CHUNK_IN_ARENA(chunk, arena))
                break;
        }
        assert(arena != NULL);
        currentArena = arena;
    }

    i = CHUNK_ARENA_INDEX(chunk, arena);
    arena->bitmap |= BITMAP_BIT(i);
    used_chunks--;
}

void
free_chunk_arenas()
{
    ChunkArenaPtr arena;
    int i, rc;

    for(i = 0; i < numArenas; i++) {
        arena = &(chunkArenas[i]);
        if(arena->bitmap == EMPTY_BITMAP && arena->chunks) {
            rc = free_arena(arena->chunks, CHUNK_SIZE * ARENA_CHUNKS);
            if(rc < 0) {
                do_log_error(L_ERROR, errno, "Couldn't unmap memory");
                continue;
            }
            arena->chunks = NULL;
        }
    }
    if(currentArena && currentArena->chunks == NULL)
        currentArena = NULL;
}

int
totalChunkArenaSize()
{
    ChunkArenaPtr arena;
    int i, size = 0;

    for(i = 0; i < numArenas; i++) {
        arena = &(chunkArenas[i]);
        if(arena->chunks)
            size += (CHUNK_SIZE * ARENA_CHUNKS);
    }
    return size;
}
#endif
