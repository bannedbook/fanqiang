# -*- coding: utf-8 -*-
# Copyright (c) 2014 Rackspace
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
# implied.
# See the License for the specific language governing permissions and
# limitations under the License.
import re

from .compat import to_bytes
from .misc import NON_PCT_ENCODED


def normalize_scheme(scheme):
    return scheme.lower()


def normalize_authority(authority):
    userinfo, host, port = authority
    result = ''
    if userinfo:
        result += normalize_percent_characters(userinfo) + '@'
    if host:
        result += host.lower()
    if port:
        result += ':' + port
    return result


def normalize_path(path):
    if not path:
        return path

    path = normalize_percent_characters(path)
    return remove_dot_segments(path)


def normalize_query(query):
    return normalize_percent_characters(query)


def normalize_fragment(fragment):
    return normalize_percent_characters(fragment)


PERCENT_MATCHER = re.compile('%[A-Fa-f0-9]{2}')


def normalize_percent_characters(s):
    """All percent characters should be upper-cased.

    For example, ``"%3afoo%DF%ab"`` should be turned into ``"%3Afoo%DF%AB"``.
    """
    matches = set(PERCENT_MATCHER.findall(s))
    for m in matches:
        if not m.isupper():
            s = s.replace(m, m.upper())
    return s


def remove_dot_segments(s):
    # See http://tools.ietf.org/html/rfc3986#section-5.2.4 for pseudo-code
    segments = s.split('/')  # Turn the path into a list of segments
    output = []  # Initialize the variable to use to store output

    for segment in segments:
        # '.' is the current directory, so ignore it, it is superfluous
        if segment == '.':
            continue
        # Anything other than '..', should be appended to the output
        elif segment != '..':
            output.append(segment)
        # In this case segment == '..', if we can, we should pop the last
        # element
        elif output:
            output.pop()

    # If the path starts with '/' and the output is empty or the first string
    # is non-empty
    if s.startswith('/') and (not output or output[0]):
        output.insert(0, '')

    # If the path starts with '/.' or '/..' ensure we add one more empty
    # string to add a trailing '/'
    if s.endswith(('/.', '/..')):
        output.append('')

    return '/'.join(output)


def encode_component(uri_component, encoding):
    if uri_component is None:
        return uri_component

    uri_bytes = to_bytes(uri_component, encoding)

    encoded_uri = bytearray()

    for i in range(0, len(uri_bytes)):
        # Will return a single character bytestring on both Python 2 & 3
        byte = uri_bytes[i:i+1]
        byte_ord = ord(byte)
        if byte_ord < 128 and byte.decode() in NON_PCT_ENCODED:
            encoded_uri.extend(byte)
            continue
        encoded_uri.extend('%{0:02x}'.format(byte_ord).encode())

    return encoded_uri.decode(encoding)
