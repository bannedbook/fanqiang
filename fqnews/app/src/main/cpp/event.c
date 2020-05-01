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

#ifdef HAVE_FORK
static volatile sig_atomic_t exitFlag = 0;
#else
static int exitFlag = 0;
#endif
static int in_signalCondition = 0;

static TimeEventHandlerPtr timeEventQueue;
static TimeEventHandlerPtr timeEventQueueLast;

struct timeval current_time;
struct timeval null_time = {0,0};

static int fdEventSize = 0;
static int fdEventNum = 0;
static struct pollfd *poll_fds = NULL;
static FdEventHandlerPtr *fdEvents = NULL, *fdEventsLast = NULL;
int diskIsClean = 1;

static int fds_invalid = 0;

static inline int
timeval_cmp(struct timeval *t1, struct timeval *t2)
{
    if(t1->tv_sec < t2->tv_sec)
        return -1;
    else if(t1->tv_sec > t2->tv_sec)
        return +1;
    else if(t1->tv_usec < t2->tv_usec)
        return -1;
    else if(t1->tv_usec > t2->tv_usec)
        return +1;
    else
        return 0;
}

static inline void
timeval_minus(struct timeval *d,
              const struct timeval *s1, const struct timeval *s2)
{
    if(s1->tv_usec >= s2->tv_usec) {
        d->tv_usec = s1->tv_usec - s2->tv_usec;
        d->tv_sec = s1->tv_sec - s2->tv_sec;
    } else {
        d->tv_usec = s1->tv_usec + 1000000 - s2->tv_usec;
        d->tv_sec = s1->tv_sec - s2->tv_sec - 1;
    }
}

int
timeval_minus_usec(const struct timeval *s1, const struct timeval *s2)
{
    return (s1->tv_sec - s2->tv_sec) * 1000000 + s1->tv_usec - s2->tv_usec;
}

#ifdef HAVE_FORK
static void
sigexit(int signo)
{
    if(signo == SIGUSR1)
        exitFlag = 1;
    else if(signo == SIGUSR2)
        exitFlag = 2;
    else
        exitFlag = 3;
}
#endif

void
initEvents()
{
#ifdef HAVE_FORK
    struct sigaction sa;
    sigset_t ss;

    sigemptyset(&ss);
    sa.sa_handler = SIG_IGN;
    sa.sa_mask = ss;
    sa.sa_flags = 0;
    sigaction(SIGPIPE, &sa, NULL);

    sigemptyset(&ss);
    sa.sa_handler = sigexit;
    sa.sa_mask = ss;
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);

    sigemptyset(&ss);
    sa.sa_handler = sigexit;
    sa.sa_mask = ss;
    sa.sa_flags = 0;
    sigaction(SIGHUP, &sa, NULL);

    sigemptyset(&ss);
    sa.sa_handler = sigexit;
    sa.sa_mask = ss;
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    sigemptyset(&ss);
    sa.sa_handler = sigexit;
    sa.sa_mask = ss;
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);

    sigemptyset(&ss);
    sa.sa_handler = sigexit;
    sa.sa_mask = ss;
    sa.sa_flags = 0;
    sigaction(SIGUSR2, &sa, NULL);
#endif

    timeEventQueue = NULL;
    timeEventQueueLast = NULL;
    fdEventSize = 0;
    fdEventNum = 0;
    poll_fds = NULL;
    fdEvents = NULL;
    fdEventsLast = NULL;
}

void
uninitEvents(void)
{
#ifdef HAVE_FORK
    struct sigaction sa;
    sigset_t ss;

    sigemptyset(&ss);
    sa.sa_handler = SIG_DFL;
    sa.sa_mask = ss;
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);

    sigemptyset(&ss);
    sa.sa_handler = SIG_DFL;
    sa.sa_mask = ss;
    sa.sa_flags = 0;
    sigaction(SIGHUP, &sa, NULL);

    sigemptyset(&ss);
    sa.sa_handler = SIG_DFL;
    sa.sa_mask = ss;
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    sigemptyset(&ss);
    sa.sa_handler = SIG_DFL;
    sa.sa_mask = ss;
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);

    sigemptyset(&ss);
    sa.sa_handler = SIG_DFL;
    sa.sa_mask = ss;
    sa.sa_flags = 0;
    sigaction(SIGUSR2, &sa, NULL);
