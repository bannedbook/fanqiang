#!/usr/bin/python2

def get(old,wc,rc,cc):
    if ('xxx' in (rc, wc, cc)):
        return "0",255

    if ('add' in (rc, wc, cc)):
        events = []
        if rc == 'add' or (rc != 'del' and 'r' in old):
            events.append("EPOLLIN")
        if wc == 'add' or (wc != 'del' and 'w' in old):
            events.append("EPOLLOUT")
        if cc == 'add' or (cc != 'del' and 'c' in old):
            events.append("EPOLLRDHUP")

        if old == "0":
            op = "EPOLL_CTL_ADD"
        else:
            op = "EPOLL_CTL_MOD"
        return "|".join(events), op

    if ('del' in (rc, wc, cc)):
        delevents = []
        modevents = []
        op = "EPOLL_CTL_DEL"

        if 'r' in old:
            modevents.append("EPOLLIN")
        if 'w' in old:
            modevents.append("EPOLLOUT")
        if 'c' in old:
            modevents.append("EPOLLRDHUP")

        for item, event in [(rc,"EPOLLIN"),
                            (wc,"EPOLLOUT"),
                            (cc,"EPOLLRDHUP")]:
            if item == 'del':
                delevents.append(event)
                if event in modevents:
                    modevents.remove(event)

        if modevents:
            return "|".join(modevents), "EPOLL_CTL_MOD"
        else:
            return "|".join(delevents), "EPOLL_CTL_DEL"

    return 0, 0


def fmt(op, ev, old, wc, rc, cc):
    entry = "{ %s, %s },"%(op, ev)
    print "\t/* old=%3s, write:%3s, read:%3s, close:%3s */\n\t%s" % (
        old, wc, rc, cc, entry)
    return len(entry)

for old in ('0','r','w','rw','c','cr','cw','crw'):
    for wc in ('0', 'add', 'del', 'xxx'):
        for rc in ('0', 'add', 'del', 'xxx'):
            for cc in ('0', 'add', 'del', 'xxx'):

                op,ev = get(old,wc,rc,cc)

                fmt(op, ev, old, wc, rc, cc)
