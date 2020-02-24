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
import sys


if sys.version_info >= (3, 0):
    unicode = str  # Python 3.x


def to_str(b, encoding):
    if hasattr(b, 'decode') and not isinstance(b, unicode):
        b = b.decode('utf-8')
    return b


def to_bytes(s, encoding):
    if hasattr(s, 'encode') and not isinstance(s, bytes):
        s = s.encode('utf-8')
    return s
