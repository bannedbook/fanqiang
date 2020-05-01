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

#ifndef NO_STANDARD_RESOLVER
#ifndef NO_FANCY_RESOLVER
int dnsUseGethostbyname = 1;
#else 
const int dnsUseGethostbyname = 3;
#endif
#else
#ifndef NO_FANCY_RESOLVER
const int dnsUseGethostbyname = 0;
#else
#error use no resolver at all?
#endif
#endif

#ifndef NO_FANCY_RESOLVER
AtomPtr dnsNameServer = NULL;
int dnsMaxTimeout = 60;
#endif

#ifndef NO_STANDARD_RESOLVER
int dnsGethostbynameTtl = 1200;
#endif

int dnsNegativeTtl = 120;

#ifdef HAVE_IPv6
int dnsQueryIPv6 = 2;
#else
const int dnsQueryIPv6 = 0;
#endif

typedef struct _DnsQuery {
    unsigned id;
    AtomPtr name;
    ObjectPtr object;
    AtomPtr inet4, inet6;
    time_t ttl4, ttl6;
    time_t time;
    int timeout;
    TimeEventHandlerPtr timeout_handler;
    struct _DnsQuery *next;
} DnsQueryRec, *DnsQueryPtr;

union {
    struct sockaddr sa;
    struct sockaddr_in sin;
#ifdef HAVE_IPv6
    struct sockaddr_in6 sin6;
#endif
} nameserverAddress_storage;

#ifndef NO_FANCY_RESOLVER
static AtomPtr atomLocalhost, atomLocalhostDot;

#define nameserverAddress nameserverAddress_storage.sa

static DnsQueryPtr inFlightDnsQueries;
static DnsQueryPtr inFlightDnsQueriesLast;
#endif

static int really_do_gethostbyname(AtomPtr name, ObjectPtr object);
static int really_do_dns(AtomPtr name, ObjectPtr object);

#ifndef NO_FANCY_RESOLVER
static int stringToLabels(char *buf, int offset, int n, char *string);
static int labelsToString(char *buf, int offset, int n, char *d, 
                          int m, int *j_return);
static int dnsBuildQuery(int id, char *buf, int offset, int n,
                         AtomPtr name, int af);
static int dnsReplyHandler(int abort, FdEventHandlerPtr event);
static int dnsReplyId(char *buf, int offset, int n, int *id_return);
static int dnsDecodeReply(char *buf, int offset, int n,
                          int *id_return,
                          AtomPtr *name_return, AtomPtr *value_return,
                          int *af_return, unsigned *ttl_return);
static int dnsHandler(int status, ConditionHandlerPtr chandler);
static int dnsGethostbynameFallback(int id, AtomPtr message);
static int sendQuery(DnsQueryPtr query);

static int idSeed;
#endif

#ifndef NO_FANCY_RESOLVER
static int
parseResolvConf(char *filename)
{
    FILE *f;
    char buf[512];
    char *p, *q;
    int n;
    AtomPtr nameserver = NULL;

    f = fopen(filename, "r");
    if(f == NULL) {
        do_log_error(L_ERROR, errno, "DNS: couldn't open %s", filename);
        return 0;
    }

    while(1) {
        p = fgets(buf, 512, f);
        if(p == NULL)
            break;

        n = strlen(buf);
        if(buf[n - 1] != '\n') {
            int c;
            do_log(L_WARN, "DNS: overly long line in %s -- skipping.\n",
                   filename);
            do {
                c = fgetc(f);
                if(c == EOF)
                    break;
            } while(c != '\n');
            if(c == EOF)
                break;
        }
        
        while(*p == ' ' || *p == '\t')
            p++;
        if(strcasecmp_n("nameserver", p, 10) != 0)
            continue;
        p += 10;
        while(*p == ' ' || *p == '\t')
            p++;
        q = p;
        while(*q == '.' || *q == ':' || digit(*q) || letter(*q))
            q++;
        if(*q != ' ' && *q != '\t' && *q != '\r' && *q != '\n') {
            do_log(L_WARN, "DNS: couldn't parse line in %s -- skipping.\n",
                   filename);
            continue;
        }
        nameserver = internAtomLowerN(p, q - p);
        break;
    }

    fclose(f);
    if(nameserver) {
        dnsNameServer = nameserver;
        return 1;
    } else {
        return 0;
    }
}
#endif

void
preinitDns()
{
#ifdef HAVE_IPv6
    int fd;
#endif

    assert(sizeof(struct in_addr) == 4);
#ifdef HAVE_IPv6
    assert(sizeof(struct in6_addr) == 16);
#endif

#ifndef NO_STANDARD_RESOLVER
    CONFIG_VARIABLE(dnsGethostbynameTtl, CONFIG_TIME,
                    "TTL for gethostbyname addresses.");
#endif

#ifdef HAVE_IPv6
    fd = socket(PF_INET6, SOCK_STREAM, 0);
    if(fd < 0) {
        if(errno == EPROTONOSUPPORT || errno == EAFNOSUPPORT) {
            dnsQueryIPv6 = 0;
        } else {
            do_log_error(L_WARN, errno, "DNS: couldn't create socket");
        }
    } else {
        close(fd);
    }
#endif

#ifndef NO_FANCY_RESOLVER
#ifndef WIN32
    parseResolvConf("/etc/resolv.conf");
#endif
    if(dnsNameServer == NULL || dnsNameServer->string[0] == '\0')
        dnsNameServer = internAtom("127.0.0.1");
    CONFIG_VARIABLE(dnsMaxTimeout, CONFIG_TIME,
                    "Max timeout for DNS queries.");
    CONFIG_VARIABLE(dnsNegativeTtl, CONFIG_TIME,
                    "TTL for negative DNS replies with no TTL.");
    CONFIG_VARIABLE(dnsNameServer, CONFIG_ATOM_LOWER,
                    "The name server to use.");
#ifndef NO_STANDARD_RESOLVER
    CONFIG_VARIABLE(dnsUseGethostbyname, CONFIG_TETRASTATE,
                    "Use the system resolver.");
#endif
#endif

#ifdef HAVE_IPv6
    CONFIG_VARIABLE(dnsQueryIPv6, CONFIG_TETRASTATE,
                    "Query for IPv6 addresses.");
#endif
}

