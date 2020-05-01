/*
Copyright (c) 2003-2010 by Juliusz Chroboczek

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

extern int maxDiskEntries;

extern AtomPtr diskCacheRoot;
extern AtomPtr additionalDiskCacheRoot;

typedef struct _DiskCacheEntry {
    char *filename;
    ObjectPtr object;
    int fd;
    off_t offset;
    off_t size;
    int body_offset;
    short local;
    short metadataDirty;
    struct _DiskCacheEntry *next;
    struct _DiskCacheEntry *previous;
} *DiskCacheEntryPtr, DiskCacheEntryRec;

typedef struct _DiskObject {
    char *location;
    char *filename;
    int body_offset;
    int length;
    int size;
    time_t age;
    time_t access;
    time_t date;
    time_t last_modified;
    time_t expires;
    struct _DiskObject *next;
} DiskObjectRec, *DiskObjectPtr;

struct stat;

extern int maxDiskCacheEntrySize;

void preinitDiskcache(void);
void initDiskcache(void);
int destroyDiskEntry(ObjectPtr object, int);
int diskEntrySize(ObjectPtr object);
ObjectPtr objectGetFromDisk(ObjectPtr);
int objectFillFromDisk(ObjectPtr object, int offset, int chunks);
int writeoutMetadata(ObjectPtr object);
int writeoutToDisk(ObjectPtr object, int upto, int max);
void dirtyDiskEntry(ObjectPtr object);
int revalidateDiskEntry(ObjectPtr object);
DiskObjectPtr readDiskObject(char *filename, struct stat *sb);
void indexDiskObjects(FILE *out, const char *root, int r);
void expireDiskObjects(void);
