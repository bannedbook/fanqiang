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

int disableLocalInterface = 0;
int disableConfiguration = 0;
int disableIndexing = 1;
int disableServersList = 1;

AtomPtr atomInitForbidden;
AtomPtr atomReopenLog;
AtomPtr atomDiscardObjects;
AtomPtr atomWriteoutObjects;
AtomPtr atomFreeChunkArenas;

void
preinitLocal()
{
    atomInitForbidden = internAtom("init-forbidden");
    atomReopenLog = internAtom("reopen-log");
    atomDiscardObjects = internAtom("discard-objects");
    atomWriteoutObjects = internAtom("writeout-objects");
    atomFreeChunkArenas = internAtom("free-chunk-arenas");

    /* These should not be settable for obvious reasons */
    CONFIG_VARIABLE(disableLocalInterface, CONFIG_BOOLEAN,
                    "Disable the local configuration pages.");
    CONFIG_VARIABLE(disableConfiguration, CONFIG_BOOLEAN,
                    "Disable reconfiguring Polipo at runtime.");
    CONFIG_VARIABLE(disableIndexing, CONFIG_BOOLEAN,
                    "Disable indexing of the local cache.");
    CONFIG_VARIABLE(disableServersList, CONFIG_BOOLEAN,
                    "Disable the list of known servers.");
}

static void fillSpecialObject(ObjectPtr, void (*)(FILE*, char*), void*);

int
httpLocalRequest(ObjectPtr object, int method, int from, int to,
                 HTTPRequestPtr requestor, void *closure)
{
    if(object->requestor == NULL)
        object->requestor = requestor;

    if(!disableLocalInterface && urlIsSpecial(object->key, object->key_size))
        return httpSpecialRequest(object, method, from, to, 
                                  requestor, closure);

    if(method >= METHOD_POST) {
        abortObject(object, 405, internAtom("Method not allowed"));
    } else if(object->flags & OBJECT_INITIAL) {
        /* objectFillFromDisk already did the real work but we have to
           make sure we don't get into an infinite loop. */
        abortObject(object, 404, internAtom("Not found"));
    }
    object->age = current_time.tv_sec;
    object->date = current_time.tv_sec;

    object->flags &= ~OBJECT_VALIDATING;
    notifyObject(object);
    return 1;
}

void
alternatingHttpStyle(FILE *out, char *id)
{
    fprintf(out,
            "<style type=\"text/css\">\n"
            "#%s tbody tr.even td { background-color: #eee; }\n"
            "#%s tbody tr.odd  td { background-color: #fff; }\n"
            "</style>\n", id, id);
}

static void
printConfig(FILE *out, char *dummy)
{
    fprintf(out,
            "<!DOCTYPE HTML PUBLIC "
            "\"-//W3C//DTD HTML 4.01 Transitional//EN\" "
            "\"http://www.w3.org/TR/html4/loose.dtd\">\n"
            "<html><head>\n"
            "<title>Polipo configuration</title>\n"
            "</head><body>\n"
            "<h1>Polipo configuration</h1>\n");
    printConfigVariables(out, 1);
    fprintf(out, "<p><a href=\"/polipo/\">back</a></p>");
    fprintf(out, "</body></html>\n");
}

#ifndef NO_DISK_CACHE

static void
recursiveIndexDiskObjects(FILE *out, char *root)
{
    indexDiskObjects(out, root, 1);
}

static void
plainIndexDiskObjects(FILE *out, char *root)
{
    indexDiskObjects(out, root, 0);
}
#endif

static void
serversList(FILE *out, char *dummy)
{
    listServers(out);
}

static int
matchUrl(char *base, ObjectPtr object)
{
    int n = strlen(base);
    if(object->key_size < n)
        return 0;
    if(memcmp(base, object->key, n) != 0)
        return 0;
    return (object->key_size == n) || (((char*)object->key)[n] == '?');
}
    
