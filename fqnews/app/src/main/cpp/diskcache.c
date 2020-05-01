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

#include "polipo.h"

#ifndef NO_DISK_CACHE

#include "md5import.h"

int maxDiskEntries = 32;

/* Because the functions in this file can be called during object
   expiry, we cannot use get_chunk. */

AtomPtr diskCacheRoot;
AtomPtr localDocumentRoot;

DiskCacheEntryPtr diskEntries = NULL, diskEntriesLast = NULL;
int numDiskEntries = 0;
int diskCacheDirectoryPermissions = 0700;
int diskCacheFilePermissions = 0600;
int diskCacheWriteoutOnClose = (64 * 1024);

int maxDiskCacheEntrySize = -1;

int diskCacheUnlinkTime = 32 * 24 * 60 * 60;
int diskCacheTruncateTime = 4 * 24 * 60 * 60 + 12 * 60 * 60;
int diskCacheTruncateSize =  1024 * 1024;
int preciseExpiry = 0;

static DiskCacheEntryRec negativeEntry = {
    NULL, NULL,
    -1, -1, -1, -1, 0, 0, NULL, NULL
};

#ifndef LOCAL_ROOT
#define LOCAL_ROOT "/usr/share/polipo/www/"
#endif

#ifndef DISK_CACHE_ROOT
#define DISK_CACHE_ROOT "/var/cache/polipo/"
#endif

static int maxDiskEntriesSetter(ConfigVariablePtr, void*);
static int atomSetterFlush(ConfigVariablePtr, void*);
static int reallyWriteoutToDisk(ObjectPtr object, int upto, int max);

void 
preinitDiskcache()
{
    diskCacheRoot = internAtom(DISK_CACHE_ROOT);
    localDocumentRoot = internAtom(LOCAL_ROOT);

    CONFIG_VARIABLE_SETTABLE(diskCacheDirectoryPermissions, CONFIG_OCTAL,
                             configIntSetter,
                             "Access rights for new directories.");
    CONFIG_VARIABLE_SETTABLE(diskCacheFilePermissions, CONFIG_OCTAL,
                             configIntSetter,
                             "Access rights for new cache files.");
    CONFIG_VARIABLE_SETTABLE(diskCacheWriteoutOnClose, CONFIG_INT,
                             configIntSetter,
                             "Number of bytes to write out eagerly.");
    CONFIG_VARIABLE_SETTABLE(diskCacheRoot, CONFIG_ATOM, atomSetterFlush,
                             "Root of the disk cache.");
    CONFIG_VARIABLE_SETTABLE(localDocumentRoot, CONFIG_ATOM, atomSetterFlush,
                             "Root of the local tree.");
    CONFIG_VARIABLE_SETTABLE(maxDiskEntries, CONFIG_INT, maxDiskEntriesSetter,
                    "File descriptors used by the on-disk cache.");
    CONFIG_VARIABLE(diskCacheUnlinkTime, CONFIG_TIME,
                    "Time after which on-disk objects are removed.");
    CONFIG_VARIABLE(diskCacheTruncateTime, CONFIG_TIME,
                    "Time after which on-disk objects are truncated.");
    CONFIG_VARIABLE(diskCacheTruncateSize, CONFIG_INT, 
                    "Size to which on-disk objects are truncated.");
    CONFIG_VARIABLE(preciseExpiry, CONFIG_BOOLEAN,
                    "Whether to consider all files for purging.");
    CONFIG_VARIABLE_SETTABLE(maxDiskCacheEntrySize, CONFIG_INT,
                             configIntSetter,
                             "Maximum size of objects cached on disk.");
}

static int
maxDiskEntriesSetter(ConfigVariablePtr var, void *value)
{
    int i;
    assert(var->type == CONFIG_INT && var->value.i == &maxDiskEntries);
    i = *(int*)value;
    if(i < 0 || i > 1000000)
        return -3;
    maxDiskEntries = i;
    while(numDiskEntries > maxDiskEntries)
        destroyDiskEntry(diskEntriesLast->object, 0);
    return 1;
}

static int
atomSetterFlush(ConfigVariablePtr var, void *value)
{
    discardObjects(1, 0);
    return configAtomSetter(var, value);
}

static int
checkRoot(AtomPtr root)
{
    struct stat ss;
    int rc;

    if(!root || root->length == 0)
        return 0;

#ifdef WIN32  /* Require "x:/" or "x:\\" */
    rc = isalpha(root->string[0]) && (root->string[1] == ':') &&
         ((root->string[2] == '/') || (root->string[2] == '\\'));
    if(!rc) {
        return -2;
    }
#else
    if(root->string[0] != '/') {
        return -2;
    }
#endif

    rc = stat(root->string, &ss);
    if(rc < 0)
        return -1;
    else if(!S_ISDIR(ss.st_mode)) {
        errno = ENOTDIR;
        return -1;
    }
    return 1;
}

static AtomPtr
maybeAddSlash(AtomPtr atom)
{
    AtomPtr newAtom = NULL;
    if(!atom) return NULL;
    if(atom->length > 0 && atom->string[atom->length - 1] != '/') {
        newAtom = atomCat(atom, "/");
        releaseAtom(atom);
        return newAtom;
    }
    return atom;
}

void
initDiskcache()
{
    int rc;

    diskCacheRoot = expandTilde(maybeAddSlash(diskCacheRoot));
    rc = checkRoot(diskCacheRoot);
    if(rc <= 0) {
        switch(rc) {
        case 0: break;
        case -1: do_log_error(L_WARN, errno, "Disabling disk cache"); break;
        case -2: 
            do_log(L_WARN, "Disabling disk cache: path %s is not absolute.\n",
                   diskCacheRoot->string); 
            break;
        default: abort();
        }
        releaseAtom(diskCacheRoot);
        diskCacheRoot = NULL;
    }

    localDocumentRoot = expandTilde(maybeAddSlash(localDocumentRoot));
    rc = checkRoot(localDocumentRoot);
    if(rc <= 0) {
        switch(rc) {
        case 0: break;
        case -1: do_log_error(L_WARN, errno, "Disabling local tree"); break;
        case -2: 
            do_log(L_WARN, "Disabling local tree: path is not absolute.\n"); 
            break;
        default: abort();
        }
        releaseAtom(localDocumentRoot);
        localDocumentRoot = NULL;
    }
}

#ifdef DEBUG_DISK_CACHE
#define CHECK_ENTRY(entry) check_entry((entry))
static void
check_entry(DiskCacheEntryPtr entry)
{
    if(entry && entry->fd < 0)
        assert(entry == &negativeEntry);
    if(entry && entry->fd >= 0) {
        assert((!entry->previous) == (entry == diskEntries));
        assert((!entry->next) == (entry == diskEntriesLast));
        if(entry->size >= 0)
            assert(entry->size + entry->body_offset >= entry->offset);
        assert(entry->body_offset >= 0);
        if(entry->offset >= 0) {
            off_t offset;
            offset = lseek(entry->fd, 0, SEEK_CUR);
            assert(offset == entry->offset);
        }
        if(entry->size >= 0) {
            int rc;
            struct stat ss;
            rc = fstat(entry->fd, &ss);
            assert(rc >= 0);
            assert(ss.st_size == entry->size + entry->body_offset);
        }
    }
}
#else
#define CHECK_ENTRY(entry) do {} while(0)
#endif

int
diskEntrySize(ObjectPtr object)
{
    struct stat buf;
    int rc;
    DiskCacheEntryPtr entry = object->disk_entry;

    if(!entry || entry == &negativeEntry)
        return -1;

    if(entry->size >= 0)
        return entry->size;

    rc = fstat(entry->fd, &buf);
    if(rc < 0) {
        do_log_error(L_ERROR, errno, "Couldn't stat");
        return -1;
    }

    if(buf.st_size <= entry->body_offset)
        entry->size = 0;
    else
        entry->size =  buf.st_size - entry->body_offset;
    CHECK_ENTRY(entry);
    if(object->length >= 0 && entry->size == object->length)
        object->flags |= OBJECT_DISK_ENTRY_COMPLETE;
    return entry->size;
}

static int
entrySeek(DiskCacheEntryPtr entry, off_t offset)
{
    off_t rc;

    CHECK_ENTRY(entry);
    assert(entry != &negativeEntry);
    if(entry->offset == offset)
        return 1;
    if(offset > entry->body_offset) {
        /* Avoid extending the file by mistake */
        if(entry->size < 0)
            diskEntrySize(entry->object);
        if(entry->size < 0)
            return -1;
        if(entry->size + entry->body_offset < offset)
            return -1;
    }
    rc = lseek(entry->fd, offset, SEEK_SET);
    if(rc < 0) {
        do_log_error(L_ERROR, errno, "Couldn't seek");
        entry->offset = -1;
        return -1;
    }
    entry->offset = offset;
    return 1;
}

/* Given a local URL, constructs the filename where it can be found. */