void
initDns()
{
#ifndef NO_FANCY_RESOLVER
    int rc;
    struct timeval t;
    struct sockaddr_in *sin = (struct sockaddr_in*)&nameserverAddress;
#ifdef HAVE_IPv6
    struct sockaddr_in6 *sin6 = (struct sockaddr_in6*)&nameserverAddress;
#endif

    atomLocalhost = internAtom("localhost");
    atomLocalhostDot = internAtom("localhost.");
    inFlightDnsQueries = NULL;
    inFlightDnsQueriesLast = NULL;

    gettimeofday(&t, NULL);
    idSeed = t.tv_usec & 0xFFFF;
    sin->sin_family = AF_INET;
    sin->sin_port = htons(53);
    rc = inet_aton(dnsNameServer->string, &sin->sin_addr);
#ifdef HAVE_IPv6
    if(rc != 1) {
        sin6->sin6_family = AF_INET6;
        sin6->sin6_port = htons(53);
        rc = inet_pton(AF_INET6, dnsNameServer->string, &sin6->sin6_addr);
    }
#endif
    if(rc != 1) {
        do_log(L_ERROR, "DNS: couldn't parse name server %s.\n",
               dnsNameServer->string);
        exit(1);
    }
#endif
}

int
do_gethostbyname(char *origname,
                 int count,
                 int (*handler)(int, GethostbynameRequestPtr),
                 void *data)
{
    ObjectPtr object;
    int n = strlen(origname);
    AtomPtr name;
    GethostbynameRequestRec request;
    int done, rc;

    memset(&request, 0, sizeof(request));
    request.name = NULL;
    request.addr = NULL;
    request.error_message = NULL;
    request.count = count;
    request.handler = handler;
    request.data = data;

    if(n <= 0 || n > 131) {
        if(n <= 0) {
            request.error_message = internAtom("empty name");
            do_log(L_ERROR, "Empty DNS name.\n");
            done = handler(-EINVAL, &request);
        } else {
            request.error_message = internAtom("name too long");
            do_log(L_ERROR, "DNS name too long.\n");
            done = handler(-ENAMETOOLONG, &request);
        }
        assert(done);
        releaseAtom(request.error_message);
        return 1;
    }

    if(origname[n - 1] == '.')
        n--;

    name = internAtomLowerN(origname, n);

    if(name == NULL) {
        request.error_message = internAtom("couldn't allocate name");
        do_log(L_ERROR, "Couldn't allocate DNS name.\n");
        done = handler(-ENOMEM, &request);
        assert(done);
        releaseAtom(request.error_message);
        return 1;
    }

    request.name = name;
    request.addr = NULL;
    request.error_message = NULL;
    request.count = count;
    request.object = NULL;
    request.handler = handler;
    request.data = data;

    object = findObject(OBJECT_DNS, name->string, name->length);
    if(object == NULL || objectMustRevalidate(object, NULL)) {
        if(object) {
            privatiseObject(object, 0);
            releaseObject(object);
        }
        object = makeObject(OBJECT_DNS, name->string, name->length, 1, 0,
                            NULL, NULL);
        if(object == NULL) {
            request.error_message = internAtom("Couldn't allocate object");
            do_log(L_ERROR, "Couldn't allocate DNS object.\n");
            done = handler(-ENOMEM, &request);
            assert(done);
            releaseAtom(name);
            releaseAtom(request.error_message);
            return 1;
        }
    }

    if((object->flags & (OBJECT_INITIAL | OBJECT_INPROGRESS)) ==
       OBJECT_INITIAL) {
        if(dnsUseGethostbyname >= 3)
            rc = really_do_gethostbyname(name, object);
        else
            rc = really_do_dns(name, object);
        if(rc < 0) {
            assert(!(object->flags & (OBJECT_INITIAL | OBJECT_INPROGRESS)));
            goto fail;
        }
    }

    if(dnsUseGethostbyname >= 3)
        assert(!(object->flags & OBJECT_INITIAL));

#ifndef NO_FANCY_RESOLVER    
    if(object->flags & OBJECT_INITIAL) {
        ConditionHandlerPtr chandler;
        assert(object->flags & OBJECT_INPROGRESS);
        request.object = object;
        chandler = conditionWait(&object->condition, dnsHandler,
                                 sizeof(request), &request);
        if(chandler == NULL)
            goto fail;
        return 1;
    }
#endif

    if(object->headers && object->headers->length > 0) {
        if(object->headers->string[0] == DNS_A)
            assert(((object->headers->length - 1) % 
                    sizeof(HostAddressRec)) == 0);
        else
            assert(object->headers->string[0] == DNS_CNAME);
        request.addr = retainAtom(object->headers);
    } else if(object->message) {
        request.error_message = retainAtom(object->message);
    }

    releaseObject(object);

    if(request.addr && request.addr->length > 0)
        done = handler(1, &request);
    else
        done = handler(-EDNS_HOST_NOT_FOUND, &request);
    assert(done);

    releaseAtom(request.addr); request.addr = NULL;
    releaseAtom(request.name); request.name = NULL;
    releaseAtom(request.error_message); request.error_message = NULL;
    return 1;

 fail:
    releaseNotifyObject(object);
    done = handler(-errno, &request);
    assert(done);
    releaseAtom(name);
    return 1;
}

static int
dnsDelayedErrorNotifyHandler(TimeEventHandlerPtr event)
{
    int done;
    GethostbynameRequestRec request =
        *(GethostbynameRequestPtr)event->data;
    done = request.handler(-EDNS_HOST_NOT_FOUND, &request);
    assert(done);
    releaseAtom(request.name); request.name = NULL;
    releaseAtom(request.addr); request.addr = NULL;
    releaseAtom(request.error_message); request.error_message = NULL;
    return 1;
}
    
static int
dnsDelayedDoneNotifyHandler(TimeEventHandlerPtr event)
{
    int done;
    GethostbynameRequestRec request = *(GethostbynameRequestPtr)event->data;
    done = request.handler(1, &request);
    assert(done);
    releaseAtom(request.name); request.name = NULL;
    releaseAtom(request.addr); request.addr = NULL;
    releaseAtom(request.error_message); request.error_message = NULL;
    return 1;
}

static int
dnsDelayedNotify(int error, GethostbynameRequestPtr request)
{
    TimeEventHandlerPtr handler;

    if(error)
        handler = scheduleTimeEvent(0,
                                    dnsDelayedErrorNotifyHandler,
                                    sizeof(*request), request);
    else
        handler = scheduleTimeEvent(0,
                                    dnsDelayedDoneNotifyHandler,
                                    sizeof(*request), request);
    if(handler == NULL) {
        do_log(L_ERROR, "Couldn't schedule DNS notification.\n");
        return -1;
    }
    return 1;
}

#ifdef HAVE_IPv6
AtomPtr
rfc2732(AtomPtr name)
{
    char buf[40]; /* 8*4 (hexdigits) + 7 (colons) + 1 ('\0') */
    int rc;
    AtomPtr a = NULL;

    if(name->length < 40+2 && 
       name->string[0] == '[' && name->string[name->length - 1] == ']') {
        struct in6_addr in6a;
        memcpy(buf, name->string + 1, name->length - 2);
        buf[name->length - 2] = '\0';
        rc = inet_pton(AF_INET6, buf, &in6a);
        if(rc == 1) {
            char s[1 + sizeof(HostAddressRec)];
            memset(s, 0, sizeof(s));
            s[0] = DNS_A;
            s[1] = 6;
            memcpy(s + 2, &in6a, 16);
            a = internAtomN(s, 1 + sizeof(HostAddressRec));
            if(a == NULL)
                return NULL;
        }
    }
    return a;
}

