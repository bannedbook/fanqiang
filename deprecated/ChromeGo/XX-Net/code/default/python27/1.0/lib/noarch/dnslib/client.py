# -*- coding: utf-8 -*-

"""
    DNS Client - DiG-like CLI utility.

    Mostly useful for testing. Can optionally compare results from two
    nameservers (--diff) or compare results against DiG (--dig).

    Usage: python -m dnslib.client [options|--help]

    See --help for usage.
"""

from __future__ import print_function

try:
    from subprocess import getoutput
except ImportError:
    from commands import getoutput

import binascii,code,pprint

from dnslib.dns import DNSRecord,DNSHeader,DNSQuestion,QTYPE
from dnslib.digparser import DigParser

if __name__ == '__main__':

    import argparse,sys,time

    p = argparse.ArgumentParser(description="DNS Client")
    p.add_argument("--server","-s",default="8.8.8.8",
                    metavar="<address:port>",
                    help="Server address:port (default:8.8.8.8:53) (port is optional)")
    p.add_argument("--query",action='store_true',default=False,
                    help="Show query (default: False)")
    p.add_argument("--hex",action='store_true',default=False,
                    help="Dump packet in hex (default: False)")
    p.add_argument("--tcp",action='store_true',default=False,
                    help="Use TCP (default: UDP)")
    p.add_argument("--noretry",action='store_true',default=False,
                    help="Don't retry query using TCP if truncated (default: false)")
    p.add_argument("--diff",default="",
                    help="Compare response from alternate nameserver (format: address:port / default: false)")
    p.add_argument("--dig",action='store_true',default=False,
                    help="Compare result with DiG - if ---diff also specified use alternative nameserver for DiG request (default: false)")
    p.add_argument("--short",action='store_true',default=False,
                    help="Short output - rdata only (default: false)")
    p.add_argument("--debug",action='store_true',default=False,
                    help="Drop into CLI after request (default: false)")
    p.add_argument("domain",metavar="<domain>",
                    help="Query domain")
    p.add_argument("qtype",metavar="<type>",default="A",nargs="?",
                    help="Query type (default: A)")
    args = p.parse_args()

    # Construct request
    q = DNSRecord(q=DNSQuestion(args.domain,getattr(QTYPE,args.qtype)))

    address,_,port = args.server.partition(':')
    port = int(port or 53)

    if args.query:
        print(";; Sending%s:" % (" (TCP)" if args.tcp else ""))
        if args.hex:
            print(";; QUERY:",binascii.hexlify(q.pack()).decode())
        print(q)
        print()

    a_pkt = q.send(address,port,tcp=args.tcp)
    a = DNSRecord.parse(a_pkt)

    if a.header.tc and args.noretry == False:
        # Truncated - retry in TCP mode
        a_pkt = q.send(address,port,tcp=True)
        a = DNSRecord.parse(a_pkt)

    if args.dig or args.diff:
        if args.diff:
            address,_,port = args.diff.partition(':')
            port = int(port or 53)

        if args.dig:
            dig = getoutput("dig +qr -p %d %s %s @%s" % (
                                port, args.domain, args.qtype, address))
            dig_reply = list(iter(DigParser(dig)))
            # DiG might have retried in TCP mode so get last q/a
            q_diff = dig_reply[-2]
            a_diff = dig_reply[-1]
        else:
            q_diff = DNSRecord(header=DNSHeader(id=q.header.id),
                               q=DNSQuestion(args.domain,
                                             getattr(QTYPE,args.qtype)))
            q_diff = q
            diff = q_diff.send(address,port,tcp=args.tcp)
            a_diff = DNSRecord.parse(diff)
            if a_diff.header.tc and args.noretry == False:
                diff = q_diff.send(address,port,tcp=True)
                a_diff = DNSRecord.parse(diff)

    if args.short:
        print(a.short())
    else:
        print(";; Got answer:")
        if args.hex:
            print(";; RESPONSE:",binascii.hexlify(a_pkt).decode())
            if args.diff and not args.dig:
                print(";; DIFF    :",binascii.hexlify(diff).decode())
        print(a)
        print()

        if args.dig or args.diff:
            if q != q_diff:
                print(";;; ERROR: Diff Question differs")
                for (d1,d2) in q.diff(q_diff):
                    if d1:
                        print(";; - %s" % d1)
                    if d2:
                        print(";; + %s" % d2)
            if a != a_diff:
                print(";;; ERROR: Diff Response differs")
                for (d1,d2) in a.diff(a_diff):
                    if d1:
                        print(";; - %s" % d1)
                    if d2:
                        print(";; + %s" % d2)

    if args.debug:
        code.interact(local=locals())