int
localFilename(char *buf, int n, char *key, int len)
{
    int i, j;
    if(len <= 0 || key[0] != '/') return -1;

    if(urlIsSpecial(key, len)) return -1;

    if(checkRoot(localDocumentRoot) <= 0)
        return -1;

    if(n <= localDocumentRoot->length)
        return -1;

    i = 0;
    if(key[i] != '/')
        return -1;

    memcpy(buf, localDocumentRoot->string, localDocumentRoot->length);
    j = localDocumentRoot->length;
    if(buf[j - 1] == '/')
        j--;

    while(i < len) {
        if(j >= n - 1)
            return -1;
        if(key[i] == '/' && i < len - 2)
            if(key[i + 1] == '.' && 
               (key[i + 2] == '.' || key[i + 2] == '/'))
                return -1;
        buf[j++] = key[i++];
    }

    if(buf[j - 1] == '/') {
        if(j >= n - 11)
            return -1;
        memcpy(buf + j, "index.html", 10);
        j += 10;
    }

    buf[j] = '\0';
    return j;
}

static void
md5(unsigned char *restrict key, int len, unsigned char *restrict dst)
{
    static MD5_CTX ctx;
    MD5Init(&ctx);
    MD5Update(&ctx, key, len);
    MD5Final(&ctx);
    memcpy(dst, ctx.digest, 16);
}

/* Check whether a character can be stored in a filename.  This is
   needed since we want to support deficient file systems. */
static int
fssafe(char c)
{
    if(c <= 31 || c >= 127)
        return 0;
    if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
       (c >= '0' && c <= '9') ||  c == '.' || c == '-' || c == '_')
        return 1;
    return 0;
}

/* Given a URL, returns the directory name within which all files
   starting with this URL can be found. */
static int
urlDirname(char *buf, int n, const char *url, int len)
{
    int i, j;
    if(len < 8)
        return -1;
    if(lwrcmp(url, "http://", 7) != 0)
        return -1;

    if(checkRoot(localDocumentRoot) <= 0)
        return -1;

    if(n <= diskCacheRoot->length)
        return -1;

    memcpy(buf, diskCacheRoot->string, diskCacheRoot->length);
    j = diskCacheRoot->length;

    if(buf[j - 1] != '/')
        buf[j++] = '/';

    for(i = 7; i < len; i++) {
        if(i >= len || url[i] == '/')
            break;
        if(url[i] == '.' && i != len - 1 && url[i + 1] == '.')
            return -1;
        if(url[i] == '%' || !fssafe(url[i])) {
            if(j + 3 >= n) return -1;
            buf[j++] = '%';
            buf[j++] = i2h((url[i] & 0xF0) >> 4);
            buf[j++] = i2h(url[i] & 0x0F);
        } else {
            buf[j++] = url[i]; if(j >= n) return -1;
        }
    }
    buf[j++] = '/'; if(j >= n) return -1;
    buf[j] = '\0';
    return j;
}

/* Given a URL, returns the filename where the cached data can be
   found. */
static int
urlFilename(char *restrict buf, int n, const char *url, int len)
{
    int j;
    unsigned char md5buf[18];
    j = urlDirname(buf, n, url, len);
    if(j < 0 || j + 24 >= n)
        return -1;
    md5((unsigned char*)url, len, md5buf);
    b64cpy(buf + j, (char*)md5buf, 16, 1);
    buf[j + 24] = '\0';
    return j + 24;
}

static char *
dirnameUrl(char *url, int n, char *name, int len)
{
    int i, j, k, c1, c2;
    k = diskCacheRoot->length;
    if(len < k)
        return NULL;
    if(memcmp(name, diskCacheRoot->string, k) != 0)
        return NULL;
    if(n < 8)
        return NULL;
    memcpy(url, "http://", 7);
    if(name[len - 1] == '/')
        len --;
    j = 7;
    for(i = k; i < len; i++) {
        if(name[i] == '%') {
            if(i >= len - 2)
                return NULL;
            c1 = h2i(name[i + 1]);
            c2 = h2i(name[i + 2]);
            if(c1 < 0 || c2 < 0)
                return NULL;
            url[j++] = c1 * 16 + c2; if(j >= n) goto fail;
            i += 2;             /* skip extra digits */
        } else if(i < len - 1 && 
                  name[i] == '.' && name[i + 1] == '/') {
                return NULL;
        } else if(i == len - 1 && name[i] == '.') {
            return NULL;
        } else {
            url[j++] = name[i]; if(j >= n) goto fail;
        }
    }
    url[j++] = '/'; if(j >= n) goto fail;
    url[j] = '\0';
    return url;

 fail:
    return NULL;
}

/* Create a file and all intermediate directories. */
static int
createFile(const char *name, int path_start)
{
    int fd;
    char buf[1024];
    int n;
    int rc;

    if(name[path_start] == '/')
        path_start++;

    if(path_start < 2 || name[path_start - 1] != '/' ) {
        do_log(L_ERROR, "Incorrect name %s (%d).\n", name, path_start);
        return -1;
    }

    fd = open(name, O_RDWR | O_CREAT | O_EXCL | O_BINARY,
	      diskCacheFilePermissions);
    if(fd >= 0)
        return fd;
    if(errno != ENOENT) {
        do_log_error(L_ERROR, errno, "Couldn't create disk file %s", name);
        return -1;
    }
    
    n = path_start;
    while(name[n] != '\0' && n < 1024) {
        while(name[n] != '/' && name[n] != '\0' && n < 512)
            n++;
        if(name[n] != '/' || n >= 1024)
            break;
        memcpy(buf, name, n + 1);
        buf[n + 1] = '\0';
        rc = mkdir(buf, diskCacheDirectoryPermissions);
        if(rc < 0 && errno != EEXIST) {
            do_log_error(L_ERROR, errno, "Couldn't create directory %s", buf);
            return -1;
        }
        n++;
    }
    fd = open(name, O_RDWR | O_CREAT | O_EXCL | O_BINARY,
	      diskCacheFilePermissions);
    if(fd < 0) {
        do_log_error(L_ERROR, errno, "Couldn't create file %s", name);
        return -1;
    }

    return fd;
}

static int
chooseBodyOffset(int n, ObjectPtr object)
{
    int length = MAX(object->size, object->length);
    int body_offset;

    if(object->length >= 0 && object->length + n < 4096 - 4)
        return -1;              /* no gap for small objects */

    if(n <= 128)
        body_offset = 256;
    else if(n <= 192)
        body_offset = 384;
    else if(n <= 256)
        body_offset = 512;
    else if(n <= 384)
        body_offset = 768;
    else if(n <= 512)
        body_offset = 1024;
    else if(n <= 1024)
        body_offset = 2048;
    else if(n < 2048)
        body_offset = 4096;
    else
        body_offset = ((n + 32 + 4095) / 4096 + 1) * 4096;

    /* Tweak the gap so that we don't use up a full disk block for
       a small tail */
    if(object->length >= 0 && object->length < 64 * 1024) {
        int last = (body_offset + object->length) % 4096;
        int gap = body_offset - n - 32;
        if(last < gap / 2)
            body_offset -= last;
    }

    /* Rewriting large objects is expensive -- don't use small gaps.
       This has the additional benefit of block-aligning large bodies. */
    if(length >= 64 * 1024) {
        int min_gap, min_offset;
        if(length >= 512 * 1024)
            min_gap = 4096;
        else if(length >= 256 * 1024)
            min_gap = 2048;
        else
            min_gap = 1024;

        min_offset = ((n + 32 + min_gap - 1) / min_gap + 1) * min_gap;
        body_offset = MAX(body_offset, min_offset);
    }

    return body_offset;
}
 
/* Assumes the file descriptor is at offset 0.  Returns -1 on failure,
   otherwise the offset at which the file descriptor is left. */
/* If chunk is not null, it should be the first chunk of the object,
   and will be written out in the same operation if possible. */
