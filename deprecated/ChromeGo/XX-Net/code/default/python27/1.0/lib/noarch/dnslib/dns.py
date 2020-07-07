# -*- coding: utf-8 -*-
"""
    DNS - main dnslib module

    Contains core DNS packet handling code
"""

from __future__ import print_function

import base64,binascii,collections,copy,os.path,random,socket,\
       string,struct,textwrap,time

from itertools import chain

try:
    from itertools import zip_longest
except ImportError:
    from itertools import izip_longest as zip_longest

from dnslib.bit import get_bits,set_bits
from dnslib.bimap import Bimap,BimapError
from dnslib.buffer import Buffer,BufferError
from dnslib.label import DNSLabel,DNSLabelError,DNSBuffer
from dnslib.lex import WordLexer
from dnslib.ranges import B,H,I,IP4,IP6,ntuple_range

class DNSError(Exception):
    pass

# DNS codes

QTYPE =  Bimap('QTYPE',
                {1:'A', 2:'NS', 5:'CNAME', 6:'SOA', 12:'PTR', 15:'MX',
                 16:'TXT', 17:'RP', 18:'AFSDB', 24:'SIG', 25:'KEY', 28:'AAAA',
                 29:'LOC', 33:'SRV', 35:'NAPTR', 36:'KX', 37:'CERT', 39:'DNAME',
                 41:'OPT', 42:'APL', 43:'DS', 44:'SSHFP', 45:'IPSECKEY',
                 46:'RRSIG', 47:'NSEC', 48:'DNSKEY', 49:'DHCID', 50:'NSEC3',
                 51:'NSEC3PARAM', 52:'TLSA', 55:'HIP', 99:'SPF', 249:'TKEY',
                 250:'TSIG', 251:'IXFR', 252:'AXFR', 255:'ANY', 257:'TYPE257',
                 32768:'TA', 32769:'DLV'},
                DNSError)
CLASS =  Bimap('CLASS',
                {1:'IN', 2:'CS', 3:'CH', 4:'Hesiod', 254:'None', 255:'*'},
                DNSError)
QR =     Bimap('QR',
                {0:'QUERY', 1:'RESPONSE'},
                DNSError)
RCODE =  Bimap('RCODE',
                {0:'NOERROR', 1:'FORMERR', 2:'SERVFAIL', 3:'NXDOMAIN',
                 4:'NOTIMP', 5:'REFUSED', 6:'YXDOMAIN', 7:'YXRRSET',
                 8:'NXRRSET', 9:'NOTAUTH', 10:'NOTZONE'},
                DNSError)
OPCODE = Bimap('OPCODE',{0:'QUERY', 1:'IQUERY', 2:'STATUS', 5:'UPDATE'},
                DNSError)

def label(label,origin=None):
    if label.endswith("."):
        return DNSLabel(label)
    else:
        return (origin if isinstance(origin,DNSLabel)
                       else DNSLabel(origin)).add(label)