#endif
}

#ifdef HAVE_FORK
void
interestingSignals(sigset_t *ss)
{
    sigemptyset(ss);
    sigaddset(ss, SIGTERM);
    sigaddset(ss, SIGHUP);
    sigaddset(ss, SIGINT);
    sigaddset(ss, SIGUSR1);
    sigaddset(ss, SIGUSR2);
}
#endif

void
timeToSleep(struct timeval *time)
{
    if(!timeEventQueue) {
        time->tv_sec = ~0L;
        time->tv_usec = ~0L;
    } else {
        *time = timeEventQueue->time;
    }
}

static TimeEventHandlerPtr
enqueueTimeEvent(TimeEventHandlerPtr event)
{
    TimeEventHandlerPtr otherevent;

    /* We try to optimise two cases -- the event happens very soon, or
       it happens after most of the other events. */
    if(timeEventQueue == NULL ||
       timeval_cmp(&event->time, &timeEventQueue->time) < 0) {
        /* It's the first event */
        event->next = timeEventQueue;
        event->previous = NULL;
        if(timeEventQueue) {
            timeEventQueue->previous = event;
        } else {
            timeEventQueueLast = event;
        }
        timeEventQueue = event;
    } else if(timeval_cmp(&event->time, &timeEventQueueLast->time) >= 0) {
        /* It's the last one */
        event->next = NULL;
        event->previous = timeEventQueueLast;
        timeEventQueueLast->next = event;
        timeEventQueueLast = event;
    } else {
        /* Walk from the end */
        otherevent = timeEventQueueLast;
        while(otherevent->previous &&
              timeval_cmp(&event->time, &otherevent->previous->time) < 0) {
            otherevent = otherevent->previous;
        }
        event->next = otherevent;
        event->previous = otherevent->previous;
        if(otherevent->previous) {
            otherevent->previous->next = event;
        } else {
            timeEventQueue = event;
        }
        otherevent->previous = event;
    }
    return event;
}

TimeEventHandlerPtr
scheduleTimeEvent(int seconds,
                  int (*handler)(TimeEventHandlerPtr), int dsize, void *data)
{
    struct timeval when;
    TimeEventHandlerPtr event;

    if(seconds >= 0) {
        when = current_time;
        when.tv_sec += seconds;
    } else {
        when.tv_sec = 0;
        when.tv_usec = 0;
    }

    event = malloc(sizeof(TimeEventHandlerRec) - 1 + dsize);
    if(event == NULL) {
        do_log(L_ERROR, "Couldn't allocate time event handler -- "
               "discarding all objects.\n");
        exitFlag = 2;
        return NULL;
    }

    event->time = when;
    event->handler = handler;
    /* Let the compiler optimise the common case */
    if(dsize == sizeof(void*))
        memcpy(event->data, data, sizeof(void*));
    else if(dsize > 0)
        memcpy(event->data, data, dsize);

    return enqueueTimeEvent(event);
}

void
cancelTimeEvent(TimeEventHandlerPtr event)
{
    if(event == timeEventQueue)
        timeEventQueue = event->next;
    if(event == timeEventQueueLast)
        timeEventQueueLast = event->previous;
    if(event->next)
        event->next->previous = event->previous;
    if(event->previous)
        event->previous->next = event->next;
    free(event);
}

int
allocateFdEventNum(int fd)
{
    int i;
    if(fdEventNum < fdEventSize) {
        i = fdEventNum;
        fdEventNum++;
    } else {
        struct pollfd *new_poll_fds;
        FdEventHandlerPtr *new_fdEvents, *new_fdEventsLast;
        int new_size = 3 * fdEventSize / 2 + 1;

        new_poll_fds = realloc(poll_fds, new_size * sizeof(struct pollfd));
        if(!new_poll_fds)
            return -1;
        new_fdEvents = realloc(fdEvents, new_size * sizeof(FdEventHandlerPtr));
        if(!new_fdEvents)
            return -1;
        new_fdEventsLast = realloc(fdEventsLast, 
                                   new_size * sizeof(FdEventHandlerPtr));
        if(!new_fdEventsLast)
            return -1;

        poll_fds = new_poll_fds;
        fdEvents = new_fdEvents;
        fdEventsLast = new_fdEventsLast;
        fdEventSize = new_size;
        i = fdEventNum;
        fdEventNum++;
    }

    poll_fds[i].fd = fd;
    poll_fds[i].events = POLLERR | POLLHUP | POLLNVAL;
    poll_fds[i].revents = 0;
    fdEvents[i] = NULL;
    fdEventsLast[i] = NULL;
    fds_invalid = 1;
    return i;
}

