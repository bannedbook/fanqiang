# -*- coding: utf-8 -*-
"""

    digparser
    ---------

    Encode/decode DNS packets from DiG textual representation. Parses
    question (if present: +qr flag) & answer sections and returns list
    of DNSRecord objects.

    Unsupported RR types are skipped (this is different from the packet
    parser which will store and encode the RDATA as a binary blob)

    >>> dig = os.path.join(os.path.dirname(__file__),"test","dig","google.com-A.dig")
    >>> with open(dig) as f:
    ...     l = DigParser(f)
    ...     for record in l:
    ...         print('---')
    ...         print(repr(record))
    ---
    <DNS Header: id=0x5c9a type=QUERY opcode=QUERY flags=RD rcode='NOERROR' q=1 a=0 ns=0 ar=0>
    <DNS Question: 'google.com.' qtype=A qclass=IN>
    ---
    <DNS Header: id=0x5c9a type=RESPONSE opcode=QUERY flags=RD,RA rcode='NOERROR' q=1 a=16 ns=0 ar=0>
    <DNS Question: 'google.com.' qtype=A qclass=IN>
    <DNS RR: 'google.com.' rtype=A rclass=IN ttl=299 rdata='62.252.169.183'>
    <DNS RR: 'google.com.' rtype=A rclass=IN ttl=299 rdata='62.252.169.152'>
    <DNS RR: 'google.com.' rtype=A rclass=IN ttl=299 rdata='62.252.169.172'>
    <DNS RR: 'google.com.' rtype=A rclass=IN ttl=299 rdata='62.252.169.177'>
    <DNS RR: 'google.com.' rtype=A rclass=IN ttl=299 rdata='62.252.169.157'>
    <DNS RR: 'google.com.' rtype=A rclass=IN ttl=299 rdata='62.252.169.153'>
    <DNS RR: 'google.com.' rtype=A rclass=IN ttl=299 rdata='62.252.169.182'>
    <DNS RR: 'google.com.' rtype=A rclass=IN ttl=299 rdata='62.252.169.168'>
    <DNS RR: 'google.com.' rtype=A rclass=IN ttl=299 rdata='62.252.169.178'>
    <DNS RR: 'google.com.' rtype=A rclass=IN ttl=299 rdata='62.252.169.162'>
    <DNS RR: 'google.com.' rtype=A rclass=IN ttl=299 rdata='62.252.169.187'>
    <DNS RR: 'google.com.' rtype=A rclass=IN ttl=299 rdata='62.252.169.167'>
    <DNS RR: 'google.com.' rtype=A rclass=IN ttl=299 rdata='62.252.169.148'>
    <DNS RR: 'google.com.' rtype=A rclass=IN ttl=299 rdata='62.252.169.173'>
    <DNS RR: 'google.com.' rtype=A rclass=IN ttl=299 rdata='62.252.169.158'>
    <DNS RR: 'google.com.' rtype=A rclass=IN ttl=299 rdata='62.252.169.163'>

    >>> dig = os.path.join(os.path.dirname(__file__),"test","dig","google.com-ANY.dig")
    >>> with open(dig) as f:
    ...     l = DigParser(f)
    ...     for record in l:
    ...         print('---')
    ...         print(repr(record))
    ---
    <DNS Header: id=0xfc6b type=QUERY opcode=QUERY flags=RD rcode='NOERROR' q=1 a=0 ns=0 ar=0>
    <DNS Question: 'google.com.' qtype=ANY qclass=IN>
    ---
    <DNS Header: id=0xa6fc type=QUERY opcode=QUERY flags=RD rcode='NOERROR' q=1 a=0 ns=0 ar=0>
    <DNS Question: 'google.com.' qtype=ANY qclass=IN>
    ---
    <DNS Header: id=0xa6fc type=RESPONSE opcode=QUERY flags=RD,RA rcode='NOERROR' q=1 a=28 ns=0 ar=0>
    <DNS Question: 'google.com.' qtype=ANY qclass=IN>
    <DNS RR: 'google.com.' rtype=A rclass=IN ttl=299 rdata='62.252.169.183'>
    <DNS RR: 'google.com.' rtype=A rclass=IN ttl=299 rdata='62.252.169.152'>
    <DNS RR: 'google.com.' rtype=A rclass=IN ttl=299 rdata='62.252.169.172'>
    <DNS RR: 'google.com.' rtype=A rclass=IN ttl=299 rdata='62.252.169.177'>
    <DNS RR: 'google.com.' rtype=A rclass=IN ttl=299 rdata='62.252.169.157'>
    <DNS RR: 'google.com.' rtype=A rclass=IN ttl=299 rdata='62.252.169.153'>
    <DNS RR: 'google.com.' rtype=A rclass=IN ttl=299 rdata='62.252.169.182'>
    <DNS RR: 'google.com.' rtype=A rclass=IN ttl=299 rdata='62.252.169.168'>
    <DNS RR: 'google.com.' rtype=A rclass=IN ttl=299 rdata='62.252.169.178'>
    <DNS RR: 'google.com.' rtype=A rclass=IN ttl=299 rdata='62.252.169.162'>
    <DNS RR: 'google.com.' rtype=A rclass=IN ttl=299 rdata='62.252.169.187'>
    <DNS RR: 'google.com.' rtype=A rclass=IN ttl=299 rdata='62.252.169.167'>
    <DNS RR: 'google.com.' rtype=A rclass=IN ttl=299 rdata='62.252.169.148'>
    <DNS RR: 'google.com.' rtype=A rclass=IN ttl=299 rdata='62.252.169.173'>
    <DNS RR: 'google.com.' rtype=A rclass=IN ttl=299 rdata='62.252.169.158'>
    <DNS RR: 'google.com.' rtype=A rclass=IN ttl=299 rdata='62.252.169.163'>
    <DNS RR: 'google.com.' rtype=NS rclass=IN ttl=21599 rdata='ns4.google.com.'>
    <DNS RR: 'google.com.' rtype=MX rclass=IN ttl=599 rdata='50 alt4.aspmx.l.google.com.'>
    <DNS RR: 'google.com.' rtype=NS rclass=IN ttl=21599 rdata='ns2.google.com.'>
    <DNS RR: 'google.com.' rtype=MX rclass=IN ttl=599 rdata='10 aspmx.l.google.com.'>
    <DNS RR: 'google.com.' rtype=NS rclass=IN ttl=21599 rdata='ns3.google.com.'>
    <DNS RR: 'google.com.' rtype=SOA rclass=IN ttl=21599 rdata='ns1.google.com. dns-admin.google.com. 2014021800 7200 1800 1209600 300'>
    <DNS RR: 'google.com.' rtype=MX rclass=IN ttl=599 rdata='40 alt3.aspmx.l.google.com.'>
    <DNS RR: 'google.com.' rtype=MX rclass=IN ttl=599 rdata='20 alt1.aspmx.l.google.com.'>
    <DNS RR: 'google.com.' rtype=TYPE257 rclass=IN ttl=21599 rdata='0005697373756573796d616e7465632e636f6d'>
    <DNS RR: 'google.com.' rtype=TXT rclass=IN ttl=3599 rdata='v=spf1 include:_spf.google.com ip4:216.73.93.70/31 ip4:216.73.93.72/31 ~all'>
    <DNS RR: 'google.com.' rtype=MX rclass=IN ttl=599 rdata='30 alt2.aspmx.l.google.com.'>
    <DNS RR: 'google.com.' rtype=NS rclass=IN ttl=21599 rdata='ns1.google.com.'>

"""