/* Used for sorting host addresses depending on the value of dnsQueryIPv6 */
int
compare_hostaddr(const void *av, const void *bv)
{
    const HostAddressRec *a = av, *b = bv;
    int r;
    if(a->af == 4) {
        if(b->af == 4)
            r = 0;
        else
            r = -1;
    } else {
        if(b->af == 6)
            r = 0;
        else
            r = 1;
    }
    if(dnsQueryIPv6 >= 2)
        return -r;
    else
        return r;
}

#ifndef NO_STANDARD_RESOLVER
static int
really_do_gethostbyname(AtomPtr name, ObjectPtr object)
{
    struct addrinfo *ai, *entry, hints;
    int rc;
    int error, i;
    char buf[1024];
    AtomPtr a;

    a = rfc2732(name);
    if(a) {
        object->headers = a;
        object->age = current_time.tv_sec;
        object->expires = current_time.tv_sec + 240;
        object->flags &= ~(OBJECT_INITIAL | OBJECT_INPROGRESS);
        notifyObject(object);
        return 0;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_protocol = IPPROTO_TCP;
    if(dnsQueryIPv6 <= 0)
        hints.ai_family = AF_INET;
    else if(dnsQueryIPv6 >= 3)
        hints.ai_family = AF_INET6;

    rc = getaddrinfo(name->string, NULL, &hints, &ai);

    switch(rc) {
    case 0: error = 0; break;
    case EAI_FAMILY:
#ifdef EAI_ADDRFAMILY
    case EAI_ADDRFAMILY:
#endif
    case EAI_SOCKTYPE:
        error = EAFNOSUPPORT; break;
    case EAI_BADFLAGS: error = EINVAL; break;
    case EAI_SERVICE: error = EDNS_NO_RECOVERY; break;
#ifdef EAI_NONAME
    case EAI_NONAME:
#endif
#ifdef EAI_NODATA
    case EAI_NODATA:
#endif
        error = EDNS_NO_ADDRESS; break;
    case EAI_FAIL: error = EDNS_NO_RECOVERY; break;
    case EAI_AGAIN: error = EDNS_TRY_AGAIN; break;
#ifdef EAI_MEMORY
    case EAI_MEMORY: error = ENOMEM; break;
#endif
    case EAI_SYSTEM: error = errno; break;
    default: error = EUNKNOWN;
    }

    if(error == EDNS_NO_ADDRESS) {
        object->headers = NULL;
        object->age = current_time.tv_sec;
        object->expires = current_time.tv_sec + dnsNegativeTtl;
        object->flags &= ~(OBJECT_INITIAL | OBJECT_INPROGRESS);
        notifyObject(object);
        return 0;
    } else if(error) {
        do_log_error(L_ERROR, error, "Getaddrinfo failed");
        object->flags &= ~OBJECT_INPROGRESS;
        abortObject(object, 404,
                    internAtomError(error, "Getaddrinfo failed"));
        notifyObject(object);
        return 0;
    }

    entry = ai;
    buf[0] = DNS_A;
    i = 0;
    while(entry) {
        HostAddressRec host;
        int host_valid = 0;
        if(entry->ai_family == AF_INET && entry->ai_protocol == IPPROTO_TCP) {
            if(dnsQueryIPv6 < 3) {
                host.af = 4;
                memset(host.data, 0, sizeof(host.data));
                memcpy(&host.data,
                       &((struct sockaddr_in*)entry->ai_addr)->sin_addr, 
                       4);
                host_valid = 1;
            }
        } else if(entry->ai_family == AF_INET6 && 
                  entry->ai_protocol == IPPROTO_TCP) {
            if(dnsQueryIPv6 > 0) {
                host.af = 6;
                memset(&host.data, 0, sizeof(host.data));
                memcpy(&host.data,
                       &((struct sockaddr_in6*)entry->ai_addr)->sin6_addr, 
                       16);
                host_valid = 1;
            }
        }
        if(host_valid) {
            if(i >= 1024 / sizeof(HostAddressRec) - 2) {
                do_log(L_ERROR, "Too many addresses for host %s\n", 
                       name->string);
                break;
            }
            memcpy(buf + 1 + i * sizeof(HostAddressRec), 
                   &host, sizeof(HostAddressRec));
            i++;
        }
        entry = entry->ai_next;
    }
    freeaddrinfo(ai);
    if(i == 0) {
        do_log(L_ERROR, "Getaddrinfo returned no useful addresses\n");
        object->flags &= ~OBJECT_INPROGRESS;
        abortObject(object, 404,
                    internAtom("Getaddrinfo returned no useful addresses"));
        notifyObject(object);
        return 0;
    }

    if(1 <= dnsQueryIPv6 && dnsQueryIPv6 <= 2)
        qsort(buf + 1, i, sizeof(HostAddressRec), compare_hostaddr);

    a = internAtomN(buf, 1 + i * sizeof(HostAddressRec));
    if(a == NULL) {
        object->flags &= ~OBJECT_INPROGRESS;
        abortObject(object, 501, internAtom("Couldn't allocate address"));
        notifyObject(object);
        return 0;
    }
    object->headers = a;
    object->age = current_time.tv_sec;
    object->expires = current_time.tv_sec + dnsGethostbynameTtl;
    object->flags &= ~(OBJECT_INITIAL | OBJECT_INPROGRESS);
    notifyObject(object);
    return 0;
}
#endif
    
#else

#ifndef NO_STANDARD_RESOLVER
static int
really_do_gethostbyname(AtomPtr name, ObjectPtr object)
{
    struct hostent *host;
    char *s;
    AtomPtr a;
    int i, j;
    int error;

    host = gethostbyname(name->string);
    if(host == NULL) {
        switch(h_errno) {
        case HOST_NOT_FOUND: error = EDNS_HOST_NOT_FOUND; break;
#ifdef NO_ADDRESS
        case NO_ADDRESS: error = EDNS_NO_ADDRESS; break;
#endif
        case NO_RECOVERY: error = EDNS_NO_RECOVERY; break;
        case TRY_AGAIN: error = EDNS_TRY_AGAIN; break;
        default: error = EUNKNOWN; break;
        }
        if(error == EDNS_HOST_NOT_FOUND) {
            object->headers = NULL;
            object->age = current_time.tv_sec;
            object->expires = current_time.tv_sec + dnsNegativeTtl;
            object->flags &= ~(OBJECT_INITIAL | OBJECT_INPROGRESS);
            object->flags &= ~OBJECT_INPROGRESS;
            notifyObject(object);
            return 0;
        } else {
            do_log_error(L_ERROR, error, "Gethostbyname failed");
            abortObject(object, 404, 
                        internAtomError(error, "Gethostbyname failed"));
            object->flags &= ~OBJECT_INPROGRESS;
            notifyObject(object);
            return 0;
        }
    }
    if(host->h_addrtype != AF_INET) {
        do_log(L_ERROR, "Address is not AF_INET.\n");
        object->flags &= ~OBJECT_INPROGRESS;
        abortObject(object, 404, internAtom("Address is not AF_INET"));
        notifyObject(object);
        return -1;
    }
    if(host->h_length != sizeof(struct in_addr)) {
        do_log(L_ERROR, "Address size inconsistent.\n");
        object->flags &= ~OBJECT_INPROGRESS;
        abortObject(object, 404, internAtom("Address size inconsistent"));
        notifyObject(object);
        return 0;
    }
    i = 0;
    while(host->h_addr_list[i] != NULL) i++;
    s = malloc(1 + i * sizeof(HostAddressRec));
    if(s == NULL) {
        a = NULL;
    } else {
        memset(s, 0, 1 + i * sizeof(HostAddressRec));
        s[0] = DNS_A;
        for(j = 0; j < i; j++) {
            s[j * sizeof(HostAddressRec) + 1] = 4;
            memcpy(&s[j * sizeof(HostAddressRec) + 2], host->h_addr_list[j],
                   sizeof(struct in_addr));
        }
        a = internAtomN(s, i * sizeof(HostAddressRec) + 1);
        free(s);
    }
    if(!a) {
        object->flags &= ~OBJECT_INPROGRESS;
        abortObject(object, 501, internAtom("Couldn't allocate address"));
        notifyObject(object);
        return 0;
    }
    object->headers = a;
    object->age = current_time.tv_sec;
    object->expires = current_time.tv_sec + dnsGethostbynameTtl;
    object->flags &= ~(OBJECT_INITIAL | OBJECT_INPROGRESS);
    notifyObject(object);
    return 0;

}
#endif

#endif

#ifdef NO_STANDARD_RESOLVER
static int
really_do_gethostbyname(AtomPtr name, ObjectPtr object)
{
    abort();
}
#endif

#ifndef NO_FANCY_RESOLVER

static int dnsSocket = -1;
static FdEventHandlerPtr dnsSocketHandler = NULL;

static int
dnsHandler(int status, ConditionHandlerPtr chandler)
{
    GethostbynameRequestRec request = *(GethostbynameRequestPtr)chandler->data;
    ObjectPtr object = request.object;

    assert(!(object->flags & OBJECT_INPROGRESS));

    if(object->headers) {
        request.addr = retainAtom(object->headers);
        dnsDelayedNotify(0, &request);
    } else {
        if(object->message)
            request.error_message = retainAtom(object->message);
        dnsDelayedNotify(1, &request);
    }
    releaseObject(object);
    return 1;
}

static int
queryInFlight(DnsQueryPtr query)
{
    DnsQueryPtr other;
    other = inFlightDnsQueries;
    while(other) {
        if(other == query)
            return 1;
        other = other->next;
    }
    return 0;
}

static void
removeQuery(DnsQueryPtr query)
{
    DnsQueryPtr previous;
    if(query == inFlightDnsQueries) {
        inFlightDnsQueries = query->next;
        if(inFlightDnsQueries == NULL)
            inFlightDnsQueriesLast = NULL;
    } else {
        previous = inFlightDnsQueries;
        while(previous->next) {
            if(previous->next == query)
                break;
            previous = previous->next;
        }
        assert(previous->next != NULL);
        previous->next = query->next;
        if(previous->next == NULL)
            inFlightDnsQueriesLast = previous;
    }
}

static void
insertQuery(DnsQueryPtr query) 
{
    if(inFlightDnsQueriesLast)
        inFlightDnsQueriesLast->next = query;
    else
        inFlightDnsQueries = query;
    inFlightDnsQueriesLast = query;
}

static DnsQueryPtr
findQuery(int id, AtomPtr name)
{
    DnsQueryPtr query;
    query = inFlightDnsQueries;
    while(query) {
        if(query->id == id && (name == NULL || query->name == name))
            return query;
        query = query->next;
    }
    return NULL;
}

static int
dnsTimeoutHandler(TimeEventHandlerPtr event)
{
    DnsQueryPtr query = *(DnsQueryPtr*)event->data;
    ObjectPtr object = query->object;
    int rc;

    /* People are reporting that this does happen.  And I have no idea why. */
    if(!queryInFlight(query)) {
        do_log(L_ERROR, "BUG: timing out martian query (%s, flags: 0x%x).\n",
               scrub(query->name->string), (unsigned)object->flags);
        return 1;
    }

    query->timeout = MAX(10, query->timeout * 2);

    if(query->timeout > dnsMaxTimeout) {
        abortObject(object, 501, internAtom("Timeout"));
        goto fail;
    } else {
        rc = sendQuery(query);
        if(rc < 0) {
            if(rc != -EWOULDBLOCK && rc != -EAGAIN && rc != -ENOBUFS) {
                abortObject(object, 501,
                            internAtomError(-rc,
                                            "Couldn't send DNS query"));
                goto fail;
            }
            /* else let it timeout */
        }
        query->timeout_handler =
            scheduleTimeEvent(query->timeout, dnsTimeoutHandler,
                              sizeof(query), &query);
        if(query->timeout_handler == NULL) {
            do_log(L_ERROR, "Couldn't schedule DNS timeout handler.\n");
            abortObject(object, 501,
                        internAtom("Couldn't schedule DNS timeout handler"));
            goto fail;
        }
        return 1;
    }

 fail:
    removeQuery(query);
    object->flags &= ~OBJECT_INPROGRESS;
    if(query->inet4) releaseAtom(query->inet4);
    if(query->inet6) releaseAtom(query->inet6);
    free(query);
    releaseNotifyObject(object);
    return 1;
}

static int
establishDnsSocket()
{
    int rc;
#ifdef HAVE_IPv6
    int inet6 = (nameserverAddress.sa_family == AF_INET6);
    int pf = inet6 ? PF_INET6 : PF_INET;
    int sa_size = 
        inet6 ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in);
#else
    int pf = PF_INET;
    int sa_size = sizeof(struct sockaddr_in);
#endif

    if(dnsSocket < 0) {
        assert(!dnsSocketHandler);
        dnsSocket = socket(pf, SOCK_DGRAM, 0);
        if(dnsSocket < 0) {
            do_log_error(L_ERROR, errno, "Couldn't create DNS socket");
            return -errno;
        }

        rc = connect(dnsSocket, &nameserverAddress, sa_size);
        if(rc < 0) {
            CLOSE(dnsSocket);
            dnsSocket = -1;
            do_log_error(L_ERROR, errno, "Couldn't create DNS \"connection\"");
            return -errno;
        }
    }

    if(!dnsSocketHandler) {
        dnsSocketHandler = 
            registerFdEvent(dnsSocket, POLLIN, dnsReplyHandler, 0, NULL);
        if(dnsSocketHandler == NULL) {
            do_log(L_ERROR, "Couldn't register DNS socket handler.\n");
            CLOSE(dnsSocket);
            dnsSocket = -1;
            return -ENOMEM;
        }
    }

    return 1;
}

