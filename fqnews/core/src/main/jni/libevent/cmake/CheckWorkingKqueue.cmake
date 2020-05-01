include(CheckCSourceRuns)

check_c_source_runs(
"
#include <sys/types.h>
#include <sys/time.h>
#include <sys/event.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int
main(int argc, char **argv)
{
    int kq;
    int n;
    int fd[2];
    struct kevent ev;
    struct timespec ts;
    char buf[8000];

    if (pipe(fd) == -1)
        exit(1);
    if (fcntl(fd[1], F_SETFL, O_NONBLOCK) == -1)
        exit(1);

    while ((n = write(fd[1], buf, sizeof(buf))) == sizeof(buf))
        ;

        if ((kq = kqueue()) == -1)
        exit(1);

    memset(&ev, 0, sizeof(ev));
    ev.ident = fd[1];
    ev.filter = EVFILT_WRITE;
    ev.flags = EV_ADD | EV_ENABLE;
    n = kevent(kq, &ev, 1, NULL, 0, NULL);
    if (n == -1)
        exit(1);

    read(fd[0], buf, sizeof(buf));

    ts.tv_sec = 0;
    ts.tv_nsec = 0;
    n = kevent(kq, NULL, 0, &ev, 1, &ts);
    if (n == -1 || n == 0)
        exit(1);

    exit(0);
}

" EVENT__HAVE_WORKING_KQUEUE)