static int
writeHeaders(int fd, int *body_offset_return,
             ObjectPtr object, char *chunk, int chunk_len)
{
    int n, rc, error = -1;
    int body_offset = *body_offset_return;
    char *buf = NULL;
    int buf_is_chunk = 0;
    int bufsize = 0;

    if(object->flags & OBJECT_LOCAL)
        return -1;

    if(body_offset > CHUNK_SIZE)
        goto overflow;

    /* get_chunk might trigger object expiry */
    bufsize = CHUNK_SIZE;
    buf_is_chunk = 1;
    buf = maybe_get_chunk();
    if(!buf) {
        bufsize = 2048;
        buf_is_chunk = 0;
        buf = malloc(2048);
        if(buf == NULL) {
            do_log(L_ERROR, "Couldn't allocate buffer.\n");
            return -1;
        }
    }

 format_again:
    n = snnprintf(buf, 0, bufsize, "HTTP/1.1 %3d %s",
                  object->code, object->message->string);

    n = httpWriteObjectHeaders(buf, n, bufsize, object, 0, -1);
    if(n < 0)
        goto overflow;

    n = snnprintf(buf, n, bufsize, "\r\nX-Polipo-Location: ");
    n = snnprint_n(buf, n, bufsize, object->key, object->key_size);

    if(object->age >= 0 && object->age != object->date) {
        n = snnprintf(buf, n, bufsize, "\r\nX-Polipo-Date: ");
        n = format_time(buf, n, bufsize, object->age);
    }

    if(object->atime >= 0) {
        n = snnprintf(buf, n, bufsize, "\r\nX-Polipo-Access: ");
        n = format_time(buf, n, bufsize, object->atime);
    }

    if(n < 0)
        goto overflow;

    if(body_offset < 0)
        body_offset = chooseBodyOffset(n, object);

    if(body_offset > bufsize)
        goto overflow;

    if(body_offset > 0 && body_offset != n + 4)
        n = snnprintf(buf, n, bufsize, "\r\nX-Polipo-Body-Offset: %d",
                      body_offset);

    n = snnprintf(buf, n, bufsize, "\r\n\r\n");
    if(n < 0)
        goto overflow;

    if(body_offset < 0)
        body_offset = n;
    if(n > body_offset) {
        error = -2;
        goto fail;
    }

    if(n < body_offset)
        memset(buf + n, 0, body_offset - n);

 again:
#ifdef HAVE_READV_WRITEV
    if(chunk_len > 0) {
        struct iovec iov[2];
        iov[0].iov_base = buf;
        iov[0].iov_len = body_offset;
        iov[1].iov_base = chunk;
        iov[1].iov_len = chunk_len;
        rc = writev(fd, iov, 2);
    } else
#endif
        rc = write(fd, buf, body_offset);

    if(rc < 0 && errno == EINTR)
        goto again;

    if(rc < body_offset)
        goto fail;
    if(object->length >= 0 && 
       rc - body_offset >= object->length)
        object->flags |= OBJECT_DISK_ENTRY_COMPLETE;

    *body_offset_return = body_offset;
    if(buf_is_chunk)
        dispose_chunk(buf);
    else
        free(buf);
    return rc;

 overflow:
    if(bufsize < bigBufferSize) {
        char *oldbuf = buf;
        buf = malloc(bigBufferSize);
        if(!buf) {
            do_log(L_ERROR, "Couldn't allocate big buffer.\n");
            goto fail;
        }
        bufsize = bigBufferSize;
        if(oldbuf) {
            if(buf_is_chunk)
                dispose_chunk(oldbuf);
            else
                free(oldbuf);
        }
        buf_is_chunk = 0;
        goto format_again;
    }
    /* fall through */

 fail:
    if(buf_is_chunk)
        dispose_chunk(buf);
    else
        free(buf);
    return error;
}

typedef struct _MimeEntry {
    char *extension;
    char *mime;
} MimeEntryRec;

static const MimeEntryRec mimeEntries[] = {
    { "html", "text/html" },
    { "htm", "text/html" },
    { "text", "text/plain" },
    { "txt", "text/plain" },
    { "png", "image/png" },
    { "gif", "image/gif" },
    { "jpeg", "image/jpeg" },
    { "jpg", "image/jpeg" },
    { "ico", "image/x-icon" },
    { "pdf", "application/pdf" },
    { "ps", "application/postscript" },
    { "tar", "application/x-tar" },
    { "pac", "application/x-ns-proxy-autoconfig" },
    { "css", "text/css" },
    { "js",  "application/x-javascript" },
    { "xml", "text/xml" },
    { "swf", "application/x-shockwave-flash" },
};

static char*
localObjectMimeType(ObjectPtr object, char **encoding_return)
{
    char *name = object->key;
    int nlen = object->key_size;
    int i;

    assert(nlen >= 1);

    if(name[nlen - 1] == '/') {
        *encoding_return = NULL;
        return "text/html";
    }

    if(nlen < 3) {
        *encoding_return = NULL;
        return "application/octet-stream";
    }

    if(memcmp(name + nlen - 3, ".gz", 3) == 0) {
        *encoding_return = "x-gzip";
        nlen -= 3;
    } else if(memcmp(name + nlen - 2, ".Z", 2) == 0) {
        *encoding_return = "x-compress";
        nlen -= 2;
    } else {
        *encoding_return = NULL;
    }

    for(i = 0; i < sizeof(mimeEntries) / sizeof(mimeEntries[0]); i++) {
        int len = strlen(mimeEntries[i].extension);
        if(nlen > len && 
           name[nlen - len - 1] == '.' &&
           memcmp(name + nlen - len, mimeEntries[i].extension, len) == 0)
            return mimeEntries[i].mime;
    }

    return "application/octet-stream";
}

/* Same interface as validateEntry -- see below */
int
validateLocalEntry(ObjectPtr object, int fd,
                   int *body_offset_return, off_t *offset_return)
{
    struct stat ss;
    char buf[512];
    int n, rc;
    char *encoding;

    rc = fstat(fd, &ss);
    if(rc < 0) {
        do_log_error(L_ERROR, errno, "Couldn't stat");
        return -1;
    }

    if(S_ISREG(ss.st_mode)) {
        if(/*!(ss.st_mode & S_IROTH) ||*/
           (object->length >= 0 && object->length != ss.st_size) ||
           (object->last_modified >= 0 && 
            object->last_modified != ss.st_mtime))
            return -1;
    } else {
        notifyObject(object);
        return -1;
    }
    
    n = snnprintf(buf, 0, 512, "%lx-%lx-%lx",
                  (unsigned long)ss.st_ino,
                  (unsigned long)ss.st_size,
                  (unsigned long)ss.st_mtime);
    if(n >= 512)
        n = -1;

    if(n > 0 && object->etag) {
        if(strlen(object->etag) != n ||
           memcmp(object->etag, buf, n) != 0)
            return -1;
    }

    if(!(object->flags & OBJECT_INITIAL)) {
        if(!object->last_modified && !object->etag)
            return -1;
    }
       
    if(object->flags & OBJECT_INITIAL) {
        object->length = ss.st_size;
        object->last_modified = ss.st_mtime;
        object->date = current_time.tv_sec;
        object->age = current_time.tv_sec;
        object->code = 200;
        if(n > 0)
            object->etag = strdup(buf); /* okay if fails */
        object->message = internAtom("Okay");
        n = snnprintf(buf, 0, 512,
                      "\r\nServer: Polipo"
                      "\r\nContent-Type: %s",
                      localObjectMimeType(object, &encoding));
        if(encoding != NULL)
            n = snnprintf(buf, n, 512,
                          "\r\nContent-Encoding: %s", encoding);
        if(n < 0)
            return -1;
        object->headers = internAtomN(buf, n);
        if(object->headers == NULL)
            return -1;
        object->flags &= ~OBJECT_INITIAL;
    }

    if(body_offset_return)
        *body_offset_return = 0;
    if(offset_return)
        *offset_return = 0;
    return 0;
}

/* Assumes fd is at offset 0.
   Returns -1 if not valid, 1 if metadata should be written out, 0
   otherwise. */
