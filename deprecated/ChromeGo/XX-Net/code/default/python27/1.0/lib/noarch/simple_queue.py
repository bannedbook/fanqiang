import time
import threading
import sys


# This simple Queue fix the performance problem in the system build-in Queue.
# Every get with time out will run in thread sleep check sleep check...
# cost too many CPU and delay queue response.

# This solution will run a thread to check timeout, default is 0.1 second.
# And message send queue with no delay, use thread lock/release.


list_lock = threading.Lock()
th_lock = threading.Lock()
queue_list = []
timer_th = None

timeout_interval = 0.1
# User can change  timeout_interval to bigger to reduce CPU load.


def timer_thread():
    global timer_th
    while True:
        with list_lock:
            to_del = []
            wait_count = 0
            for q in queue_list:
                wait_count += q.check()

                c = sys.getrefcount(q)
                # print(c, id(q))
                if c <= 3:
                    # reference of object, less then 3 means no out side use.
                    to_del.append(q)

            for q in to_del:
                # print("del queue")
                queue_list.remove(q)

            if wait_count == 0:
                break

        time.sleep(timeout_interval)

    with th_lock:
        timer_th = None
    # print("simple queue timer exit")


def _add_queue(qq):
    with list_lock:
        queue_list.append(qq)


def _add_wait():
    global timer_th
    with th_lock:
        if not timer_th:
            timer_th = threading.Thread(target=timer_thread)
            timer_th.start()


class Queue(object):
    def __init__(self):
        self.lock = threading.Lock()
        self.queue = []
        self.waiters = []
        self.running = True
        _add_queue(self)

    def __sizeof__(self):
        return len(self.queue)

    def reset(self):
        self.running = False
        self.queue = []
        self.notify_all()
        self.running = True

    def check(self):
        if not self.waiters:
            return 0

        try:
            if time.time() > self.waiters[0][0]:
                self.notify()
        except:
            pass

        return 1

    def put(self, item):
        with self.lock:
            self.queue.append(item)
            self.notify()

    def get(self, timeout=None):
        if not timeout:
            with self.lock:
                if not self.queue:
                    return
                else:
                    return self.queue.pop(0)

        end_time = time.time() + timeout
        while self.running:
            with self.lock:
                if self.queue:
                    return self.queue.pop(0)

            if time.time() > end_time:
                return

            self.wait(end_time)

    def notify_all(self):
        while self.waiters:
            self.notify()

    def notify(self):
        if len(self.waiters) == 0:
            return

        try:
            end_time, lock = self.waiters.pop(0)
            lock.release()
        except:
            pass

    def wait(self, end_time):
        with self.lock:
            lock = threading.Lock()
            lock.acquire()

            if len(self.waiters) == 0:
                self.waiters.append((end_time, lock))
            else:
                is_max = True
                for i in range(0, len(self.waiters)):
                    try:
                        iend_time, ilock = self.waiters[i]
                        if iend_time > end_time:
                            is_max = False
                            break
                    except Exception as e:
                        if i >= len(self.waiters):
                            break
                        #xlog.warn("get %d from size:%d fail.", i, len(self.waiters))
                        continue

                if is_max:
                    self.waiters.append((end_time, lock))
                else:
                    self.waiters.insert(i, (end_time, lock))

            _add_wait()

        lock.acquire()


def test_pub(q, x):
    time.sleep(2)
    print("put x")
    q.put(x)


def test():
    q1 = Queue()
    q1.put("a")
    print(q1.get())

    threading.Thread(target=test_pub, args=(q1, "b")).start()
    print(q1.get(15))


if __name__ == '__main__':
    test()