int 
httpSpecialRequest(ObjectPtr object, int method, int from, int to,
                   HTTPRequestPtr requestor, void *closure)
{
    char buffer[1024];
    int hlen;

    if(method >= METHOD_POST) {
        return httpSpecialSideRequest(object, method, from, to,
                                      requestor, closure);
    }

    if(!(object->flags & OBJECT_INITIAL)) {
        privatiseObject(object, 0);
        supersedeObject(object);
        object->flags &= ~(OBJECT_VALIDATING | OBJECT_INPROGRESS);
        notifyObject(object);
        return 1;
    }

    hlen = snnprintf(buffer, 0, 1024,
                     "\r\nServer: polipo"
                     "\r\nContent-Type: text/html");
    object->date = current_time.tv_sec;
    object->age = current_time.tv_sec;
    object->headers = internAtomN(buffer, hlen);
    object->code = 200;
    object->message = internAtom("Okay");
    object->flags &= ~OBJECT_INITIAL;
    object->flags |= OBJECT_DYNAMIC;

    if(object->key_size == 8 && memcmp(object->key, "/polipo/", 8) == 0) {
        objectPrintf(object, 0,
                     "<!DOCTYPE HTML PUBLIC "
                     "\"-//W3C//DTD HTML 4.01 Transitional//EN\" "
                     "\"http://www.w3.org/TR/html4/loose.dtd\">\n"
                     "<html><head>\n"
                     "<title>Polipo</title>\n"
                     "</head><body>\n"
                     "<h1>Polipo</h1>\n"
                     "<p><a href=\"status?\">Status report</a>.</p>\n"
                     "<p><a href=\"config?\">Current configuration</a>.</p>\n"
                     "<p><a href=\"servers?\">Known servers</a>.</p>\n"
#ifndef NO_DISK_CACHE
                     "<p><a href=\"index?\">Disk cache index</a>.</p>\n"
#endif
                     "</body></html>\n");
        object->length = object->size;
    } else if(matchUrl("/polipo/status", object)) {
        objectPrintf(object, 0,
                     "<!DOCTYPE HTML PUBLIC "
                     "\"-//W3C//DTD HTML 4.01 Transitional//EN\" "
                     "\"http://www.w3.org/TR/html4/loose.dtd\">\n"
                     "<html><head>\n"
                     "<title>Polipo status report</title>\n"
                     "</head><body>\n"
                     "<h1>Polipo proxy on %s:%d: status report</h1>\n"
                     "<p>The %s proxy on %s:%d is %s.</p>\n"
                     "<p>There are %d public and %d private objects "
                     "currently in memory using %d KB in %d chunks "
                     "(%d KB allocated).</p>\n"
                     "<p>There are %d atoms.</p>"
                     "<p><form method=POST action=\"/polipo/status?\">"
                     "<input type=submit name=\"init-forbidden\" "
                     "value=\"Read forbidden file\"></form>\n"
                     "<form method=POST action=\"/polipo/status?\">"
                     "<input type=submit name=\"writeout-objects\" "
                     "value=\"Write out in-memory cache\"></form>\n"
                     "<form method=POST action=\"/polipo/status?\">"
                     "<input type=submit name=\"discard-objects\" "
                     "value=\"Discard in-memory cache\"></form>\n"
                     "<form method=POST action=\"/polipo/status?\">"
                     "<input type=submit name=\"reopen-log\" "
                     "value=\"Reopen log file\"></form>\n"
                     "<form method=POST action=\"/polipo/status?\">"
                     "<input type=submit name=\"free-chunk-arenas\" "
                     "value=\"Free chunk arenas\"></form></p>\n"
                     "<p><a href=\"/polipo/\">back</a></p>"
                     "</body></html>\n",
                     proxyName->string, proxyPort,
                     cacheIsShared ? "shared" : "private",
                     proxyName->string, proxyPort,
                     proxyOffline ? "off line" :
                     (relaxTransparency ? 
                      "on line (transparency relaxed)" :
                      "on line"),
                     publicObjectCount, privateObjectCount,
                     used_chunks * CHUNK_SIZE / 1024, used_chunks,
                     totalChunkArenaSize() / 1024,
                     used_atoms);
        object->expires = current_time.tv_sec;
        object->length = object->size;
    } else if(matchUrl("/polipo/config", object)) {
        fillSpecialObject(object, printConfig, NULL);
        object->expires = current_time.tv_sec + 5;
#ifndef NO_DISK_CACHE
    } else if(matchUrl("/polipo/index", object)) {
        int len;
        char *root;
        if(disableIndexing) {
            abortObject(object, 403, internAtom("Action not allowed"));
            notifyObject(object);
            return 1;
        }
        len = MAX(0, object->key_size - 14);
        root = strdup_n((char*)object->key + 14, len);
        if(root == NULL) {
            abortObject(object, 503, internAtom("Couldn't allocate root"));
            notifyObject(object);
            return 1;
        }
        writeoutObjects(1);
        fillSpecialObject(object, plainIndexDiskObjects, root);
        free(root);
        object->expires = current_time.tv_sec + 5;
    } else if(matchUrl("/polipo/recursive-index", object)) {
        int len;
        char *root;
        if(disableIndexing) {
            abortObject(object, 403, internAtom("Action not allowed"));
            notifyObject(object);
            return 1;
        }
        len = MAX(0, object->key_size - 24);
        root = strdup_n((char*)object->key + 24, len);
        if(root == NULL) {
            abortObject(object, 503, internAtom("Couldn't allocate root"));
            notifyObject(object);
            return 1;
        }
        writeoutObjects(1);
        fillSpecialObject(object, recursiveIndexDiskObjects, root);
        free(root);
        object->expires = current_time.tv_sec + 20;
#endif
    } else if(matchUrl("/polipo/servers", object)) {
        if(disableServersList) {
            abortObject(object, 403, internAtom("Action not allowed"));
            notifyObject(object);
            return 1;
        }
        fillSpecialObject(object, serversList, NULL);
        object->expires = current_time.tv_sec + 2;
    } else {
        abortObject(object, 404, internAtom("Not found"));
    }

    object->flags &= ~OBJECT_VALIDATING;
    notifyObject(object);
    return 1;
}

