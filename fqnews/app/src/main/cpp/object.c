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

int mindlesslyCacheVary = 0;
int objectHashTableSize = 0;
int log2ObjectHashTableSize;

static ObjectPtr object_list = NULL;
static ObjectPtr object_list_end = NULL;

int objectExpiryScheduled;

int publicObjectCount;
int privateObjectCount;
int cacheIsShared = 1;
int publicObjectLowMark = 0, objectHighMark = 2048;

static ObjectPtr *objectHashTable;
int maxExpiresAge = (30 * 24 + 1) * 3600;
int maxAge = (14 * 24 + 1) * 3600;
float maxAgeFraction = 0.1;
int maxNoModifiedAge = 23 * 60;
int maxWriteoutWhenIdle = 64 * 1024;
int maxObjectsWhenIdle = 32;
int idleTime = 20;
int dontCacheCookies = 0;

void
preinitObject()
{
    CONFIG_VARIABLE_SETTABLE(idleTime, CONFIG_TIME, configIntSetter,
                             "Time to remain idle before writing out.");
    CONFIG_VARIABLE_SETTABLE(maxWriteoutWhenIdle, CONFIG_INT, configIntSetter,
                             "Amount of data to write at a time when idle.");
    CONFIG_VARIABLE_SETTABLE(maxObjectsWhenIdle, CONFIG_INT, configIntSetter,
                             "Number of objects to write at a time "
                             "when idle.");
    CONFIG_VARIABLE_SETTABLE(cacheIsShared, CONFIG_BOOLEAN, configIntSetter,
                             "If false, ignore s-maxage and private.");
    CONFIG_VARIABLE_SETTABLE(mindlesslyCacheVary, CONFIG_BOOLEAN,
                             configIntSetter,
                             "If true, mindlessly cache negotiated objects.");
    CONFIG_VARIABLE(objectHashTableSize, CONFIG_INT,
                    "Size of the object hash table (0 = auto).");
    CONFIG_VARIABLE(objectHighMark, CONFIG_INT,
                    "High object count mark.");
    CONFIG_VARIABLE(publicObjectLowMark, CONFIG_INT,
                    "Low object count mark (0 = auto).");
    CONFIG_VARIABLE_SETTABLE(maxExpiresAge, CONFIG_TIME, configIntSetter,
                             "Max age for objects with Expires header.");
    CONFIG_VARIABLE_SETTABLE(maxAge, CONFIG_TIME, configIntSetter,
                             "Max age for objects without Expires header.");
    CONFIG_VARIABLE_SETTABLE(maxAgeFraction, CONFIG_FLOAT, configFloatSetter,
                             "Fresh fraction of modification time.");
    CONFIG_VARIABLE_SETTABLE(maxNoModifiedAge, CONFIG_TIME, configIntSetter,
                             "Max age for objects without Last-modified.");
    CONFIG_VARIABLE_SETTABLE(dontCacheCookies, CONFIG_BOOLEAN, configIntSetter,
                             "Work around cachable cookies.");
}

void
initObject()
{
    int q;
    if(objectHighMark < 16) {
        objectHighMark = 16;
        do_log(L_WARN, "Impossibly low objectHighMark -- setting to %d.\n",
               objectHighMark);
    }

    q = 0;
    if(publicObjectLowMark == 0) q = 1;
    if(publicObjectLowMark < 8 || publicObjectLowMark >= objectHighMark - 4) {
        publicObjectLowMark = objectHighMark / 2;
        if(!q)
            do_log(L_WARN, "Impossible publicObjectLowMark value -- "
                   "setting to %d.\n", publicObjectLowMark);
    }

    q = 1;
    if(objectHashTableSize <= objectHighMark / 2 ||
       objectHashTableSize > objectHighMark * 1024) {
        if(objectHashTableSize != 0) q = 0;
        objectHashTableSize = objectHighMark * 16;
    }
    log2ObjectHashTableSize = log2_ceil(objectHashTableSize);
    objectHashTableSize = 1 << log2ObjectHashTableSize;
    if(!q)
        do_log(L_WARN, "Suspicious objectHashTableSize value -- "
               "setting to %d.\n", objectHashTableSize);

    object_list = NULL;
    object_list_end = NULL;
    publicObjectCount = 0;
    privateObjectCount = 0;
    objectHashTable = calloc(1 << log2ObjectHashTableSize,
                             sizeof(ObjectPtr));
    if(!objectHashTable) {
        do_log(L_ERROR, "Couldn't allocate object hash table.\n");
        exit(1);
    }
}