static int
sendQuery(DnsQueryPtr query)
{
    char buf[512];
    int buflen;
    int rc;
    int af[2];
    int i;

    if(dnsSocket < 0)
        return -1;

    if(dnsQueryIPv6 <= 0) {
        af[0] = 4; af[1] = 0;
    } else if(dnsQueryIPv6 <= 2) {
        af[0] = 4; af[1] = 6;
    } else {
        af[0] = 6; af[1] = 0;
    }

    for(i = 0; i < 2; i++) {
        if(af[i] == 0)
            continue;
        if(af[i] == 4 && query->inet4)
            continue;
        else if(af[i] == 6 && query->inet6)
            continue;

        buflen = dnsBuildQuery(query->id, buf, 0, 512, query->name, af[i]);
        if(buflen <= 0) {
            do_log(L_ERROR, "Couldn't build DNS query.\n");
            return buflen;
        }

        rc = send(dnsSocket, buf, buflen, 0);
        if(rc < buflen) {
            if(rc >= 0) {
                do_log(L_ERROR, "Couldn't send DNS query: partial send.\n");
                return -EAGAIN;
            } else {
                do_log_error(L_ERROR, errno, "Couldn't send DNS query");
                return -errno;
            }
        }
    }
    return 1;
}

static int
really_do_dns(AtomPtr name, ObjectPtr object)
{
    int rc;
    DnsQueryPtr query;
    AtomPtr message = NULL;
    int id;
    AtomPtr a = NULL;

    if(a == NULL) {
        if(name == atomLocalhost || name == atomLocalhostDot) {
            char s[1 + sizeof(HostAddressRec)];
            memset(s, 0, sizeof(s));
            s[0] = DNS_A;
            s[1] = 4;
            s[2] = 127;
            s[3] = 0;
            s[4] = 0;
            s[5] = 1;
            a = internAtomN(s, 1 + sizeof(HostAddressRec));
            if(a == NULL) {
                abortObject(object, 501,
                            internAtom("Couldn't allocate address"));
                notifyObject(object);
                errno = ENOMEM;
                return -1;
            }
        }
    }

    if(a == NULL) {
        struct in_addr ina;
        rc = inet_aton(name->string, &ina);
        if(rc == 1) {
            char s[1 + sizeof(HostAddressRec)];
            memset(s, 0, sizeof(s));
            s[0] = DNS_A;
            s[1] = 4;
            memcpy(s + 2, &ina, 4);
            a = internAtomN(s, 1 + sizeof(HostAddressRec));
            if(a == NULL) {
                abortObject(object, 501,
                            internAtom("Couldn't allocate address"));
                notifyObject(object);
                errno = ENOMEM;
                return -1;
            }
        }
    }
#ifdef HAVE_IPv6
    if(a == NULL)
        a = rfc2732(name);
#endif

    if(a) {
        object->headers = a;
        object->age = current_time.tv_sec;
        object->expires = current_time.tv_sec + 240;
        object->flags &= ~(OBJECT_INITIAL | OBJECT_INPROGRESS);
        notifyObject(object);
        return 0;
    }

    rc = establishDnsSocket();
    if(rc < 0) {
        do_log_error(L_ERROR, -rc, "Couldn't establish DNS socket.\n");
        message = internAtomError(-rc, "Couldn't establish DNS socket");
        goto fallback;
    }

    /* The id is used to speed up detecting replies to queries that
       are no longer current -- see dnsReplyHandler. */
    id = (idSeed++) & 0xFFFF;

    query = malloc(sizeof(DnsQueryRec));
    if(query == NULL) {
        do_log(L_ERROR, "Couldn't allocate DNS query.\n");
        message = internAtom("Couldn't allocate DNS query");
        goto fallback;
    }
    query->id = id;
    query->inet4 = NULL;
    query->inet6 = NULL;
    query->name = name;
    query->time = current_time.tv_sec;
    query->object = retainObject(object);
    query->timeout = 4;
    query->timeout_handler = NULL;
    query->next = NULL;

    query->timeout_handler = 
        scheduleTimeEvent(query->timeout, dnsTimeoutHandler,
                          sizeof(query), &query);
    if(query->timeout_handler == NULL) {
        do_log(L_ERROR, "Couldn't schedule DNS timeout handler.\n");
        message = internAtom("Couldn't schedule DNS timeout handler");
        goto free_fallback;
    }
    insertQuery(query);

    object->flags |= OBJECT_INPROGRESS;
    rc = sendQuery(query);
    if(rc < 0) {
        if(rc != -EWOULDBLOCK && rc != -EAGAIN && rc != -ENOBUFS) {
            object->flags &= ~OBJECT_INPROGRESS;
            message = internAtomError(-rc, "Couldn't send DNS query");
            goto remove_fallback;
        }
        /* else let it timeout */
    }
    releaseAtom(message);
    return 1;

 remove_fallback:
    removeQuery(query);
 free_fallback:
    releaseObject(query->object);
    cancelTimeEvent(query->timeout_handler);
    free(query);
 fallback:
    if(dnsUseGethostbyname >= 1) {
        releaseAtom(message);
        do_log(L_WARN, "Falling back on gethostbyname.\n");
        return really_do_gethostbyname(name, object);
    } else {
        abortObject(object, 501, message);
        notifyObject(object);
        return 1;
    }
}