int 
httpSpecialSideRequest(ObjectPtr object, int method, int from, int to,
                       HTTPRequestPtr requestor, void *closure)
{
    HTTPConnectionPtr client = requestor->connection;

    assert(client->request == requestor);

    if(method != METHOD_POST) {
        httpClientError(requestor, 405, internAtom("Method not allowed"));
        requestor->connection->flags &= ~CONN_READER;
        return 1;
    }

    if(requestor->flags & REQUEST_WAIT_CONTINUE) {
        httpClientError(requestor, 417, internAtom("Expectation failed"));
        requestor->connection->flags &= ~CONN_READER;
        return 1;
    }

    return httpSpecialDoSide(requestor);
}

int
httpSpecialDoSide(HTTPRequestPtr requestor)
{
    HTTPConnectionPtr client = requestor->connection;

    if(client->reqlen - client->reqbegin >= client->bodylen) {
        AtomPtr data;
        data = internAtomN(client->reqbuf + client->reqbegin,
                           client->reqlen - client->reqbegin);
        client->reqbegin = 0;
        client->reqlen = 0;
        if(data == NULL) {
            do_log(L_ERROR, "Couldn't allocate data.\n");
            httpClientError(requestor, 500,
                            internAtom("Couldn't allocate data"));
            return 1;
        }
        httpSpecialDoSideFinish(data, requestor);
        return 1;
    }

    if(client->reqlen - client->reqbegin >= CHUNK_SIZE) {
        httpClientError(requestor, 500, internAtom("POST too large"));
        return 1;
    }

    if(client->reqbegin > 0 && client->reqlen > client->reqbegin) {
        memmove(client->reqbuf, client->reqbuf + client->reqbegin,
                client->reqlen - client->reqbegin);
    }
    client->reqlen -= client->reqbegin;
    client->reqbegin = 0;

    do_stream(IO_READ | IO_NOTNOW, client->fd,
              client->reqlen, client->reqbuf, CHUNK_SIZE,
              httpSpecialClientSideHandler, client);
    return 1;
}

