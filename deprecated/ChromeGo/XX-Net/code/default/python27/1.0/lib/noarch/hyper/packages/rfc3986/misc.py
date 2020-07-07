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
"""
rfc3986.misc
~~~~~~~~~~~~

This module contains important constants, patterns, and compiled regular
expressions for parsing and validating URIs and their components.
"""

import re

# These are enumerated for the named tuple used as a superclass of
# URIReference
URI_COMPONENTS = ['scheme', 'authority', 'path', 'query', 'fragment']

important_characters = {
    'generic_delimiters': ":/?#[]@",
    'sub_delimiters': "!$&'()*+,;=",
    # We need to escape the '*' in this case
    're_sub_delimiters': "!$&'()\*+,;=",
    'unreserved_chars': ('ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz'
                         '0123456789._~-'),
    # We need to escape the '-' in this case:
    're_unreserved': 'A-Za-z0-9._~\-',
    }
# For details about delimiters and reserved characters, see:
# http://tools.ietf.org/html/rfc3986#section-2.2
GENERIC_DELIMITERS = set(important_characters['generic_delimiters'])
SUB_DELIMITERS = set(important_characters['sub_delimiters'])
RESERVED_CHARS = GENERIC_DELIMITERS.union(SUB_DELIMITERS)
# For details about unreserved characters, see:
# http://tools.ietf.org/html/rfc3986#section-2.3
UNRESERVED_CHARS = set(important_characters['unreserved_chars'])
NON_PCT_ENCODED = RESERVED_CHARS.union(UNRESERVED_CHARS).union('%')

# Extracted from http://tools.ietf.org/html/rfc3986#appendix-B
component_pattern_dict = {
    'scheme': '[^:/?#]+',
    'authority': '[^/?#]*',
    'path': '[^?#]*',
    'query': '[^#]*',
    'fragment': '.*',
    }

# See http://tools.ietf.org/html/rfc3986#appendix-B
# In this case, we name each of the important matches so we can use
# SRE_Match#groupdict to parse the values out if we so choose. This is also
# modified to ignore other matches that are not important to the parsing of
# the reference so we can also simply use SRE_Match#groups.
expression = ('(?:(?P<scheme>{scheme}):)?(?://(?P<authority>{authority}))?'
              '(?P<path>{path})(?:\?(?P<query>{query}))?'
              '(?:#(?P<fragment>{fragment}))?'
              ).format(**component_pattern_dict)

URI_MATCHER = re.compile(expression)

# #########################
# Authority Matcher Section
# #########################

# Host patterns, see: http://tools.ietf.org/html/rfc3986#section-3.2.2
# The pattern for a regular name, e.g.,  www.google.com, api.github.com
reg_name = '(({0})*|[{1}]*)'.format(
    '%[0-9A-Fa-f]{2}',
    important_characters['re_sub_delimiters'] +
    important_characters['re_unreserved']
    )
# The pattern for an IPv4 address, e.g., 192.168.255.255, 127.0.0.1,
ipv4 = '(\d{1,3}.){3}\d{1,3}'
# Hexadecimal characters used in each piece of an IPv6 address
hexdig = '[0-9A-Fa-f]{1,4}'
# Least-significant 32 bits of an IPv6 address
ls32 = '({hex}:{hex}|{ipv4})'.format(hex=hexdig, ipv4=ipv4)
# Substitutions into the following patterns for IPv6 patterns defined
# http://tools.ietf.org/html/rfc3986#page-20
subs = {'hex': hexdig, 'ls32': ls32}

# Below: h16 = hexdig, see: https://tools.ietf.org/html/rfc5234 for details
# about ABNF (Augmented Backus-Naur Form) use in the comments
variations = [
    #                            6( h16 ":" ) ls32
    '(%(hex)s:){6}%(ls32)s' % subs,
    #                       "::" 5( h16 ":" ) ls32
    '::(%(hex)s:){5}%(ls32)s' % subs,
    # [               h16 ] "::" 4( h16 ":" ) ls32
    '(%(hex)s)?::(%(hex)s:){4}%(ls32)s' % subs,
    # [ *1( h16 ":" ) h16 ] "::" 3( h16 ":" ) ls32
    '((%(hex)s:)?%(hex)s)?::(%(hex)s:){3}%(ls32)s' % subs,
    # [ *2( h16 ":" ) h16 ] "::" 2( h16 ":" ) ls32
    '((%(hex)s:){0,2}%(hex)s)?::(%(hex)s:){2}%(ls32)s' % subs,
    # [ *3( h16 ":" ) h16 ] "::"    h16 ":"   ls32
    '((%(hex)s:){0,3}%(hex)s)?::%(hex)s:%(ls32)s' % subs,
    # [ *4( h16 ":" ) h16 ] "::"              ls32
    '((%(hex)s:){0,4}%(hex)s)?::%(ls32)s' % subs,
    # [ *5( h16 ":" ) h16 ] "::"              h16
    '((%(hex)s:){0,5}%(hex)s)?::%(hex)s' % subs,
    # [ *6( h16 ":" ) h16 ] "::"
    '((%(hex)s:){0,6}%(hex)s)?::' % subs,
    ]