ObjectPtr
findObject(int type, const void *key, int key_size)
{
    int h;
    ObjectPtr object;

    if(key_size >= 50000)
        return NULL;

    h = hash(type, key, key_size, log2ObjectHashTableSize);
    object = objectHashTable[h];
    if(!object)
        return NULL;
    if(object->type != type || object->key_size != key_size ||
       memcmp(object->key, key, key_size) != 0) {
        return NULL;
    }
    if(object->next)
        object->next->previous = object->previous;
    if(object->previous)
        object->previous->next = object->next;
    if(object_list == object)
        object_list = object->next;
    if(object_list_end == object)
        object_list_end = object->previous;
    object->previous = NULL;
    object->next = object_list;
    if(object_list)
        object_list->previous = object;
    object_list = object;
    if(!object_list_end)
        object_list_end = object;
    return retainObject(object);
}

ObjectPtr
makeObject(int type, const void *key, int key_size, int public, int fromdisk,
           RequestFunction request, void* request_closure)
{
    ObjectPtr object;
    int h;

    object = findObject(type, key, key_size);
    if(object != NULL) {
        if(public)
            return object;
        else
            privatiseObject(object, 0);
    }

    if(publicObjectCount + privateObjectCount >= objectHighMark) {
        if(!objectExpiryScheduled)
            discardObjects(0, 0);
        if(publicObjectCount + privateObjectCount >= objectHighMark) {
            return NULL;
        }
    }

    if(publicObjectCount >= publicObjectLowMark && 
       !objectExpiryScheduled) {
        TimeEventHandlerPtr event;
        event = scheduleTimeEvent(-1, discardObjectsHandler, 0, NULL);
        if(event)
            objectExpiryScheduled = 1;
        else
            do_log(L_ERROR, "Couldn't schedule object expiry.\n");
    }

    object = malloc(sizeof(ObjectRec));
    if(object == NULL)
        return NULL;

    object->type = type;
    object->request = request;
    object->request_closure = request_closure;
    object->key = malloc(key_size + 1);
    if(object->key == NULL) {
        free(object);
        return NULL;
    }
    memcpy(object->key, key, key_size);
    /* In order to make it more convenient to use keys as strings,
       they are NUL-terminated. */
    object->key[key_size] = '\0';
    object->key_size = key_size;
    object->flags = (public?OBJECT_PUBLIC:0) | OBJECT_INITIAL;
    if(public) {
        h = hash(object->type, object->key, object->key_size, 
                 log2ObjectHashTableSize);
        if(objectHashTable[h]) {
            writeoutToDisk(objectHashTable[h], objectHashTable[h]->size, -1);
            privatiseObject(objectHashTable[h], 0);
            assert(!objectHashTable[h]);
        }
        objectHashTable[h] = object;
        object->next = object_list;
        object->previous = NULL;
        if(object_list)
            object_list->previous = object;
        object_list = object;
        if(!object_list_end)
            object_list_end = object;
    } else {
        object->next = NULL;
        object->previous = NULL;
    }
    object->abort_data = NULL;
    object->code = 0;
    object->message = NULL;
    initCondition(&object->condition);
    object->headers = NULL;
    object->via = NULL;
    object->numchunks = 0;
    object->chunks = NULL;
    object->length = -1;
    object->date = -1;
    object->age = -1;
    object->expires = -1;
    object->last_modified = -1;
    object->atime = -1;
    object->etag = NULL;
    object->cache_control = 0;
    object->max_age = -1;
    object->s_maxage = -1;
    object->size = 0;
    object->requestor = NULL;
    object->disk_entry = NULL;
    if(object->flags & OBJECT_PUBLIC)
        publicObjectCount++;
    else
        privateObjectCount++;
    object->refcount = 1;

    if(public && fromdisk)
        objectGetFromDisk(object);
    return object;
}

void 
objectMetadataChanged(ObjectPtr object, int revalidate)
{
    if(revalidate) {
        revalidateDiskEntry(object);
    } else {
        object->flags &= ~OBJECT_DISK_ENTRY_COMPLETE;
        dirtyDiskEntry(object);
    }
    return;
}

