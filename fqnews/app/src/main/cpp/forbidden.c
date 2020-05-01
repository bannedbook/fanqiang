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

#ifndef NO_FORBIDDEN

#include <regex.h>
#include <assert.h>

typedef struct _Domain {
    int length;
    char domain[1];
} DomainRec, *DomainPtr;

AtomPtr forbiddenFile = NULL;
AtomPtr forbiddenUrl = NULL;
int forbiddenRedirectCode = 302;

AtomPtr redirector = NULL;
int redirectorRedirectCode = 302;

DomainPtr *forbiddenDomains = NULL;
regex_t *forbiddenRegex = NULL;

AtomPtr uncachableFile = NULL;
DomainPtr *uncachableDomains = NULL;
regex_t *uncachableRegex = NULL;

AtomPtr forbiddenTunnelsFile = NULL;
DomainPtr *forbiddenTunnelsDomains = NULL;
regex_t *forbiddenTunnelsRegex = NULL;


/* these three are only used internally by {parse,read}DomainFile */
/* to avoid having to pass it all as parameters */
static DomainPtr *domains;
static char *regexbuf;
static int rlen, rsize, dlen, dsize;

#ifndef NO_REDIRECTOR
static pid_t redirector_pid = 0;
static int redirector_read_fd = -1, redirector_write_fd = -1;
#define REDIRECTOR_BUFFER_SIZE 1024
static char *redirector_buffer = NULL;
RedirectRequestPtr redirector_request_first = NULL,
    redirector_request_last = NULL;
#endif

static int atomSetterForbidden(ConfigVariablePtr, void*);

void
preinitForbidden(void)
{
    CONFIG_VARIABLE_SETTABLE(forbiddenUrl, CONFIG_ATOM, configAtomSetter,
                             "URL to which forbidden requests "
                             "should be redirected.");
    CONFIG_VARIABLE_SETTABLE(forbiddenRedirectCode, CONFIG_INT,
                             configIntSetter,
                             "Redirect code, 301 or 302.");
    CONFIG_VARIABLE_SETTABLE(forbiddenFile, CONFIG_ATOM, atomSetterForbidden,
                             "File specifying forbidden URLs.");
#ifndef NO_REDIRECTOR
    CONFIG_VARIABLE_SETTABLE(redirector, CONFIG_ATOM, atomSetterForbidden,
                             "Squid-style redirector.");
    CONFIG_VARIABLE_SETTABLE(redirectorRedirectCode, CONFIG_INT,
                             configIntSetter,
                             "Redirect code to use with redirector.");
#endif
    CONFIG_VARIABLE_SETTABLE(uncachableFile, CONFIG_ATOM, atomSetterForbidden,
                             "File specifying uncachable URLs.");

    CONFIG_VARIABLE_SETTABLE(forbiddenTunnelsFile, CONFIG_ATOM, atomSetterForbidden,
                             "File specifying forbidden tunnels.");
}

static int
atomSetterForbidden(ConfigVariablePtr var, void *value)
{
    initForbidden();
    return configAtomSetter(var, value);
}

int
readDomainFile(char *filename)
{
    FILE *in;
    char buf[512];
    char *rs;
    int i, j, is_regex, start;

    in = fopen(filename, "r");
    if(in == NULL) {
        if(errno != ENOENT)
            do_log_error(L_ERROR, errno, "Couldn't open file %s", filename);
        return -1;
    }

    while(1) {
        rs = fgets(buf, 512, in);
        if(rs == NULL)
            break;
        for(i = 0; i < 512; i++) {
            if(buf[i] != ' ' && buf[i] != '\t')
                break;
        }
        start = i;
        for(i = start; i < 512; i++) {
            if(buf[i] == '#' || buf[i] == '\r' || buf[i] == '\n')
                break;
        }
        while(i > start) {
            if(buf[i - 1] != ' ' && buf[i - 1] != '\t')
                break;
            i--;
        }

        if(i <= start)
            continue;

        /* The significant part of the line is now between start and i */

        is_regex = 0;
        for(j = start; j < i; j++) {
            if(buf[j] == '\\' || buf[j] == '*' || buf[j] == '/') {
                is_regex = 1;
                break;
            }
        }

        if(is_regex) {
            while(rlen + i - start + 8 >= rsize) {
                char *new_regexbuf;
                new_regexbuf = realloc(regexbuf, rsize * 2 + 1);
                if(new_regexbuf == NULL) {
                    do_log(L_ERROR, "Couldn't reallocate regex.\n");
                    fclose(in);
                    return -1;
                }
                regexbuf = new_regexbuf;
                rsize = rsize * 2 + 1;
            }
            if(rlen != 0)
                rlen = snnprintf(regexbuf, rlen, rsize, "|");
            rlen = snnprintf(regexbuf, rlen, rsize, "(");
            rlen = snnprint_n(regexbuf, rlen, rsize, buf + start, i - start);
            rlen = snnprintf(regexbuf, rlen, rsize, ")");
        } else {
            DomainPtr new_domain;
            if(dlen >= dsize - 1) {
                DomainPtr *new_domains;
                new_domains = realloc(domains, (dsize * 2 + 1) *
                                      sizeof(DomainPtr));
                if(new_domains == NULL) {
                    do_log(L_ERROR,
                           "Couldn't reallocate domain list.\n");
                    fclose(in);
                    return -1;
                }
                domains = new_domains;
                dsize = dsize * 2 + 1;
            }
            new_domain = malloc(sizeof(DomainRec) - 1 + i - start);
            if(new_domain == NULL) {
                do_log(L_ERROR, "Couldn't allocate domain.\n");
                fclose(in);
                return -1;
            }
            new_domain->length = i - start;
            memcpy(new_domain->domain, buf + start, i - start);
            domains[dlen++] = new_domain;
        }
    }
    fclose(in);
    return 1;
}