void
deallocateFdEventNum(int i)
{
    if(i < fdEventNum - 1) {
        memmove(&poll_fds[i], &poll_fds[i + 1], 
                (fdEventNum - i - 1) * sizeof(struct pollfd));
        memmove(&fdEvents[i], &fdEvents[i + 1],
                (fdEventNum - i - 1) * sizeof(FdEventHandlerPtr));
        memmove(&fdEventsLast[i], &fdEventsLast[i + 1],
                (fdEventNum - i - 1) * sizeof(FdEventHandlerPtr));
    }
    fdEventNum--;
    fds_invalid = 1;
}

FdEventHandlerPtr 
makeFdEvent(int fd, int poll_events, 
            int (*handler)(int, FdEventHandlerPtr), int dsize, void *data)
{
    FdEventHandlerPtr event;

    event = malloc(sizeof(FdEventHandlerRec) - 1 + dsize);
    if(event == NULL) {
        do_log(L_ERROR, "Couldn't allocate fd event handler -- "
               "discarding all objects.\n");
        exitFlag = 2;
        return NULL;
    }
    event->fd = fd;
    event->poll_events = poll_events;
    event->handler = handler;
    /* Let the compiler optimise the common cases */
    if(dsize == sizeof(void*))
        memcpy(event->data, data, sizeof(void*));
    else if(dsize == sizeof(StreamRequestRec))
        memcpy(event->data, data, sizeof(StreamRequestRec));
    else if(dsize > 0)
        memcpy(event->data, data, dsize);
    return event;
}

FdEventHandlerPtr
registerFdEventHelper(FdEventHandlerPtr event)
{
    int i;
    int fd = event->fd;

    for(i = 0; i < fdEventNum; i++)
        if(poll_fds[i].fd == fd)
            break;

    if(i >= fdEventNum)
        i = allocateFdEventNum(fd);
    if(i < 0) {
        free(event);
        return NULL;
    }

    event->next = NULL;
    event->previous = fdEventsLast[i];
    if(fdEvents[i] == NULL) {
        fdEvents[i] = event;
    } else {
        fdEventsLast[i]->next = event;
    }
    fdEventsLast[i] = event;
    poll_fds[i].events |= event->poll_events;

    return event;
}

FdEventHandlerPtr 
registerFdEvent(int fd, int poll_events, 
                int (*handler)(int, FdEventHandlerPtr), int dsize, void *data)
{
    FdEventHandlerPtr event;

    event = makeFdEvent(fd, poll_events, handler, dsize, data);
    if(event == NULL)
        return NULL;

    return registerFdEventHelper(event);
}

static int
recomputePollEvents(FdEventHandlerPtr event) 
{
    int pe = 0;
    while(event) {
        pe |= event->poll_events;
        event = event->next;
    }
    return pe | POLLERR | POLLHUP | POLLNVAL;
}

static void
unregisterFdEventI(FdEventHandlerPtr event, int i)
{
    assert(i < fdEventNum && poll_fds[i].fd == event->fd);

    if(fdEvents[i] == event) {
        assert(!event->previous);
        fdEvents[i] = event->next;
    } else {
        event->previous->next = event->next;
    }

    if(fdEventsLast[i] == event) {
        assert(!event->next);
        fdEventsLast[i] = event->previous;
    } else {
        event->next->previous = event->previous;
    }

    free(event);

    if(fdEvents[i] == NULL) {
        deallocateFdEventNum(i);
    } else {
        poll_fds[i].events = recomputePollEvents(fdEvents[i]) | 
            POLLERR | POLLHUP | POLLNVAL;
    }
}

void 
unregisterFdEvent(FdEventHandlerPtr event)
{
    int i;

    for(i = 0; i < fdEventNum; i++) {
        if(poll_fds[i].fd == event->fd) {
            unregisterFdEventI(event, i);
            return;
        }
    }
    abort();
}

