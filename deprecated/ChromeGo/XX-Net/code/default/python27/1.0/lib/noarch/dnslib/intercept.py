# -*- coding: utf-8 -*-

"""
    InterceptResolver - proxy requests to upstream server
                        (optionally intercepting)

"""
from __future__ import print_function

import binascii,copy,socket,struct,sys

from dnslib import DNSRecord,RR,QTYPE,RCODE,parse_time
from dnslib.server import DNSServer,DNSHandler,BaseResolver,DNSLogger
from dnslib.label import DNSLabel

class InterceptResolver(BaseResolver):

    """
        Intercepting resolver

        Proxy requests to upstream server optionally intercepting requests
        matching local records
    """

    def __init__(self,address,port,ttl,intercept,skip,nxdomain):
        """
            address/port    - upstream server
            ttl             - default ttl for intercept records
            intercept       - list of wildcard RRs to respond to (zone format)
            skip            - list of wildcard labels to skip
            nxdomain        - list of wildcard labels to retudn NXDOMAIN
        """
        self.address = address
        self.port = port
        self.ttl = parse_time(ttl)
        self.skip = skip
        self.nxdomain = nxdomain
        self.zone = []
        for i in intercept:
            if i == '-':
                i = sys.stdin.read()
            for rr in RR.fromZone(i,ttl=self.ttl):
                self.zone.append((rr.rname,QTYPE[rr.rtype],rr))

    def resolve(self,request,handler):
        reply = request.reply()
        qname = request.q.qname
        qtype = QTYPE[request.q.qtype]
        # Try to resolve locally unless on skip list
        if not any([qname.matchGlob(s) for s in self.skip]):
            for name,rtype,rr in self.zone:
                if qname.matchGlob(name) and (qtype in (rtype,'ANY','CNAME')):
                    a = copy.copy(rr)
                    a.rname = qname
                    reply.add_answer(a)
        # Check for NXDOMAIN
        if any([qname.matchGlob(s) for s in self.nxdomain]):
            reply.header.rcode = getattr(RCODE,'NXDOMAIN')
            return reply
        # Otherwise proxy
        if not reply.rr:
            if handler.protocol == 'udp':
                proxy_r = request.send(self.address,self.port)
            else:
                proxy_r = request.send(self.address,self.port,tcp=True)
            reply = DNSRecord.parse(proxy_r)
        return reply

if __name__ == '__main__':

    import argparse,sys,time

    p = argparse.ArgumentParser(description="DNS Intercept Proxy")
    p.add_argument("--port","-p",type=int,default=53,
                    metavar="<port>",
                    help="Local proxy port (default:53)")
    p.add_argument("--address","-a",default="",
                    metavar="<address>",
                    help="Local proxy listen address (default:all)")
    p.add_argument("--upstream","-u",default="8.8.8.8:53",
            metavar="<dns server:port>",
                    help="Upstream DNS server:port (default:8.8.8.8:53)")
    p.add_argument("--tcp",action='store_true',default=False,
                    help="TCP proxy (default: UDP only)")
    p.add_argument("--intercept","-i",action="append",
                    metavar="<zone record>",
                    help="Intercept requests matching zone record (glob) ('-' for stdin)")
    p.add_argument("--skip","-s",action="append",
                    metavar="<label>",
                    help="Don't intercept matching label (glob)")
    p.add_argument("--nxdomain","-x",action="append",
                    metavar="<label>",
                    help="Return NXDOMAIN (glob)")
    p.add_argument("--ttl","-t",default="60s",
                    metavar="<ttl>",
                    help="Intercept TTL (default: 60s)")
    p.add_argument("--log",default="request,reply,truncated,error",
                    help="Log hooks to enable (default: +request,+reply,+truncated,+error,-recv,-send,-data)")
    p.add_argument("--log-prefix",action='store_true',default=False,
                    help="Log prefix (timestamp/handler/resolver) (default: False)")
    args = p.parse_args()

    args.dns,_,args.dns_port = args.upstream.partition(':')
    args.dns_port = int(args.dns_port or 53)

    resolver = InterceptResolver(args.dns,
                                 args.dns_port,
                                 args.ttl,
                                 args.intercept or [],
                                 args.skip or [],
                                 args.nxdomain or [])
    logger = DNSLogger(args.log,args.log_prefix)

    print("Starting Intercept Proxy (%s:%d -> %s:%d) [%s]" % (
                        args.address or "*",args.port,
                        args.dns,args.dns_port,
                        "UDP/TCP" if args.tcp else "UDP"))

    for rr in resolver.zone:
        print("    | ",rr[2].toZone(),sep="")
    if resolver.nxdomain:
        print("    NXDOMAIN:",", ".join(resolver.nxdomain))
    if resolver.skip:
        print("    Skipping:",", ".join(resolver.skip))
    print()


    DNSHandler.log = {
        'log_request',      # DNS Request
        'log_reply',        # DNS Response
        'log_truncated',    # Truncated
        'log_error',        # Decoding error
    }

    udp_server = DNSServer(resolver,
                           port=args.port,
                           address=args.address,
                           logger=logger)
    udp_server.start_thread()

    if args.tcp:
        tcp_server = DNSServer(resolver,
                               port=args.port,
                               address=args.address,
                               tcp=True,
                               logger=logger)
        tcp_server.start_thread()

    while udp_server.isAlive():
        time.sleep(1)

