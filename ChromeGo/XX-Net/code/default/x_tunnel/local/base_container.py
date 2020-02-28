import threading
import time
import socket
import xstruct as struct
import select

import utils

from xlog import getLogger
xlog = getLogger("x_tunnel")


class WriteBuffer(object):
    def __init__(self, s=None):
        if isinstance(s, str):
            self.string_len = len(s)
            self.buffer_list = [s]
        else:
            self.reset()

    def reset(self):
        self.buffer_list = []
        self.string_len = 0

    def __len__(self):
        return self.string_len

    def __add__(self, other):
        self.append(other)
        return self

    def insert(self, s):
        if isinstance(s, WriteBuffer):
            self.buffer_list = s.buffer_list + self.buffer_list
            self.string_len += s.string_len
        elif isinstance(s, str):
            self.buffer_list.insert(0, s)
            self.string_len += len(s)
        else:
            raise Exception("WriteBuffer append not string or StringBuffer")

    def append(self, s):
        if isinstance(s, WriteBuffer):
            self.buffer_list.extend(s.buffer_list)
            self.string_len += s.string_len
        elif isinstance(s, str):
            self.buffer_list.append(s)
            self.string_len += len(s)
        else:
            raise Exception("WriteBuffer append not string or StringBuffer")

    def __str__(self):
        return self.get_string()

    def get_string(self):
        return "".join(self.buffer_list)


class ReadBuffer(object):
    def __init__(self, buf, begin=0, size=None):
        buf_len = len(buf)
        if size is None:
            if begin > buf_len:
                raise Exception("ReadBuffer buf_len:%d, start:%d" % (buf_len, begin))
            size = buf_len - begin
        elif begin + size > buf_len:
            raise Exception("ReadBuffer buf_len:%d, start:%d len:%d" % (buf_len, begin, size))

        self.size = size
        self.buf = memoryview(buf)
        self.begin = begin

    def __len__(self):
        return self.size

    def get(self, size=None):
        if size is None:
            size = self.size
        elif size > self.size:
            raise Exception("ReadBuffer get %d but left %d" % (size, self.size))

        data = self.buf[self.begin:self.begin + size]
        self.begin += size
        self.size -= size
        return data

    def get_buf(self, size=None):
        if size is None:
            size = self.size
        elif size > self.size:
            raise Exception("ReadBuffer get %d but left %d" % (size, self.size))

        buf = ReadBuffer(self.buf, self.begin, size)

        self.begin += size
        self.size -= size
        return buf


class AckPool():
    def __init__(self):
        self.mutex = threading.Lock()
        self.reset()

    def reset(self):
        # xlog.info("Ack_pool reset")
        self.mutex.acquire()
        self.ack_buffer = WriteBuffer()
        self.mutex.release()
        # xlog.info("Ack_pool reset finished")

    def put(self, data):
        # xlog.debug("Ack_pool put len:%d", len(data))
        self.mutex.acquire()
        self.ack_buffer.append(data)
        self.mutex.release()

    def get(self):
        self.mutex.acquire()
        data = self.ack_buffer
        self.ack_buffer = WriteBuffer()
        self.mutex.release()
        # xlog.debug("Ack_pool get len:%d", len(data))
        return data

    def status(self):
        out_string = "Ack_pool:len %d<br>\r\n" % len(self.ack_buffer)
        return out_string


class WaitQueue():
    def __init__(self):
        self.lock = threading.Lock()
        self.waiters = []
        # (end_time, Lock())

        self.running = True

    def stop(self):
        self.running = False
        xlog.info("WaitQueue stop")
        for end_time, lock in self.waiters:
            lock.release()
        self.waiters = []
        xlog.info("WaitQueue stop finished")

    def notify(self):
        # xlog.debug("notify")
        if len(self.waiters) == 0:
            # xlog.debug("notify none.")
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
                        xlog.warn("get %d from size:%d fail.", i, len(self.waiters))
                        continue

                if is_max:
                    self.waiters.append((end_time, lock))
                else:
                    self.waiters.insert(i, (end_time, lock))

        lock.acquire()

    def status(self):
        out_string = "waiters[%d]:<br>\n" % len(self.waiters)
        for i in range(0, len(self.waiters)):
            end_time, lock = self.waiters[i]
            out_string += "%d<br>\r\n" % ((end_time - time.time()))

        return out_string


