# Greentea host test script for Mbed TLS on-target test suite testing.
#
# Copyright (C) 2018, Arm Limited, All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# This file is part of Mbed TLS (https://tls.mbed.org)


"""
Mbed TLS on-target test suite tests are implemented as Greentea
tests. Greentea tests are implemented in two parts: target test and
host test. Target test is a C application that is built for the
target platform and executes on the target. Host test is a Python
class derived from mbed_host_tests.BaseHostTest. Target communicates
with the host over serial for the test data and sends back the result.

Python tool mbedgt (Greentea) is responsible for flashing the test
binary on to the target and dynamically loading this host test module.

Greentea documentation can be found here:
https://github.com/ARMmbed/greentea
"""


import re
import os
import binascii
from mbed_host_tests import BaseHostTest, event_callback


class TestDataParserError(Exception):
    """Indicates error in test data, read from .data file."""
    pass


class TestDataParser(object):
    """
    Parses test name, dependencies, test function name and test parameters
    from the data file.
    """

    def __init__(self):
        """
        Constructor
        """
        self.tests = []

    def parse(self, data_file):
        """
        Data file parser.

        :param data_file: Data file path
        """
        with open(data_file, 'r') as data_f:
            self.__parse(data_f)

    @staticmethod
    def __escaped_split(inp_str, split_char):
        """
        Splits inp_str on split_char except when escaped.

        :param inp_str: String to split
        :param split_char: Split character
        :return: List of splits
        """
        split_colon_fn = lambda x: re.sub(r'\\' + split_char, split_char, x)
        if len(split_char) > 1:
            raise ValueError('Expected split character. Found string!')
        out = map(split_colon_fn, re.split(r'(?<!\\)' + split_char, inp_str))
        out = [x for x in out if x]
        return out

    def __parse(self, data_f):
        """
        Parses data file using supplied file object.

        :param data_f: Data file object
        :return:
        """
        for line in data_f:
            line = line.strip()
            if not line:
                continue
            # Read test name
            name = line

            # Check dependencies
            dependencies = []
            line = data_f.next().strip()
            match = re.search('depends_on:(.*)', line)
            if match:
                dependencies = [int(x) for x in match.group(1).split(':')]
                line = data_f.next().strip()

            # Read test vectors
            line = line.replace('\\n', '\n')
            parts = self.__escaped_split(line, ':')
            function_name = int(parts[0])
            args = parts[1:]
            args_count = len(args)
            if args_count % 2 != 0:
                err_str_fmt = "Number of test arguments({}) should be even: {}"
                raise TestDataParserError(err_str_fmt.format(args_count, line))
            grouped_args = [(args[i * 2], args[(i * 2) + 1])
                            for i in range(len(args)/2)]
            self.tests.append((name, function_name, dependencies,
                               grouped_args))

    def get_test_data(self):
        """
        Returns test data.
        """
        return self.tests