int
validateEntry(ObjectPtr object, int fd, 
              int *body_offset_return, off_t *offset_return)
{
    char *buf;
    int buf_is_chunk, bufsize;
    int rc, n;
    int dummy;
    int code;
    AtomPtr headers;
    time_t date, last_modified, expires, polipo_age, polipo_access;
    int length;
    off_t offset = -1;
    int body_offset;
    char *etag;
    AtomPtr via;
    CacheControlRec cache_control;
    char *location;
    AtomPtr message;
    int dirty = 0;

    if(object->flags & OBJECT_LOCAL)
        return validateLocalEntry(object, fd,
                                  body_offset_return, offset_return);

    if(!(object->flags & OBJECT_PUBLIC) && (object->flags & OBJECT_INITIAL))
        return 0;

    /* get_chunk might trigger object expiry */
    bufsize = CHUNK_SIZE;
    buf_is_chunk = 1;
    buf = maybe_get_chunk();
    if(!buf) {
        bufsize = 2048;
        buf_is_chunk = 0;
        buf = malloc(2048);
        if(buf == NULL) {
            do_log(L_ERROR, "Couldn't allocate buffer.\n");
            return -1;
        }
    }

 again:
    rc = read(fd, buf, bufsize);
    if(rc < 0) {
        if(errno == EINTR)
            goto again;
        do_log_error(L_ERROR, errno, "Couldn't read disk entry");
        goto fail;
    }
    offset = rc;

 parse_again:
    n = findEndOfHeaders(buf, 0, rc, &dummy);
    if(n < 0) {
        char *oldbuf = buf;
        if(bufsize < bigBufferSize) {
            buf = malloc(bigBufferSize);
            if(!buf) {
                do_log(L_ERROR, "Couldn't allocate big buffer.\n");
                goto fail;
            }
            bufsize = bigBufferSize;
            memcpy(buf, oldbuf, offset);
            if(buf_is_chunk)
                dispose_chunk(oldbuf);
            else
                free(oldbuf);
            buf_is_chunk = 0;
        again2:
            rc = read(fd, buf + offset, bufsize - offset);
            if(rc < 0) {
                if(errno == EINTR)
                    goto again2;
                do_log_error(L_ERROR, errno, "Couldn't read disk entry");
                goto fail;
            }
            offset += rc;
            goto parse_again;
        }
        do_log(L_ERROR, "Couldn't parse disk entry.\n");
        goto fail;
    }

    rc = httpParseServerFirstLine(buf, &code, &dummy, &message);
    if(rc < 0) {
        do_log(L_ERROR, "Couldn't parse disk entry.\n");
        goto fail;
    }

    if(object->code != 0 && object->code != code) {
        releaseAtom(message);
        goto fail;
    }

    rc = httpParseHeaders(0, NULL, buf, rc, NULL,
                          &headers, &length, &cache_control, NULL, NULL,
                          &date, &last_modified, &expires, &polipo_age,
                          &polipo_access, &body_offset,
                          NULL, &etag, NULL,
                          NULL, NULL, &location, &via, NULL);
    if(rc < 0) {
        releaseAtom(message);
        goto fail;
    }
    if(body_offset < 0)
        body_offset = n;

    if(!location || strlen(location) != object->key_size ||
       memcmp(location, object->key, object->key_size) != 0) {
        do_log(L_ERROR, "Inconsistent cache file for %s.\n", scrub(location));
        goto invalid;
    }

    if(polipo_age < 0)
        polipo_age = date;

    if(polipo_age < 0) {
        do_log(L_ERROR, "Undated disk entry for %s.\n", scrub(location));
        goto invalid;
    }

    if(!(object->flags & OBJECT_INITIAL)) {
        if((last_modified >= 0) != (object->last_modified >= 0))
            goto invalid;

        if((object->cache_control & CACHE_MISMATCH) ||
           (cache_control.flags & CACHE_MISMATCH))
            goto invalid;

        if(last_modified >= 0 && object->last_modified >= 0 &&
           last_modified != object->last_modified)
            goto invalid;

        if(length >= 0 && object->length >= 0)
            if(length != object->length)
                goto invalid;

        if(!!etag != !!object->etag)
            goto invalid;

        if(etag && object->etag && strcmp(etag, object->etag) != 0)
            goto invalid;

        /* If we don't have a usable ETag, and either CACHE_VARY or we
           don't have a last-modified date, we validate disk entries by
           using their date. */
        if(!(etag && object->etag) &&
           (!(last_modified >= 0 && object->last_modified >= 0) ||
            ((cache_control.flags & CACHE_VARY) ||
             (object->cache_control & CACHE_VARY)))) {
            if(date >= 0 && date != object->date)
                goto invalid;
            if(polipo_age >= 0 && polipo_age != object->age)
                goto invalid;
        }
        if((object->cache_control & CACHE_VARY) && dontTrustVaryETag >= 1) {
            /* Check content-type to work around mod_gzip bugs */
            if(!httpHeaderMatch(atomContentType, object->headers, headers) ||
               !httpHeaderMatch(atomContentEncoding, object->headers, headers))
                goto invalid;
        }
    }

    if(location)
        free(location);

    if(headers) {
        if(!object->headers)
            object->headers = headers;
        else
            releaseAtom(headers);
    }

    if(object->code == 0) {
        object->code = code;
        object->message = retainAtom(message);
    }
    if(object->date <= date)
        object->date = date;
    else 
        dirty = 1;
    if(object->last_modified < 0)
        object->last_modified = last_modified;
    if(object->expires < 0)
        object->expires = expires;
    else if(object->expires > expires)
        dirty = 1;
    if(object->age < 0)
        object->age = polipo_age;
    else if(object->age > polipo_age)
        dirty = 1;
    if(object->atime <= polipo_access)
        object->atime = polipo_access;
    else
        dirty = 1;

    object->cache_control |= cache_control.flags;
    object->max_age = cache_control.max_age;
    object->s_maxage = cache_control.s_maxage;

    if(object->age < 0) object->age = object->date;
    if(object->age < 0) object->age = 0; /* a long time ago */
    if(object->length < 0) object->length = length;
    if(!object->etag)
        object->etag = etag;
    else {
        if(etag)
            free(etag);
    }
    releaseAtom(message);

    if(object->flags & OBJECT_INITIAL) object->via = via;
    object->flags &= ~OBJECT_INITIAL;
    if(offset > body_offset) {
        /* We need to make sure we don't invoke object expiry recursively */
        objectSetChunks(object, 1);
        if(object->numchunks >= 1) {
            if(object->chunks[0].data == NULL)
                object->chunks[0].data = maybe_get_chunk();
            if(object->chunks[0].data)
                objectAddData(object, buf + body_offset,
                              0, MIN(offset - body_offset, CHUNK_SIZE));
        }
    }

    httpTweakCachability(object);

    if(buf_is_chunk)
        dispose_chunk(buf);
    else
        free(buf);
    if(body_offset_return) *body_offset_return = body_offset;
    if(offset_return) *offset_return = offset;
    return dirty;

 invalid:
    releaseAtom(message);
    if(etag) free(etag);
    if(location) free(location);
    if(via) releaseAtom(via);
    /* fall through */

 fail:
    if(buf_is_chunk)
        dispose_chunk(buf);
    else
        free(buf);
    return -1;
}

void
dirtyDiskEntry(ObjectPtr object)
{
    DiskCacheEntryPtr entry = object->disk_entry;
    if(entry && entry != &negativeEntry) entry->metadataDirty = 1;
}

int
revalidateDiskEntry(ObjectPtr object)
{
    DiskCacheEntryPtr entry = object->disk_entry;
    int rc;
    int body_offset;

    if(!entry || entry == &negativeEntry)
        return 1;

    CHECK_ENTRY(entry);
    rc = entrySeek(entry, 0);
    if(rc < 0) return 0;

    rc = validateEntry(object, entry->fd, &body_offset, &entry->offset);
    if(rc < 0) {
        destroyDiskEntry(object, 0);
        return 0;
    }
    if(body_offset != entry->body_offset) {
        do_log(L_WARN, "Inconsistent body offset (%d != %d).\n",
               body_offset, entry->body_offset);
        destroyDiskEntry(object, 0);
        return 0;
    }

    entry->metadataDirty |= !!rc;
    CHECK_ENTRY(entry);
    return 1;
}

static DiskCacheEntryPtr
makeDiskEntry(ObjectPtr object, int create)
{
    DiskCacheEntryPtr entry = NULL;
    char buf[1024];
    int fd = -1;
    int negative = 0, size = -1, name_len = -1;
    char *name = NULL;
    off_t offset = -1;
    int body_offset = -1;
    int rc;
    int local = (object->flags & OBJECT_LOCAL) != 0;
    int dirty = 0;

   if(local && create)
       return NULL;

    if(!local && !(object->flags & OBJECT_PUBLIC))
        return NULL;

    if(maxDiskCacheEntrySize >= 0) {
        if(object->length > 0) {
            if(object->length > maxDiskCacheEntrySize)
                return NULL;
        } else {
            if(object->size > maxDiskCacheEntrySize)
                return NULL;
        }
    }

    if(object->disk_entry) {
        entry = object->disk_entry;
        CHECK_ENTRY(entry);
        if(entry != &negativeEntry) {
            /* We'll keep the entry -- put it at the front. */
            if(entry != diskEntries && entry != &negativeEntry) {
                entry->previous->next = entry->next;
                if(entry->next)
                    entry->next->previous = entry->previous;
                else
                    diskEntriesLast = entry->previous;
                entry->next = diskEntries;
                diskEntries->previous = entry;
                entry->previous = NULL;
                diskEntries = entry;
            }
            return entry;
        } else {
            if(entry == &negativeEntry) {
                negative = 1;
                if(!create) return NULL;
                object->disk_entry = NULL;
            }
            entry = NULL;
            destroyDiskEntry(object, 0);
        }
    }

    if(numDiskEntries > maxDiskEntries)
        destroyDiskEntry(diskEntriesLast->object, 0);

    if(!local) {
        if(diskCacheRoot == NULL || diskCacheRoot->length <= 0)
            return NULL;
        name_len = urlFilename(buf, 1024, object->key, object->key_size);
        if(name_len < 0) return NULL;
        if(!negative)
            fd = open(buf, O_RDWR | O_BINARY);
        if(fd >= 0) {
            rc = validateEntry(object, fd, &body_offset, &offset);
            if(rc >= 0) {
                dirty = rc;
            } else {
                close(fd);
                fd = -1;
                rc = unlink(buf);
                if(rc < 0 && errno != ENOENT) {
                    do_log_error(L_WARN,  errno,
                                 "Couldn't unlink stale disk entry %s", 
                                 scrub(buf));
                    /* But continue -- it's okay to have stale entries. */
                }
            }
        }

        if(fd < 0 && create && name_len > 0 && 
           !(object->flags & OBJECT_INITIAL)) {
            fd = createFile(buf, diskCacheRoot->length);
            if(fd < 0)
                return NULL;

            if(fd >= 0) {
                char *data = NULL;
                int dsize = 0;
                if(object->numchunks > 0) {
                    data = object->chunks[0].data;
                    dsize = object->chunks[0].size;
                }
                rc = writeHeaders(fd, &body_offset, object, data, dsize);
                if(rc < 0) {
                    do_log_error(L_ERROR, errno, "Couldn't write headers");
                    rc = unlink(buf);
                    if(rc < 0 && errno != ENOENT)
                        do_log_error(L_ERROR, errno,
                                     "Couldn't unlink truncated entry %s", 
                                     scrub(buf));
                    close(fd);
                    return NULL;
                }
                assert(rc >= body_offset);
                size = rc - body_offset;
                offset = rc;
                dirty = 0;
            }
        }
    } else {
        /* local */
        if(localDocumentRoot == NULL || localDocumentRoot->length == 0)
            return NULL;

        name_len = 
            localFilename(buf, 1024, object->key, object->key_size);
        if(name_len < 0)
            return NULL;
        fd = open(buf, O_RDONLY | O_BINARY);
        if(fd >= 0) {
            if(validateEntry(object, fd, &body_offset, NULL) < 0) {
                close(fd);
                fd = -1;
            }
        }
        offset = 0;
    }

    if(fd < 0) {
        object->disk_entry = &negativeEntry;
        return NULL;
    }
    assert(body_offset >= 0);

    name = strdup_n(buf, name_len);
    if(name == NULL) {
        do_log(L_ERROR, "Couldn't allocate name.\n");
        close(fd);
        fd = -1;
        return NULL;
    }

    entry = malloc(sizeof(DiskCacheEntryRec));
    if(entry == NULL) {
        do_log(L_ERROR, "Couldn't allocate entry.\n");
        free(name);
        close(fd);
        return NULL;
    }

    entry->filename = name;
    entry->object = object;
    entry->fd = fd;
    entry->body_offset = body_offset;
    entry->local = local;
    entry->offset = offset;
    entry->size = size;
    entry->metadataDirty = dirty;

    entry->next = diskEntries;
    if(diskEntries)
        diskEntries->previous = entry;
    diskEntries = entry;
    if(diskEntriesLast == NULL)
        diskEntriesLast = entry;
    entry->previous = NULL;
    numDiskEntries++;

    object->disk_entry = entry;

    CHECK_ENTRY(entry);
    return entry;
}

