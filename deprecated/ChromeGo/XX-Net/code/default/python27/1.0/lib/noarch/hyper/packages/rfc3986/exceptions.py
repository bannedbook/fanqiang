# -*- coding: utf-8 -*-
class RFC3986Exception(Exception):
    pass


class InvalidAuthority(RFC3986Exception):
    def __init__(self, authority):
        super(InvalidAuthority, self).__init__(
            "The authority ({0}) is not valid.".format(authority))


class InvalidPort(RFC3986Exception):
    def __init__(self, port):
        super(InvalidPort, self).__init__(
            'The port ("{0}") is not valid.'.format(port))


class ResolutionError(RFC3986Exception):
    def __init__(self, uri):
        super(ResolutionError, self).__init__(
            "{0} is not an absolute URI.".format(uri.unsplit()))