class DNSRecord(object):

    """
        Main DNS class - corresponds to DNS packet & comprises DNSHeader,
        DNSQuestion and RR sections (answer,ns,ar)

        >>> d = DNSRecord()
        >>> d.add_question(DNSQuestion("abc.com")) # Or DNSRecord.question("abc.com")
        >>> d.add_answer(RR("abc.com",QTYPE.CNAME,ttl=60,rdata=CNAME("ns.abc.com")))
        >>> d.add_auth(RR("abc.com",QTYPE.SOA,ttl=60,rdata=SOA("ns.abc.com","admin.abc.com",(20140101,3600,3600,3600,3600))))
        >>> d.add_ar(RR("ns.abc.com",ttl=60,rdata=A("1.2.3.4")))
        >>> print(d)
        ;; ->>HEADER<<- opcode: QUERY, status: NOERROR, id: ...
        ;; flags: rd; QUERY: 1, ANSWER: 1, AUTHORITY: 1, ADDITIONAL: 1
        ;; QUESTION SECTION:
        ;abc.com.                       IN      A
        ;; ANSWER SECTION:
        abc.com.                60      IN      CNAME   ns.abc.com.
        ;; AUTHORITY SECTION:
        abc.com.                60      IN      SOA     ns.abc.com. admin.abc.com. 20140101 3600 3600 3600 3600
        ;; ADDITIONAL SECTION:
        ns.abc.com.             60      IN      A       1.2.3.4
        >>> str(d) == str(DNSRecord.parse(d.pack()))
        True
    """

    @classmethod
    def parse(cls,packet):
        """
            Parse DNS packet data and return DNSRecord instance
            Recursively parses sections (calling appropriate parse method)
        """
        buffer = DNSBuffer(packet)
        try:
            header = DNSHeader.parse(buffer)
            questions = []
            rr = []
            auth = []
            ar = []
            for i in range(header.q):
                questions.append(DNSQuestion.parse(buffer))
            for i in range(header.a):
                rr.append(RR.parse(buffer))
            for i in range(header.auth):
                auth.append(RR.parse(buffer))
            for i in range(header.ar):
                ar.append(RR.parse(buffer))
            return cls(header,questions,rr,auth=auth,ar=ar)
        except DNSError:
            raise
        except (BufferError,BimapError) as e:
            raise DNSError("Error unpacking DNSRecord [offset=%d]: %s" % (
                                    buffer.offset,e))

    @classmethod
    def question(cls,qname,qtype="A",qclass="IN"):
        """
            Shortcut to create question

            >>> q = DNSRecord.question("www.google.com")
            >>> print(q)
            ;; ->>HEADER<<- opcode: QUERY, status: NOERROR, id: ...
            ;; flags: rd; QUERY: 1, ANSWER: 0, AUTHORITY: 0, ADDITIONAL: 0
            ;; QUESTION SECTION:
            ;www.google.com.                IN      A

            >>> q = DNSRecord.question("www.google.com","NS")
            >>> print(q)
            ;; ->>HEADER<<- opcode: QUERY, status: NOERROR, id: ...
            ;; flags: rd; QUERY: 1, ANSWER: 0, AUTHORITY: 0, ADDITIONAL: 0
            ;; QUESTION SECTION:
            ;www.google.com.                IN      NS
        """
        return DNSRecord(q=DNSQuestion(qname,getattr(QTYPE,qtype),
                                             getattr(CLASS,qclass)))


    def __init__(self,header=None,questions=None,
                      rr=None,q=None,a=None,auth=None,ar=None):
        """
            Create new DNSRecord
        """
        self.header = header or DNSHeader()
        self.questions = questions or []
        self.rr = rr or []
        self.auth = auth or []
        self.ar = ar or []
        # Shortcuts to add a single Question/Answer
        if q:
            self.questions.append(q)
        if a:
            self.rr.append(a)
        self.set_header_qa()

    def reply(self,ra=1,aa=1):
        """
            Create skeleton reply packet

            >>> q = DNSRecord.question("abc.com")
            >>> a = q.reply()
            >>> a.add_answer(RR("abc.com",QTYPE.A,rdata=A("1.2.3.4"),ttl=60))
            >>> print(a)
            ;; ->>HEADER<<- opcode: QUERY, status: NOERROR, id: ...
            ;; flags: qr aa rd ra; QUERY: 1, ANSWER: 1, AUTHORITY: 0, ADDITIONAL: 0
            ;; QUESTION SECTION:
            ;abc.com.                       IN      A
            ;; ANSWER SECTION:
            abc.com.                60      IN      A       1.2.3.4
        """
        return DNSRecord(DNSHeader(id=self.header.id,
                                   bitmap=self.header.bitmap,
                                   qr=1,ra=ra,aa=aa),
                         q=self.q)

    def replyZone(self,zone,ra=1,aa=1):
        """
            Create reply with response data in zone-file format
            >>> q = DNSRecord.question("abc.com")
            >>> a = q.replyZone("abc.com 60 A 1.2.3.4")
            >>> print(a)
            ;; ->>HEADER<<- opcode: QUERY, status: NOERROR, id: ...
            ;; flags: qr aa rd ra; QUERY: 1, ANSWER: 1, AUTHORITY: 0, ADDITIONAL: 0
            ;; QUESTION SECTION:
            ;abc.com.                       IN      A
            ;; ANSWER SECTION:
            abc.com.                60      IN      A       1.2.3.4
        """
        return DNSRecord(DNSHeader(id=self.header.id,
                                   bitmap=self.header.bitmap,
                                   qr=1,ra=ra,aa=aa),
                         q=self.q,
                         rr=RR.fromZone(zone))

    def add_question(self,*q):
        """
            Add question(s)

            >>> q = DNSRecord()
            >>> q.add_question(DNSQuestion("abc.com"),
            ...                DNSQuestion("abc.com",QTYPE.MX))
            >>> print(q)
            ;; ->>HEADER<<- opcode: QUERY, status: NOERROR, id: ...
            ;; flags: rd; QUERY: 2, ANSWER: 0, AUTHORITY: 0, ADDITIONAL: 0
            ;; QUESTION SECTION:
            ;abc.com.                       IN      A
            ;abc.com.                       IN      MX
        """
        self.questions.extend(q)
        self.set_header_qa()

    def add_answer(self,*rr):
        """
            Add answer(s)

            >>> q = DNSRecord.question("abc.com")
            >>> a = q.reply()
            >>> a.add_answer(*RR.fromZone("abc.com A 1.2.3.4"))
            >>> print(a)
            ;; ->>HEADER<<- opcode: QUERY, status: NOERROR, id: ...
            ;; flags: qr aa rd ra; QUERY: 1, ANSWER: 1, AUTHORITY: 0, ADDITIONAL: 0
            ;; QUESTION SECTION:
            ;abc.com.                       IN      A
            ;; ANSWER SECTION:
            abc.com.                0       IN      A       1.2.3.4
        """
        self.rr.extend(rr)
        self.set_header_qa()

    def add_auth(self,*auth):
        """
            Add authority records

            >>> q = DNSRecord.question("abc.com")
            >>> a = q.reply()
            >>> a.add_answer(*RR.fromZone("abc.com 60 A 1.2.3.4"))
            >>> a.add_auth(*RR.fromZone("abc.com 3600 NS nsa.abc.com"))
            >>> print(a)
            ;; ->>HEADER<<- opcode: QUERY, status: NOERROR, id: ...
            ;; flags: qr aa rd ra; QUERY: 1, ANSWER: 1, AUTHORITY: 1, ADDITIONAL: 0
            ;; QUESTION SECTION:
            ;abc.com.                       IN      A
            ;; ANSWER SECTION:
            abc.com.                60      IN      A       1.2.3.4
            ;; AUTHORITY SECTION:
            abc.com.                3600    IN      NS      nsa.abc.com.
        """
        self.auth.extend(auth)
        self.set_header_qa()

    def add_ar(self,*ar):
        """
            Add additional records

            >>> q = DNSRecord.question("abc.com")
            >>> a = q.reply()
            >>> a.add_answer(*RR.fromZone("abc.com 60 CNAME x.abc.com"))
            >>> a.add_ar(*RR.fromZone("x.abc.com 3600 A 1.2.3.4"))
            >>> print(a)
            ;; ->>HEADER<<- opcode: QUERY, status: NOERROR, id: ...
            ;; flags: qr aa rd ra; QUERY: 1, ANSWER: 1, AUTHORITY: 0, ADDITIONAL: 1
            ;; QUESTION SECTION:
            ;abc.com.                       IN      A
            ;; ANSWER SECTION:
            abc.com.                60      IN      CNAME   x.abc.com.
            ;; ADDITIONAL SECTION:
            x.abc.com.              3600    IN      A       1.2.3.4
        """
        self.ar.extend(ar)
        self.set_header_qa()

    def set_header_qa(self):
        """
            Reset header q/a/auth/ar counts to match numver of records
            (normally done transparently)
        """
        self.header.q = len(self.questions)
        self.header.a = len(self.rr)
        self.header.auth = len(self.auth)
        self.header.ar = len(self.ar)

    # Shortcut to get first question
    def get_q(self):
        return self.questions[0] if self.questions else DNSQuestion()
    q = property(get_q)

    # Shortcut to get first answer
    def get_a(self):
        return self.rr[0] if self.rr else RR()
    a = property(get_a)

    def pack(self):
        """
            Pack record into binary packet
            (recursively packs each section into buffer)

            >>> q = DNSRecord.question("abc.com")
            >>> q.header.id = 1234
            >>> a = q.replyZone("abc.com A 1.2.3.4")
            >>> a.header.aa = 0
            >>> pkt = a.pack()
            >>> print(DNSRecord.parse(pkt))
            ;; ->>HEADER<<- opcode: QUERY, status: NOERROR, id: 1234
            ;; flags: qr rd ra; QUERY: 1, ANSWER: 1, AUTHORITY: 0, ADDITIONAL: 0
            ;; QUESTION SECTION:
            ;abc.com.                       IN      A
            ;; ANSWER SECTION:
            abc.com.                0       IN      A       1.2.3.4
        """
        self.set_header_qa()
        buffer = DNSBuffer()
        self.header.pack(buffer)
        for q in self.questions:
            q.pack(buffer)
        for rr in self.rr:
            rr.pack(buffer)
        for auth in self.auth:
            auth.pack(buffer)
        for ar in self.ar:
            ar.pack(buffer)
        return buffer.data

    def truncate(self):
        """
            Return truncated copy of DNSRecord (with TC flag set)
            (removes all Questions & RRs and just returns header)

            >>> q = DNSRecord.question("abc.com")
            >>> a = q.reply()
            >>> a.add_answer(*RR.fromZone('abc.com IN TXT %s' % ('x' * 255)))
            >>> a.add_answer(*RR.fromZone('abc.com IN TXT %s' % ('x' * 255)))
            >>> a.add_answer(*RR.fromZone('abc.com IN TXT %s' % ('x' * 255)))
            >>> len(a.pack())
            829
            >>> t = a.truncate()
            >>> print(t)
            ;; ->>HEADER<<- opcode: QUERY, status: NOERROR, id: ...
            ;; flags: qr aa tc rd ra; QUERY: 0, ANSWER: 0, AUTHORITY: 0, ADDITIONAL: 0

        """
        return DNSRecord(DNSHeader(id=self.header.id,
                                   bitmap=self.header.bitmap,
                                   tc=1))

    def send(self,dest,port=53,tcp=False,timeout=None):
        """
            Send packet to nameserver and return response
        """
        data = self.pack()
        if tcp:
            if len(data) > 65535:
                raise ValueError("Packet length too long: %d" % len(data))
            data = struct.pack("!H",len(data)) + data
            sock = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
            if timeout is not None:
                sock.settimeout(timeout)
            sock.connect((dest,port))
            sock.sendall(data)
            response = sock.recv(8192)
            length = struct.unpack("!H",bytes(response[:2]))[0]
            while len(response) - 2 < length:
                response += sock.recv(8192)
            sock.close()
            response = response[2:]
        else:
            sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
            if timeout is not None:
                sock.settimeout(timeout)
            sock.sendto(self.pack(),(dest,port))
            response,server = sock.recvfrom(8192)
            sock.close()
        return response

    def format(self,prefix="",sort=False):
        """
            Formatted 'repr'-style representation of record
            (optionally with prefix and/or sorted RRs)
        """
        s = sorted if sort else lambda x:x
        sections = [ repr(self.header) ]
        sections.extend(s([repr(q) for q in self.questions]))
        sections.extend(s([repr(rr) for rr in self.rr]))
        sections.extend(s([repr(rr) for rr in self.auth]))
        sections.extend(s([repr(rr) for rr in self.ar]))
        return prefix + ("\n" + prefix).join(sections)

    def toZone(self,prefix=""):
        """
            Formatted 'DiG' (zone) style output
            (with optional prefix)
        """
        z = self.header.toZone().split("\n")
        if self.questions:
            z.append(";; QUESTION SECTION:")
            [ z.extend(q.toZone().split("\n")) for q in self.questions ]
        if self.rr:
            z.append(";; ANSWER SECTION:")
            [ z.extend(rr.toZone().split("\n")) for rr in self.rr ]
        if self.auth:
            z.append(";; AUTHORITY SECTION:")
            [ z.extend(rr.toZone().split("\n")) for rr in self.auth ]
        if self.ar:
            z.append(";; ADDITIONAL SECTION:")
            [ z.extend(rr.toZone().split("\n")) for rr in self.ar ]
        return prefix + ("\n" + prefix).join(z)

    def short(self):
        """
            Just return RDATA
        """
        return "\n".join([rr.rdata.toZone() for rr in self.rr])

    def __eq__(self,other):
        """
            Test for equality by diffing records
        """
        if type(other) != type(self):
            return False
        else:
            return self.diff(other) == []

    def __ne__(self,other):
        return not(self.__eq__(other))

    def diff(self,other):
        """
            Diff records - recursively diff sections (sorting RRs)
        """
        err = []
        if self.header != other.header:
            err.append((self.header,other.header))
        for section in ('questions','rr','auth','ar'):
            if section == 'questions':
                k = lambda x:tuple(map(str,(x.qname,x.qtype)))
            else:
                k = lambda x:tuple(map(str,(x.rname,x.rtype,x.rdata)))
            a = dict([(k(rr),rr) for rr in getattr(self,section)])
            b = dict([(k(rr),rr) for rr in getattr(other,section)])
            sa = set(a)
            sb = set(b)
            for e in sorted(sa.intersection(sb)):
                if a[e] != b[e]:
                    err.append((a[e],b[e]))
            for e in sorted(sa.difference(sb)):
                err.append((a[e],None))
            for e in sorted(sb.difference(sa)):
                err.append((None,b[e]))
        return err

    def __repr__(self):
        return self.format()

    def __str__(self):
        return self.toZone()