ipv6 = '(({0})|({1})|({2})|({3})|({4})|({5})|({6})|({7}))'.format(*variations)

ipv_future = 'v[0-9A-Fa-f]+.[%s]+' % (
    important_characters['re_unreserved'] +
    important_characters['re_sub_delimiters'] +
    ':')

ip_literal = '\[({0}|{1})\]'.format(ipv6, ipv_future)

# Pattern for matching the host piece of the authority
HOST_PATTERN = '({0}|{1}|{2})'.format(reg_name, ipv4, ip_literal)

SUBAUTHORITY_MATCHER = re.compile((
    '^(?:(?P<userinfo>[A-Za-z0-9_.~\-%:]+)@)?'  # userinfo
    '(?P<host>{0}?)'  # host
    ':?(?P<port>\d+)?$'  # port
    ).format(HOST_PATTERN))

IPv4_MATCHER = re.compile('^' + ipv4 + '$')


# ####################
# Path Matcher Section
# ####################

# See http://tools.ietf.org/html/rfc3986#section-3.3 for more information
# about the path patterns defined below.

# Percent encoded character values
pct_encoded = '%[A-Fa-f0-9]{2}'
pchar = ('([' + important_characters['re_unreserved']
         + important_characters['re_sub_delimiters']
         + ':@]|%s)' % pct_encoded)
segments = {
    'segment': pchar + '*',
    # Non-zero length segment
    'segment-nz': pchar + '+',
    # Non-zero length segment without ":"
    'segment-nz-nc': pchar.replace(':', '') + '+'
    }

# Path types taken from Section 3.3 (linked above)
path_empty = '^$'
path_rootless = '%(segment-nz)s(/%(segment)s)*' % segments
path_noscheme = '%(segment-nz-nc)s(/%(segment)s)*' % segments
path_absolute = '/(%s)?' % path_rootless
path_abempty = '(/%(segment)s)*' % segments

# Matcher used to validate path components
PATH_MATCHER = re.compile('^(%s|%s|%s|%s|%s)$' % (
    path_abempty, path_absolute, path_noscheme, path_rootless, path_empty
    ))


# ##################################
# Query and Fragment Matcher Section
# ##################################

QUERY_MATCHER = re.compile(
    '^([/?:@' + important_characters['re_unreserved']
    + important_characters['re_sub_delimiters']
    + ']|%s)*$' % pct_encoded)

FRAGMENT_MATCHER = QUERY_MATCHER

# Scheme validation, see: http://tools.ietf.org/html/rfc3986#section-3.1
SCHEME_MATCHER = re.compile('^[A-Za-z][A-Za-z0-9+.\-]*$')

# Relative reference matcher

# See http://tools.ietf.org/html/rfc3986#section-4.2 for details
relative_part = '(//%s%s|%s|%s|%s)' % (
    component_pattern_dict['authority'], path_abempty, path_absolute,
    path_noscheme, path_empty
    )

RELATIVE_REF_MATCHER = re.compile('^%s(\?%s)?(#%s)?$' % (
    relative_part, QUERY_MATCHER.pattern, FRAGMENT_MATCHER.pattern
    ))

# See http://tools.ietf.org/html/rfc3986#section-3 for definition
hier_part = '(//%s%s|%s|%s|%s)' % (
    component_pattern_dict['authority'], path_abempty, path_absolute,
    path_rootless, path_empty
    )

# See http://tools.ietf.org/html/rfc3986#section-4.3
ABSOLUTE_URI_MATCHER = re.compile('^%s:%s(\?%s)?$' % (
    component_pattern_dict['scheme'], hier_part, QUERY_MATCHER.pattern[1:-1]
    ))


# Path merger as defined in http://tools.ietf.org/html/rfc3986#section-5.2.3
def merge_paths(base_uri, relative_path):
    """Merge a base URI's path with a relative URI's path."""
    if base_uri.path is None and base_uri.authority is not None:
        return '/' + relative_path
    else:
        path = base_uri.path or ''
        index = path.rfind('/')
        return path[:index] + '/' + relative_path