ObjectPtr
retainObject(ObjectPtr object)
{
    do_log(D_REFCOUNT, "O 0x%lx %d++\n",
           (unsigned long)object, object->refcount);
    object->refcount++;
    return object;
}

void
releaseObject(ObjectPtr object)
{
    do_log(D_REFCOUNT, "O 0x%lx %d--\n",
           (unsigned long)object, object->refcount);
    object->refcount--;
    if(object->refcount == 0) {
        assert(!object->condition.handlers && 
               !(object->flags & OBJECT_INPROGRESS));
        if(!(object->flags & OBJECT_PUBLIC))
            destroyObject(object);
    }
}

void
releaseNotifyObject(ObjectPtr object)
{
    do_log(D_REFCOUNT, "O 0x%lx %d--\n",
           (unsigned long)object, object->refcount);
    object->refcount--;
    if(object->refcount > 0) {
        notifyObject(object);
    } else {
        assert(!object->condition.handlers && 
               !(object->flags & OBJECT_INPROGRESS));
        if(!(object->flags & OBJECT_PUBLIC))
            destroyObject(object);
    }
}

void 
lockChunk(ObjectPtr object, int i)
{
    do_log(D_LOCK, "Lock 0x%lx[%d]: ", (unsigned long)object, i);
    assert(i >= 0);
    if(i >= object->numchunks)
        objectSetChunks(object, i + 1);
    object->chunks[i].locked++;
    do_log(D_LOCK, "%d\n", object->chunks[i].locked);
}

void 
unlockChunk(ObjectPtr object, int i)
{
    do_log(D_LOCK, "Unlock 0x%lx[%d]: ", (unsigned long)object, i);
    assert(i >= 0 && i < object->numchunks);
    assert(object->chunks[i].locked > 0);
    object->chunks[i].locked--;
    do_log(D_LOCK, "%d\n", object->chunks[i].locked);
}

int
objectSetChunks(ObjectPtr object, int numchunks)
{
    int n;

    if(numchunks <= object->numchunks)
        return 0;

    if(object->length >= 0)
        n = MAX(numchunks, (object->length + (CHUNK_SIZE - 1)) / CHUNK_SIZE);
    else
        n = MAX(numchunks, 
                MAX(object->numchunks + 2, object->numchunks * 5 / 4));
    
    if(n == 0) {
        assert(object->chunks == NULL);
    } else if(object->numchunks == 0) {
        object->chunks = calloc(n, sizeof(ChunkRec));
        if(object->chunks == NULL) {
            return -1;
        }
        object->numchunks = n;
    } else {
        ChunkPtr newchunks;
        newchunks = realloc(object->chunks, n * sizeof(ChunkRec));
        if(newchunks == NULL)
            return -1;
        memset(newchunks + object->numchunks, 0,
               (n - object->numchunks) * sizeof(ChunkRec));
        object->chunks = newchunks;
        object->numchunks = n;
    }
    return 0;
}

ObjectPtr
objectPartial(ObjectPtr object, int length, struct _Atom *headers)
{
    object->headers = headers;

    if(length >= 0) {
        if(object->size > length) {
            abortObject(object, 502,
                        internAtom("Inconsistent Content-Length"));
            notifyObject(object);
            return object;
        }
    }

    if(length >= 0)
        object->length = length;

    object->flags &= ~OBJECT_INITIAL;
    revalidateDiskEntry(object);
    notifyObject(object);
    return object;
}

static int
objectAddChunk(ObjectPtr object, const char *data, int offset, int plen)
{
    int i = offset / CHUNK_SIZE;
    int rc;

    assert(offset % CHUNK_SIZE == 0);
    assert(plen <= CHUNK_SIZE);

    if(object->numchunks <= i) {
        rc = objectSetChunks(object, i + 1);
        if(rc < 0)
            return -1;
    }

    lockChunk(object, i);

    if(object->chunks[i].data == NULL) {
        object->chunks[i].data = get_chunk();
        if(object->chunks[i].data == NULL)
            goto fail;
    }

    if(object->chunks[i].size >= plen) {
        unlockChunk(object, i);
        return 0;
    }

    if(object->size < offset + plen)
        object->size = offset + plen;
    object->chunks[i].size = plen;
    memcpy(object->chunks[i].data, data, plen);
    unlockChunk(object, i);
    return 0;

 fail:
    unlockChunk(object, i);
    return -1;
}