void
parseDomainFile(AtomPtr file,
                DomainPtr **domains_return, regex_t **regex_return)
{
    struct stat ss;
    int rc;

    if(*domains_return) {
        DomainPtr *domain = *domains_return;
        while(*domain) {
            free(*domain);
            domain++;
        }
        free(*domains_return);
        *domains_return = NULL;
    }

    if(*regex_return) {
        regfree(*regex_return);
        *regex_return = NULL;
    }

    if(!file || file->length == 0)
        return;

    domains = malloc(64 * sizeof(DomainPtr));
    if(domains == NULL) {
        do_log(L_ERROR, "Couldn't allocate domain list.\n");
        return;
    }
    dlen = 0;
    dsize = 64;

    regexbuf = malloc(512);
    if(regexbuf == NULL) {
        do_log(L_ERROR, "Couldn't allocate regex.\n");
        free(domains);
        return;
    }
    rlen = 0;
    rsize = 512;

    rc = stat(file->string, &ss);
    if(rc < 0) {
        if(errno != ENOENT)
            do_log_error(L_WARN, errno, "Couldn't stat file %s", file->string);
    } else {
        if(!S_ISDIR(ss.st_mode))
            readDomainFile(file->string);
        else {
            char *fts_argv[2];
            FTS *fts;
            FTSENT *fe;
            fts_argv[0] = file->string;
            fts_argv[1] = NULL;
            fts = fts_open(fts_argv, FTS_LOGICAL, NULL);
            if(fts) {
                while(1) {
                    fe = fts_read(fts);
                    if(!fe) break;
                    if(fe->fts_info != FTS_D && fe->fts_info != FTS_DP &&
                       fe->fts_info != FTS_DC && fe->fts_info != FTS_DNR)
                        readDomainFile(fe->fts_accpath);
                }
                fts_close(fts);
            } else {
                do_log_error(L_ERROR, errno,
                             "Couldn't scan directory %s", file->string);
            }
        }
    }

    if(dlen > 0) {
        domains[dlen] = NULL;
    } else {
        free(domains);
        domains = NULL;
    }

    regex_t *regex;

    if(rlen > 0) {
        regex = malloc(sizeof(regex_t));
        rc = regcomp(regex, regexbuf, REG_EXTENDED | REG_NOSUB);
        if(rc != 0) {
            char errbuf[100];
            regerror(rc, regex, errbuf, 100);
            do_log(L_ERROR, "Couldn't compile regex: %s.\n", errbuf);
            free(regex);
            regex = NULL;
        }
    } else {
        regex = NULL;
    }
    free(regexbuf);

    *domains_return = domains;
    *regex_return = regex;

    return;
}