/* Rewrite a disk cache entry, used when the body offset needs to change. */
static int
rewriteEntry(ObjectPtr object)
{
    int old_body_offset = object->disk_entry->body_offset;
    int fd, rc, n;
    DiskCacheEntryPtr entry;
    char* buf;
    int buf_is_chunk, bufsize;
    int offset;

    fd = dup(object->disk_entry->fd);
    if(fd < 0) {
        do_log_error(L_ERROR, errno, "Couldn't duplicate file descriptor");
        return -1;
    }

    rc = destroyDiskEntry(object, 1);
    if(rc < 0) {
        close(fd);
        return -1;
    }
    entry = makeDiskEntry(object, 1);
    if(!entry) {
        close(fd);
        return -1;
    }

    offset = diskEntrySize(object);
    if(offset < 0) {
        close(fd);
        return -1;
    }

    bufsize = CHUNK_SIZE;
    buf_is_chunk = 1;
    buf = maybe_get_chunk();
    if(!buf) {
        bufsize = 2048;
        buf_is_chunk = 0;
        buf = malloc(2048);
        if(buf == NULL) {
            do_log(L_ERROR, "Couldn't allocate buffer.\n");
            close(fd);
            return -1;
        }
    }

    rc = lseek(fd, old_body_offset + offset, SEEK_SET);
    if(rc < 0)
        goto done;

    while(1) {
        CHECK_ENTRY(entry);
        n = read(fd, buf, bufsize);
        if(n < 0 && errno == EINTR)
            continue;
        if(n <= 0)
            goto done;
        rc = entrySeek(entry, entry->body_offset + offset);
        if(rc < 0)
            goto done;
    write_again:
        rc = write(entry->fd, buf, n);
        if(rc >= 0) {
            entry->offset += rc;
            entry->size += rc;
        } else if(errno == EINTR) {
            goto write_again;
        }
        if(rc < n)
            goto done;
    }

 done:
    CHECK_ENTRY(entry);
    if(object->length >= 0 && entry->size == object->length)
        object->flags |= OBJECT_DISK_ENTRY_COMPLETE;
    close(fd);
    if(buf_is_chunk)
        dispose_chunk(buf);
    else
        free(buf);
    return 1;
}
            
int
destroyDiskEntry(ObjectPtr object, int d)
{
    DiskCacheEntryPtr entry = object->disk_entry;
    int rc, urc = 1;

    assert(!entry || !entry->local || !d);

    if(d && !entry)
        entry = makeDiskEntry(object, 0);

    CHECK_ENTRY(entry);

    if(!entry || entry == &negativeEntry) {
        return 1;
    }

    assert(entry->object == object);

    if(maxDiskCacheEntrySize >= 0 && object->size > maxDiskCacheEntrySize) {
        /* See writeoutToDisk */
        d = 1;
    }

    if(d) {
        entry->object->flags &= ~OBJECT_DISK_ENTRY_COMPLETE;
        if(entry->filename) {
            urc = unlink(entry->filename);
            if(urc < 0)
                do_log_error(L_WARN, errno, 
                             "Couldn't unlink %s", scrub(entry->filename));
        }
    } else {
        if(entry && entry->metadataDirty)
            writeoutMetadata(object);
        makeDiskEntry(object, 0);
        /* rewriteDiskEntry may change the disk entry */
        entry = object->disk_entry;
        if(entry == NULL || entry == &negativeEntry)
            return 0;
        if(diskCacheWriteoutOnClose > 0) {
            reallyWriteoutToDisk(object, -1, diskCacheWriteoutOnClose);
            entry = object->disk_entry;
            if(entry == NULL || entry == &negativeEntry)
                return 0;
        }
    }
 again:
    rc = close(entry->fd);
    if(rc < 0 && errno == EINTR)
        goto again;

    entry->fd = -1;

    if(entry->filename)
        free(entry->filename);
    entry->filename = NULL;

    if(entry->previous)
        entry->previous->next = entry->next;
    else
        diskEntries = entry->next;
    if(entry->next)
        entry->next->previous = entry->previous;
    else
        diskEntriesLast = entry->previous;

    numDiskEntries--;
    assert(numDiskEntries >= 0);

    free(entry);
    object->disk_entry = NULL;
    if(urc < 0)
        return -1;
    else
        return 1;
}

ObjectPtr 
objectGetFromDisk(ObjectPtr object)
{
    DiskCacheEntryPtr entry = makeDiskEntry(object, 0);
    if(!entry) return NULL;
    return object;
}


int 
objectFillFromDisk(ObjectPtr object, int offset, int chunks)
{
    DiskCacheEntryPtr entry;
    int rc, result;
    int i, j, k;
    int complete;

    if(object->type != OBJECT_HTTP)
        return 0;

    if(object->flags & OBJECT_LINEAR)
        return 0;

    if(object->length >= 0) {
        chunks = MIN(chunks, 
                     (object->length - offset + CHUNK_SIZE - 1) / CHUNK_SIZE);
    }

    rc = objectSetChunks(object, offset / CHUNK_SIZE + chunks);
    if(rc < 0)
        return 0;

    complete = 1;
    if(object->flags & OBJECT_INITIAL) {
        complete = 0;
    } else if((object->length < 0 || object->size < object->length) &&
              object->size < (offset / CHUNK_SIZE + chunks) * CHUNK_SIZE) {
        complete = 0;
    } else {
        for(k = 0; k < chunks; k++) {
            int s;
            i = offset / CHUNK_SIZE + k;
            s = MIN(CHUNK_SIZE, object->size - i * CHUNK_SIZE);
            if(object->chunks[i].size < s) {
                complete = 0;
                break;
            }
        }
    }

    if(complete)
        return 1;

    /* This has the side-effect of revalidating the entry, which is
       what makes HEAD requests work. */
    entry = makeDiskEntry(object, 0);
    if(!entry)
        return 0;
                
    for(k = 0; k < chunks; k++) {
        i = offset / CHUNK_SIZE + k;
        if(!object->chunks[i].data)
            object->chunks[i].data = get_chunk();
        if(!object->chunks[i].data) {
            chunks = k;
            break;
        }
        lockChunk(object, i);
    }

    result = 0;

    for(k = 0; k < chunks; k++) {
        int o;
        i = offset / CHUNK_SIZE + k;
        j = object->chunks[i].size;
        o = i * CHUNK_SIZE + j;

        if(object->chunks[i].size == CHUNK_SIZE)
            continue;

        if(entry->size >= 0 && entry->size <= o)
            break;

        if(entry->offset != entry->body_offset + o) {
            rc = entrySeek(entry, entry->body_offset + o);
            if(rc < 0) {
                result = 0;
                break;
            }
        }

        CHECK_ENTRY(entry);
        again:
        rc = read(entry->fd, object->chunks[i].data + j, CHUNK_SIZE - j);
        if(rc < 0) {
            if(errno == EINTR)
                goto again;
            entry->offset = -1;
            do_log_error(L_ERROR, errno, "Couldn't read");
            break;
        }

        entry->offset += rc;
        object->chunks[i].size += rc;
        if(object->size < o + rc)
            object->size = o + rc;

        if(entry->object->length >= 0 && entry->size < 0 &&
           entry->offset - entry->body_offset == entry->object->length)
            entry->size = entry->object->length;
            
        if(rc < CHUNK_SIZE - j) {
            /* Paranoia: the read may have been interrupted half-way. */
            if(entry->size < 0) {
                if(rc == 0 ||
                   (entry->object->length >= 0 &&
                    entry->object->length == 
                    entry->offset - entry->body_offset))
                    entry->size = entry->offset - entry->body_offset;
                break;
            } else if(entry->size != entry->offset - entry->body_offset) {
                if(rc == 0 || 
                   entry->size < entry->offset - entry->body_offset) {
                    do_log(L_WARN,
                           "Disk entry size changed behind our back: "
                           "%ld -> %ld (%d).\n",
                           (long)entry->size,
                           (long)entry->offset - entry->body_offset,
                           object->size);
                    entry->size = -1;
                }
            }
            break;
        }

        CHECK_ENTRY(entry);
        result = 1;
    }

    CHECK_ENTRY(object->disk_entry);
    for(k = 0; k < chunks; k++) {
        i = offset / CHUNK_SIZE + k;
        unlockChunk(object, i);
    }

    if(result > 0) {
        notifyObject(object);
        return 1;
    } else {
        return 0;
    }
}