class DNSHeader(object):

    """
        DNSHeader section
    """

    # Ensure attribute values match packet
    id = H('id')
    bitmap = H('bitmap')
    q = H('q')
    a = H('a')
    auth = H('auth')
    ar = H('ar')

    @classmethod
    def parse(cls,buffer):
        """
            Implements parse interface
        """
        try:
            (id,bitmap,q,a,auth,ar) = buffer.unpack("!HHHHHH")
            return cls(id,bitmap,q,a,auth,ar)
        except (BufferError,BimapError) as e:
            raise DNSError("Error unpacking DNSHeader [offset=%d]: %s" % (
                                buffer.offset,e))

    def __init__(self,id=None,bitmap=None,q=0,a=0,auth=0,ar=0,**args):
        if id is None:
            self.id = random.randint(0,65535)
        else:
            self.id = id
        if bitmap is None:
            self.bitmap = 0
            self.rd = 1
        else:
            self.bitmap = bitmap
        self.q = q
        self.a = a
        self.auth = auth
        self.ar = ar
        for k,v in args.items():
            if k.lower() == "qr":
                self.qr = v
            elif k.lower() == "opcode":
                self.opcode = v
            elif k.lower() == "aa":
                self.aa = v
            elif k.lower() == "tc":
                self.tc = v
            elif k.lower() == "rd":
                self.rd = v
            elif k.lower() == "ra":
                self.ra = v
            elif k.lower() == "rcode":
                self.rcode = v

    # Accessors for header properties (automatically pack/unpack
    # into bitmap)
    def get_qr(self):
        return get_bits(self.bitmap,15)

    def set_qr(self,val):
        self.bitmap = set_bits(self.bitmap,val,15)

    qr = property(get_qr,set_qr)

    def get_opcode(self):
        return get_bits(self.bitmap,11,4)

    def set_opcode(self,val):
        self.bitmap = set_bits(self.bitmap,val,11,4)

    opcode = property(get_opcode,set_opcode)

    def get_aa(self):
        return get_bits(self.bitmap,10)

    def set_aa(self,val):
        self.bitmap = set_bits(self.bitmap,val,10)

    aa = property(get_aa,set_aa)

    def get_tc(self):
        return get_bits(self.bitmap,9)

    def set_tc(self,val):
        self.bitmap = set_bits(self.bitmap,val,9)

    tc = property(get_tc,set_tc)

    def get_rd(self):
        return get_bits(self.bitmap,8)

    def set_rd(self,val):
        self.bitmap = set_bits(self.bitmap,val,8)

    rd = property(get_rd,set_rd)

    def get_ra(self):
        return get_bits(self.bitmap,7)

    def set_ra(self,val):
        self.bitmap = set_bits(self.bitmap,val,7)

    ra = property(get_ra,set_ra)

    def get_rcode(self):
        return get_bits(self.bitmap,0,4)

    def set_rcode(self,val):
        self.bitmap = set_bits(self.bitmap,val,0,4)

    rcode = property(get_rcode,set_rcode)

    def pack(self,buffer):
        buffer.pack("!HHHHHH",self.id,self.bitmap,
                              self.q,self.a,self.auth,self.ar)

    def __repr__(self):
        f = [ self.aa and 'AA',
              self.tc and 'TC',
              self.rd and 'RD',
              self.ra and 'RA' ]
        if OPCODE.get(self.opcode) == 'UPDATE':
            f1='zo'
            f2='pr'
            f3='up'
            f4='ad'
        else:
            f1='q'
            f2='a'
            f3='ns'
            f4='ar'
        return "<DNS Header: id=0x%x type=%s opcode=%s flags=%s " \
                            "rcode='%s' %s=%d %s=%d %s=%d %s=%d>" % (
                    self.id,
                    QR.get(self.qr),
                    OPCODE.get(self.opcode),
                    ",".join(filter(None,f)),
                    RCODE.get(self.rcode),
                    f1, self.q, f2, self.a, f3, self.auth, f4, self.ar )

    def toZone(self):
        f = [ self.qr and 'qr',
              self.aa and 'aa',
              self.tc and 'tc',
              self.rd and 'rd',
              self.ra and 'ra' ]
        z1 = ';; ->>HEADER<<- opcode: %s, status: %s, id: %d' % (
                    OPCODE.get(self.opcode),RCODE.get(self.rcode),self.id)
        z2 = ';; flags: %s; QUERY: %d, ANSWER: %d, AUTHORITY: %d, ADDITIONAL: %d' % (
                      " ".join(filter(None,f)),
                      self.q,self.a,self.auth,self.ar)
        return z1 + "\n" + z2

    def __str__(self):
        return self.toZone()

    def __ne__(self,other):
        return not(self.__eq__(other))

    def __eq__(self,other):
        if type(other) != type(self):
            return False
        else:
            # Ignore id
            attrs = ('qr','aa','tc','rd','ra','opcode','rcode')
            return all([getattr(self,x) == getattr(other,x) for x in attrs])