static int
dnsReplyHandler(int abort, FdEventHandlerPtr event)
{
    int fd = event->fd;
    char buf[2048];
    int len, rc;
    ObjectPtr object;
    unsigned ttl = 0;
    AtomPtr name, value, message = NULL;
    int id;
    int af;
    DnsQueryPtr query;
    AtomPtr cname = NULL;

    if(abort) {
        dnsSocketHandler = NULL;
        rc = establishDnsSocket();
        if(rc < 0) {
            do_log(L_ERROR, "Couldn't reestablish DNS socket.\n");
            /* At this point, we should abort all in-flight
               DNS requests.  Oh, well, they'll timeout anyway. */
        }
        return 1;
    }

    len = recv(fd, buf, 2048, 0);
    if(len <= 0) {
        if(errno == EINTR || errno == EAGAIN) return 0;
        /* This is where we get ECONNREFUSED for an ICMP port unreachable */
        do_log_error(L_ERROR, errno, "DNS: recv failed");
        dnsGethostbynameFallback(-1, message);
        return 0;
    }

    /* This could be a late reply to a query that timed out and was
       resent, a reply to a query that timed out, or a reply to an
       AAAA query when we already got a CNAME reply to the associated
       A.  We filter such replies straight away, without trying to
       parse them. */
    rc = dnsReplyId(buf, 0, len, &id);
    if(rc < 0) {
        do_log(L_WARN, "Short DNS reply.\n");
        return 0;
    }
    if(!findQuery(id, NULL)) {
        return 0;
    }

    rc = dnsDecodeReply(buf, 0, len, &id, &name, &value, &af, &ttl);
    if(rc < 0) {
        assert(value == NULL);
        /* We only want to fallback on gethostbyname if we received a
           reply that we could not understand.  What about truncated
           replies? */
        if(rc < 0) {
            do_log_error(L_WARN, -rc, "DNS");
            if(dnsUseGethostbyname >= 2 ||
               (dnsUseGethostbyname && 
                (rc != -EDNS_HOST_NOT_FOUND && rc != -EDNS_NO_RECOVERY &&
                 rc != -EDNS_FORMAT))) {
                dnsGethostbynameFallback(id, message);
                return 0;
            } else {
                message = internAtom(pstrerror(-rc));
            }
        } else {
            assert(name != NULL && id >= 0 && af >= 0);
        }
    }

    query = findQuery(id, name);
    if(query == NULL) {
        /* Duplicate id ? */
        releaseAtom(value);
        releaseAtom(name);
        return 0;
    }

    /* We're going to use the information in this reply.  If it was an
       error, construct an empty atom to distinguish it from information
       we're still waiting for. */
    if(value == NULL)
        value = internAtom("");

 again:
    if(af == 4) {
        if(query->inet4 == NULL) {
            query->inet4 = value;
            query->ttl4 = current_time.tv_sec + ttl;
        } else
            releaseAtom(value);
    } else if(af == 6) {
        if(query->inet6 == NULL) {
            query->inet6 = value;
            query->ttl6 = current_time.tv_sec + ttl;
        } else
            releaseAtom(value);
    } else if(af == 0) {
        /* Ignore errors in this case. */
        if(query->inet4 && query->inet4->length == 0) {
            releaseAtom(query->inet4);
            query->inet4 = NULL;
        }
        if(query->inet6 && query->inet6->length == 0) {
            releaseAtom(query->inet6);
            query->inet6 = NULL;
        }
        if(query->inet4 || query->inet6) {
            do_log(L_WARN, "Host %s has both %s and CNAME -- "
                   "ignoring CNAME.\n", scrub(query->name->string),
                   query->inet4 ? "A" : "AAAA");
            releaseAtom(value);
            value = internAtom("");
            af = query->inet4 ? 4 : 6;
            goto again;
        } else {
            cname = value;
        }
    }

    if(rc >= 0 && !cname &&
       ((dnsQueryIPv6 < 3 && query->inet4 == NULL) ||
        (dnsQueryIPv6 > 0 && query->inet6 == NULL)))
        return 0;

    /* This query is complete */

    cancelTimeEvent(query->timeout_handler);
    object = query->object;

    if(object->flags & OBJECT_INITIAL) {
        assert(!object->headers);
        if(cname) {
            assert(query->inet4 == NULL && query->inet6 == NULL);
            object->headers = cname;
            object->expires = current_time.tv_sec + ttl;
        } else if((!query->inet4 || query->inet4->length == 0) &&
                  (!query->inet6 || query->inet6->length == 0)) {
            releaseAtom(query->inet4);
            releaseAtom(query->inet6);
            object->expires = current_time.tv_sec + dnsNegativeTtl;
            abortObject(object, 500, retainAtom(message));
        } else if(!query->inet4 || query->inet4->length == 0) {
            object->headers = query->inet6;
            object->expires = query->ttl6;
            releaseAtom(query->inet4);
        } else if(!query->inet6 || query->inet6->length == 0) {
            object->headers = query->inet4;
            object->expires = query->ttl4;
            releaseAtom(query->inet6);
        } else {
            /* need to merge results */
            char buf[1024];
            if(query->inet4->length + query->inet6->length > 1024) {
                releaseAtom(query->inet4);
                releaseAtom(query->inet6);
                abortObject(object, 500, internAtom("DNS reply too long"));
            } else {
                if(dnsQueryIPv6 <= 1) {
                    memcpy(buf, query->inet4->string, query->inet4->length);
                    memcpy(buf + query->inet4->length,
                           query->inet6->string + 1, query->inet6->length - 1);
                } else {
                    memcpy(buf, query->inet6->string, query->inet6->length);
                    memcpy(buf + query->inet6->length,
                           query->inet4->string + 1, query->inet4->length - 1);
                }
                object->headers =
                    internAtomN(buf, 
                                query->inet4->length + 
                                query->inet6->length - 1);
                if(object->headers == NULL)
                    abortObject(object, 500, 
                                internAtom("Couldn't allocate DNS atom"));
            }
            object->expires = MIN(query->ttl4, query->ttl6);
        }
        object->age = current_time.tv_sec;
        object->flags &= ~(OBJECT_INITIAL | OBJECT_INPROGRESS);
    } else {
        do_log(L_WARN, "DNS object ex nihilo for %s.\n",
               scrub(query->name->string));
    }
    
    removeQuery(query);
    free(query);

    releaseAtom(name);
    releaseAtom(message);
    releaseNotifyObject(object);
    return 0;
}

