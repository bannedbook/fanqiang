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

from __future__ import absolute_import, division, print_function

import six

from cryptography import utils


class DHPrivateNumbers(object):
    def __init__(self, x, public_numbers):
        if not isinstance(x, six.integer_types):
            raise TypeError("x must be an integer.")

        if not isinstance(public_numbers, DHPublicNumbers):
            raise TypeError("public_numbers must be an instance of "
                            "DHPublicNumbers.")

        self._x = x
        self._public_numbers = public_numbers

    def __eq__(self, other):
        if not isinstance(other, DHPrivateNumbers):
            return NotImplemented

        return (
            self._x == other._x and
            self._public_numbers == other._public_numbers
        )

    def __ne__(self, other):
        return not self == other

    public_numbers = utils.read_only_property("_public_numbers")
    x = utils.read_only_property("_x")


class DHPublicNumbers(object):
    def __init__(self, y, parameter_numbers):
        if not isinstance(y, six.integer_types):
            raise TypeError("y must be an integer.")

        if not isinstance(parameter_numbers, DHParameterNumbers):
            raise TypeError(
                "parameters must be an instance of DHParameterNumbers.")

        self._y = y
        self._parameter_numbers = parameter_numbers

    def __eq__(self, other):
        if not isinstance(other, DHPublicNumbers):
            return NotImplemented

        return (
            self._y == other._y and
            self._parameter_numbers == other._parameter_numbers
        )

    def __ne__(self, other):
        return not self == other

    y = utils.read_only_property("_y")
    parameter_numbers = utils.read_only_property("_parameter_numbers")


class DHParameterNumbers(object):
    def __init__(self, p, g):
        if (
            not isinstance(p, six.integer_types) or
            not isinstance(g, six.integer_types)
        ):
            raise TypeError("p and g must be integers")

        self._p = p
        self._g = g

    def __eq__(self, other):
        if not isinstance(other, DHParameterNumbers):
            return NotImplemented

        return (
            self._p == other._p and
            self._g == other._g
        )

    def __ne__(self, other):
        return not self == other

    p = utils.read_only_property("_p")
    g = utils.read_only_property("_g")