int
httpSpecialClientSideHandler(int status,
                             FdEventHandlerPtr event,
                             StreamRequestPtr srequest)
{
    HTTPConnectionPtr connection = srequest->data;
    HTTPRequestPtr request = connection->request;
    int push;

    if((request->object->flags & OBJECT_ABORTED) || 
       !(request->object->flags & OBJECT_INPROGRESS)) {
        httpClientDiscardBody(connection);
        httpClientError(request, 503, internAtom("Post aborted"));
        return 1;
    }
        
    if(status < 0) {
        do_log_error(L_ERROR, -status, "Reading from client");
        if(status == -EDOGRACEFUL)
            httpClientFinish(connection, 1);
        else
            httpClientFinish(connection, 2);
        return 1;
    }

    push = MIN(srequest->offset - connection->reqlen,
               connection->bodylen - connection->reqoffset);
    if(push > 0) {
        connection->reqlen += push;
        httpSpecialDoSide(request);
    }

    do_log(L_ERROR, "Incomplete client request.\n");
    connection->flags &= ~CONN_READER;
    httpClientRawError(connection, 502,
                       internAtom("Incomplete client request"), 1);
    return 1;
}

int
httpSpecialDoSideFinish(AtomPtr data, HTTPRequestPtr requestor)
{
    ObjectPtr object = requestor->object;

    if(matchUrl("/polipo/config", object)) {
        AtomListPtr list = NULL;
        int i, rc;

        if(disableConfiguration) {
            abortObject(object, 403, internAtom("Action not allowed"));
            goto out;
        }

        list = urlDecode(data->string, data->length);
        if(list == NULL) {
            abortObject(object, 400,
                        internAtom("Couldn't parse variable to set"));
            goto out;
        }
        for(i = 0; i < list->length; i++) {
            rc = parseConfigLine(list->list[i]->string, NULL, 0, 1);
            if(rc < 0) {
                abortObject(object, 400,
                            rc == -1 ?
                            internAtom("Couldn't parse variable to set") :
                            internAtom("Variable is not settable"));
                destroyAtomList(list);
                goto out;
            }
        }
        destroyAtomList(list);
        object->date = current_time.tv_sec;
        object->age = current_time.tv_sec;
        object->headers = internAtom("\r\nLocation: /polipo/config?");
        object->code = 303;
        object->message = internAtom("Done");
        object->flags &= ~OBJECT_INITIAL;
        object->length = 0;
    } else if(matchUrl("/polipo/status", object)) {
        AtomListPtr list = NULL;
        int i;

        if(disableConfiguration) {
            abortObject(object, 403, internAtom("Action not allowed"));
            goto out;
        }

        list = urlDecode(data->string, data->length);
        if(list == NULL) {
            abortObject(object, 400,
                        internAtom("Couldn't parse action"));
            goto out;
        }
        for(i = 0; i < list->length; i++) {
            char *equals = 
                memchr(list->list[i]->string, '=', list->list[i]->length);
            AtomPtr name = 
                equals ? 
                internAtomN(list->list[i]->string, 
                            equals - list->list[i]->string) :
                retainAtom(list->list[i]);
            if(name == atomInitForbidden)
                initForbidden();
            else if(name == atomReopenLog)
                reopenLog();
            else if(name == atomDiscardObjects)
                discardObjects(1, 0);
            else if(name == atomWriteoutObjects)
                writeoutObjects(1);
            else if(name == atomFreeChunkArenas)
                free_chunk_arenas();
            else {
                abortObject(object, 400, internAtomF("Unknown action %s",
                                                     name->string));
                releaseAtom(name);
                destroyAtomList(list);
                goto out;
            }
            releaseAtom(name);
        }
        destroyAtomList(list);
        object->date = current_time.tv_sec;
        object->age = current_time.tv_sec;
        object->headers = internAtom("\r\nLocation: /polipo/status?");
        object->code = 303;
        object->message = internAtom("Done");
        object->flags &= ~OBJECT_INITIAL;
        object->length = 0;
    } else {
        abortObject(object, 405, internAtom("Method not allowed"));
    }

 out:
    releaseAtom(data);
    notifyObject(object);
    requestor->connection->flags &= ~CONN_READER;
    return 1;
}