static int
dnsGethostbynameFallback(int id, AtomPtr message)
{
    DnsQueryPtr query, previous;
    ObjectPtr object;

    if(inFlightDnsQueries == NULL) {
        releaseAtom(message);
        return 1;
    }

    query = NULL;
    if(id < 0 || inFlightDnsQueries->id == id) {
        previous = NULL;
        query = inFlightDnsQueries;
    } else {
        previous = inFlightDnsQueries;
        while(previous->next) {
            if(previous->next->id == id) {
                query = previous->next;
                break;
            }
            previous = previous->next;
        }
        if(!query) {
            previous = NULL;
            query = inFlightDnsQueries;
        }
    }

    if(previous == NULL) {
        inFlightDnsQueries = query->next;
        if(inFlightDnsQueries == NULL)
            inFlightDnsQueriesLast = NULL;
    } else {
        previous->next = query->next;
        if(query->next == NULL)
            inFlightDnsQueriesLast = NULL;
    }

    object = makeObject(OBJECT_DNS, query->name->string, query->name->length,
                        1, 0, NULL, NULL);
    if(!object) {
        do_log(L_ERROR, "Couldn't make DNS object.\n");
        releaseAtom(query->name);
        releaseAtom(message);
        releaseObject(query->object);
        cancelTimeEvent(query->timeout_handler);
        free(query);
        return -1;
    }
    if(dnsUseGethostbyname >= 1) {
        releaseAtom(message);
        do_log(L_WARN, "Falling back to using system resolver.\n");
        really_do_gethostbyname(retainAtom(query->name), object);
    } else {
        releaseAtom(object->message);
        object->message = message;
        object->flags &= ~OBJECT_INPROGRESS;
        releaseNotifyObject(object);
    }
    cancelTimeEvent(query->timeout_handler);
    releaseAtom(query->name);
    if(query->inet4) releaseAtom(query->inet4);
    if(query->inet6) releaseAtom(query->inet6);
    releaseObject(query->object);
    free(query);
    return 1;
}