class DNSQuestion(object):

    """
        DNSQuestion section
    """

    @classmethod
    def parse(cls,buffer):
        try:
            qname = buffer.decode_name()
            qtype,qclass = buffer.unpack("!HH")
            return cls(qname,qtype,qclass)
        except (BufferError,BimapError) as e:
            raise DNSError("Error unpacking DNSQuestion [offset=%d]: %s" % (
                                buffer.offset,e))

    def __init__(self,qname=None,qtype=1,qclass=1):
        self.qname = qname
        self.qtype = qtype
        self.qclass = qclass

    def set_qname(self,qname):
        if isinstance(qname,DNSLabel):
            self._qname = qname
        else:
            self._qname = DNSLabel(qname)

    def get_qname(self):
        return self._qname

    qname = property(get_qname,set_qname)

    def pack(self,buffer):
        buffer.encode_name(self.qname)
        buffer.pack("!HH",self.qtype,self.qclass)

    def toZone(self):
       return ';%-30s %-7s %s' % (self.qname,CLASS.get(self.qclass),
                                             QTYPE.get(self.qtype))

    def __repr__(self):
        return "<DNS Question: '%s' qtype=%s qclass=%s>" % (
                    self.qname, QTYPE.get(self.qtype), CLASS.get(self.qclass))

    def __str__(self):
        return self.toZone()

    def __ne__(self,other):
        return not(self.__eq__(other))

    def __eq__(self,other):
        if type(other) != type(self):
            return False
        else:
            # List of attributes to compare when diffing
            attrs = ('qname','qtype','qclass')
            return all([getattr(self,x) == getattr(other,x) for x in attrs])

class EDNSOption(object):

    """
        EDNSOption pseudo-section

        Very rudimentary support for EDNS0 options however this has not been
        tested due to a lack of data (anyone wanting to improve support or
        provide test data please raise an issue)

    """
    def __init__(self,code,data):
        self.code = code
        self.data = data

    def pack(self,buffer):
        buffer.pack("!HH",self.code,len(self.data))
        buffer.append(self.data)

    def __repr__(self):
        return "<EDNS Option: Code=%d Data='%s'>" % (
                    self.code,binascii.hexlify(self.data).decode())

    def toZone(self):
        return ";EDNS: code: %s; data: %s" % (
                    self.code,binascii.hexlify(self.data).decode())

    def __str__(self):
        return self.toZone()

    def __ne__(self,other):
        return not(self.__eq__(other))

    def __eq__(self,other):
        if type(other) != type(self):
            return False
        else:
            # List of attributes to compare when diffing
            attrs = ('code','data')
            return all([getattr(self,x) == getattr(other,x) for x in attrs])