static int
objectAddChunkEnd(ObjectPtr object, const char *data, int offset, int plen)
{
    int i = offset / CHUNK_SIZE;
    int rc;

    assert(offset % CHUNK_SIZE != 0 && 
           offset % CHUNK_SIZE + plen <= CHUNK_SIZE);

    if(object->numchunks <= i) {
        rc = objectSetChunks(object, i + 1);
        if(rc < 0)
            return -1;
    }

    lockChunk(object, i);

    if(object->chunks[i].data == NULL)
        object->chunks[i].data = get_chunk();
    if(object->chunks[i].data == NULL)
        goto fail;

    if(offset > object->size) {
        goto fail;
    }

    if(object->chunks[i].size < offset % CHUNK_SIZE) {
        goto fail;
    }

    if(object->size < offset + plen)
        object->size = offset + plen;
    object->chunks[i].size = offset % CHUNK_SIZE + plen;
    memcpy(object->chunks[i].data + (offset % CHUNK_SIZE),
           data, plen);

    unlockChunk(object, i);
    return 0;

 fail:
    unlockChunk(object, i);
    return -1;
}

int
objectAddData(ObjectPtr object, const char *data, int offset, int len)
{
    int rc;

    do_log(D_OBJECT_DATA, "Adding data to 0x%lx (%d) at %d: %d bytes\n",
           (unsigned long)object, object->length, offset, len);

    if(len == 0)
        return 1;

    if(object->length >= 0) {
        if(offset + len > object->length) {
            do_log(L_ERROR, 
                   "Inconsistent object length (%d, should be at least %d).\n",
                   object->length, offset + len);
            object->length = offset + len;
        }
    }
            
    object->flags &= ~OBJECT_FAILED;

    if(offset + len >= object->numchunks * CHUNK_SIZE) {
        rc = objectSetChunks(object, (offset + len - 1) / CHUNK_SIZE + 1);
        if(rc < 0) {
            return -1;
        }
    }

    if(offset % CHUNK_SIZE != 0) {
        int plen = CHUNK_SIZE - offset % CHUNK_SIZE;
        if(plen >= len)
            plen = len;
        rc = objectAddChunkEnd(object, data, offset, plen);
        if(rc < 0) {
            return -1;
        }            
        offset += plen;
        data += plen;
        len -= plen;
    }

    while(len > 0) {
        int plen = (len >= CHUNK_SIZE) ? CHUNK_SIZE : len;
        rc = objectAddChunk(object, data, offset, plen);
        if(rc < 0) {
            return -1;
        }
        offset += plen;
        data += plen;
        len -= plen;
    }

    return 1;
}

void
objectPrintf(ObjectPtr object, int offset, const char *format, ...)
{
    char *buf;
    int rc;

    va_list args;
    va_start(args, format);
    buf = vsprintf_a(format, args);
    va_end(args);

    if(buf == NULL) {
        abortObject(object, 500, internAtom("Couldn't allocate string"));
        return;
    }

    rc = objectAddData(object, buf, offset, strlen(buf));
    free(buf);
    if(rc < 0)
        abortObject(object, 500, internAtom("Couldn't add data to object"));
}

int 
objectHoleSize(ObjectPtr object, int offset)
{
    int size = 0, i;

    if(offset < 0 || offset / CHUNK_SIZE >= object->numchunks)
        return -1;

    if(offset % CHUNK_SIZE != 0) {
        if(object->chunks[offset / CHUNK_SIZE].size > offset % CHUNK_SIZE)
            return 0;
        else {
            size += CHUNK_SIZE - offset % CHUNK_SIZE;
            offset += CHUNK_SIZE - offset % CHUNK_SIZE;
            if(offset < 0) {
                /* Overflow */
                return -1;
            }
        }
    }

    for(i = offset / CHUNK_SIZE; i < object->numchunks; i++) {
        if(object->chunks[i].size == 0)
            size += CHUNK_SIZE;
        else
            break;
    }
    if(i >= object->numchunks)
        return -1;
    return size;
}