class MbedTlsTest(BaseHostTest):
    """
    Host test for Mbed TLS unit tests. This script is loaded at
    run time by Greentea for executing Mbed TLS test suites. Each
    communication from the target is received in this object as
    an event, which is then handled by the event handler method
    decorated by the associated event. Ex: @event_callback('GO').

    Target test sends requests for dispatching next test. It reads
    tests from the intermediate data file and sends test function
    identifier, dependency identifiers, expression identifiers and
    the test data in binary form. Target test checks dependencies
    , evaluate integer constant expressions and dispatches the test
    function with received test parameters. After test function is
    finished, target sends the result. This class handles the result
    event and prints verdict in the form that Greentea understands.

    """
    # status/error codes from suites/helpers.function
    DEPENDENCY_SUPPORTED = 0
    KEY_VALUE_MAPPING_FOUND = DEPENDENCY_SUPPORTED
    DISPATCH_TEST_SUCCESS = DEPENDENCY_SUPPORTED

    KEY_VALUE_MAPPING_NOT_FOUND = -1    # Expression Id not found.
    DEPENDENCY_NOT_SUPPORTED = -2       # Dependency not supported.
    DISPATCH_TEST_FN_NOT_FOUND = -3     # Test function not found.
    DISPATCH_INVALID_TEST_DATA = -4     # Invalid parameter type.
    DISPATCH_UNSUPPORTED_SUITE = -5     # Test suite not supported/enabled.

    def __init__(self):
        """
        Constructor initialises test index to 0.
        """
        super(MbedTlsTest, self).__init__()
        self.tests = []
        self.test_index = -1
        self.dep_index = 0
        self.suite_passed = True
        self.error_str = dict()
        self.error_str[self.DEPENDENCY_SUPPORTED] = \
            'DEPENDENCY_SUPPORTED'
        self.error_str[self.KEY_VALUE_MAPPING_NOT_FOUND] = \
            'KEY_VALUE_MAPPING_NOT_FOUND'
        self.error_str[self.DEPENDENCY_NOT_SUPPORTED] = \
            'DEPENDENCY_NOT_SUPPORTED'
        self.error_str[self.DISPATCH_TEST_FN_NOT_FOUND] = \
            'DISPATCH_TEST_FN_NOT_FOUND'
        self.error_str[self.DISPATCH_INVALID_TEST_DATA] = \
            'DISPATCH_INVALID_TEST_DATA'
        self.error_str[self.DISPATCH_UNSUPPORTED_SUITE] = \
            'DISPATCH_UNSUPPORTED_SUITE'

    def setup(self):
        """
        Setup hook implementation. Reads test suite data file and parses out
        tests.
        """
        binary_path = self.get_config_item('image_path')
        script_dir = os.path.split(os.path.abspath(__file__))[0]
        suite_name = os.path.splitext(os.path.basename(binary_path))[0]
        data_file = ".".join((suite_name, 'datax'))
        data_file = os.path.join(script_dir, '..', 'mbedtls',
                                 suite_name, data_file)
        if os.path.exists(data_file):
            self.log("Running tests from %s" % data_file)
            parser = TestDataParser()
            parser.parse(data_file)
            self.tests = parser.get_test_data()
            self.print_test_info()
        else:
            self.log("Data file not found: %s" % data_file)
            self.notify_complete(False)

    def print_test_info(self):
        """
        Prints test summary read by Greentea to detect test cases.
        """
        self.log('{{__testcase_count;%d}}' % len(self.tests))
        for name, _, _, _ in self.tests:
            self.log('{{__testcase_name;%s}}' % name)

    @staticmethod
    def align_32bit(data_bytes):
        """
        4 byte aligns input byte array.

        :return:
        """
        data_bytes += bytearray((4 - (len(data_bytes))) % 4)

    @staticmethod
    def hex_str_bytes(hex_str):
        """
        Converts Hex string representation to byte array

        :param hex_str: Hex in string format.
        :return: Output Byte array
        """
        if hex_str[0] != '"' or hex_str[len(hex_str) - 1] != '"':
            raise TestDataParserError("HEX test parameter missing '\"':"
                                      " %s" % hex_str)
        hex_str = hex_str.strip('"')
        if len(hex_str) % 2 != 0:
            raise TestDataParserError("HEX parameter len should be mod of "
                                      "2: %s" % hex_str)

        data_bytes = binascii.unhexlify(hex_str)
        return data_bytes

    @staticmethod
    def int32_to_big_endian_bytes(i):
        """
        Coverts i to byte array in big endian format.

        :param i: Input integer
        :return: Output bytes array in big endian or network order
        """
        data_bytes = bytearray([((i >> x) & 0xff) for x in [24, 16, 8, 0]])
        return data_bytes

    def test_vector_to_bytes(self, function_id, dependencies, parameters):
        """
        Converts test vector into a byte array that can be sent to the target.

        :param function_id: Test Function Identifier
        :param dependencies: Dependency list
        :param parameters: Test function input parameters
        :return: Byte array and its length
        """
        data_bytes = bytearray([len(dependencies)])
        if dependencies:
            data_bytes += bytearray(dependencies)
        data_bytes += bytearray([function_id, len(parameters)])
        for typ, param in parameters:
            if typ == 'int' or typ == 'exp':
                i = int(param)
                data_bytes += 'I' if typ == 'int' else 'E'
                self.align_32bit(data_bytes)
                data_bytes += self.int32_to_big_endian_bytes(i)
            elif typ == 'char*':
                param = param.strip('"')
                i = len(param) + 1  # + 1 for null termination
                data_bytes += 'S'
                self.align_32bit(data_bytes)
                data_bytes += self.int32_to_big_endian_bytes(i)
                data_bytes += bytearray(list(param))
                data_bytes += '\0'   # Null terminate
            elif typ == 'hex':
                binary_data = self.hex_str_bytes(param)
                data_bytes += 'H'
                self.align_32bit(data_bytes)
                i = len(binary_data)
                data_bytes += self.int32_to_big_endian_bytes(i)
                data_bytes += binary_data
        length = self.int32_to_big_endian_bytes(len(data_bytes))
        return data_bytes, length

    def run_next_test(self):
        """
        Fetch next test information and execute the test.

        """
        self.test_index += 1
        self.dep_index = 0
        if self.test_index < len(self.tests):
            name, function_id, dependencies, args = self.tests[self.test_index]
            self.run_test(name, function_id, dependencies, args)
        else:
            self.notify_complete(self.suite_passed)

    def run_test(self, name, function_id, dependencies, args):
        """
        Execute the test on target by sending next test information.

        :param name: Test name
        :param function_id: function identifier
        :param dependencies: Dependencies list
        :param args: test parameters
        :return:
        """
        self.log("Running: %s" % name)

        param_bytes, length = self.test_vector_to_bytes(function_id,
                                                        dependencies, args)
        self.send_kv(length, param_bytes)

    @staticmethod
    def get_result(value):
        """
        Converts result from string type to integer
        :param value: Result code in string
        :return: Integer result code. Value is from the test status
                 constants defined under the MbedTlsTest class.
        """
        try:
            return int(value)
        except ValueError:
            ValueError("Result should return error number. "
                       "Instead received %s" % value)

    @event_callback('GO')
    def on_go(self, _key, _value, _timestamp):
        """
        Sent by the target to start first test.

        :param _key: Event key
        :param _value: Value. ignored
        :param _timestamp: Timestamp ignored.
        :return:
        """
        self.run_next_test()

    @event_callback("R")
    def on_result(self, _key, value, _timestamp):
        """
        Handle result. Prints test start, finish required by Greentea
        to detect test execution.

        :param _key: Event key
        :param value: Value. ignored
        :param _timestamp: Timestamp ignored.
        :return:
        """
        int_val = self.get_result(value)
        name, _, _, _ = self.tests[self.test_index]
        self.log('{{__testcase_start;%s}}' % name)
        self.log('{{__testcase_finish;%s;%d;%d}}' % (name, int_val == 0,
                                                     int_val != 0))
        if int_val != 0:
            self.suite_passed = False
        self.run_next_test()

    @event_callback("F")
    def on_failure(self, _key, value, _timestamp):
        """
        Handles test execution failure. That means dependency not supported or
        Test function not supported. Hence marking test as skipped.

        :param _key: Event key
        :param value: Value. ignored
        :param _timestamp: Timestamp ignored.
        :return:
        """
        int_val = self.get_result(value)
        if int_val in self.error_str:
            err = self.error_str[int_val]
        else:
            err = 'Unknown error'
        # For skip status, do not write {{__testcase_finish;...}}
        self.log("Error: %s" % err)
        self.run_next_test()