void
initForbidden(void)
{
    redirectorKill();

    if(forbiddenFile)
        forbiddenFile = expandTilde(forbiddenFile);

    if(forbiddenFile == NULL) {
        forbiddenFile = expandTilde(internAtom("~/.polipo-forbidden"));
        if(forbiddenFile) {
            if(access(forbiddenFile->string, F_OK) < 0) {
                releaseAtom(forbiddenFile);
                forbiddenFile = NULL;
            }
        }
    }

    if(forbiddenFile == NULL) {
        if(access("/etc/polipo/forbidden", F_OK) >= 0)
            forbiddenFile = internAtom("/etc/polipo/forbidden");
    }

    parseDomainFile(forbiddenFile, &forbiddenDomains, &forbiddenRegex);


    if(uncachableFile)
        uncachableFile = expandTilde(uncachableFile);

    if(uncachableFile == NULL) {
        uncachableFile = expandTilde(internAtom("~/.polipo-uncachable"));
        if(uncachableFile) {
            if(access(uncachableFile->string, F_OK) < 0) {
                releaseAtom(uncachableFile);
                uncachableFile = NULL;
            }
        }
    }

    if(uncachableFile == NULL) {
        if(access("/etc/polipo/uncachable", F_OK) >= 0)
            uncachableFile = internAtom("/etc/polipo/uncachable");
    }

    parseDomainFile(uncachableFile, &uncachableDomains, &uncachableRegex);

    if(forbiddenTunnelsFile)
        forbiddenTunnelsFile = expandTilde(forbiddenTunnelsFile);
    
    if(forbiddenTunnelsFile == NULL) {
        forbiddenTunnelsFile = expandTilde(internAtom("~/.polipo-forbiddenTunnels"));
        if(forbiddenTunnelsFile) {
            if(access(forbiddenTunnelsFile->string, F_OK) < 0) {
                releaseAtom(forbiddenTunnelsFile);
                forbiddenTunnelsFile = NULL;
            }
        }
    }
    
    if(forbiddenTunnelsFile == NULL) {
        if(access("/etc/polipo/forbiddenTunnels", F_OK) >= 0)
            forbiddenTunnelsFile = internAtom("/etc/polipo/forbiddenTunnels");
    }
    
    parseDomainFile(forbiddenTunnelsFile, &forbiddenTunnelsDomains, &forbiddenTunnelsRegex);
    //
    
    return;
}

int
tunnelIsMatched(char *url, int lurl, char *hostname, int lhost)
{
    DomainPtr *domain, *domains;
    
    domains=forbiddenTunnelsDomains;
    if (domains) {
	domain = domains;
	while(*domain) {
	    if (lhost == (*domain)->length && 
		memcmp(hostname, (*domain)->domain, lhost)==0)
		return 1;
	    domain++;
	}
    }

    if(forbiddenTunnelsRegex) {
	if(!regexec(forbiddenTunnelsRegex, url, 0, NULL, 0))
	    return 1;
    }
    return 0;
}

int
urlIsMatched(char *url, int length, DomainPtr *domains, regex_t *regex)
{
    /* This requires url to be NUL-terminated. */
    assert(url[length] == '\0');

    if(length < 8)
        return 0;

    if(lwrcmp(url, "http://", 7) != 0)
        return 0;

    if(domains) {
        int i;
        DomainPtr *domain;
        for(i = 8; i < length; i++) {
            if(url[i] == '/')
                break;
        }
        domain = domains;
        while(*domain) {
            if((*domain)->length <= (i - 7) &&
               (url[i - (*domain)->length - 1] == '.' ||
                url[i - (*domain)->length - 1] == '/') &&
               memcmp(url + i - (*domain)->length,
                      (*domain)->domain,
                      (*domain)->length) == 0)
                return 1;
            domain++;
        }
    }

    if(regex)
        return !regexec(regex, url, 0, NULL, 0);

    return 0;
}

int
urlIsUncachable(char *url, int length)
{
    return urlIsMatched(url, length, uncachableDomains, uncachableRegex);
}

int
urlForbidden(AtomPtr url,
             int (*handler)(int, AtomPtr, AtomPtr, AtomPtr, void*),
             void *closure)
{
    int forbidden = urlIsMatched(url->string, url->length,
                                 forbiddenDomains, forbiddenRegex);
    int code = 0;
    AtomPtr message = NULL, headers = NULL;


    if(forbidden) {
        message = internAtomF("Forbidden URL %s", url->string);
        if(forbiddenUrl) {
            code = forbiddenRedirectCode;
            headers = internAtomF("\r\nLocation: %s", forbiddenUrl->string);
        } else {
            code = 403;
        }
    }

#ifndef NO_REDIRECTOR
    if(code == 0 && redirector) {
        RedirectRequestPtr request;
        request = malloc(sizeof(RedirectRequestRec));
        if(request == NULL) {
            do_log(L_ERROR, "Couldn't allocate redirect request.\n");
            goto done;
        }
        request->url = url;
        request->handler = handler;
        request->data = closure;
        if(redirector_request_first == NULL)
            redirector_request_first = request;
        else
            redirector_request_last->next = request;
        redirector_request_last = request;
        request->next = NULL;
        if(request == redirector_request_first)
            redirectorTrigger();
        return 1;
    }

#endif

 done:
    handler(code, url, message, headers, closure);
    return 1;
}