int 
writeoutToDisk(ObjectPtr object, int upto, int max)
{
    if(maxDiskCacheEntrySize >= 0 && object->size > maxDiskCacheEntrySize) {
        /* An object was created with an unknown length, and then grew
           beyond maxDiskCacheEntrySize.  Destroy the disk entry. */
        destroyDiskEntry(object, 1);
        return 0;
    }

    return reallyWriteoutToDisk(object, upto, max);
}
        
static int 
reallyWriteoutToDisk(ObjectPtr object, int upto, int max)
{
    DiskCacheEntryPtr entry;
    int rc;
    int i, j;
    int offset;
    int bytes = 0;

    if(upto < 0)
        upto = object->size;

    if((object->cache_control & CACHE_NO_STORE) || 
       (object->flags & OBJECT_LOCAL))
        return 0;

    if((object->flags & OBJECT_DISK_ENTRY_COMPLETE) && !object->disk_entry)
        return 0;

    entry = makeDiskEntry(object, 1);
    if(!entry) return 0;

    assert(!entry->local);

    if(object->flags & OBJECT_DISK_ENTRY_COMPLETE)
        goto done;

    diskEntrySize(object);
    if(entry->size < 0)
        return 0;

    if(object->length >= 0 && entry->size >= object->length) {
        object->flags |= OBJECT_DISK_ENTRY_COMPLETE;
        goto done;
    }

    if(entry->size >= upto)
        goto done;

    offset = entry->size;

    /* Avoid a seek in case we start writing at the beginning */
    if(offset == 0 && entry->metadataDirty) {
        writeoutMetadata(object);
        /* rewriteDiskEntry may change the entry */
        entry = makeDiskEntry(object, 0);
        if(entry == NULL)
            return 0;
    }

    rc = entrySeek(entry, offset + entry->body_offset);
    if(rc < 0) return 0;

    do {
        if(max >= 0 && bytes >= max)
            break;
        CHECK_ENTRY(entry);
        assert(entry->offset == offset + entry->body_offset);
        i = offset / CHUNK_SIZE;
        j = offset % CHUNK_SIZE;
        if(i >= object->numchunks)
            break;
        if(object->chunks[i].size <= j)
            break;
    again:
        rc = write(entry->fd, object->chunks[i].data + j,
                   object->chunks[i].size - j);
        if(rc < 0) {
            if(errno == EINTR)
                goto again;
            do_log_error(L_ERROR, errno, "Couldn't write disk entry");
            break;
        }
        entry->offset += rc;
        offset += rc;
        bytes += rc;
        if(entry->size < offset)
            entry->size = offset;
    } while(j + rc >= CHUNK_SIZE);

 done:
    CHECK_ENTRY(entry);
    if(entry->metadataDirty)
        writeoutMetadata(object);

    return bytes;
}

int 
writeoutMetadata(ObjectPtr object)
{
    DiskCacheEntryPtr entry;
    int rc;

    if((object->cache_control & CACHE_NO_STORE) || 
       (object->flags & OBJECT_LOCAL))
        return 0;
    
    entry = makeDiskEntry(object, 0);
    if(entry == NULL || entry == &negativeEntry)
        goto fail;

    assert(!entry->local);

    rc = entrySeek(entry, 0);
    if(rc < 0) goto fail;

    rc = writeHeaders(entry->fd, &entry->body_offset, object, NULL, 0);
    if(rc == -2) {
        rc = rewriteEntry(object);
        if(rc < 0) return 0;
        return 1;
    }
    if(rc < 0) goto fail;
    entry->offset = rc;
    entry->metadataDirty = 0;
    return 1;

 fail:
    /* We need this in order to avoid trying to write this entry out
       multiple times. */
    if(entry && entry != &negativeEntry)
        entry->metadataDirty = 0;
    return 0;
}

static void
mergeDobjects(DiskObjectPtr dst, DiskObjectPtr src)
{
    if(dst->filename == NULL) {
        dst->filename = src->filename;
        dst->body_offset = src->body_offset;
    } else
        free(src->filename);
    free(src->location);
    if(dst->length < 0)
        dst->length = src->length;
    if(dst->size < 0)
        dst->size = src->size;
    if(dst->age < 0)
        dst->age = src->age;
    if(dst->date < 0)
        dst->date = src->date;
    if(dst->last_modified < 0)
        dst->last_modified = src->last_modified;
    free(src);
}

DiskObjectPtr
readDiskObject(char *filename, struct stat *sb)
{
    int fd, rc, n, dummy, code;
    int length, size;
    time_t date, last_modified, age, atime, expires;
    char *location = NULL, *fn = NULL;
    DiskObjectPtr dobject;
    char *buf;
    int buf_is_chunk, bufsize;
    int body_offset;
    struct stat ss;

    fd = -1;

    if(sb == NULL) {
        rc = stat(filename, &ss);
        if(rc < 0) {
            do_log_error(L_WARN, errno, "Couldn't stat %s", scrub(filename));
            return NULL;
        }
        sb = &ss;
    }

    buf_is_chunk = 1;
    bufsize = CHUNK_SIZE;
    buf = get_chunk();
    if(buf == NULL) {
        do_log(L_ERROR, "Couldn't allocate buffer.\n");
        return NULL;
    }

    if(S_ISREG(sb->st_mode)) {
        fd = open(filename, O_RDONLY | O_BINARY);
        if(fd < 0)
            goto fail;
    again:
        rc = read(fd, buf, bufsize);
        if(rc < 0)
            goto fail;
        
        n = findEndOfHeaders(buf, 0, rc, &dummy);
        if(n < 0) {
            long lrc;
            if(buf_is_chunk) {
                dispose_chunk(buf);
                buf_is_chunk = 0;
                bufsize = bigBufferSize;
                buf = malloc(bigBufferSize);
                if(buf == NULL)
                    goto fail2;
                lrc = lseek(fd, 0, SEEK_SET);
                if(lrc < 0)
                    goto fail;
                goto again;
            }
            goto fail;
        }
        
        rc = httpParseServerFirstLine(buf, &code, &dummy, NULL);
        if(rc < 0)
            goto fail;

        rc = httpParseHeaders(0, NULL, buf, rc, NULL,
                              NULL, &length, NULL, NULL, NULL, 
                              &date, &last_modified, &expires, &age,
                              &atime, &body_offset, NULL,
                              NULL, NULL, NULL, NULL, &location, NULL, NULL);
        if(rc < 0 || location == NULL)
            goto fail;
        if(body_offset < 0)
            body_offset = n;
    
        size = sb->st_size - body_offset;
        if(size < 0)
            size = 0;
    } else if(S_ISDIR(sb->st_mode)) {
        char *n;
        n = dirnameUrl(buf, 512, (char*)filename, strlen(filename));
        if(n == NULL)
            goto fail;
        location = strdup(n);
        if(location == NULL)
            goto fail;
        length = -1;
        size = -1;
        body_offset = -1;
        age = -1;
        atime = -1;
        date = -1;
        last_modified = -1;
        expires = -1;
    } else {
        goto fail;
    }

    dobject = malloc(sizeof(DiskObjectRec));
    if(!dobject)
        goto fail;
    
    fn = strdup(filename);
    if(!fn)
        goto fail;

    if(buf_is_chunk)
        dispose_chunk(buf);
    else
        free(buf);

    dobject->location = location;
    dobject->filename = fn;
    dobject->length = length;
    dobject->body_offset = body_offset;
    dobject->size = size;
    dobject->age = age;
    dobject->access = atime;
    dobject->date = date;
    dobject->last_modified = last_modified;
    dobject->expires = expires;
    if(fd >= 0) close(fd);
    return dobject;

 fail:
    if(buf_is_chunk)
        dispose_chunk(buf);
    else
        free(buf);
 fail2:
    if(fd >= 0) close(fd);
    if(location) free(location);
    return NULL;
}    
    