class RR(object):

    """
        DNS Resource Record
        Contains RR header and RD (resource data) instance
    """

    rtype = H('rtype')
    rclass = H('rclass')
    ttl = I('ttl')
    rdlength = H('rdlength')

    @classmethod
    def parse(cls,buffer):
        try:
            rname = buffer.decode_name()
            rtype,rclass,ttl,rdlength = buffer.unpack("!HHIH")
            if rtype == QTYPE.OPT:
                options = []
                option_buffer = Buffer(buffer.get(rdlength))
                while option_buffer.remaining() > 4:
                    code,length = option_buffer.unpack("!HH")
                    data = option_buffer.get(length)
                    options.append(EDNSOption(code,data))
                rdata = options
            else:
                if rdlength:
                    rdata = RDMAP.get(QTYPE.get(rtype),RD).parse(
                                            buffer,rdlength)
                else:
                    rdata = ''
            return cls(rname,rtype,rclass,ttl,rdata)
        except (BufferError,BimapError) as e:
            raise DNSError("Error unpacking RR [offset=%d]: %s" % (
                                buffer.offset,e))

    @classmethod
    def fromZone(cls,zone,origin="",ttl=0):
        """
            Parse RR data from zone file and return list of RRs
        """
        return list(ZoneParser(zone,origin=origin,ttl=ttl))

    def __init__(self,rname=None,rtype=1,rclass=1,ttl=0,rdata=None):
        self.rname = rname
        self.rtype = rtype
        self.rclass = rclass
        self.ttl = ttl
        self.rdata = rdata
        # TODO Add property getters/setters
        if self.rtype == QTYPE.OPT:
            self.edns_len = self.rclass
            self.edns_do = get_bits(self.ttl,15)
            self.edns_ver = get_bits(self.ttl,16,8)
            self.edns_rcode = get_bits(self.ttl,24,8)

    def set_rname(self,rname):
        if isinstance(rname,DNSLabel):
            self._rname = rname
        else:
            self._rname = DNSLabel(rname)

    def get_rname(self):
        return self._rname

    rname = property(get_rname,set_rname)

    def pack(self,buffer):
        buffer.encode_name(self.rname)
        buffer.pack("!HHI",self.rtype,self.rclass,self.ttl)
        rdlength_ptr = buffer.offset
        buffer.pack("!H",0)
        start = buffer.offset
        if self.rtype == QTYPE.OPT:
            for opt in self.rdata:
                opt.pack(buffer)
        else:
            self.rdata.pack(buffer)
        end = buffer.offset
        buffer.update(rdlength_ptr,"!H",end-start)

    def __repr__(self):
        if self.rtype == QTYPE.OPT:
            s = ["<DNS OPT: edns_ver=%d do=%d ext_rcode=%d udp_len=%d>" % (
                        self.edns_ver,self.edns_do,self.edns_rcode,self.edns_len)]
            s.extend([repr(opt) for opt in self.rdata])
            return "\n".join(s)
        else:
            return "<DNS RR: '%s' rtype=%s rclass=%s ttl=%d rdata='%s'>" % (
                    self.rname, QTYPE.get(self.rtype), CLASS.get(self.rclass),
                    self.ttl, self.rdata)

    def toZone(self):
        if self.rtype == QTYPE.OPT:
            edns = [ ";OPT PSEUDOSECTION",
                     ";EDNS: version: %d, flags: %s; udp: %d" % (
                             self.edns_ver,
                             "do" if self.edns_do else "",
                             self.edns_len)
                    ]
            edns.extend([str(opt) for opt in self.rdata])
            return "\n".join(edns)
        else:
            return '%-23s %-7s %-7s %-7s %s' % (self.rname,self.ttl,
                                                CLASS.get(self.rclass),
                                                QTYPE.get(self.rtype),
                                                self.rdata.toZone())

    def __str__(self):
        return self.toZone()

    def __ne__(self,other):
        return not(self.__eq__(other))

    def __eq__(self,other):
        if type(other) != type(self):
            return False
        else:
            # List of attributes to compare when diffing (ignore ttl)
            attrs = ('rname','rclass','rtype','rdata')
            return all([getattr(self,x) == getattr(other,x) for x in attrs])


class RD(object):
    """
        Base RD object - also used as placeholder for unknown RD types

        To create a new RD type subclass this and add to RDMAP (below)

        Subclass should implement (as a mininum):

            parse (parse from packet data)
            __init__ (create class)
            __repr__ (return in zone format)
            fromZone (create from zone format)

            (toZone uses __repr__ by default)

        Unknown rdata types default to RD and store rdata as a binary
        blob (this allows round-trip encoding/decoding)
    """

    @classmethod
    def parse(cls,buffer,length):
        """
            Unpack from buffer
        """
        try:
            data = buffer.get(length)
            return cls(data)
        except (BufferError,BimapError) as e:
            raise DNSError("Error unpacking RD [offset=%d]: %s" %
                                    (buffer.offset,e))

    @classmethod
    def fromZone(cls,rd,origin=None):
        """
            Create new record from zone format data
            RD is a list of strings parsed from DiG output
        """
        # Unknown rata - assume hexdump in zone format
        # (DiG prepends "\\# <len>" to the hexdump so get last item)
        return cls(binascii.unhexlify(rd[-1].encode('ascii')))

    def __init__(self,data=b""):
        # Assume raw bytes
        self.data = bytes(data)

    def pack(self,buffer):
        """
            Pack record into buffer
        """
        buffer.append(self.data)

    def __repr__(self):
        """
            Default 'repr' format should be equivalent to RD zone format
        """
        # For unknown rdata just default to hex
        return binascii.hexlify(self.data).decode()

    def toZone(self):
        return repr(self)

    # Comparison operations - in most cases only need to override 'attrs'
    # in subclass (__eq__ will automatically compare defined atttrs)

    # Attributes for comparison
    attrs = ('data',)

    def __eq__(self,other):
        if type(other) != type(self):
            return False
        else:
            return all([getattr(self,x) == getattr(other,x) for x in self.attrs])

    def __ne__(self,other):
        return not(self.__eq__(other))