#ifndef NO_REDIRECTOR
static void
logExitStatus(int status)
{
    if(WIFEXITED(status) && WEXITSTATUS(status) == 142)
        /* See child code in runRedirector */
        do_log(L_ERROR, "Couldn't start redirector.\n");
    else {
        char *reason =
            WIFEXITED(status) ? "with status" :
            WIFSIGNALED(status) ? "on signal" :
            "with unknown status";
        int value =
            WIFEXITED(status) ? WEXITSTATUS(status) :
            WIFSIGNALED(status) ? WTERMSIG(status) :
            status;
        do_log(L_ERROR,
               "Redirector exited %s %d.\n", reason, value);
    }
}

void
redirectorKill(void)
{
    int rc, status, dead;

    if(redirector_read_fd >= 0) {
        rc = waitpid(redirector_pid, &status, WNOHANG);
        dead = (rc > 0);
        close(redirector_read_fd);
        redirector_read_fd = -1;
        close(redirector_write_fd);
        redirector_write_fd = -1;
        if(!dead) {
            rc = kill(redirector_pid, SIGTERM);
            if(rc < 0 && errno != ESRCH) {
                do_log_error(L_ERROR, errno, "Couldn't kill redirector");
                redirector_pid = -1;
                return;
            }
            do {
                rc = waitpid(redirector_pid, &status, 0);
            } while(rc < 0 && errno == EINTR);
            if(rc < 0)
                do_log_error(L_ERROR, errno,
                             "Couldn't wait for redirector's death");
        } else
            logExitStatus(status);
        redirector_pid = -1;
    }
}

static void
redirectorDestroyRequest(RedirectRequestPtr request)
{
    assert(redirector_request_first == request);
    redirector_request_first = request->next;
    if(redirector_request_first == NULL)
        redirector_request_last = NULL;
    free(request);
}

void
redirectorTrigger(void)
{
    RedirectRequestPtr request = redirector_request_first;
    int rc;

    if(!request)
        return;

    if(redirector_read_fd < 0) {
        rc = runRedirector(&redirector_pid,
                           &redirector_read_fd, &redirector_write_fd);
        if(rc < 0) {
            request->handler(rc, request->url, NULL, NULL, request->data);
            redirectorDestroyRequest(request);
            return;
        }
    }
    do_stream_2(IO_WRITE, redirector_write_fd, 0,
                request->url->string, request->url->length,
                "\n", 1,
                redirectorStreamHandler1, request);
}

int
redirectorStreamHandler1(int status,
                         FdEventHandlerPtr event,
                         StreamRequestPtr srequest)
{
    RedirectRequestPtr request = (RedirectRequestPtr)srequest->data;

    if(status) {
        if(status >= 0)
            status = -EPIPE;
        do_log_error(L_ERROR, -status, "Write to redirector failed");
        goto fail;
    }

    if(!streamRequestDone(srequest))
        return 0;

    do_stream(IO_READ, redirector_read_fd, 0,
              redirector_buffer, REDIRECTOR_BUFFER_SIZE,
              redirectorStreamHandler2, request);
    return 1;

 fail:
    request->handler(status < 0 ? status : -EPIPE,
                     request->url, NULL, NULL, request->data);
    redirectorDestroyRequest(request);
    redirectorKill();
    return 1;
}

int
redirectorStreamHandler2(int status,
                         FdEventHandlerPtr event,
                         StreamRequestPtr srequest)
{
    RedirectRequestPtr request = (RedirectRequestPtr)srequest->data;
    char *c;
    AtomPtr message;
    AtomPtr headers;
    int code;

    if(status < 0) {
        do_log_error(L_ERROR, -status, "Read from redirector failed");
        request->handler(status, request->url, NULL, NULL, request->data);
        goto kill;
    }
    c = memchr(redirector_buffer, '\n', srequest->offset);
    if(!c) {
        if(!status && srequest->offset < REDIRECTOR_BUFFER_SIZE)
            return 0;
        do_log(L_ERROR, "Redirector returned incomplete reply.\n");
        request->handler(-EREDIRECTOR, request->url, NULL, NULL, request->data);
        goto kill;
    }
    *c = '\0';

    if(srequest->offset > c + 1 - redirector_buffer)
        do_log(L_WARN, "Stray bytes in redirector output.\n");

    if(c > redirector_buffer + 1 &&
       (c - redirector_buffer != request->url->length ||
        memcmp(redirector_buffer, request->url->string,
               request->url->length) != 0)) {
        code = redirectorRedirectCode;
        message = internAtom("Redirected by external redirector");
        if(message == NULL) {
            request->handler(-ENOMEM, request->url, NULL, NULL, request->data);
            goto kill;
        }

        headers = internAtomF("\r\nLocation: %s", redirector_buffer);
        if(headers == NULL) {
            releaseAtom(message);
            request->handler(-ENOMEM, request->url, NULL, NULL, request->data);
            goto kill;
        }
    } else {
        code = 0;
        message = NULL;
        headers = NULL;
    }
    request->handler(code, request->url,
                     message, headers, request->data);
    goto cont;

 cont:
    redirectorDestroyRequest(request);
    redirectorTrigger();
    return 1;

 kill:
    redirectorKill();
    goto cont;
}