#ifdef HAVE_FORK
static void
fillSpecialObject(ObjectPtr object, void (*fn)(FILE*, char*), void* closure)
{
    int rc;
    int filedes[2];
    pid_t pid;
    sigset_t ss, old_mask;

    if(object->flags & OBJECT_INPROGRESS)
        return;

    rc = pipe(filedes);
    if(rc < 0) {
        do_log_error(L_ERROR, errno, "Couldn't create pipe");
        abortObject(object, 503,
                    internAtomError(errno, "Couldn't create pipe"));
        return;
    }

    fflush(stdout);
    fflush(stderr);
    flushLog();

    /* Block signals that we handle specially until the child can
       disable the handlers. */
    interestingSignals(&ss);
    /* I'm a little confused.  POSIX doesn't allow EINTR here, but I
       think that both Linux and SVR4 do. */
    do {
        rc = sigprocmask(SIG_BLOCK, &ss, &old_mask);
    } while (rc < 0 && errno == EINTR);
    if(rc < 0) {
        do_log_error(L_ERROR, errno, "Sigprocmask failed");
        abortObject(object, 503, internAtomError(errno, "Sigprocmask failed"));
        close(filedes[0]);
        close(filedes[1]);
        return;
    }
    
    pid = fork();
    if(pid < 0) {
        do_log_error(L_ERROR, errno, "Couldn't fork");
        abortObject(object, 503, internAtomError(errno, "Couldn't fork"));
        close(filedes[0]);
        close(filedes[1]);
        do {
            rc = sigprocmask(SIG_SETMASK, &old_mask, NULL);
        } while (rc < 0 && errno == EINTR);
        if(rc < 0) {
            do_log_error(L_ERROR, errno, "Couldn't restore signal mask");
            polipoExit();
        }
        return;
    }

    if(pid > 0) {
        SpecialRequestPtr request;
        close(filedes[1]);
        do {
            rc = sigprocmask(SIG_SETMASK, &old_mask, NULL);
        } while (rc < 0 && errno == EINTR);
        if(rc < 0) {
            do_log_error(L_ERROR, errno, "Couldn't restore signal mask");
            polipoExit();
            return;
        }

        request = malloc(sizeof(SpecialRequestRec));
        if(request == NULL) {
            kill(pid, SIGTERM);
            close(filedes[0]);
            abortObject(object, 503,
                        internAtom("Couldn't allocate request\n"));
            notifyObject(object);
            return;
        } else {
            request->buf = get_chunk();
            if(request->buf == NULL) {
                kill(pid, SIGTERM);
                close(filedes[0]);
                free(request);
                abortObject(object, 503,
                            internAtom("Couldn't allocate request\n"));
                notifyObject(object);
                return;
            }
        }
        object->flags |= OBJECT_INPROGRESS;
        retainObject(object);
        request->object = object;
        request->fd = filedes[0];
        request->pid = pid;
        request->offset = 0;
        /* Under any sensible scheduler, the child will run before the
           parent.  So no need for IO_NOTNOW. */
        do_stream(IO_READ, filedes[0], 0, request->buf, CHUNK_SIZE,
                  specialRequestHandler, request);
    } else {
        /* child */
        close(filedes[0]);
        uninitEvents();
        do {
            rc = sigprocmask(SIG_SETMASK, &old_mask, NULL);
        } while (rc < 0 && errno == EINTR);
        if(rc < 0)
            exit(1);

        if(filedes[1] != 1)
            dup2(filedes[1], 1);

        (*fn)(stdout, closure);
        exit(0);
    }
}