/* Returns 2 if the data is wholly in memory, 1 if it's available on disk.
   If the client request was a Range request, from & to specify the requested
   range; otherwise 'from' is 0 and 'to' is -1. */
int
objectHasData(ObjectPtr object, int from, int to)
{
    int first, last, i, upto;

    if(to < 0) {
        if(object->length >= 0)
            to = object->length;
        else
            return 0;
    }

    first = from / CHUNK_SIZE;
    last = to / CHUNK_SIZE;

    if(from >= to)
        return 2;

    if(to > object->size) {
        upto = to;
        goto disk;
    }

    if(last > object->numchunks ||
       object->chunks[last].size > to % CHUNK_SIZE) {
        upto = to;
        goto disk;
    }

    for(i = last - 1; i >= first; i--) {
        if(object->chunks[i].size < CHUNK_SIZE) {
            upto = (i + 1) * CHUNK_SIZE;
            goto disk;
        }
    }

    return 2;

 disk:
    if(object->flags & OBJECT_DISK_ENTRY_COMPLETE)
        return 1;

    if(diskEntrySize(object) >= upto)
        return 1;

    return 0;
}

void
destroyObject(ObjectPtr object)
{
    int i;

    assert(object->refcount == 0 && !object->requestor);
    assert(!object->condition.handlers && 
           (object->flags & OBJECT_INPROGRESS) == 0);

    if(object->disk_entry)
        destroyDiskEntry(object, 0);

    if(object->flags & OBJECT_PUBLIC) {
        privatiseObject(object, 0);
    } else {
        object->type = -1;
        if(object->message) releaseAtom(object->message);
        if(object->key) free(object->key);
        if(object->headers) releaseAtom(object->headers);
        if(object->etag) free(object->etag);
        if(object->via) releaseAtom(object->via);
        for(i = 0; i < object->numchunks; i++) {
            assert(!object->chunks[i].locked);
            if(object->chunks[i].data)
                dispose_chunk(object->chunks[i].data);
            object->chunks[i].data = NULL;
            object->chunks[i].size = 0;
        }
        if(object->chunks) free(object->chunks);
        privateObjectCount--;
        free(object);
    }
}

void
privatiseObject(ObjectPtr object, int linear) 
{
    int i, h;
    if(!(object->flags & OBJECT_PUBLIC)) {
        if(linear)
            object->flags |= OBJECT_LINEAR;
        return;
    }

    if(object->disk_entry)
        destroyDiskEntry(object, 0);
    object->flags &= ~OBJECT_PUBLIC;

    for(i = 0; i < object->numchunks; i++) {
        if(object->chunks[i].locked)
            break;
        if(object->chunks[i].data) {
            object->chunks[i].size = 0;
            dispose_chunk(object->chunks[i].data);
            object->chunks[i].data = NULL;
        }
    }

    h = hash(object->type, object->key, object->key_size, 
             log2ObjectHashTableSize);
    assert(objectHashTable[h] == object);
    objectHashTable[h] = NULL;

    if(object->previous)
        object->previous->next = object->next;
    if(object_list == object)
        object_list = object->next;
    if(object->next)
        object->next->previous = object->previous;
    if(object_list_end == object)
        object_list_end = object->previous;
    object->previous = NULL;
    object->next = NULL;

    publicObjectCount--;
    privateObjectCount++;

    if(object->refcount == 0)
        destroyObject(object);
    else {
        if(linear)
            object->flags |= OBJECT_LINEAR;
    }
}

void
abortObject(ObjectPtr object, int code, AtomPtr message)
{
    int i;

    assert(code != 0);

    object->flags &= ~(OBJECT_INITIAL | OBJECT_VALIDATING);
    object->flags |= OBJECT_ABORTED;
    object->code = code;
    if(object->message) releaseAtom(object->message);
    object->message = message;
    object->length = 0;
    object->date = object->age;
    object->expires = object->age;
    object->last_modified = -1;
    if(object->etag) free(object->etag);
    object->etag = NULL;
    if(object->headers) releaseAtom(object->headers); 
    object->headers = NULL;
    object->size = 0;
    for(i = 0; i < object->numchunks; i++) {
        if(object->chunks[i].data) {
            if(!object->chunks[i].locked) {
                dispose_chunk(object->chunks[i].data);
                object->chunks[i].data = NULL;
                object->chunks[i].size = 0;
            }
        }
    }
    privatiseObject(object, 0);
}