DiskObjectPtr
processObject(DiskObjectPtr dobjects, char *filename, struct stat *sb)
{
    DiskObjectPtr dobject = NULL;
    int c = 0;

    dobject = readDiskObject((char*)filename, sb);
    if(dobject == NULL)
        return dobjects;

    if(!dobjects ||
       (c = strcmp(dobject->location, dobjects->location)) <= 0) {
        if(dobjects && c == 0) {
            mergeDobjects(dobjects, dobject);
        } else {
            dobject->next = dobjects;
            dobjects = dobject;
        }
    } else {
        DiskObjectPtr other = dobjects;
        while(other->next) {
            c = strcmp(dobject->location, other->next->location);
            if(c < 0)
                break;
            other = other->next;
        }
        if(strcmp(dobject->location, other->location) == 0) {
            mergeDobjects(other, dobject);
        } else {
            dobject->next = other->next;
            other->next = dobject;
        }
    }
    return dobjects;
}

/* Determine whether p is below root */
static int
filter(DiskObjectPtr p, const char *root, int n, int recursive)
{
    char *cp;
    int m = strlen(p->location);
    if(m < n)
        return 0;
    if(memcmp(root, p->location, n) != 0)
        return 0;
    if(recursive)
        return 1;
    if(m == 0 || p->location[m - 1] == '/')
        return 1;
    cp = strchr(p->location + n, '/');
    if(cp && cp - p->location != m - 1)
        return 0;
    return 1;
}

/* Filter out all disk objects that are not under root */
DiskObjectPtr
filterDiskObjects(DiskObjectPtr from, const char *root, int recursive)
{
    int n = strlen(root);
    DiskObjectPtr p, q;

    while(from && !filter(from, root, n, recursive)) {
        p = from;
        from = p->next;
        free(p->location);
        free(p);
    }

    p = from;
    while(p && p->next) {
        if(!filter(p->next, root, n, recursive)) {
            q = p->next;
            p->next = q->next;
            free(q->location);
            free(q);
        } else {
            p = p->next;
        }
    }
    return from;
}

DiskObjectPtr
insertRoot(DiskObjectPtr from, const char *root)
{
    DiskObjectPtr p;

    p = from;
    while(p) {
        if(strcmp(root, p->location) == 0)
            return from;
        p = p->next;
    }

    p = malloc(sizeof(DiskObjectRec));
    if(!p) return from;
    p->location = strdup(root);
    if(p->location == NULL) {
        free(p);
        return from;
    }
    p->filename = NULL;
    p->length = -1;
    p->size = -1;
    p->age = -1;
    p->access = -1;
    p->last_modified = -1;
    p->expires = -1;
    p->next = from;
    return p;
}

/* Insert all missing directories in a sorted list of dobjects */
DiskObjectPtr
insertDirs(DiskObjectPtr from)
{
    DiskObjectPtr p, q, new;
    int n, m;
    char *cp;

    p = NULL; q = from;
    while(q) {
        n = strlen(q->location);
        if(n > 0 && q->location[n - 1] != '/') {
            cp = strrchr(q->location, '/');
            m = cp - q->location + 1;
            if(cp && (!p || strlen(p->location) < m ||
                      memcmp(p->location, q->location, m) != 0)) {
                new = malloc(sizeof(DiskObjectRec));
                if(!new) break;
                new->location = strdup_n(q->location, m);
                if(new->location == NULL) {
                    free(new);
                    break;
                }
                new->filename = NULL;
                new->length = -1;
                new->size = -1;
                new->age = -1;
                new->access = -1;
                new->last_modified = -1;
                new->expires = -1;
                new->next = q;
                if(p)
                    p->next = new;
                else
                    from = new;
            }
        }
        p = q;
        q = q->next;
    }
    return from;
}
        
void
indexDiskObjects(FILE *out, const char *root, int recursive)
{
    int n, i, isdir;
    DIR *dir;
    struct dirent *dirent;
    char buf[1024];
    char *fts_argv[2];
    FTS *fts;
    FTSENT *fe;
    DiskObjectPtr dobjects = NULL;
    char *of = root[0] == '\0' ? "" : " of ";

    fprintf(out, "<!DOCTYPE HTML PUBLIC "
            "\"-//W3C//DTD HTML 4.01 Transitional//EN\" "
            "\"http://www.w3.org/TR/html4/loose.dtd\">\n"
            "<html><head>\n"
            "<title>%s%s%s</title>\n"
            "</head><body>\n"
            "<h1>%s%s%s</h1>\n",
            recursive ? "Recursive index" : "Index", of, root,
            recursive ? "Recursive index" : "Index", of, root);

    if(diskCacheRoot == NULL || diskCacheRoot->length <= 0) {
        fprintf(out, "<p>No <tt>diskCacheRoot</tt>.</p>\n");
        goto trailer;
    }

    if(diskCacheRoot->length >= 1024) {
        fprintf(out,
                "<p>The value of <tt>diskCacheRoot</tt> is "
                "too long (%d).</p>\n",
                diskCacheRoot->length);
        goto trailer;
    }

    if(strlen(root) < 8) {
        memcpy(buf, diskCacheRoot->string, diskCacheRoot->length);
        buf[diskCacheRoot->length] = '\0';
        n = diskCacheRoot->length;
    } else {
        n = urlDirname(buf, 1024, root, strlen(root));
    }
    if(n > 0) {
        if(recursive) {
            dir = NULL;
            fts_argv[0] = buf;
            fts_argv[1] = NULL;
            fts = fts_open(fts_argv, FTS_LOGICAL, NULL);
            if(fts) {
                while(1) {
                    fe = fts_read(fts);
                    if(!fe) break;
                    if(fe->fts_info != FTS_DP)
                        dobjects =
                            processObject(dobjects,
                                          fe->fts_path,
                                          fe->fts_info == FTS_NS ||
                                          fe->fts_info == FTS_NSOK ?
                                          fe->fts_statp : NULL);
                }
                fts_close(fts);
            }
        } else {
            dir = opendir(buf);
            if(dir) {
                while(1) {
                    dirent = readdir(dir);
                    if(!dirent) break;
                    if(n + strlen(dirent->d_name) < 1024) {
                        strcpy(buf + n, dirent->d_name);
                    } else {
                        continue;
                    }
                    dobjects = processObject(dobjects, buf, NULL);
                }
                closedir(dir);
            } else {
                fprintf(out, "<p>Couldn't open directory: %s (%d).</p>\n",
                        strerror(errno), errno);
                goto trailer;
            }
        }
    }

    if(dobjects) {
        DiskObjectPtr dobject;
        int entryno;
        dobjects = insertRoot(dobjects, root);
        dobjects = insertDirs(dobjects);
        dobjects = filterDiskObjects(dobjects, root, recursive);
        dobject = dobjects;
        buf[0] = '\0';
        alternatingHttpStyle(out, "diskcachelist");
        fprintf(out, "<table id=diskcachelist>\n");
        fprintf(out, "<tbody>\n");
        entryno = 0;
        while(dobjects) {
            dobject = dobjects;
            i = strlen(dobject->location);
            isdir = (i == 0 || dobject->location[i - 1] == '/');
            if(entryno % 2)
                fprintf(out, "<tr class=odd>");
            else
                fprintf(out, "<tr class=even>");
            if(dobject->size >= 0) {
                fprintf(out, "<td><a href=\"%s\"><tt>",
                        dobject->location);
                htmlPrint(out,
                          dobject->location, strlen(dobject->location));
                fprintf(out, "</tt></a></td> ");
                if(dobject->length >= 0) {
                    if(dobject->size == dobject->length)
                        fprintf(out, "<td>%d</td> ", dobject->length);
                    else
                        fprintf(out, "<td>%d/%d</td> ",
                               dobject->size, dobject->length);
                } else {
                    /* Avoid a trigraph. */
                    fprintf(out, "<td>%d/<em>??" "?</em></td> ", dobject->size);
                }
                if(dobject->last_modified >= 0) {
                    struct tm *tm = gmtime(&dobject->last_modified);
                    if(tm == NULL)
                        n = -1;
                    else
                        n = strftime(buf, 1024, "%d.%m.%Y", tm);
                } else
                    n = -1;
                if(n > 0) {
                    buf[n] = '\0';
                    fprintf(out, "<td>%s</td> ", buf);
                } else {
                    fprintf(out, "<td></td>");
                }
                
                if(dobject->date >= 0) {
                    struct tm *tm = gmtime(&dobject->date);
                    if(tm == NULL)
                        n = -1;
                    else
                        n = strftime(buf, 1024, "%d.%m.%Y", tm);
                } else
                    n = -1;
                if(n > 0) {
                    buf[n] = '\0';
                    fprintf(out, "<td>%s</td>", buf);
                } else {
                    fprintf(out, "<td></td>");
                }
            } else {
                fprintf(out, "<td><tt>");
                htmlPrint(out, dobject->location,
                          strlen(dobject->location));
                fprintf(out, "</tt></td><td></td><td></td><td></td>");
            }
            if(isdir) {
                fprintf(out, "<td><a href=\"/polipo/index?%s\">plain</a></td>"
                        "<td><a href=\"/polipo/recursive-index?%s\">"
                        "recursive</a></td>",
                        dobject->location, dobject->location);
            }
            fprintf(out, "</tr>\n");
            entryno++;
            dobjects = dobject->next;
            free(dobject->location);
            free(dobject->filename);
            free(dobject);
        }
        fprintf(out, "</tbody>\n");
        fprintf(out, "</table>\n");
    }

 trailer:
    fprintf(out, "<p><a href=\"/polipo/\">back</a></p>\n");
    fprintf(out, "</body></html>\n");
    return;
}