int
specialRequestHandler(int status, 
                      FdEventHandlerPtr event, StreamRequestPtr srequest)
{
    SpecialRequestPtr request = srequest->data;
    int rc;
    int killed = 0;

    if(status < 0) {
        kill(request->pid, SIGTERM);
        killed = 1;
        request->object->flags &= ~OBJECT_INPROGRESS;
        abortObject(request->object, 502,
                    internAtomError(-status, "Couldn't read from client"));
        goto done;
    }

    if(srequest->offset > 0) {
        rc = objectAddData(request->object, request->buf, 
                           request->offset, srequest->offset);
        if(rc < 0) {
            kill(request->pid, SIGTERM);
            killed = 1;
            request->object->flags &= ~OBJECT_INPROGRESS;
            abortObject(request->object, 503,
                        internAtom("Couldn't add data to connection"));
            goto done;
        }
        request->offset += srequest->offset;
    }
    if(status) {
        request->object->flags &= ~OBJECT_INPROGRESS;
        request->object->length = request->object->size;
        goto done;
    }

    /* If we're the only person interested in this object, let's abort
       it now. */
    if(request->object->refcount <= 1) {
        kill(request->pid, SIGTERM);
        killed = 1;
        request->object->flags &= ~OBJECT_INPROGRESS;
        abortObject(request->object, 500, internAtom("Aborted"));
        goto done;
    }
    notifyObject(request->object);
    do_stream(IO_READ | IO_NOTNOW, request->fd, 0, request->buf, CHUNK_SIZE,
              specialRequestHandler, request);
    return 1;

 done:
    close(request->fd);
    dispose_chunk(request->buf);
    releaseNotifyObject(request->object);
    /* That's a blocking wait.  It shouldn't block for long, as we've
       either already killed the child, or else we got EOF from it. */
    do {
        rc = waitpid(request->pid, &status, 0);
    } while(rc < 0 && errno == EINTR);
    if(rc < 0) {
        do_log(L_ERROR, "Wait for %d: %d\n", (int)request->pid, errno);
    } else {
        int normal = 
            (WIFEXITED(status) && WEXITSTATUS(status) == 0) ||
            (killed && WIFSIGNALED(status) && WTERMSIG(status) == SIGTERM);
        char *reason =
            WIFEXITED(status) ? "with status" : 
            WIFSIGNALED(status) ? "on signal" :
            "with unknown status";
        int value =
            WIFEXITED(status) ? WEXITSTATUS(status) :
            WIFSIGNALED(status) ? WTERMSIG(status) :
            status;
        do_log(normal ? D_CHILD : L_ERROR, 
               "Child %d exited %s %d.\n",
               (int)request->pid, reason, value);
    }
    free(request);
    return 1;
}
#else
static void
fillSpecialObject(ObjectPtr object, void (*fn)(FILE*, char*), void* closure)
{
    FILE *tmp = NULL;
    char *buf = NULL;
    int rc, len, offset;

    if(object->flags & OBJECT_INPROGRESS)
        return;

    buf = get_chunk();
    if(buf == NULL) {
        abortObject(object, 503, internAtom("Couldn't allocate chunk"));
        goto done;
    }

    tmp = tmpfile();
    if(tmp == NULL) {
        abortObject(object, 503, internAtom(pstrerror(errno)));
        goto done;
    }

    (*fn)(tmp, closure);
    fflush(tmp);

    rewind(tmp);
    offset = 0;
    while(1) {
        len = fread(buf, 1, CHUNK_SIZE, tmp);
        if(len <= 0 && ferror(tmp)) {
            abortObject(object, 503, internAtom(pstrerror(errno)));
            goto done;
        }
        if(len <= 0)
            break;

        rc = objectAddData(object, buf, offset, len);
        if(rc < 0) {
            abortObject(object, 503, internAtom("Couldn't add data to object"));
            goto done;
        }

        offset += len;
    }

    object->length = offset;

 done:
    if(buf)
        dispose_chunk(buf);
    if(tmp)
        fclose(tmp);
    notifyObject(object);
}
#endif