class TXT(RD):

    @classmethod
    def parse(cls,buffer,length):
        try:
            (txtlength,) = buffer.unpack("!B")
            # First byte is TXT length (not in RFC?)
            if txtlength < length:
                data = buffer.get(txtlength)
            else:
                raise DNSError("Invalid TXT record: len(%d) > RD len(%d)" %
                                        (txtlength,length))
            return cls(data)
        except (BufferError,BimapError) as e:
            raise DNSError("Error unpacking TXT [offset=%d]: %s" %
                                        (buffer.offset,e))

    @classmethod
    def fromZone(cls,rd,origin=None):
        return cls(rd[0].encode())

    def pack(self,buffer):
        if len(self.data) > 255:
            raise DNSError("TXT record too long: %s" % self.data)
        buffer.pack("!B",len(self.data))
        buffer.append(self.data)

    def toZone(self):
        return '"%s"' % repr(self)

    def __repr__(self):
        # Pyyhon 2/3 hack
        # FIXME UnicodeDecodeError: 'utf-8' codec can't decode byte 0xfc in position 1
        return self.data if isinstance(self.data,str) else self.data.decode(errors='replace')

class A(RD):

    data = IP4('data')

    @classmethod
    def parse(cls,buffer,length):
        try:
            data = buffer.unpack("!BBBB")
            return cls(data)
        except (BufferError,BimapError) as e:
            raise DNSError("Error unpacking A [offset=%d]: %s" %
                                (buffer.offset,e))

    @classmethod
    def fromZone(cls,rd,origin=None):
        return cls(rd[0])

    def __init__(self,data):
        if type(data) in (tuple,list):
            self.data = tuple(data)
        else:
            self.data = tuple(map(int,data.rstrip(".").split(".")))

    def pack(self,buffer):
        buffer.pack("!BBBB",*self.data)

    def __repr__(self):
        return "%d.%d.%d.%d" % self.data

def _parse_ipv6(a):
    """
        Parse IPv6 address. Ideally we would use the ipaddress module in
        Python3.3 but can't rely on having this.

        Does not handle dotted-quad addresses or subnet prefix

        >>> _parse_ipv6("::") == (0,) * 16
        True
        >>> _parse_ipv6("1234:5678::abcd:0:ff00")
        (18, 52, 86, 120, 0, 0, 0, 0, 0, 0, 171, 205, 0, 0, 255, 0)

    """
    l,_,r = a.partition("::")
    l_groups = list(chain(*[divmod(int(x,16),256) for x in l.split(":") if x]))
    r_groups = list(chain(*[divmod(int(x,16),256) for x in r.split(":") if x]))
    zeros = [0] * (16 - len(l_groups) - len(r_groups))
    return tuple(l_groups + zeros + r_groups)

def _format_ipv6(a):
    """
        Format IPv6 address (from tuple of 16 bytes) compressing sequence of
        zero bytes to '::'. Ideally we would use the ipaddress module in
        Python3.3 but can't rely on having this.

        >>> _format_ipv6([0]*16)
        '::'
        >>> _format_ipv6(_parse_ipv6("::0012:5678"))
        '::12:5678'
        >>> _format_ipv6(_parse_ipv6("1234:0:5678::ff:0:1"))
        '1234:0:5678::ff:0:1'
    """
    left = []
    right = []
    current = 'left'
    for i in range(0,16,2):
        group = (a[i] << 8) + a[i+1]
        if current == 'left':
            if group == 0 and i < 14:
                if (a[i+2] << 8) + a[i+3] == 0:
                    current = 'right'
                else:
                    left.append("0")
            else:
                left.append("%x" % group)
        else:
            if group == 0 and len(right) == 0:
                pass
            else:
                right.append("%x" % group)
    if len(left) < 8:
        return ":".join(left) + "::" + ":".join(right)
    else:
        return ":".join(left)

class AAAA(RD):

    """
        Basic support for AAAA record - accepts IPv6 address data as either
        a tuple of 16 bytes or in text format
    """

    data = IP6('data')

    @classmethod
    def parse(cls,buffer,length):
        try:
            data = buffer.unpack("!16B")
            return cls(data)
        except (BufferError,BimapError) as e:
            raise DNSError("Error unpacking AAAA [offset=%d]: %s" %
                                        (buffer.offset,e))

    @classmethod
    def fromZone(cls,rd,origin=None):
        return cls(rd[0])

    def __init__(self,data):
        if type(data) in (tuple,list):
            self.data = tuple(data)
        else:
            self.data = _parse_ipv6(data)

    def pack(self,buffer):
        buffer.pack("!16B",*self.data)

    def __repr__(self):
        return _format_ipv6(self.data)

class MX(RD):

    preference = H('preference')

    @classmethod
    def parse(cls,buffer,length):
        try:
            (preference,) = buffer.unpack("!H")
            mx = buffer.decode_name()
            return cls(mx,preference)
        except (BufferError,BimapError) as e:
            raise DNSError("Error unpacking MX [offset=%d]: %s" %
                                        (buffer.offset,e))

    @classmethod
    def fromZone(cls,rd,origin=None):
        return cls(label(rd[1],origin),int(rd[0]))

    def __init__(self,label=None,preference=10):
        self.label = label
        self.preference = preference

    def set_label(self,label):
        if isinstance(label,DNSLabel):
            self._label = label
        else:
            self._label = DNSLabel(label)

    def get_label(self):
        return self._label

    label = property(get_label,set_label)

    def pack(self,buffer):
        buffer.pack("!H",self.preference)
        buffer.encode_name(self.label)

    def __repr__(self):
        return "%d %s" % (self.preference,self.label)

    attrs = ('preference','label')

class CNAME(RD):

    @classmethod
    def parse(cls,buffer,length):
        try:
            label = buffer.decode_name()
            return cls(label)
        except (BufferError,BimapError) as e:
            raise DNSError("Error unpacking CNAME [offset=%d]: %s" %
                                        (buffer.offset,e))

    @classmethod
    def fromZone(cls,rd,origin=None):
        return cls(label(rd[0],origin))

    def __init__(self,label=None):
        self.label = label

    def set_label(self,label):
        if isinstance(label,DNSLabel):
            self._label = label
        else:
            self._label = DNSLabel(label)

    def get_label(self):
        return self._label

    label = property(get_label,set_label)

    def pack(self,buffer):
        buffer.encode_name(self.label)

    def __repr__(self):
        return "%s" % (self.label)

    attrs = ('label',)

