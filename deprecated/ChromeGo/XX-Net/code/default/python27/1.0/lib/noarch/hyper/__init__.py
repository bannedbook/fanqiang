# -*- coding: utf-8 -*-
"""
hyper
~~~~~~

A module for providing an abstraction layer over the differences between
HTTP/1.1 and HTTP/2.
"""
__version__ = '0.5.0'

from .common.connection import HTTPConnection
from .http20.connection import HTTP20Connection
from .http20.response import HTTP20Response, HTTP20Push
from .http11.connection import HTTP11Connection
from .http11.response import HTTP11Response

# Throw import errors on Python <2.7 and 3.0-3.2.
import sys as _sys
if _sys.version_info < (2,7) or (3,0) <= _sys.version_info < (3,3):
    raise ImportError("hyper only supports Python 2.7 and Python 3.3 or higher.")

__all__ = [
    HTTPConnection,
    HTTP20Response,
    HTTP20Push,
    HTTP20Connection,
    HTTP11Connection,
    HTTP11Response,
]

# Set default logging handler.
import logging
logging.getLogger(__name__).addHandler(logging.NullHandler())