from __future__ import print_function

import glob,os.path,string

from dnslib.lex import WordLexer
from dnslib.dns import (DNSRecord,DNSHeader,DNSQuestion,DNSError,
                        RR,RD,RDMAP,QR,RCODE,CLASS,QTYPE)

class DigParser:

    """
        Parse Dig output
    """

    def __init__(self,dig,debug=False):
        self.debug = debug
        self.l = WordLexer(dig)
        self.l.commentchars = ';'
        self.l.nltok = ('NL',None)
        self.i = iter(self.l)

    def parseHeader(self,l1,l2):
        _,_,_,opcode,_,status,_,_id = l1.split()
        _,flags,_ = l2.split(';')
        header = DNSHeader(id=int(_id),bitmap=0)
        header.opcode = getattr(QR,opcode.rstrip(','))
        header.rcode = getattr(RCODE,status.rstrip(','))
        for f in ('qr','aa','tc','rd','ra'):
            if f in flags:
                setattr(header,f,1)
        return header

    def expect(self,expect):
        t,val = next(self.i)
        if t != expect:
            raise ValueError("Invalid Token: %s (expecting: %s)" % (t,expect))
        return val

    def parseQuestions(self,q,dns):
        for qname,qclass,qtype in q:
            dns.add_question(DNSQuestion(qname,
                                         getattr(QTYPE,qtype),
                                         getattr(CLASS,qclass)))

    def parseAnswers(self,a,auth,ar,dns):
        sect_map = {'a':'add_answer','auth':'add_auth','ar':'add_ar'}
        for sect in 'a','auth','ar':
            f = getattr(dns,sect_map[sect])
            for rr in locals()[sect]:
                rname,ttl,rclass,rtype = rr[:4]
                rdata = rr[4:]
                rd = RDMAP.get(rtype,RD)
                try:
                    if rd == RD and \
                       any([ x not in string.hexdigits for x in rdata[-1]]):
                        # Only support hex encoded data for fallback RD
                        pass
                    else:
                        f(RR(rname=rname,
                                ttl=int(ttl),
                                rtype=getattr(QTYPE,rtype),
                                rclass=getattr(CLASS,rclass),
                                rdata=rd.fromZone(rdata)))
                except DNSError as e:
                    if self.debug:
                        print("DNSError:",e,rr)
                    else:
                        # Skip records we dont understand
                        pass

    def __iter__(self):
        return self.parse()

    def parse(self):
        dns = None
        section = None
        paren = False
        rr = []
        try:
            while True:
                tok,val = next(self.i)
                if tok == 'COMMENT':
                    if 'Sending:' in val or 'Got answer:' in val:
                        if dns:
                            self.parseQuestions(q,dns)
                            self.parseAnswers(a,auth,ar,dns)
                            yield(dns)
                        dns = DNSRecord()
                        q,a,auth,ar = [],[],[],[]
                    elif val.startswith('; ->>HEADER<<-'):
                        self.expect('NL')
                        val2 = self.expect('COMMENT')
                        dns.header = self.parseHeader(val,val2)
                    elif val.startswith('; QUESTION'):
                        section = q
                    elif val.startswith('; ANSWER'):
                        section = a
                    elif val.startswith('; AUTHORITY'):
                        section = auth
                    elif val.startswith('; ADDITIONAL'):
                        section = ar
                    elif val.startswith(';') or tok[1].startswith('<<>>'):
                        pass
                    elif dns and section == q:
                        q.append(val.split())
                elif tok == 'ATOM':
                    if val == '(':
                        paren = True
                    elif val == ')':
                        paren = False
                    else:
                        rr.append(val)
                elif tok == 'NL' and not paren and rr:
                    if self.debug:
                        print(">>",rr)
                    section.append(rr)
                    rr = []
        except StopIteration:
            if rr:
                self.section.append(rr)
            if dns:
                self.parseQuestions(q,dns)
                self.parseAnswers(a,auth,ar,dns)
                yield(dns)

if __name__ == '__main__':

    import argparse,doctest,sys

    p = argparse.ArgumentParser(description="DigParser Test")
    p.add_argument("--dig",action='store_true',default=False,
                    help="Parse DiG output (stdin)")
    p.add_argument("--debug",action='store_true',default=False,
                    help="Debug output")

    args = p.parse_args()

    if args.dig:
        l = DigParser(sys.stdin,args.debug)
        for record in l:
            print(repr(record))
    else:
        doctest.testmod()