static int
stringToLabels(char *buf, int offset, int n, char *string)
{
    int i = offset;
    int j = 0, k = 0;
    while(1) {
        while(string[k] != '.' && string[k] != '\0')
            k++;
        if(k >= j + 256) return -1;
        buf[i] = (unsigned char)(k - j); i++; if(i >= n) return -1;
        while(j < k) {
            buf[i] = string[j]; i++; j++; if(i >= n) return -1;
        }
        if(string[j] == '\0') {
            buf[i] = '\0';
            i++; if(i >= n) return -1;
            break;
        }
        j++; k++;
    }

    return i;
}

#ifdef UNALIGNED_ACCESS
#define DO_NTOHS(_d, _s) _d = ntohs(*(short*)(_s));
#define DO_NTOHL(_d, _s) _d = ntohl(*(unsigned*)(_s))
#define DO_HTONS(_d, _s) *(short*)(_d) = htons(_s);
#define DO_HTONL(_d, _s) *(unsigned*)(_d) = htonl(_s)
#else
#define DO_NTOHS(_d, _s) \
    do { short _dd; \
         memcpy(&(_dd), (_s), sizeof(short)); \
         _d = ntohs(_dd); } while(0)
#define DO_NTOHL(_d, _s) \
    do { unsigned _dd; \
         memcpy(&(_dd), (_s), sizeof(unsigned)); \
         _d = ntohl(_dd); } while(0)
#define DO_HTONS(_d, _s) \
    do { unsigned short _dd; \
         _dd = htons(_s); \
         memcpy((_d), &(_dd), sizeof(unsigned short)); } while(0);
#define DO_HTONL(_d, _s) \
    do { unsigned _dd; \
         _dd = htonl(_s); \
         memcpy((_d), &(_dd), sizeof(unsigned)); } while(0);
#endif

static int
labelsToString(char *buf, int offset, int n, char *d, int m, int *j_return)
{
    int i = offset, j, k;
    int ll, rc;

    j = 0;
    while(1) {
        if(i >= n) return -1;
        ll = *(unsigned char*)&buf[i]; i++;
        if(ll == 0) {
            break;
        }
        if((ll & (3 << 6)) == (3 << 6)) {
            /* RFC 1035, 4.1.4 */
            int o;
            if(i >= n) return -1;
            o = (ll & ~(3 << 6)) << 8 | *(unsigned char*)&buf[i];
            i++;
            rc = labelsToString(buf, o, n, &d[j], m - j, &k);
            if(rc < 0)
                return -1;
            j += k;
            break;
        } else if((ll & (3 << 6)) == 0) {
            for(k = 0; k < ll; k++) {
                if(i >= n || j >= m) return -1;
                d[j++] = buf[i++];
            }
            if(i >= n) return -1;
            if(buf[i] != '\0') {
                if(j >= m) return -1;
                d[j++] = '.';
            }
        } else {
            return -1;
        }
    }
    *j_return = j;
    return i;
}

static int
dnsBuildQuery(int id, char *buf, int offset, int n, AtomPtr name, int af)
{
    int i = offset;
    int type;
    switch(af) {
    case 4: type = 1; break;
    case 6: type = 28; break;
    default: return -EINVAL;
    }

    if(i + 12 >= n) return -1;
    DO_HTONS(&buf[i], id); i += 2;
    DO_HTONS(&buf[i], 1<<8); i += 2;
    DO_HTONS(&buf[i], 1); i += 2;
    DO_HTONS(&buf[i], 0); i += 2;
    DO_HTONS(&buf[i], 0); i += 2;
    DO_HTONS(&buf[i], 0); i += 2;

    i = stringToLabels(buf, i, n, name->string);
    if(i < 0) return -ENAMETOOLONG;
    
    if(i + 4 >= n) return -ENAMETOOLONG;
    DO_HTONS(&buf[i], type); i += 2;
    DO_HTONS(&buf[i], 1); i += 2;
    return i;
}

static int
dnsReplyId(char *buf, int offset, int n, int *id_return)
{
    if(n - offset < 12)
        return -1;
    DO_NTOHS(*id_return, &buf[offset]);
    return 1;
}