class PTR(CNAME):
    pass

class NS(CNAME):
    pass

class SOA(RD):

    times = ntuple_range('times',5,0,4294967295)
    @classmethod
    def parse(cls,buffer,length):
        try:
            mname = buffer.decode_name()
            rname = buffer.decode_name()
            times = buffer.unpack("!IIIII")
            return cls(mname,rname,times)
        except (BufferError,BimapError) as e:
            raise DNSError("Error unpacking SOA [offset=%d]: %s" %
                                        (buffer.offset,e))

    @classmethod
    def fromZone(cls,rd,origin=None):
        return cls(label(rd[0],origin),label(rd[1],origin),[int(t) for t in rd[2:]])

    def __init__(self,mname=None,rname=None,times=None):
        self.mname = mname
        self.rname = rname
        self.times = tuple(times) if times else (0,0,0,0,0)

    def set_mname(self,mname):
        if isinstance(mname,DNSLabel):
            self._mname = mname
        else:
            self._mname = DNSLabel(mname)

    def get_mname(self):
        return self._mname

    mname = property(get_mname,set_mname)

    def set_rname(self,rname):
        if isinstance(rname,DNSLabel):
            self._rname = rname
        else:
            self._rname = DNSLabel(rname)

    def get_rname(self):
        return self._rname

    rname = property(get_rname,set_rname)

    def pack(self,buffer):
        buffer.encode_name(self.mname)
        buffer.encode_name(self.rname)
        buffer.pack("!IIIII", *self.times)

    def __repr__(self):
        return "%s %s %s" % (self.mname,self.rname,
                             " ".join(map(str,self.times)))

    attrs = ('mname','rname','times')

class SRV(RD):

    priority = H('priority')
    weight = H('weight')
    port = H('port')

    @classmethod
    def parse(cls,buffer,length):
        try:
            priority,weight,port = buffer.unpack("!HHH")
            target = buffer.decode_name()
            return cls(priority,weight,port,target)
        except (BufferError,BimapError) as e:
            raise DNSError("Error unpacking SRV [offset=%d]: %s" %
                                        (buffer.offset,e))

    @classmethod
    def fromZone(cls,rd,origin=None):
        return cls(int(rd[0]),int(rd[1]),int(rd[2]),rd[3])

    def __init__(self,priority=0,weight=0,port=0,target=None):
        self.priority = priority
        self.weight = weight
        self.port = port
        self.target = target

    def set_target(self,target):
        if isinstance(target,DNSLabel):
            self._target = target
        else:
            self._target = DNSLabel(target)

    def get_target(self):
        return self._target

    target = property(get_target,set_target)

    def pack(self,buffer):
        buffer.pack("!HHH",self.priority,self.weight,self.port)
        buffer.encode_name(self.target)

    def __repr__(self):
        return "%d %d %d %s" % (self.priority,self.weight,self.port,self.target)

    attrs = ('priority','weight','port','target')

class NAPTR(RD):

    order = H('order')
    preference = H('preference')

    @classmethod
    def parse(cls, buffer, length):
        try:
            order, preference = buffer.unpack('!HH')
            (length,) = buffer.unpack('!B')
            flags = buffer.get(length)
            (length,) = buffer.unpack('!B')
            service = buffer.get(length)
            (length,) = buffer.unpack('!B')
            regexp = buffer.get(length)
            replacement = buffer.decode_name()
            return cls(order, preference, flags, service, regexp, replacement)
        except (BufferError,BimapError) as e:
            raise DNSError("Error unpacking NAPTR [offset=%d]: %s" %
                                    (buffer.offset,e))

    @classmethod
    def fromZone(cls,rd,origin=None):
        encode = lambda s : s.encode()
        _label = lambda s : label(s,origin)
        m = (int,int,encode,encode,encode,_label)
        return cls(*[ f(v) for f,v in zip(m,rd)])

    def __init__(self,order,preference,flags,service,regexp,replacement=None):
        self.order = order
        self.preference = preference
        self.flags = flags
        self.service = service
        self.regexp = regexp
        self.replacement = replacement

    def set_replacement(self,replacement):
        if isinstance(replacement,DNSLabel):
            self._replacement = replacement
        else:
            self._replacement = DNSLabel(replacement)

    def get_replacement(self):
        return self._replacement

    replacement = property(get_replacement,set_replacement)

    def pack(self, buffer):
        buffer.pack('!HH', self.order, self.preference)
        buffer.pack('!B', len(self.flags))
        buffer.append(self.flags)
        buffer.pack('!B', len(self.service))
        buffer.append(self.service)
        buffer.pack('!B', len(self.regexp))
        buffer.append(self.regexp)
        buffer.encode_name(self.replacement)

    def __repr__(self):
        return '%d %d "%s" "%s" "%s" %s' %(
            self.order,self.preference,self.flags.decode(),
            self.service.decode(),
            self.regexp.decode().replace('\\','\\\\'),
            self.replacement or '.'
        )

    attrs = ('order','preference','flags','service','regexp','replacement')

class DNSKEY(RD):

    flags = H('flags')
    protocol = B('protocol')
    algorithm = B('algorithm')

    @classmethod
    def parse(cls,buffer,length):
        try:
            (flags,protocol,algorithm) = buffer.unpack("!HBB")
            key = buffer.get(length - 4)
            return cls(flags,protocol,algorithm,key)
        except (BufferError,BimapError) as e:
            raise DNSError("Error unpacking DNSKEY [offset=%d]: %s" %
                                        (buffer.offset,e))

    @classmethod
    def fromZone(cls,rd,origin=None):
        return cls(int(rd[0]),int(rd[1]),int(rd[2]),
                   base64.b64decode(("".join(rd[3:])).encode('ascii')))

    def __init__(self,flags,protocol,algorithm,key):
        self.flags = flags
        self.protocol = protocol
        self.algorithm = algorithm
        self.key = key

    def pack(self,buffer):
        buffer.pack("!HBB",self.flags,self.protocol,self.algorithm)
        buffer.append(self.key)

    def __repr__(self):
        return "%d %d %d %s" % (self.flags,self.protocol,self.algorithm,
                                base64.b64encode(self.key).decode())

    attrs = ('flags','protocol','algorithm','key')