class SendBuffer():
    def __init__(self, max_payload):
        self.mutex = threading.Lock()
        self.max_payload = max_payload
        self.reset()

    def reset(self):
        self.pool_size = 0
        self.last_put_time = time.time()
        with self.mutex:
            self.head_sn = 1
            self.tail_sn = 1
            self.block_list = {}
            self.last_block = WriteBuffer()

    def put(self, data):
        dlen = len(data)
        if dlen == 0:
            xlog.warn("SendBuffer put 0")
            return

        # xlog.debug("SendBuffer put len:%d", len(data))
        self.last_put_time = time.time()
        with self.mutex:
            self.pool_size += dlen
            self.last_block.append(data)

            if len(self.last_block) > self.max_payload:
                self.block_list[self.head_sn] = self.last_block
                self.last_block = WriteBuffer()
                self.head_sn += 1
                return True

    def get(self):
        with self.mutex:
            if self.tail_sn < self.head_sn:
                data = self.block_list[self.tail_sn]
                del self.block_list[self.tail_sn]
                sn = self.tail_sn
                self.tail_sn += 1

                self.pool_size -= len(data)
                # xlog.debug("send_pool get, sn:%r len:%d ", sn, len(data))
                return data, sn

            if len(self.last_block) > 0:
                data = self.last_block
                sn = self.tail_sn
                self.last_block = WriteBuffer()
                self.head_sn += 1
                self.tail_sn += 1

                self.pool_size -= len(data)
                # xlog.debug("send_pool get, sn:%r len:%d ", sn, len(data))
                return data, sn

        #xlog.debug("Get:%s", utils.str2hex(data))
        # xlog.debug("SendBuffer get wake after no data, tail:%d", self.tail_sn)
        return "", 0

    def status(self):
        out_string = "SendBuffer:<br>\n"
        out_string += " size:%d<br>\n" % self.pool_size
        out_string += " last_put_time:%f<br>\n" % (time.time() - self.last_put_time)
        out_string += " head_sn:%d<br>\n" % self.head_sn
        out_string += " tail_sn:%d<br>\n" % self.tail_sn
        out_string += "block_list:[%d]<br>\n" % len(self.block_list)
        for sn in sorted(self.block_list.iterkeys()):
            data = self.block_list[sn]
            out_string += "[%d] len:%d<br>\r\n" % (sn, len(data))

        return out_string


class BlockReceivePool():
    def __init__(self, process_callback):
        self.lock = threading.Lock()
        self.process_callback = process_callback
        self.reset()

    def reset(self):
        # xlog.info("recv_pool reset")
        self.next_sn = 1
        self.block_list = []

    def put(self, sn, data):
        self.lock.acquire()
        try:
            if sn < self.next_sn:
                # xlog.warn("recv_pool put timeout sn:%d", sn)
                return False
            elif sn > self.next_sn:
                # xlog.debug("recv_pool put unorder sn:%d", sn)
                if sn in self.block_list:
                    # xlog.warn("recv_pool put sn:%d exist", sn)
                    return False
                else:
                    self.block_list.append(sn)
                    self.process_callback(data)
                    return True
            else:
                # xlog.debug("recv_pool put sn:%d in order", sn)
                self.process_callback(data)
                self.next_sn = sn + 1

                while sn + 1 in self.block_list:
                    sn += 1
                    # xlog.debug("recv_pool sn:%d processed", sn)
                    self.block_list.remove(sn)
                    self.next_sn = sn + 1
                return True
        except Exception as e:
            raise Exception("recv_pool put sn:%d len:%d error:%r" % (sn, len(data), e))
        finally:
            self.lock.release()

    def status(self):
        out_string = "Block_receive_pool:<br>\r\n"
        out_string += " next_sn:%d<br>\r\n" % self.next_sn
        for sn in sorted(self.block_list):
            out_string += "[%d] <br>\r\n" % (sn)

        return out_string