static int
checkForZeroes(char *buf, int n)
{
    int i, j;
    unsigned long *lbuf = (unsigned long *)buf;
    assert(n % sizeof(unsigned long) == 0);

    for(i = 0; i * sizeof(unsigned long) < n; i++) {
        if(lbuf[i] != 0L)
            return i * sizeof(unsigned long);
    }
    for(j = 0; i * sizeof(unsigned long) + j < n; j++) {
        if(buf[i * sizeof(unsigned long) + j] != 0)
            break;
    }

    return i * sizeof(unsigned long) + j;
}

static int
copyFile(int from, char *filename, int n)
{
    char *buf;
    int to, offset, nread, nzeroes, rc;

    buf = malloc(CHUNK_SIZE);
    if(buf == NULL)
        return -1;

    to = open(filename, O_RDWR | O_CREAT | O_EXCL | O_BINARY,
	      diskCacheFilePermissions);
    if(to < 0) {
        free(buf);
        return -1;
    }

    offset = 0;
    while(offset < n) {
        nread = read(from, buf, MIN(CHUNK_SIZE, n - offset));
        if(nread <= 0)
            break;
        nzeroes = checkForZeroes(buf, nread & -8);
        if(nzeroes > 0) {
            /* I like holes */
            rc = lseek(to, nzeroes, SEEK_CUR);
            if(rc != offset + nzeroes) {
                if(rc < 0)
                    do_log_error(L_ERROR, errno, "Couldn't extend file");
                else
                    do_log(L_ERROR, 
                           "Couldn't extend file: "
                           "unexpected offset %d != %d + %d.\n",
                           rc, offset, nread);
                break;
            }
        }
        if(nread > nzeroes) {
            rc = write(to, buf + nzeroes, nread - nzeroes);
            if(rc != nread - nzeroes) {
                if(rc < 0)
                    do_log_error(L_ERROR, errno, "Couldn't write");
                else
                    do_log(L_ERROR, "Short write.\n");
                break;
            }
        }
        offset += nread;
    }
    free(buf);
    close(to);
    if(offset <= 0)
        unlink(filename);       /* something went wrong straight away */
    return 1;
}

static long int
expireFile(char *filename, struct stat *sb,
           int *considered, int *unlinked, int *truncated)
{
    DiskObjectPtr dobject = NULL;
    time_t t;
    int fd, rc;
    long int ret = sb->st_size;

    if(!preciseExpiry) {
        t = sb->st_mtime;
        if(t > current_time.tv_sec + 1) {
            do_log(L_WARN, "File %s has access time in the future.\n",
                   filename);
            t = current_time.tv_sec;
        }
        
        if(t > current_time.tv_sec - diskCacheUnlinkTime &&
           (sb->st_size < diskCacheTruncateSize ||
            t > current_time.tv_sec - diskCacheTruncateTime))
            return ret;
    }
    
    (*considered)++;

    dobject = readDiskObject(filename, sb);
    if(!dobject) {
        do_log(L_ERROR, "Incorrect disk entry %s -- removing.\n",
               scrub(filename));
        rc = unlink(filename);
        if(rc < 0) {
            do_log_error(L_ERROR, errno,
                         "Couldn't unlink %s", scrub(filename));
            return ret;
        } else {
            (*unlinked)++;
            return 0;
        }
    }
    
    t = dobject->access;
    if(t < 0) t = dobject->age;
    if(t < 0) t = dobject->date;
    
    if(t > current_time.tv_sec)
        do_log(L_WARN, 
               "Disk entry %s (%s) has access time in the future.\n",
               scrub(dobject->location), scrub(dobject->filename));
    
    if(t < current_time.tv_sec - diskCacheUnlinkTime) {
        rc = unlink(dobject->filename);
        if(rc < 0) {
            do_log_error(L_ERROR, errno, "Couldn't unlink %s",
                         scrub(filename));
        } else {
            (*unlinked)++;
            ret = 0;
        }
    } else if(dobject->size > 
              diskCacheTruncateSize + 4 * dobject->body_offset && 
              t < current_time.tv_sec - diskCacheTruncateTime) {
        /* We need to copy rather than simply truncate in place: the
           latter would confuse a running polipo. */
        fd = open(dobject->filename, O_RDONLY | O_BINARY, 0);
        rc = unlink(dobject->filename);
        if(rc < 0) {
            do_log_error(L_ERROR, errno, "Couldn't unlink %s",
                         scrub(filename));
            close(fd);
            fd = -1;
        } else {
            (*unlinked)++;
            copyFile(fd, dobject->filename,
                     dobject->body_offset + diskCacheTruncateSize);
            close(fd);
            (*unlinked)--;
            (*truncated)++;
            ret = sb->st_size - dobject->body_offset + diskCacheTruncateSize;
        }
    }
    free(dobject->location);
    free(dobject->filename);
    free(dobject);
    return ret;
}
    
void
expireDiskObjects()
{
    int rc;
    char *fts_argv[2];
    FTS *fts;
    FTSENT *fe;
    int files = 0, considered = 0, unlinked = 0, truncated = 0;
    int dirs = 0, rmdirs = 0;
    long left = 0, total = 0;

    if(diskCacheRoot == NULL || 
       diskCacheRoot->length <= 0 || diskCacheRoot->string[0] != '/')
        return;

    fts_argv[0] = diskCacheRoot->string;
    fts_argv[1] = NULL;
    fts = fts_open(fts_argv, FTS_LOGICAL, NULL);
    if(fts == NULL) {
        do_log_error(L_ERROR, errno, "Couldn't fts_open disk cache");
    } else {
        while(1) {
            gettimeofday(&current_time, NULL);

            fe = fts_read(fts);
            if(!fe) break;

            if(fe->fts_info == FTS_D)
                continue;

            if(fe->fts_info == FTS_DP || fe->fts_info == FTS_DC ||
               fe->fts_info == FTS_DNR) {
                if(fe->fts_accpath[0] == '/' &&
                   strlen(fe->fts_accpath) <= diskCacheRoot->length)
                    continue;
                dirs++;
                rc = rmdir(fe->fts_accpath);
                if(rc >= 0)
                    rmdirs++;
                else if(errno != ENOTEMPTY && errno != EEXIST)
                    do_log_error(L_ERROR, errno,
                                 "Couldn't remove directory %s",
                                 scrub(fe->fts_accpath));
                continue;
            } else if(fe->fts_info == FTS_NS) {
                do_log_error(L_ERROR, fe->fts_errno, "Couldn't stat file %s",
                             scrub(fe->fts_accpath));
                continue;
            } else if(fe->fts_info == FTS_ERR) {
                do_log_error(L_ERROR, fe->fts_errno,
                             "Couldn't fts_read disk cache");
                break;
            }

            if(!S_ISREG(fe->fts_statp->st_mode)) {
                do_log(L_ERROR, "Unexpected file %s type 0%o.\n", 
                       fe->fts_accpath, (unsigned int)fe->fts_statp->st_mode);
                continue;
            }

            files++;
            left += expireFile(fe->fts_accpath, fe->fts_statp,
                               &considered, &unlinked, &truncated);
            total += fe->fts_statp->st_size;
        }
        fts_close(fts);
    }

    printf("Disk cache purged.\n");
    printf("%d files, %d considered, %d removed, %d truncated "
           "(%ldkB -> %ldkB).\n",
           files, considered, unlinked, truncated, total/1024, left/1024);
    printf("%d directories, %d removed.\n", dirs, rmdirs);
    return;
}

#else

void
preinitDiskcache()
{
    return;
}

void
initDiskcache()
{
    return;
}

int
writeoutToDisk(ObjectPtr object, int upto, int max)
{
    return 0;
}

int
destroyDiskEntry(ObjectPtr object, int d)
{
    return 0;
}

ObjectPtr
objectGetFromDisk(ObjectPtr object)
{
    return NULL;
}

int
objectFillFromDisk(ObjectPtr object, int offset, int chunks)
{
    return 0;
}

int
revalidateDiskEntry(ObjectPtr object)
{
    return 0;
}

void
dirtyDiskEntry(ObjectPtr object)
{
    return;
}

void
expireDiskObjects()
{
    do_log(L_ERROR, "Disk cache not supported in this version.\n");
}

int
diskEntrySize(ObjectPtr object)
{
    return -1;
}
#endif
