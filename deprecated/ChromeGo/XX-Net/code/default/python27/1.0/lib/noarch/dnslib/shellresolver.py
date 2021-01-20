# -*- coding: utf-8 -*-

from __future__ import print_function

try:
    from subprocess import getoutput
except ImportError:
    from commands import getoutput

from dnslib import RR,QTYPE,RCODE,TXT,parse_time
from dnslib.label import DNSLabel
from dnslib.server import DNSServer,DNSHandler,BaseResolver,DNSLogger

class ShellResolver(BaseResolver):
    """
        Example dynamic resolver.
        Maps DNS labels to shell commands and returns result as TXT record
        (Note: No context is passed to the shell command)

        Shell commands are passed in a a list in <label>:<cmd> format - eg:

            [ 'uptime.abc.com.:uptime', 'ls:ls' ]

        Would respond to requests to 'uptime.abc.com.' with the output
        of the 'uptime' command.

        For non-absolute labels the 'origin' parameter is prepended

    """
    def __init__(self,routes,origin,ttl):
        self.origin = DNSLabel(origin)
        self.ttl = parse_time(ttl)
        self.routes = {}
        for r in routes:
            route,_,cmd = r.partition(":")
            if route.endswith('.'):
                route = DNSLabel(route)
            else:
                route = self.origin.add(route)
            self.routes[route] = cmd

    def resolve(self,request,handler):
        reply = request.reply()
        qname = request.q.qname
        cmd = self.routes.get(qname)
        if cmd:
            output = getoutput(cmd).encode()
            reply.add_answer(RR(qname,QTYPE.TXT,ttl=self.ttl,
                                rdata=TXT(output[:254])))
        else:
            reply.header.rcode = RCODE.NXDOMAIN
        return reply

if __name__ == '__main__':

    import argparse,sys,time

    p = argparse.ArgumentParser(description="Shell DNS Resolver")
    p.add_argument("--map","-m",action="append",required=True,
                    metavar="<label>:<shell command>",
                    help="Map label to shell command (multiple supported)")
    p.add_argument("--origin","-o",default=".",
                    metavar="<origin>",
                    help="Origin domain label (default: .)")
    p.add_argument("--ttl","-t",default="60s",
                    metavar="<ttl>",
                    help="Response TTL (default: 60s)")
    p.add_argument("--port","-p",type=int,default=53,
                    metavar="<port>",
                    help="Server port (default:53)")
    p.add_argument("--address","-a",default="",
                    metavar="<address>",
                    help="Listen address (default:all)")
    p.add_argument("--udplen","-u",type=int,default=0,
                    metavar="<udplen>",
                    help="Max UDP packet length (default:0)")
    p.add_argument("--tcp",action='store_true',default=False,
                    help="TCP server (default: UDP only)")
    p.add_argument("--log",default="request,reply,truncated,error",
                    help="Log hooks to enable (default: +request,+reply,+truncated,+error,-recv,-send,-data)")
    p.add_argument("--log-prefix",action='store_true',default=False,
                    help="Log prefix (timestamp/handler/resolver) (default: False)")
    args = p.parse_args()

    resolver = ShellResolver(args.map,args.origin,args.ttl)
    logger = DNSLogger(args.log,args.log_prefix)

    print("Starting Shell Resolver (%s:%d) [%s]" % (
                        args.address or "*",
                        args.port,
                        "UDP/TCP" if args.tcp else "UDP"))

    for route,cmd in resolver.routes.items():
        print("    | ",route,"-->",cmd)
    print()

    if args.udplen:
        DNSHandler.udplen = args.udplen

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

