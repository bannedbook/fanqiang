# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

import abc
import inspect
import sys
import warnings


DeprecatedIn07 = DeprecationWarning
DeprecatedIn08 = PendingDeprecationWarning


def read_only_property(name):
    return property(lambda self: getattr(self, name))


def register_interface(iface):
    def register_decorator(klass):
        verify_interface(iface, klass)
        iface.register(klass)
        return klass
    return register_decorator


class InterfaceNotImplemented(Exception):
    pass


def verify_interface(iface, klass):
    for method in iface.__abstractmethods__:
        if not hasattr(klass, method):
            raise InterfaceNotImplemented(
                "{0} is missing a {1!r} method".format(klass, method)
            )
        if isinstance(getattr(iface, method), abc.abstractproperty):
            # Can't properly verify these yet.
            continue
        spec = inspect.getargspec(getattr(iface, method))
        actual = inspect.getargspec(getattr(klass, method))
        if spec != actual:
            raise InterfaceNotImplemented(
                "{0}.{1}'s signature differs from the expected. Expected: "
                "{2!r}. Received: {3!r}".format(
                    klass, method, spec, actual
                )
            )


if sys.version_info >= (2, 7):
    def bit_length(x):
        return x.bit_length()
else:
    def bit_length(x):
        return len(bin(x)) - (2 + (x <= 0))


class _DeprecatedValue(object):
    def __init__(self, value, message, warning_class):
        self.value = value
        self.message = message
        self.warning_class = warning_class


class _ModuleWithDeprecations(object):
    def __init__(self, module):
        self.__dict__["_module"] = module

    def __getattr__(self, attr):
        obj = getattr(self._module, attr)
        if isinstance(obj, _DeprecatedValue):
            warnings.warn(obj.message, obj.warning_class, stacklevel=2)
            obj = obj.value
        return obj

    def __setattr__(self, attr, value):
        setattr(self._module, attr, value)

    def __dir__(self):
        return ["_module"] + dir(self._module)


def deprecated(value, module_name, message, warning_class):
    module = sys.modules[module_name]
    if not isinstance(module, _ModuleWithDeprecations):
        sys.modules[module_name] = module = _ModuleWithDeprecations(module)
    return _DeprecatedValue(value, message, warning_class)
