# -*- coding: utf-8 -*-
"""
hyper/cli
~~~~~~~~~

Command line interface for Hyper inspired by Httpie.
"""
import json
import locale
import logging
import sys
from argparse import ArgumentParser, RawTextHelpFormatter
from argparse import OPTIONAL, ZERO_OR_MORE
from pprint import pformat
from textwrap import dedent

from hyper import HTTPConnection, HTTP20Connection
from hyper import __version__
from hyper.compat import is_py2, urlencode, urlsplit, write_to_stdout
from hyper.common.util import to_host_port_tuple


log = logging.getLogger('hyper')

PREFERRED_ENCODING = locale.getpreferredencoding()

# Various separators used in args
SEP_HEADERS = ':'
SEP_QUERY = '=='
SEP_DATA = '='

SEP_GROUP_ITEMS = [
    SEP_HEADERS,
    SEP_QUERY,
    SEP_DATA,
]


class KeyValue(object):
    """Base key-value pair parsed from CLI."""

    def __init__(self, key, value, sep, orig):
        self.key = key
        self.value = value
        self.sep = sep
        self.orig = orig


class KeyValueArgType(object):
    """A key-value pair argument type used with `argparse`.

    Parses a key-value arg and constructs a `KeyValue` instance.
    Used for headers, form data, and other key-value pair types.
    This class is inspired by httpie and implements simple tokenizer only.
    """
    def __init__(self, *separators):
        self.separators = separators

    def __call__(self, string):
        for sep in self.separators:
            splitted = string.split(sep, 1)
            if len(splitted) == 2:
                key, value = splitted
                return KeyValue(key, value, sep, string)


def make_positional_argument(parser):
    parser.add_argument(
        'method', metavar='METHOD', nargs=OPTIONAL, default='GET',
        help=dedent("""
        The HTTP method to be used for the request
        (GET, POST, PUT, DELETE, ...).
        """))
    parser.add_argument(
        '_url', metavar='URL',
        help=dedent("""
        The scheme defaults to 'https://' if the URL does not include one.
        """))
    parser.add_argument(
        'items',
        metavar='REQUEST_ITEM',
        nargs=ZERO_OR_MORE,
        type=KeyValueArgType(*SEP_GROUP_ITEMS),
        help=dedent("""
        Optional key-value pairs to be included in the request.
        The separator used determines the type:

        ':' HTTP headers:

            Referer:http://httpie.org  Cookie:foo=bar  User-Agent:bacon/1.0

        '==' URL parameters to be appended to the request URI:

            search==hyper

        '=' Data fields to be serialized into a JSON object:

            name=Hyper  language=Python  description='CLI HTTP client'
        """))


def make_troubleshooting_argument(parser):
    parser.add_argument(
        '--version', action='version', version=__version__,
        help='Show version and exit.')
    parser.add_argument(
        '--debug', action='store_true', default=False,
        help='Show debugging information (loglevel=DEBUG)')
    parser.add_argument(
        '--h2', action='store_true', default=False,
        help="Do HTTP/2 directly in plaintext: skip plaintext upgrade")


def set_url_info(args):
    def split_host_and_port(hostname):
        if ':' in hostname:
            return to_host_port_tuple(hostname, default_port=443)
        return hostname, None

    class UrlInfo(object):
        def __init__(self):
            self.fragment = None
            self.host = 'localhost'
            self.netloc = None
            self.path = '/'
            self.port = 443
            self.query = None
            self.scheme = 'https'
            self.secure = False

    info = UrlInfo()
    _result = urlsplit(args._url)
    for attr in vars(info).keys():
        value = getattr(_result, attr, None)
        if value:
            setattr(info, attr, value)

    if info.scheme == 'http' and not _result.port:
        info.port = 80

    # Set the secure arg is the scheme is HTTPS, otherwise do unsecured.
    info.secure = info.scheme == 'https'

    if info.netloc:
        hostname, _ = split_host_and_port(info.netloc)
        info.host = hostname  # ensure stripping port number
    else:
        if _result.path:
            _path = _result.path.split('/', 1)
            hostname, port = split_host_and_port(_path[0])
            info.host = hostname
            if info.path == _path[0]:
                info.path = '/'
            elif len(_path) == 2 and _path[1]:
                info.path = '/' + _path[1]
            if port is not None:
                info.port = port

    log.debug('Url Info: %s', vars(info))
    args.url = info


def set_request_data(args):
    body, headers, params = {}, {}, {}
    for i in args.items:
        if i.sep == SEP_HEADERS:
            if i.key:
                headers[i.key] = i.value
            else:
                # when overriding a HTTP/2 special header there will be a leading 
                # colon, which tricks the command line parser into thinking 
                # the header is empty
                k, v = i.value.split(':', 1)
                headers[':' + k] = v
        elif i.sep == SEP_QUERY:
            params[i.key] = i.value
        elif i.sep == SEP_DATA:
            value = i.value
            if is_py2:  # pragma: no cover
                value = value.decode(PREFERRED_ENCODING)
            body[i.key] = value

    if params:
        args.url.path += '?' + urlencode(params)

    if body:
        content_type = 'application/json'
        headers.setdefault('content-type', content_type)
        args.body = json.dumps(body)

    if args.method is None:
        args.method = 'POST' if args.body else 'GET'

    args.headers = headers


def parse_argument(argv=None):
    parser = ArgumentParser(formatter_class=RawTextHelpFormatter)
    parser.set_defaults(body=None, headers={})
    make_positional_argument(parser)
    make_troubleshooting_argument(parser)
    args = parser.parse_args(sys.argv[1:] if argv is None else argv)

    if args.debug:
        handler = logging.StreamHandler()
        handler.setLevel(logging.DEBUG)
        log.addHandler(handler)
        log.setLevel(logging.DEBUG)

    set_url_info(args)
    set_request_data(args)
    return args


def get_content_type_and_charset(response):
    charset = 'utf-8'
    content_type = response.headers.get('content-type')
    if content_type is None:
        return 'unknown', charset

    content_type = content_type[0].decode('utf-8').lower()
    type_and_charset = content_type.split(';', 1)
    ctype = type_and_charset[0].strip()
    if len(type_and_charset) == 2:
        charset = type_and_charset[1].strip().split('=')[1]

    return ctype, charset


def request(args):
    if not args.h2:
        conn = HTTPConnection(
            args.url.host, args.url.port, secure=args.url.secure
        )
    else:  # pragma: no cover
        conn = HTTP20Connection(
            args.url.host, args.url.port, secure=args.url.secure
        )

    conn.request(args.method, args.url.path, args.body, args.headers)
    response = conn.get_response()
    log.debug('Response Headers:\n%s', pformat(response.headers))
    ctype, charset = get_content_type_and_charset(response)
    data = response.read()
    return data


def main(argv=None):
    args = parse_argument(argv)
    log.debug('Commandline Argument: %s', args)
    data = request(args)
    write_to_stdout(data)


if __name__ == '__main__':  # pragma: no cover
    main()