void
runTimeEventQueue()
{
    TimeEventHandlerPtr event;
    int done;

    while(timeEventQueue && 
          timeval_cmp(&timeEventQueue->time, &current_time) <= 0) {
        event = timeEventQueue;
        timeEventQueue = event->next;
        if(timeEventQueue)
            timeEventQueue->previous = NULL;
        else
            timeEventQueueLast = NULL;
        done = event->handler(event);
        assert(done);
        free(event);
    }
}

static FdEventHandlerPtr
findEventHelper(int revents, FdEventHandlerPtr events)
{
    FdEventHandlerPtr event = events;
    while(event) {
        if(revents & event->poll_events)
            return event;
        event = event->next;
    }
    return NULL;
}



static FdEventHandlerPtr
findEvent(int revents, FdEventHandlerPtr events)
{
    FdEventHandlerPtr event;

    assert(!(revents & POLLNVAL));
    
    if((revents & POLLHUP) || (revents & POLLERR)) {
        event = findEventHelper(POLLOUT, events);
        if(event) return event;

        event = findEventHelper(POLLIN, events);
        if(event) return event;
        return NULL;
    }

    if(revents & POLLOUT) {
        event = findEventHelper(POLLOUT, events);
        if(event) return event;
    }

    if(revents & POLLIN) {
        event = findEventHelper(POLLIN, events);
        if(event) return event;
    }
    return NULL;
}

typedef struct _FdEventHandlerPoke {
    int fd;
    int what;
    int status;
} FdEventHandlerPokeRec, *FdEventHandlerPokePtr;

static int
pokeFdEventHandler(TimeEventHandlerPtr tevent)
{
    FdEventHandlerPokePtr poke = (FdEventHandlerPokePtr)tevent->data;
    int fd = poke->fd;
    int what = poke->what;
    int status = poke->status;
    int done;
    FdEventHandlerPtr event, next;
    int i;

    for(i = 0; i < fdEventNum; i++) {
        if(poll_fds[i].fd == fd)
            break;
    }

    if(i >= fdEventNum)
        return 1;

    event = fdEvents[i];
    while(event) {
        next = event->next;
        if(event->poll_events & what) {
            done = event->handler(status, event);
            if(done) {
                if(fds_invalid)
                    unregisterFdEvent(event);
                else
                    unregisterFdEventI(event, i);
            }
            if(fds_invalid)
                break;
        }
        event = next;
    }
    return 1;
}

void 
pokeFdEvent(int fd, int status, int what)
{
    TimeEventHandlerPtr handler;
    FdEventHandlerPokeRec poke;

    poke.fd = fd;
    poke.status = status;
    poke.what = what;

    handler = scheduleTimeEvent(0, pokeFdEventHandler,
                                sizeof(poke), &poke);
    if(!handler) {
        do_log(L_ERROR, "Couldn't allocate handler.\n");
    }
}

int
workToDo()
{
    struct timeval sleep_time;
    int rc;

    if(exitFlag)
        return 1;

    timeToSleep(&sleep_time);
    gettimeofday(&current_time, NULL);
    if(timeval_cmp(&sleep_time, &current_time) <= 0)
        return 1;
    rc = poll(poll_fds, fdEventNum, 0);
    if(rc < 0) {
        do_log_error(L_ERROR, errno, "Couldn't poll");
        return 1;
    }
    return(rc >= 1);
}
    