class RRSIG(RD):

    covered = H('covered')
    algorithm = B('algorithm')
    labels = B('labels')
    orig_ttl = I('orig_ttl')
    sig_exp = I('sig_exp')
    sig_inc = I('sig_inc')
    key_tag = H('key_tag')

    @classmethod
    def parse(cls,buffer,length):
        try:
            start = buffer.offset
            (covered,algorithm,labels,
                orig_ttl,sig_exp,sig_inc,key_tag) = buffer.unpack("!HBBIIIH")
            name = buffer.decode_name()
            sig = buffer.get(length - (buffer.offset - start))
            return cls(covered,algorithm,labels,orig_ttl,sig_exp,sig_inc,key_tag,
                            name,sig)
        except (BufferError,BimapError) as e:
            raise DNSError("Error unpacking DNSKEY [offset=%d]: %s" %
                                        (buffer.offset,e))

    @classmethod
    def fromZone(cls,rd,origin=None):
        return cls(getattr(QTYPE,rd[0]),int(rd[1]),int(rd[2]),int(rd[3]),
                        int(time.mktime(time.strptime(rd[4]+'GMT',"%Y%m%d%H%M%S%Z"))),
                        int(time.mktime(time.strptime(rd[5]+'GMT',"%Y%m%d%H%M%S%Z"))),
                        int(rd[6]),rd[7],
                        base64.b64decode(("".join(rd[8:])).encode('ascii')))

    def __init__(self,covered,algorithm,labels,orig_ttl,
                      sig_exp,sig_inc,key_tag,name,sig):
        self.covered = covered
        self.algorithm = algorithm
        self.labels = labels
        self.orig_ttl = orig_ttl
        self.sig_exp = sig_exp
        self.sig_inc = sig_inc
        self.key_tag = key_tag
        self.name = DNSLabel(name)
        self.sig = sig

    def pack(self,buffer):
        buffer.pack("!HBBIIIH",self.covered,self.algorithm,self.labels,
                               self.orig_ttl,self.sig_exp,self.sig_inc,
                               self.key_tag)
        buffer.encode_name_nocompress(self.name)
        buffer.append(self.sig)

    def __repr__(self):
        return "%s %d %d %d %s %s %d %s %s" % (
                        QTYPE.get(self.covered),
                        self.algorithm,
                        self.labels,
                        self.orig_ttl,
                        time.strftime("%Y%m%d%H%M%S",time.gmtime(self.sig_exp)),
                        time.strftime("%Y%m%d%H%M%S",time.gmtime(self.sig_inc)),
                        self.key_tag,
                        self.name,
                        base64.b64encode(self.sig).decode())

    attrs = ('covered','algorithm','labels','orig_ttl','sig_exp','sig_inc',
             'key_tag','name','sig')

# Map from RD type to class (used to pack/unpack records)
# If you add a new RD class you must add to RDMAP

RDMAP = { 'CNAME':CNAME, 'A':A, 'AAAA':AAAA, 'TXT':TXT, 'MX':MX,
          'PTR':PTR, 'SOA':SOA, 'NS':NS, 'NAPTR': NAPTR, 'SRV':SRV,
          'DNSKEY':DNSKEY, 'RRSIG':RRSIG,
        }

##
## Zone parser
## TODO  - ideally this would be in a separate file but have to deal
##         with circular dependencies
##

secs = {'s':1,'m':60,'h':3600,'d':86400,'w':604800}

def parse_time(s):
    """
        Parse time spec with optional s/m/h/d/w suffix
    """
    if s[-1].lower() in secs:
        return int(s[:-1]) * secs[s[-1].lower()]
    else:
        return int(s)

class ZoneParser:

    """
        Zone file parser

        >>> z = ZoneParser("www.example.com. 60 IN A 1.2.3.4")
        >>> list(z.parse())
        [<DNS RR: 'www.example.com.' rtype=A rclass=IN ttl=60 rdata='1.2.3.4'>]
    """

    def __init__(self,zone,origin="",ttl=0):
        self.l = WordLexer(zone)
        self.l.commentchars = ';'
        self.l.nltok = ('NL',None)
        self.l.spacetok = ('SPACE',None)
        self.i = iter(self.l)
        if type(origin) is DNSLabel:
            self.origin = origin
        else:
            self.origin= DNSLabel(origin)
        self.ttl = ttl
        self.label = DNSLabel("")
        self.prev = None

    def expect(self,expect):
        t,val = next(self.i)
        if t != expect:
            raise ValueError("Invalid Token: %s (expecting: %s)" % (t,expect))
        return val

    def parse_label(self,label):
        if label.endswith("."):
            self.label = DNSLabel(label)
        elif label == "@":
            self.label = self.origin
        elif label == '':
            pass
        else:
            self.label = self.origin.add(label)
        return self.label

    def parse_rr(self,rr):
        label = self.parse_label(rr.pop(0))
        ttl = int(rr.pop(0)) if rr[0].isdigit() else self.ttl
        rclass = rr.pop(0) if rr[0] in ('IN','CH','HS') else 'IN'
        rtype = rr.pop(0)
        rdata = rr
        rd = RDMAP.get(rtype,RD)
        return RR(rname=label,
                         ttl=ttl,
                         rclass=getattr(CLASS,rclass),
                         rtype=getattr(QTYPE,rtype),
                         rdata=rd.fromZone(rdata,self.origin))

    def __iter__(self):
        return self.parse()

    def parse(self):
        rr = []
        paren = False
        try:
            while True:
                tok,val = next(self.i)
                if tok == 'NL':
                    if not paren and rr:
                        self.prev = tok
                        yield self.parse_rr(rr)
                        rr = []
                elif tok == 'SPACE' and self.prev == 'NL' and not paren:
                    rr.append('')
                elif tok == 'ATOM':
                    if val == '(':
                        paren = True
                    elif val == ')':
                        paren = False
                    elif val == '$ORIGIN':
                        self.expect('SPACE')
                        origin = self.expect('ATOM')
                        self.origin = self.label = DNSLabel(origin)
                    elif val == '$TTL':
                        self.expect('SPACE')
                        ttl = self.expect('ATOM')
                        self.ttl = parse_time(ttl)
                    else:
                        rr.append(val)
                self.prev = tok
        except StopIteration:
            if rr:
                yield self.parse_rr(rr)

if __name__ == '__main__':
    import doctest
    doctest.testmod(optionflags=doctest.ELLIPSIS)