class Conn(object):
    def __init__(self, session, conn_id, sock, host, port, windows_size, windows_ack, is_client, xlog):
        # xlog.info("session:%s Conn:%d host:%s port:%d", session.session_id, conn_id, host, port)
        self.host = host
        self.port = port
        self.session = session
        self.conn_id = conn_id
        self.sock = sock
        self.windows_size = windows_size
        self.windows_ack = windows_ack
        self.is_client = is_client

        self.cmd_queue = {}
        self.cmd_notice = threading.Condition()
        self.recv_notice = threading.Condition()
        self.running = True
        self.received_position = 0
        self.remote_acked_position = 0
        self.sended_position = 0
        self.sended_window_position = 0
        self.recv_thread = None
        self.cmd_thread = None
        self.xlog = xlog

        self.transfered_close_to_peer = False
        if sock:
            self.next_cmd_seq = 1
        else:
            self.next_cmd_seq = 0

        self.next_recv_seq = 1

    def start(self, block):
        if self.sock:
            self.recv_thread = threading.Thread(target=self.recv_worker)
            self.recv_thread.start()
        else:
            self.recv_thread = None

        if block:
            self.cmd_thread = None
            self.cmd_processor()
        else:
            self.cmd_thread = threading.Thread(target=self.cmd_processor)
            self.cmd_thread.start()

    def status(self):
        out_string = "Conn[%d]: %s:%d<br>\r\n" % (self.conn_id, self.host, self.port)
        out_string += " received_position:%d/ Ack:%d <br>\n" % (self.received_position, self.remote_acked_position)
        out_string += " sended_position:%d/ win:%d<br>\n" % (self.sended_position, self.sended_window_position)
        out_string += " next_cmd_seq:%d<br>\n" % self.next_cmd_seq
        out_string += " next_recv_seq:%d<br>\n" % self.next_recv_seq
        out_string += " status: running:%r<br>\n" % self.running
        out_string += " transfered_close_to_peer:%r<br>\n" % self.transfered_close_to_peer
        out_string += " sock:%r<br>\n" % (self.sock is not None)
        out_string += " cmd_queue.len:%d " % len(self.cmd_queue)
        for seq in self.cmd_queue:
            out_string += "[%d]," % seq
        out_string += "<br>\n"

        return out_string

    def stop(self, reason=""):
        self.stop_thread = threading.Thread(target=self.do_stop, args=(reason,))
        self.stop_thread.start()

    def do_stop(self, reason="unknown"):
        self.xlog.debug("Conn session:%s conn:%d stop:%s", self.session.session_id, self.conn_id, reason)
        self.running = False

        self.cmd_notice.acquire()
        self.cmd_notice.notify()
        self.cmd_notice.release()

        self.recv_notice.acquire()
        self.recv_notice.notify()
        self.recv_notice.release()

        if self.recv_thread:
            self.recv_thread.join()
            self.recv_thread = None
        if self.cmd_thread:
            self.cmd_thread.join()
            self.cmd_thread = None

        self.cmd_queue = {}

        if self.sock is not None:
            self.sock.close()
            self.sock = None

        # xlog.debug("Conn session:%s conn:%d stopped", self.session.session_id, self.conn_id)
        self.session.remove_conn(self.conn_id)

    def do_connect(self, host, port):
        self.xlog.info("session_id:%s create_conn %d %s:%d", self.session.session_id, self.conn_id, host, port)
        connect_timeout = 30
        sock = None
        # start_time = time.time()
        ip = ""
        try:
            if ':' in host:
                # IPV6
                ip = host
            elif utils.check_ip_valid4(host):
                # IPV4
                ip = host
            else:
                # xlog.debug("getting ip of %s", host)
                ip = socket.gethostbyname(host)
                # xlog.debug("resolve %s to %s", host, ip)
            sock = socket.socket(socket.AF_INET if ':' not in ip else socket.AF_INET6)
            # set reuseaddr option to avoid 10048 socket error
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            # resize socket recv buffer ->256K to improve browser releated application performance
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 262144)
            # disable negal algorithm to send http request quickly.
            sock.setsockopt(socket.SOL_TCP, socket.TCP_NODELAY, True)
            # set a short timeout to trigger timeout retry more quickly.
            sock.settimeout(connect_timeout)

            sock.connect((ip, port))

            # record TCP connection time
            # conn_time = time.time() - start_time
            # xlog.debug("tcp conn %s %s time:%d", host, ip, conn_time * 1000)

            return sock, True
        except Exception as e:
            # conn_time = int((time.time() - start_time) * 1000)
            # xlog.debug("tcp conn host:%s %s:%d fail t:%d %r", host, ip, port, conn_time, e)
            if sock:
                sock.close()
            return e, False

    def put_cmd_data(self, data):
        with self.cmd_notice:
            seq = struct.unpack("<I", data.get(4))[0]
            if seq < self.next_cmd_seq:
                # xlog.warn("put_send_data %s conn:%d seq:%d next:%d",
                #               self.session.session_id, self.conn_id,
                #               seq, self.next_cmd_seq)
                return

            self.cmd_queue[seq] = data.get_buf()

            if seq == self.next_cmd_seq:
                self.cmd_notice.notify()

    def get_cmd_data(self):
        self.cmd_notice.acquire()
        try:
            while self.running:
                if self.next_cmd_seq in self.cmd_queue:
                    payload = self.cmd_queue[self.next_cmd_seq]
                    del self.cmd_queue[self.next_cmd_seq]
                    self.next_cmd_seq += 1
                    #self.xlog.debug("Conn session:%s conn:%d get data len:%d ", self.session.session_id, self.conn_id, len(payload))
                    return payload
                else:
                    self.cmd_notice.wait()
        finally:
            self.cmd_notice.release()
        return False

    def cmd_processor(self):
        while self.running:
            data = self.get_cmd_data()
            if not data:
                break

            cmd_id = struct.unpack("<B", data.get(1))[0]
            if cmd_id == 1:  # data
                self.send_to_sock(data)

            elif cmd_id == 3:  # ack:
                position = struct.unpack("<Q", data.get(8))[0]
                self.xlog.debug("Conn session:%s conn:%d ACK:%d", self.session.session_id, self.conn_id, position)
                if position > self.remote_acked_position:
                    self.remote_acked_position = position
                    self.recv_notice.acquire()
                    self.recv_notice.notify()
                    self.recv_notice.release()

            elif cmd_id == 2:  # Closed
                dat = data.get()
                if isinstance(dat, memoryview):
                    dat = dat.tobytes()
                self.xlog.debug("Conn session:%s conn:%d Peer Close:%s", self.session.session_id, self.conn_id, dat)
                if self.is_client:
                    self.transfer_peer_close("finish")
                self.stop("peer close")

            elif cmd_id == 0:  # Create connect
                if self.port or len(self.host) or self.next_cmd_seq != 1 or self.sock:
                    raise Exception("put_send_data %s conn:%d Create but host:%s port:%d next seq:%d" % (
                        self.session.session_id, self.conn_id,
                        self.host, self.port, self.next_cmd_seq))

                self.sock_type = struct.unpack("<B", data.get(1))[0]
                host_len = struct.unpack("<H", data.get(2))[0]
                self.host = data.get(host_len)
                self.port = struct.unpack("<H", data.get(2))[0]

                sock, res = self.do_connect(self.host, self.port)
                if res is False:
                    self.xlog.debug("Conn session:%s conn:%d %s:%d Create fail", self.session.session_id, self.conn_id,
                               self.host, self.port)
                    self.transfer_peer_close("connect fail")
                else:
                    self.xlog.info("Conn session:%s conn:%d %s:%d", self.session.session_id, self.conn_id, self.host,
                              self.port)
                    self.sock = sock
                    self.recv_thread = threading.Thread(target=self.recv_worker)
                    self.recv_thread.start()
            else:
                self.xlog.error("Conn session:%s conn:%d unknown cmd_id:%d",
                                self.session.session_id, self.conn_id, cmd_id)
                raise Exception("put_send_data unknown cmd_id:%d" % cmd_id)

    def send_to_sock(self, data):
        sock = self.sock
        if not sock:
            return

        payload_len = len(data)
        buf = data.buf
        start = data.begin
        end = data.begin + payload_len
        while start < end:
            send_size = min(end - start, 65535)
            try:
                sended = sock.send(buf[start:start + send_size])
            except Exception as e:
                self.xlog.info("%s conn_id:%d send closed", self.session.session_id, self.conn_id)
                sock.close()
                self.sock = None
                if self.is_client:
                    self.do_stop(reason="send fail.")
                return

            start += sended

        self.sended_position += payload_len
        if self.sended_position - self.sended_window_position > self.windows_ack:
            self.sended_window_position = self.sended_position
            self.transfer_ack(self.sended_position)
            # xlog.debug("Conn:%d ack:%d", self.conn_id, self.sended_window_position)

    def transfer_peer_close(self, reason=""):
        with self.recv_notice:
            if self.transfered_close_to_peer:
                return
            self.transfered_close_to_peer = True

            cmd = struct.pack("<IB", self.next_recv_seq, 2)
            self.session.send_conn_data(self.conn_id, cmd + reason)
            self.next_recv_seq += 1

    def transfer_received_data(self, data):
        with self.recv_notice:

            if self.transfered_close_to_peer:
                return

            buf = WriteBuffer(struct.pack("<IB", self.next_recv_seq, 1))
            buf.append(data)
            self.next_recv_seq += 1
            self.received_position += len(data)

            if self.received_position < 16 * 1024:
                no_delay = True
            else:
                no_delay = False

            self.session.send_conn_data(self.conn_id, buf, no_delay)

    def transfer_ack(self, position):
        with self.recv_notice:
            if self.transfered_close_to_peer:
                return

            cmd_position = struct.pack("<IBQ", self.next_recv_seq, 3, position)
            self.session.send_conn_data(self.conn_id, cmd_position)
            self.next_recv_seq += 1

    def recv_worker(self):
        sock = self.sock
        fdset = [sock, ]
        while self.running:

            self.recv_notice.acquire()
            try:
                if self.received_position > self.remote_acked_position + self.windows_size:
                    # xlog.debug("Conn session:%s conn:%d recv blocked, rcv:%d, ack:%d", self.session.session_id, self.conn_id, self.received_position, self.remote_acked_position)
                    self.recv_notice.wait()
                    continue
            finally:
                self.recv_notice.release()

            r, w, e = select.select(fdset, [], [], 1)
            if sock in r:
                try:
                    data = sock.recv(65535)
                except:
                    data = ""

                data_len = len(data)
                if data_len == 0:
                    # xlog.debug("Conn session:%s conn:%d recv socket closed", self.session.session_id, self.conn_id)
                    self.transfer_peer_close("recv closed")

                    sock.close()
                    self.sock = None

                    self.recv_thread = None
                    if self.is_client:
                        self.do_stop(reason="recv fail.")

                    return

                self.transfer_received_data(data)
                # xlog.debug("Conn session:%s conn:%d Recv len:%d id:%d", self.session.session_id, self.conn_id, data_len, self.recv_id)

                # xlog.debug("Conn session:%s conn:%d Recv worker stopped", self.session.session_id, self.conn_id)