int
runRedirector(pid_t *pid_return, int *read_fd_return, int *write_fd_return)
{
    int rc, rc2, status;
    pid_t pid;
    int filedes1[2], filedes2[2];
    sigset_t ss, old_mask;

    assert(redirector);

    if(redirector_buffer == NULL) {
        redirector_buffer = malloc(REDIRECTOR_BUFFER_SIZE);
        if(redirector_buffer == NULL)
            return -errno;
    }

    rc = pipe(filedes1);
    if(rc < 0) {
        rc = -errno;
        goto fail1;
    }


    rc = pipe(filedes2);
    if(rc < 0) {
        rc = -errno;
        goto fail2;
    }

    fflush(stdout);
    fflush(stderr);
    flushLog();

    interestingSignals(&ss);
    do {
        rc = sigprocmask(SIG_BLOCK, &ss, &old_mask);
    } while (rc < 0 && errno == EINTR);
    if(rc < 0) {
        rc = -errno;
        goto fail3;
    }

    pid = fork();
    if(pid < 0) {
        rc = -errno;
        goto fail4;
    }

    if(pid > 0) {
        do {
            rc = sigprocmask(SIG_SETMASK, &old_mask, NULL);
        } while(rc < 0 && errno == EINTR);

        if(rc < 0) {
            rc = -errno;
            goto fail4;
        }

        rc = setNonblocking(filedes1[1], 1);
        if(rc >= 0)
            rc = setNonblocking(filedes2[0], 1);
        if(rc < 0) {
            rc = -errno;
            goto fail4;
        }

        /* This is completely unnecesary -- if the redirector cannot be
           started, redirectorStreamHandler1 will get EPIPE straight away --,
           but it improves error messages somewhat. */
        rc = waitpid(pid, &status, WNOHANG);
        if(rc > 0) {
            logExitStatus(status);
            rc = -EREDIRECTOR;
            goto fail4;
        } else if(rc < 0) {
            rc = -errno;
            goto fail4;
        }

        *read_fd_return = filedes2[0];
        *write_fd_return = filedes1[1];

        *pid_return = pid;
        /* This comes at the end so that the fail* labels can work */
        close(filedes1[0]);
        close(filedes2[1]);
    } else {
        close(filedes1[1]);
        close(filedes2[0]);
        uninitEvents();
        do {
            rc = sigprocmask(SIG_SETMASK, &old_mask, NULL);
        } while (rc < 0 && errno == EINTR);
        if(rc < 0)
            exit(142);

        if(filedes1[0] != 0)
            dup2(filedes1[0], 0);
        if(filedes2[1] != 1)
            dup2(filedes2[1], 1);

        execlp(redirector->string, redirector->string, NULL);
        exit(142);
        /* NOTREACHED */
    }
    return 1;

 fail4:
    do {
        rc2 = sigprocmask(SIG_SETMASK, &old_mask, NULL);
    } while(rc2 < 0 && errno == EINTR);
 fail3:
    close(filedes2[0]);
    close(filedes2[1]);
 fail2:
    close(filedes1[0]);
    close(filedes1[1]);
 fail1:
    free(redirector_buffer);
    redirector_buffer = NULL;
    return rc;
}

#else

void
redirectorKill(void)
{
    return;
}

#endif

#else

void
preinitForbidden()
{
    return;
}

void
initForbidden()
{
    return;
}

int
tunnelIsMatched(char *url, int lurl, char *hostname, int lhost)
{
    return 0;
}

int
urlIsUncachable(char *url, int length)
{
    return 0;
}

int
urlForbidden(AtomPtr url,
             int (*handler)(int, AtomPtr, AtomPtr, AtomPtr, void*),
             void *closure)
{
    handler(0, url, NULL, NULL, closure);
    return 1;
}

#endif