void
eventLoop()
{
    struct timeval sleep_time, timeout;
    int rc, i, done, n;
    FdEventHandlerPtr event;
    int fd0;

    gettimeofday(&current_time, NULL);

    while(1) {
    again:
        if(exitFlag) {
            if(exitFlag < 3)
                reopenLog();
            if(exitFlag >= 2) {
                discardObjects(1, 0);
                if(exitFlag >= 3)
                    return;
                free_chunk_arenas();
            } else {
                writeoutObjects(1);
            }
            initForbidden();
            exitFlag = 0;
        }

        timeToSleep(&sleep_time);
        if(sleep_time.tv_sec == -1) {
            rc = poll(poll_fds, fdEventNum, 
                      diskIsClean ? -1 : idleTime * 1000);
        } else if(timeval_cmp(&sleep_time, &current_time) <= 0) {
            runTimeEventQueue();
            continue;
        } else {
            gettimeofday(&current_time, NULL);
            if(timeval_cmp(&sleep_time, &current_time) <= 0) {
                runTimeEventQueue();
                continue;
            } else {
                int t;
                timeval_minus(&timeout, &sleep_time, &current_time);
                t = timeout.tv_sec * 1000 + (timeout.tv_usec + 999) / 1000;
                rc = poll(poll_fds, fdEventNum,
                          diskIsClean ? t : MIN(idleTime * 1000, t));
            }
        }

        gettimeofday(&current_time, NULL);

        if(rc < 0) {
            if(errno == EINTR) {
                continue;
            } else if(errno == ENOMEM) {
                free_chunk_arenas();
                do_log(L_ERROR, 
                       "Couldn't poll: out of memory.  "
                       "Sleeping for one second.\n");
                sleep(1);
            } else {
                do_log_error(L_ERROR, errno, "Couldn't poll");
                exitFlag = 3;
            }
            continue;
        }

        if(rc == 0) {
            if(!diskIsClean) {
                timeToSleep(&sleep_time);
                if(timeval_cmp(&sleep_time, &current_time) > 0)
                    writeoutObjects(0);
            }
            continue;
        }

        /* Rather than tracking all changes to the in-memory cache, we
           assume that something changed whenever we see any activity. */
        diskIsClean = 0;

        fd0 = 
            (current_time.tv_usec ^ (current_time.tv_usec >> 16)) % fdEventNum;
        n = rc;
        for(i = 0; i < fdEventNum; i++) {
            int j = (i + fd0) % fdEventNum;
            if(n <= 0)
                break;
            if(poll_fds[j].revents) {
                n--;
                event = findEvent(poll_fds[j].revents, fdEvents[j]);
                if(!event)
                    continue;
                done = event->handler(0, event);
                if(done) {
                    if(fds_invalid)
                        unregisterFdEvent(event);
                    else
                        unregisterFdEventI(event, j);
                }
                if(fds_invalid) {
                    fds_invalid = 0;
                    goto again;
                } 
            }
        }
    }
}

void
initCondition(ConditionPtr condition)
{
    condition->handlers = NULL;
}

ConditionPtr
makeCondition(void)
{
    ConditionPtr condition;
    condition = malloc(sizeof(ConditionRec));
    if(condition == NULL)
        return NULL;
    initCondition(condition);
    return condition;
}

ConditionHandlerPtr
conditionWait(ConditionPtr condition,
              int (*handler)(int, ConditionHandlerPtr),
              int dsize, void *data)
{
    ConditionHandlerPtr chandler;

    assert(!in_signalCondition);

    chandler = malloc(sizeof(ConditionHandlerRec) - 1 + dsize);
    if(!chandler)
        return NULL;

    chandler->condition = condition;
    chandler->handler = handler;
    /* Let the compiler optimise the common case */
    if(dsize == sizeof(void*))
        memcpy(chandler->data, data, sizeof(void*));
    else if(dsize > 0)
        memcpy(chandler->data, data, dsize);

    if(condition->handlers)
        condition->handlers->previous = chandler;
    chandler->next = condition->handlers;
    chandler->previous = NULL;
    condition->handlers = chandler;
    return chandler;
}

void
unregisterConditionHandler(ConditionHandlerPtr handler)
{
    ConditionPtr condition = handler->condition;

    assert(!in_signalCondition);

    if(condition->handlers == handler)
        condition->handlers = condition->handlers->next;
    if(handler->next)
        handler->next->previous = handler->previous;
    if(handler->previous)
        handler->previous->next = handler->next;

    free(handler);
}

void 
abortConditionHandler(ConditionHandlerPtr handler)
{
    int done;
    done = handler->handler(-1, handler);
    assert(done);
    unregisterConditionHandler(handler);
}

void
signalCondition(ConditionPtr condition)
{
    ConditionHandlerPtr handler;
    int done;

    assert(!in_signalCondition);
    in_signalCondition++;

    handler = condition->handlers;
    while(handler) {
        ConditionHandlerPtr next = handler->next;
        done = handler->handler(0, handler);
        if(done) {
            if(handler == condition->handlers)
                condition->handlers = next;
            if(next)
                next->previous = handler->previous;
            if(handler->previous)
                handler->previous->next = next;
            else
                condition->handlers = next;
            free(handler);
        }
        handler = next;
    }
    in_signalCondition--;
}

void
polipoExit()
{
    exitFlag = 3;
}