void 
supersedeObject(ObjectPtr object)
{
    object->flags |= OBJECT_SUPERSEDED;
    destroyDiskEntry(object, 1);
    privatiseObject(object, 0);
    notifyObject(object);
}

void
notifyObject(ObjectPtr object) 
{
    retainObject(object);
    signalCondition(&object->condition);
    releaseObject(object);
}

int
discardObjectsHandler(TimeEventHandlerPtr event)
{
    return discardObjects(0, 0);
}

void
writeoutObjects(int all)
{
    ObjectPtr object = object_list;
    int bytes;
    int objects;
    int n;

    if(diskIsClean) return;

    objects = 0;
    bytes = 0;
    while(object) {
        do {
            if(!all) {
                if(objects >= maxObjectsWhenIdle || 
                   bytes >= maxWriteoutWhenIdle) {
                    if(workToDo()) return;
                    objects = 0;
                    bytes = 0;
                }
            }
            n = writeoutToDisk(object, -1, all ? -1 : maxWriteoutWhenIdle);
            bytes += n;
        } while(!all && n == maxWriteoutWhenIdle);
        objects++;
        object = object->next;
    }
    diskIsClean = 1;
}

int
discardObjects(int all, int force)
{
    ObjectPtr object;
    int i;
    static int in_discardObjects = 0;
    TimeEventHandlerPtr event;

    if(in_discardObjects)
        return 0;

    in_discardObjects = 1;
    
    if(all || force || used_chunks >= CHUNKS(chunkHighMark) ||
       publicObjectCount >= publicObjectLowMark ||
       publicObjectCount + privateObjectCount >= objectHighMark) {
        object = object_list_end;
        while(object && 
              (all || force || used_chunks >= CHUNKS(chunkLowMark))) {
            if(force || ((object->flags & OBJECT_PUBLIC) &&
                         object->numchunks > CHUNKS(chunkLowMark) / 4)) {
                int j;
                for(j = 0; j < object->numchunks; j++) {
                    if(object->chunks[j].locked) {
                        break;
                    }
                    if(object->chunks[j].size < CHUNK_SIZE) {
                        continue;
                    }
                    writeoutToDisk(object, (j + 1) * CHUNK_SIZE, -1);
                    dispose_chunk(object->chunks[j].data);
                    object->chunks[j].data = NULL;
                    object->chunks[j].size = 0;
                }
            }
            object = object->previous;
        }
        
        i = 0;
        object = object_list_end;
        while(object && 
              (all || force ||
               used_chunks - i > CHUNKS(chunkLowMark) ||
               used_chunks > CHUNKS(chunkCriticalMark) ||
               publicObjectCount > publicObjectLowMark)) {
            ObjectPtr next_object = object->previous;
            if(object->refcount == 0) {
                i += object->numchunks;
                writeoutToDisk(object, object->size, -1);
                privatiseObject(object, 0);
            } else if(all || force) {
                writeoutToDisk(object, object->size, -1);
                destroyDiskEntry(object, 0);
            }
            object = next_object;
        }

        object = object_list_end;
        if(force || used_chunks > CHUNKS(chunkCriticalMark)) {
            if(used_chunks > CHUNKS(chunkCriticalMark)) {
                do_log(L_WARN, 
                       "Short on chunk memory -- "
                       "attempting to punch holes "
                       "in the middle of objects.\n");
            }
            while(object && 
                  (force || used_chunks > CHUNKS(chunkCriticalMark))) {
                if(force || (object->flags & OBJECT_PUBLIC)) {
                    int j;
                    for(j = object->numchunks - 1; j >= 0; j--) {
                        if(object->chunks[j].locked)
                            continue;
                        if(object->chunks[j].size < CHUNK_SIZE)
                            continue;
                        writeoutToDisk(object, (j + 1) * CHUNK_SIZE, -1);
                        dispose_chunk(object->chunks[j].data);
                        object->chunks[j].data = NULL;
                        object->chunks[j].size = 0;
                    }
                }
                object = object->previous;
            }
        }
        event = scheduleTimeEvent(2, discardObjectsHandler, 0, NULL);
        if(event) {
            objectExpiryScheduled = 1;
        } else {
            objectExpiryScheduled = 0;
            do_log(L_ERROR, "Couldn't schedule object expiry.\n");
        }
    } else {
        objectExpiryScheduled = 0;
    }

    if(all) {
        if(privateObjectCount + publicObjectCount != 0) {
            do_log(L_WARN,
                   "Discarded all objects, "
                   "%d + %d objects left (%d chunks and %d atoms used).\n",
                   publicObjectCount, privateObjectCount,
                   used_chunks, used_atoms);
        } else if(used_chunks != 0) {
            do_log(L_WARN,
                   "Discarded all objects, "
                   "%d chunks and %d atoms left.\n",
                   used_chunks, used_atoms);
        }
        diskIsClean = 1;
    }

    in_discardObjects = 0;
    return 1;
}