static int
dnsDecodeReply(char *buf, int offset, int n, int *id_return,
               AtomPtr *name_return, AtomPtr *value_return,
               int *af_return, unsigned *ttl_return)
{
    int i = offset, j, m;
    int id = -1, b23, qdcount, ancount, nscount, arcount, rdlength;
    int class, type;
    unsigned int ttl;
    char b[2048];
    int af = -1;
    AtomPtr name = NULL, value;
    char addresses[1024];
    int addr_index = 0;
    int error = EDNS_NO_ADDRESS;
    unsigned final_ttl = 7 * 24 * 3600;
    int dnserror;

    if(n - i < 12) {
        error = EDNS_INVALID;
        goto fail;
    }

    DO_NTOHS(id, &buf[i]); i += 2;
    DO_NTOHS(b23, &buf[i]); i += 2;
    DO_NTOHS(qdcount, &buf[i]); i += 2;
    DO_NTOHS(ancount, &buf[i]); i += 2;
    DO_NTOHS(nscount, &buf[i]); i += 2;
    DO_NTOHS(arcount, &buf[i]); i += 2;

    do_log(D_DNS, 
           "DNS id %d, b23 0x%x, qdcount %d, ancount %d, "
           "nscount %d, arcount %d\n",
           id, b23, qdcount, ancount, nscount, arcount);

    if((b23 & (0xF870)) != 0x8000) {
        do_log(L_ERROR, "Incorrect DNS reply (b23 = 0x%x).\n", b23);
        error = EDNS_INVALID;
        goto fail;
    }

    dnserror = b23 & 0xF;

    if(b23 & 0x200) {
        do_log(L_WARN, "Truncated DNS reply (b23 = 0x%x).\n", b23);
    }

    if(dnserror || qdcount != 1) {
        if(!dnserror)
            do_log(L_ERROR, 
                   "Unexpected number %d of DNS questions.\n", qdcount);
        if(dnserror == 1)
            error = EDNS_FORMAT;
        else if(dnserror == 2)
            error = EDNS_NO_RECOVERY;
        else if(dnserror == 3)
            error = EDNS_HOST_NOT_FOUND;
        else if(dnserror == 4 || dnserror == 5)
            error = EDNS_REFUSED;
        else if(dnserror == 0)
            error = EDNS_INVALID;
        else
            error = EUNKNOWN;
        goto fail;
    }

    /* We do this early, so that we can return the address family to
       the caller in case of error. */
    i = labelsToString(buf, i, n, b, 2048, &m);
    if(i < 0) {
        error = EDNS_FORMAT;
        goto fail;
    }
    DO_NTOHS(type, &buf[i]); i += 2;
    DO_NTOHS(class, &buf[i]); i += 2;

    if(type == 1)
        af = 4;
    else if(type == 28)
        af = 6;
    else {
        error = EDNS_FORMAT;
        goto fail;
    }

    do_log(D_DNS, "DNS q: ");
    do_log_n(D_DNS, b, m);
    do_log(D_DNS, " (%d, %d)\n", type, class);
    name = internAtomLowerN(b, m);
    if(name == NULL) {
        error = ENOMEM;
        goto fail;
    }

    if(class != 1) {
        error = EDNS_FORMAT;
        goto fail;
    }

#define PARSE_ANSWER(kind, label) \
do { \
    i = labelsToString(buf, i, 1024, b, 2048, &m); \
    if(i < 0) goto label; \
    DO_NTOHS(type, &buf[i]); i += 2; if(i > 1024) goto label; \
    DO_NTOHS(class, &buf[i]); i += 2; if(i > 1024) goto label; \
    DO_NTOHL(ttl, &buf[i]); i += 4; if(i > 1024) goto label; \
    DO_NTOHS(rdlength, &buf[i]); i += 2; if(i > 1024) goto label; \
    do_log(D_DNS, "DNS " kind ": "); \
    do_log_n(D_DNS, b, m); \
    do_log(D_DNS, " (%d, %d): %d bytes, ttl %u\n", \
           type, class, rdlength, ttl); \
   } while(0)


    for(j = 0; j < ancount; j++) {
        PARSE_ANSWER("an", fail);
        if(strcasecmp_n(name->string, b, m) == 0) {
            if(class != 1) {
                do_log(D_DNS, "DNS: %s: unknown class %d.\n", 
                       name->string, class);
                error = EDNS_UNSUPPORTED;
                goto cont;
            }
            if(type == 1 || type == 28) {
                if((type == 1 && rdlength != 4) ||
                   (type == 28 && rdlength != 16)) {
                    do_log(L_ERROR, 
                           "DNS: %s: unexpected length %d of %s record.\n",
                           scrub(name->string),
                           rdlength, type == 1 ? "A" : "AAAA");
                    error = EDNS_INVALID;
                    if(rdlength <= 0 || rdlength >= 32)
                        goto fail;
                    goto cont;
                }
                if(af == 0) {
                    do_log(L_WARN, "DNS: %s: host has both A and CNAME -- "
                           "ignoring CNAME.\n", scrub(name->string));
                    addr_index = 0;
                    af = -1;
                }
                if(type == 1) {
                    if(af < 0)
                        af = 4;
                    else if(af == 6) {
                        do_log(L_WARN, "Unexpected AAAA reply.\n");
                        goto cont;
                    }
                } else {
                    if(af < 0)
                        af = 6;
                    else if(af == 4) {
                        do_log(L_WARN, "Unexpected A reply.\n");
                        goto cont;
                    }
                }

                if(addr_index == 0) {
                    addresses[0] = DNS_A;
                    addr_index++;
                } else {
                    if(addr_index > 1000) {
                        error = EDNS_INVALID;
                        goto fail;
                    }
                }
                assert(addresses[0] == DNS_A);
                if(final_ttl > ttl)
                    final_ttl = ttl;
                memset(&addresses[addr_index], 0, sizeof(HostAddressRec));
                if(type == 1) {
                    addresses[addr_index] = 4;
                    memcpy(addresses + addr_index + 1, buf + i, 4);
                } else {
                    addresses[addr_index] = 6;
                    memcpy(addresses + addr_index + 1, buf + i, 16);
                }
                addr_index += sizeof(HostAddressRec);
            } else if(type == 5) {
                int j, k;
                if(af != 0 && addr_index > 0) {
                    do_log(L_WARN, "DNS: host has both CNAME and A -- "
                           "ignoring CNAME.\n");
                    goto cont;
                }
                af = 0;

                if(addr_index != 0) {
                    /* Only warn if the CNAMEs are not identical */
                    char tmp[512]; int jj, kk;
                    assert(addresses[0] == DNS_CNAME);
                    jj = labelsToString(buf, i, n,
                                        tmp, 512, &kk);
                    if(jj < 0 ||
                       kk != strlen(addresses + 1) ||
                       memcmp(addresses + 1, tmp, kk) != 0) {
                        do_log(L_WARN, "DNS: "
                               "%s: host has multiple CNAMEs -- "
                               "ignoring subsequent.\n",
                               scrub(name->string));

                    }
                    goto cont;
                }
                addresses[0] = DNS_CNAME;
                addr_index++;
                j = labelsToString(buf, i, n,
                                   addresses + 1, 1020, &k);
                if(j < 0) {
                    addr_index = 0;
                    error = ENAMETOOLONG;
                    continue;
                }
                addr_index = k + 1;
            } else {
                error = EDNS_NO_ADDRESS;
                i += rdlength;
                continue;
            }

        }
    cont:
        i += rdlength;
    }

#if (LOGGING_MAX & D_DNS)
    for(j = 0; j < nscount; j++) {
        PARSE_ANSWER("ns", nofail);
        i += rdlength;
    }

    for(j = 0; j < arcount; j++) {
        PARSE_ANSWER("ar", nofail);
        i += rdlength;
    }

 nofail:
#endif

#undef PARSE_ANSWER

    do_log(D_DNS, "DNS: %d bytes\n", addr_index);
    if(af < 0)
        goto fail;

    value = internAtomN(addresses, addr_index);
    if(value == NULL) {
        error = ENOMEM;
        goto fail;
    }

    assert(af >= 0);
    *id_return = id;
    *name_return = name;
    *value_return = value;
    *af_return = af;
    *ttl_return = final_ttl;
    return 1;

 fail:
    *id_return = id;
    *name_return = name;
    *value_return = NULL;
    *af_return = af;
    return -error;
}

#else

static int
really_do_dns(AtomPtr name, ObjectPtr object)
{
    abort();
}

#endif