CacheControlRec no_cache_control = {0, -1, -1, -1, -1};

int
objectIsStale(ObjectPtr object, CacheControlPtr cache_control)
{
    int stale = 0x7FFFFFFF;
    int flags;
    int max_age, s_maxage;
    time_t date;

    if(object->flags & OBJECT_INITIAL)
        return 0;

    if(object->date >= 0)
        date = object->date;
    else if(object->age >= 0)
        date = object->age;
    else
        date = current_time.tv_sec;

    if(cache_control == NULL)
        cache_control = &no_cache_control;
    flags = object->cache_control | cache_control->flags;

    if(cache_control->max_age >= 0) {
        if(object->max_age >= 0)
            max_age = MIN(cache_control->max_age, object->max_age);
        else
            max_age = cache_control->max_age;
    } else
        max_age = object->max_age;

    if(cache_control->s_maxage >= 0) {
        if(object->s_maxage >= 0)
            s_maxage = MIN(cache_control->s_maxage, object->s_maxage);
        else
            s_maxage = cache_control->s_maxage;
    } else
        s_maxage = object->s_maxage;
    
    if(max_age >= 0)
        stale = MIN(stale, object->age + max_age);

    if(cacheIsShared && s_maxage >= 0)
        stale = MIN(stale, object->age + s_maxage);

    if(object->expires >= 0 || object->max_age >= 0)
        stale = MIN(stale, object->age + maxExpiresAge);
    else
        stale = MIN(stale, object->age + maxAge);

    /* RFC 2616 14.9.3: server-side max-age overrides expires */

    if(object->expires >= 0 && object->max_age < 0) {
        /* This protects against clock skew */
        stale = MIN(stale, object->age + object->expires - date);
    }

    if(object->expires < 0 && object->max_age < 0) {
        /* No server-side information -- heuristic expiration */
        if(object->last_modified >= 0)
            /* Again, take care of clock skew */
            stale = MIN(stale,
                        object->age +
                        (date - object->last_modified) * maxAgeFraction);
        else
            stale = MIN(stale, object->age + maxNoModifiedAge);
    }

    if(!(flags & CACHE_MUST_REVALIDATE) &&
       !(cacheIsShared && (flags & CACHE_PROXY_REVALIDATE))) {
        /* Client side can relax transparency */
        if(cache_control->min_fresh >= 0) {
            if(cache_control->max_stale >= 0)
                stale = MIN(stale - cache_control->min_fresh,
                            stale + cache_control->max_stale);
            else
                stale = stale - cache_control->min_fresh;
        } else if(cache_control->max_stale >= 0) {
            stale = stale + cache_control->max_stale;
        }
    }

    return current_time.tv_sec > stale;
}

int
objectMustRevalidate(ObjectPtr object, CacheControlPtr cache_control)
{
    int flags;

    if(cache_control == NULL)
        cache_control = &no_cache_control;
    if(object)
        flags = object->cache_control | cache_control->flags;
    else
        flags = cache_control->flags;
    
    if(flags & (CACHE_NO | CACHE_NO_HIDDEN | CACHE_NO_STORE))
        return 1;

    if(cacheIsShared && (flags & CACHE_PRIVATE))
        return 1;

    if(!mindlesslyCacheVary && (flags & CACHE_VARY))
        return 1;

    if(dontCacheCookies && (flags & CACHE_COOKIE))
        return 1;

    if(object)
        return objectIsStale(object, cache_control);

    return 0;
}

