#!/usr/bin/env python3
# Unit test for generate_test_code.py
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
Unit tests for generate_test_code.py
"""


try:
    # Python 2
    from StringIO import StringIO
except ImportError:
    # Python 3
    from io import StringIO
from unittest import TestCase, main as unittest_main
try:
    # Python 2
    from mock import patch
except ImportError:
    # Python 3
    from unittest.mock import patch
from generate_test_code import gen_dependencies, gen_dependencies_one_line
from generate_test_code import gen_function_wrapper, gen_dispatch
from generate_test_code import parse_until_pattern, GeneratorInputError
from generate_test_code import parse_suite_dependencies
from generate_test_code import parse_function_dependencies
from generate_test_code import parse_function_arguments, parse_function_code
from generate_test_code import parse_functions, END_HEADER_REGEX
from generate_test_code import END_SUITE_HELPERS_REGEX, escaped_split
from generate_test_code import parse_test_data, gen_dep_check
from generate_test_code import gen_expression_check, write_dependencies
from generate_test_code import write_parameters, gen_suite_dep_checks
from generate_test_code import gen_from_test_data


class GenDep(TestCase):
    """
    Test suite for function gen_dep()
    """

    def test_dependencies_list(self):
        """
        Test that gen_dep() correctly creates dependencies for given
        dependency list.
        :return:
        """
        dependencies = ['DEP1', 'DEP2']
        dep_start, dep_end = gen_dependencies(dependencies)
        preprocessor1, preprocessor2 = dep_start.splitlines()
        endif1, endif2 = dep_end.splitlines()
        self.assertEqual(preprocessor1, '#if defined(DEP1)',
                         'Preprocessor generated incorrectly')
        self.assertEqual(preprocessor2, '#if defined(DEP2)',
                         'Preprocessor generated incorrectly')
        self.assertEqual(endif1, '#endif /* DEP2 */',
                         'Preprocessor generated incorrectly')
        self.assertEqual(endif2, '#endif /* DEP1 */',
                         'Preprocessor generated incorrectly')

    def test_disabled_dependencies_list(self):
        """
        Test that gen_dep() correctly creates dependencies for given
        dependency list.
        :return:
        """
        dependencies = ['!DEP1', '!DEP2']
        dep_start, dep_end = gen_dependencies(dependencies)
        preprocessor1, preprocessor2 = dep_start.splitlines()
        endif1, endif2 = dep_end.splitlines()
        self.assertEqual(preprocessor1, '#if !defined(DEP1)',
                         'Preprocessor generated incorrectly')
        self.assertEqual(preprocessor2, '#if !defined(DEP2)',
                         'Preprocessor generated incorrectly')
        self.assertEqual(endif1, '#endif /* !DEP2 */',
                         'Preprocessor generated incorrectly')
        self.assertEqual(endif2, '#endif /* !DEP1 */',
                         'Preprocessor generated incorrectly')

    def test_mixed_dependencies_list(self):
        """
        Test that gen_dep() correctly creates dependencies for given
        dependency list.
        :return:
        """
        dependencies = ['!DEP1', 'DEP2']
        dep_start, dep_end = gen_dependencies(dependencies)
        preprocessor1, preprocessor2 = dep_start.splitlines()
        endif1, endif2 = dep_end.splitlines()
        self.assertEqual(preprocessor1, '#if !defined(DEP1)',
                         'Preprocessor generated incorrectly')
        self.assertEqual(preprocessor2, '#if defined(DEP2)',
                         'Preprocessor generated incorrectly')
        self.assertEqual(endif1, '#endif /* DEP2 */',
                         'Preprocessor generated incorrectly')
        self.assertEqual(endif2, '#endif /* !DEP1 */',
                         'Preprocessor generated incorrectly')

    def test_empty_dependencies_list(self):
        """
        Test that gen_dep() correctly creates dependencies for given
        dependency list.
        :return:
        """
        dependencies = []
        dep_start, dep_end = gen_dependencies(dependencies)
        self.assertEqual(dep_start, '', 'Preprocessor generated incorrectly')
        self.assertEqual(dep_end, '', 'Preprocessor generated incorrectly')

    def test_large_dependencies_list(self):
        """
        Test that gen_dep() correctly creates dependencies for given
        dependency list.
        :return:
        """
        dependencies = []
        count = 10
        for i in range(count):
            dependencies.append('DEP%d' % i)
        dep_start, dep_end = gen_dependencies(dependencies)
        self.assertEqual(len(dep_start.splitlines()), count,
                         'Preprocessor generated incorrectly')
        self.assertEqual(len(dep_end.splitlines()), count,
                         'Preprocessor generated incorrectly')


class GenDepOneLine(TestCase):
    """
    Test Suite for testing gen_dependencies_one_line()
    """

    def test_dependencies_list(self):
        """
        Test that gen_dep() correctly creates dependencies for given
        dependency list.
        :return:
        """
        dependencies = ['DEP1', 'DEP2']
        dep_str = gen_dependencies_one_line(dependencies)
        self.assertEqual(dep_str, '#if defined(DEP1) && defined(DEP2)',
                         'Preprocessor generated incorrectly')

    def test_disabled_dependencies_list(self):
        """
        Test that gen_dep() correctly creates dependencies for given
        dependency list.
        :return:
        """
        dependencies = ['!DEP1', '!DEP2']
        dep_str = gen_dependencies_one_line(dependencies)
        self.assertEqual(dep_str, '#if !defined(DEP1) && !defined(DEP2)',
                         'Preprocessor generated incorrectly')

    def test_mixed_dependencies_list(self):
        """
        Test that gen_dep() correctly creates dependencies for given
        dependency list.
        :return:
        """
        dependencies = ['!DEP1', 'DEP2']
        dep_str = gen_dependencies_one_line(dependencies)
        self.assertEqual(dep_str, '#if !defined(DEP1) && defined(DEP2)',
                         'Preprocessor generated incorrectly')

    def test_empty_dependencies_list(self):
        """
        Test that gen_dep() correctly creates dependencies for given
        dependency list.
        :return:
        """
        dependencies = []
        dep_str = gen_dependencies_one_line(dependencies)
        self.assertEqual(dep_str, '', 'Preprocessor generated incorrectly')

    def test_large_dependencies_list(self):
        """
        Test that gen_dep() correctly creates dependencies for given
        dependency list.
        :return:
        """
        dependencies = []
        count = 10
        for i in range(count):
            dependencies.append('DEP%d' % i)
        dep_str = gen_dependencies_one_line(dependencies)
        expected = '#if ' + ' && '.join(['defined(%s)' %
                                         x for x in dependencies])
        self.assertEqual(dep_str, expected,
                         'Preprocessor generated incorrectly')


class GenFunctionWrapper(TestCase):
    """
    Test Suite for testing gen_function_wrapper()
    """

    def test_params_unpack(self):
        """
        Test that params are properly unpacked in the function call.

        :return:
        """
        code = gen_function_wrapper('test_a', '', ('a', 'b', 'c', 'd'))
        expected = '''
void test_a_wrapper( void ** params )
{

    test_a( a, b, c, d );
}
'''
        self.assertEqual(code, expected)

    def test_local(self):
        """
        Test that params are properly unpacked in the function call.

        :return:
        """
        code = gen_function_wrapper('test_a',
                                    'int x = 1;', ('x', 'b', 'c', 'd'))
        expected = '''
void test_a_wrapper( void ** params )
{
int x = 1;
    test_a( x, b, c, d );
}
'''
        self.assertEqual(code, expected)

    def test_empty_params(self):
        """
        Test that params are properly unpacked in the function call.

        :return:
        """
        code = gen_function_wrapper('test_a', '', ())
        expected = '''
void test_a_wrapper( void ** params )
{
    (void)params;

    test_a(  );
}
'''
        self.assertEqual(code, expected)


class GenDispatch(TestCase):
    """
    Test suite for testing gen_dispatch()
    """

    def test_dispatch(self):
        """
        Test that dispatch table entry is generated correctly.
        :return:
        """
        code = gen_dispatch('test_a', ['DEP1', 'DEP2'])
        expected = '''
#if defined(DEP1) && defined(DEP2)
    test_a_wrapper,
#else
    NULL,
#endif
'''
        self.assertEqual(code, expected)

    def test_empty_dependencies(self):
        """
        Test empty dependency list.
        :return:
        """
        code = gen_dispatch('test_a', [])
        expected = '''
    test_a_wrapper,
'''
        self.assertEqual(code, expected)


class StringIOWrapper(StringIO, object):
    """
    file like class to mock file object in tests.
    """
    def __init__(self, file_name, data, line_no=0):
        """
        Init file handle.

        :param file_name:
        :param data:
        :param line_no:
        """
        super(StringIOWrapper, self).__init__(data)
        self.line_no = line_no
        self.name = file_name

    def next(self):
        """
        Iterator method. This method overrides base class's
        next method and extends the next method to count the line
        numbers as each line is read.

        :return: Line read from file.
        """
        parent = super(StringIOWrapper, self)
        if getattr(parent, 'next', None):
            # Python 2
            line = parent.next()
        else:
            # Python 3
            line = parent.__next__()
        return line

    # Python 3
    __next__ = next

    def readline(self, length=0):
        """
        Wrap the base class readline.

        :param length:
        :return:
        """
        line = super(StringIOWrapper, self).readline()
        if line is not None:
            self.line_no += 1
        return line


class ParseUntilPattern(TestCase):
    """
    Test Suite for testing parse_until_pattern().
    """

    def test_suite_headers(self):
        """
        Test that suite headers are parsed correctly.

        :return:
        """
        data = '''#include "mbedtls/ecp.h"

#define ECP_PF_UNKNOWN     -1
/* END_HEADER */
'''
        expected = '''#line 1 "test_suite_ut.function"
#include "mbedtls/ecp.h"

#define ECP_PF_UNKNOWN     -1
'''
        stream = StringIOWrapper('test_suite_ut.function', data, line_no=0)
        headers = parse_until_pattern(stream, END_HEADER_REGEX)
        self.assertEqual(headers, expected)

    def test_line_no(self):
        """
        Test that #line is set to correct line no. in source .function file.

        :return:
        """
        data = '''#include "mbedtls/ecp.h"

#define ECP_PF_UNKNOWN     -1
/* END_HEADER */
'''
        offset_line_no = 5
        expected = '''#line %d "test_suite_ut.function"
#include "mbedtls/ecp.h"

#define ECP_PF_UNKNOWN     -1
''' % (offset_line_no + 1)
        stream = StringIOWrapper('test_suite_ut.function', data,
                                 offset_line_no)
        headers = parse_until_pattern(stream, END_HEADER_REGEX)
        self.assertEqual(headers, expected)

    def test_no_end_header_comment(self):
        """
        Test that InvalidFileFormat is raised when end header comment is
        missing.
        :return:
        """
        data = '''#include "mbedtls/ecp.h"

#define ECP_PF_UNKNOWN     -1

'''
        stream = StringIOWrapper('test_suite_ut.function', data)
        self.assertRaises(GeneratorInputError, parse_until_pattern, stream,
                          END_HEADER_REGEX)


class ParseSuiteDependencies(TestCase):
    """
    Test Suite for testing parse_suite_dependencies().
    """

    def test_suite_dependencies(self):
        """

        :return:
        """
        data = '''
 * depends_on:MBEDTLS_ECP_C
 * END_DEPENDENCIES
 */
'''
        expected = ['MBEDTLS_ECP_C']
        stream = StringIOWrapper('test_suite_ut.function', data)
        dependencies = parse_suite_dependencies(stream)
        self.assertEqual(dependencies, expected)

    def test_no_end_dep_comment(self):
        """
        Test that InvalidFileFormat is raised when end dep comment is missing.
        :return:
        """
        data = '''
* depends_on:MBEDTLS_ECP_C
'''
        stream = StringIOWrapper('test_suite_ut.function', data)
        self.assertRaises(GeneratorInputError, parse_suite_dependencies,
                          stream)

    def test_dependencies_split(self):
        """
        Test that InvalidFileFormat is raised when end dep comment is missing.
        :return:
        """
        data = '''
 * depends_on:MBEDTLS_ECP_C:A:B:   C  : D :F : G: !H
 * END_DEPENDENCIES
 */
'''
        expected = ['MBEDTLS_ECP_C', 'A', 'B', 'C', 'D', 'F', 'G', '!H']
        stream = StringIOWrapper('test_suite_ut.function', data)
        dependencies = parse_suite_dependencies(stream)
        self.assertEqual(dependencies, expected)


class ParseFuncDependencies(TestCase):
    """
    Test Suite for testing parse_function_dependencies()
    """

    def test_function_dependencies(self):
        """
        Test that parse_function_dependencies() correctly parses function
        dependencies.
        :return:
        """
        line = '/* BEGIN_CASE ' \
               'depends_on:MBEDTLS_ENTROPY_NV_SEED:MBEDTLS_FS_IO */'
        expected = ['MBEDTLS_ENTROPY_NV_SEED', 'MBEDTLS_FS_IO']
        dependencies = parse_function_dependencies(line)
        self.assertEqual(dependencies, expected)

    def test_no_dependencies(self):
        """
        Test that parse_function_dependencies() correctly parses function
        dependencies.
        :return:
        """
        line = '/* BEGIN_CASE */'
        dependencies = parse_function_dependencies(line)
        self.assertEqual(dependencies, [])

    def test_tolerance(self):
        """
        Test that parse_function_dependencies() correctly parses function
        dependencies.
        :return:
        """
        line = '/* BEGIN_CASE depends_on:MBEDTLS_FS_IO: A : !B:C : F*/'
        dependencies = parse_function_dependencies(line)
        self.assertEqual(dependencies, ['MBEDTLS_FS_IO', 'A', '!B', 'C', 'F'])


class ParseFuncSignature(TestCase):
    """
    Test Suite for parse_function_arguments().
    """

    def test_int_and_char_params(self):
        """
        Test int and char parameters parsing
        :return:
        """
        line = 'void entropy_threshold( char * a, int b, int result )'
        args, local, arg_dispatch = parse_function_arguments(line)
        self.assertEqual(args, ['char*', 'int', 'int'])
        self.assertEqual(local, '')
        self.assertEqual(arg_dispatch, ['(char *) params[0]',
                                        '*( (int *) params[1] )',
                                        '*( (int *) params[2] )'])

    def test_hex_params(self):
        """
        Test hex parameters parsing
        :return:
        """
        line = 'void entropy_threshold( char * a, data_t * h, int result )'
        args, local, arg_dispatch = parse_function_arguments(line)
        self.assertEqual(args, ['char*', 'hex', 'int'])
        self.assertEqual(local,
                         '    data_t data1 = {(uint8_t *) params[1], '
                         '*( (uint32_t *) params[2] )};\n')
        self.assertEqual(arg_dispatch, ['(char *) params[0]',
                                        '&data1',
                                        '*( (int *) params[3] )'])

    def test_unsupported_arg(self):
        """
        Test unsupported arguments (not among int, char * and data_t)
        :return:
        """
        line = 'void entropy_threshold( char * a, data_t * h, char result )'
        self.assertRaises(ValueError, parse_function_arguments, line)

    def test_no_params(self):
        """
        Test no parameters.
        :return:
        """
        line = 'void entropy_threshold()'
        args, local, arg_dispatch = parse_function_arguments(line)
        self.assertEqual(args, [])
        self.assertEqual(local, '')
        self.assertEqual(arg_dispatch, [])


class ParseFunctionCode(TestCase):
    """
    Test suite for testing parse_function_code()
    """

    def assert_raises_regex(self, exp, regex, func, *args):
        """
        Python 2 & 3 portable wrapper of assertRaisesRegex(p)? function.

        :param exp: Exception type expected to be raised by cb.
        :param regex: Expected exception message
        :param func: callable object under test
        :param args: variable positional arguments
        """
        parent = super(ParseFunctionCode, self)

        # Pylint does not appreciate that the super method called
        # conditionally can be available in other Python version
        # then that of Pylint.
        # Workaround is to call the method via getattr.
        # Pylint ignores that the method got via getattr is
        # conditionally executed. Method has to be a callable.
        # Hence, using a dummy callable for getattr default.
        dummy = lambda *x: None
        # First Python 3 assertRaisesRegex is checked, since Python 2
        # assertRaisesRegexp is also available in Python 3 but is
        # marked deprecated.
        for name in ('assertRaisesRegex', 'assertRaisesRegexp'):
            method = getattr(parent, name, dummy)
            if method is not dummy:
                method(exp, regex, func, *args)
                break
        else:
            raise AttributeError(" 'ParseFunctionCode' object has no attribute"
                                 " 'assertRaisesRegex' or 'assertRaisesRegexp'"
                                )

    def test_no_function(self):
        """
        Test no test function found.
        :return:
        """
        data = '''
No
test
function
'''
        stream = StringIOWrapper('test_suite_ut.function', data)
        err_msg = 'file: test_suite_ut.function - Test functions not found!'
        self.assert_raises_regex(GeneratorInputError, err_msg,
                                 parse_function_code, stream, [], [])

    def test_no_end_case_comment(self):
        """
        Test missing end case.
        :return:
        """
        data = '''
void test_func()
{
}
'''
        stream = StringIOWrapper('test_suite_ut.function', data)
        err_msg = r'file: test_suite_ut.function - '\
                  'end case pattern .*? not found!'
        self.assert_raises_regex(GeneratorInputError, err_msg,
                                 parse_function_code, stream, [], [])

    @patch("generate_test_code.parse_function_arguments")
    def test_function_called(self,
                             parse_function_arguments_mock):
        """
        Test parse_function_code()
        :return:
        """
        parse_function_arguments_mock.return_value = ([], '', [])
        data = '''
void test_func()
{
}
'''
        stream = StringIOWrapper('test_suite_ut.function', data)
        self.assertRaises(GeneratorInputError, parse_function_code,
                          stream, [], [])
        self.assertTrue(parse_function_arguments_mock.called)
        parse_function_arguments_mock.assert_called_with('void test_func()\n')

    @patch("generate_test_code.gen_dispatch")
    @patch("generate_test_code.gen_dependencies")
    @patch("generate_test_code.gen_function_wrapper")
    @patch("generate_test_code.parse_function_arguments")
    def test_return(self, parse_function_arguments_mock,
                    gen_function_wrapper_mock,
                    gen_dependencies_mock,
                    gen_dispatch_mock):
        """
        Test generated code.
        :return:
        """
        parse_function_arguments_mock.return_value = ([], '', [])
        gen_function_wrapper_mock.return_value = ''
        gen_dependencies_mock.side_effect = gen_dependencies
        gen_dispatch_mock.side_effect = gen_dispatch
        data = '''
void func()
{
    ba ba black sheep
    have you any wool
}
/* END_CASE */
'''
        stream = StringIOWrapper('test_suite_ut.function', data)
        name, arg, code, dispatch_code = parse_function_code(stream, [], [])

        self.assertTrue(parse_function_arguments_mock.called)
        parse_function_arguments_mock.assert_called_with('void func()\n')
        gen_function_wrapper_mock.assert_called_with('test_func', '', [])
        self.assertEqual(name, 'test_func')
        self.assertEqual(arg, [])
        expected = '''#line 1 "test_suite_ut.function"

void test_func()
{
    ba ba black sheep
    have you any wool
exit:
    ;
}
'''
        self.assertEqual(code, expected)
        self.assertEqual(dispatch_code, "\n    test_func_wrapper,\n")

    @patch("generate_test_code.gen_dispatch")
    @patch("generate_test_code.gen_dependencies")
    @patch("generate_test_code.gen_function_wrapper")
    @patch("generate_test_code.parse_function_arguments")
    def test_with_exit_label(self, parse_function_arguments_mock,
                             gen_function_wrapper_mock,
                             gen_dependencies_mock,
                             gen_dispatch_mock):
        """
        Test when exit label is present.
        :return:
        """
        parse_function_arguments_mock.return_value = ([], '', [])
        gen_function_wrapper_mock.return_value = ''
        gen_dependencies_mock.side_effect = gen_dependencies
        gen_dispatch_mock.side_effect = gen_dispatch
        data = '''
void func()
{
    ba ba black sheep
    have you any wool
exit:
    yes sir yes sir
    3 bags full
}
/* END_CASE */
'''
        stream = StringIOWrapper('test_suite_ut.function', data)
        _, _, code, _ = parse_function_code(stream, [], [])

        expected = '''#line 1 "test_suite_ut.function"

void test_func()
{
    ba ba black sheep
    have you any wool
exit:
    yes sir yes sir
    3 bags full
}
'''
        self.assertEqual(code, expected)

    def test_non_void_function(self):
        """
        Test invalid signature (non void).
        :return:
        """
        data = 'int entropy_threshold( char * a, data_t * h, int result )'
        err_msg = 'file: test_suite_ut.function - Test functions not found!'
        stream = StringIOWrapper('test_suite_ut.function', data)
        self.assert_raises_regex(GeneratorInputError, err_msg,
                                 parse_function_code, stream, [], [])

    @patch("generate_test_code.gen_dispatch")
    @patch("generate_test_code.gen_dependencies")
    @patch("generate_test_code.gen_function_wrapper")
    @patch("generate_test_code.parse_function_arguments")
    def test_functio_name_on_newline(self, parse_function_arguments_mock,
                                     gen_function_wrapper_mock,
                                     gen_dependencies_mock,
                                     gen_dispatch_mock):
        """
        Test when exit label is present.
        :return:
        """
        parse_function_arguments_mock.return_value = ([], '', [])
        gen_function_wrapper_mock.return_value = ''
        gen_dependencies_mock.side_effect = gen_dependencies
        gen_dispatch_mock.side_effect = gen_dispatch
        data = '''
void


func()
{
    ba ba black sheep
    have you any wool
exit:
    yes sir yes sir
    3 bags full
}
/* END_CASE */
'''
        stream = StringIOWrapper('test_suite_ut.function', data)
        _, _, code, _ = parse_function_code(stream, [], [])

        expected = '''#line 1 "test_suite_ut.function"

void


test_func()
{
    ba ba black sheep
    have you any wool
exit:
    yes sir yes sir
    3 bags full
}
'''
        self.assertEqual(code, expected)


class ParseFunction(TestCase):
    """
    Test Suite for testing parse_functions()
    """

    @patch("generate_test_code.parse_until_pattern")
    def test_begin_header(self, parse_until_pattern_mock):
        """
        Test that begin header is checked and parse_until_pattern() is called.
        :return:
        """
        def stop(*_unused):
            """Stop when parse_until_pattern is called."""
            raise Exception
        parse_until_pattern_mock.side_effect = stop
        data = '''/* BEGIN_HEADER */
#include "mbedtls/ecp.h"

#define ECP_PF_UNKNOWN     -1
/* END_HEADER */
'''
        stream = StringIOWrapper('test_suite_ut.function', data)
        self.assertRaises(Exception, parse_functions, stream)
        parse_until_pattern_mock.assert_called_with(stream, END_HEADER_REGEX)
        self.assertEqual(stream.line_no, 1)

    @patch("generate_test_code.parse_until_pattern")
    def test_begin_helper(self, parse_until_pattern_mock):
        """
        Test that begin helper is checked and parse_until_pattern() is called.
        :return:
        """
        def stop(*_unused):
            """Stop when parse_until_pattern is called."""
            raise Exception
        parse_until_pattern_mock.side_effect = stop
        data = '''/* BEGIN_SUITE_HELPERS */
void print_hello_world()
{
    printf("Hello World!\n");
}
/* END_SUITE_HELPERS */
'''
        stream = StringIOWrapper('test_suite_ut.function', data)
        self.assertRaises(Exception, parse_functions, stream)
        parse_until_pattern_mock.assert_called_with(stream,
                                                    END_SUITE_HELPERS_REGEX)
        self.assertEqual(stream.line_no, 1)

    @patch("generate_test_code.parse_suite_dependencies")
    def test_begin_dep(self, parse_suite_dependencies_mock):
        """
        Test that begin dep is checked and parse_suite_dependencies() is
        called.
        :return:
        """
        def stop(*_unused):
            """Stop when parse_until_pattern is called."""
            raise Exception
        parse_suite_dependencies_mock.side_effect = stop
        data = '''/* BEGIN_DEPENDENCIES
 * depends_on:MBEDTLS_ECP_C
 * END_DEPENDENCIES
 */
'''
        stream = StringIOWrapper('test_suite_ut.function', data)
        self.assertRaises(Exception, parse_functions, stream)
        parse_suite_dependencies_mock.assert_called_with(stream)
        self.assertEqual(stream.line_no, 1)

    @patch("generate_test_code.parse_function_dependencies")
    def test_begin_function_dep(self, func_mock):
        """
        Test that begin dep is checked and parse_function_dependencies() is
        called.
        :return:
        """
        def stop(*_unused):
            """Stop when parse_until_pattern is called."""
            raise Exception
        func_mock.side_effect = stop

        dependencies_str = '/* BEGIN_CASE ' \
            'depends_on:MBEDTLS_ENTROPY_NV_SEED:MBEDTLS_FS_IO */\n'
        data = '''%svoid test_func()
{
}
''' % dependencies_str
        stream = StringIOWrapper('test_suite_ut.function', data)
        self.assertRaises(Exception, parse_functions, stream)
        func_mock.assert_called_with(dependencies_str)
        self.assertEqual(stream.line_no, 1)

    @patch("generate_test_code.parse_function_code")
    @patch("generate_test_code.parse_function_dependencies")
    def test_return(self, func_mock1, func_mock2):
        """
        Test that begin case is checked and parse_function_code() is called.
        :return:
        """
        func_mock1.return_value = []
        in_func_code = '''void test_func()
{
}
'''
        func_dispatch = '''
    test_func_wrapper,
'''
        func_mock2.return_value = 'test_func', [],\
            in_func_code, func_dispatch
        dependencies_str = '/* BEGIN_CASE ' \
            'depends_on:MBEDTLS_ENTROPY_NV_SEED:MBEDTLS_FS_IO */\n'
        data = '''%svoid test_func()
{
}
''' % dependencies_str
        stream = StringIOWrapper('test_suite_ut.function', data)
        suite_dependencies, dispatch_code, func_code, func_info = \
            parse_functions(stream)
        func_mock1.assert_called_with(dependencies_str)
        func_mock2.assert_called_with(stream, [], [])
        self.assertEqual(stream.line_no, 5)
        self.assertEqual(suite_dependencies, [])
        expected_dispatch_code = '''/* Function Id: 0 */

    test_func_wrapper,
'''
        self.assertEqual(dispatch_code, expected_dispatch_code)
        self.assertEqual(func_code, in_func_code)
        self.assertEqual(func_info, {'test_func': (0, [])})

    def test_parsing(self):
        """
        Test case parsing.
        :return:
        """
        data = '''/* BEGIN_HEADER */
#include "mbedtls/ecp.h"

#define ECP_PF_UNKNOWN     -1
/* END_HEADER */

/* BEGIN_DEPENDENCIES
 * depends_on:MBEDTLS_ECP_C
 * END_DEPENDENCIES
 */

/* BEGIN_CASE depends_on:MBEDTLS_ENTROPY_NV_SEED:MBEDTLS_FS_IO */
void func1()
{
}
/* END_CASE */

/* BEGIN_CASE depends_on:MBEDTLS_ENTROPY_NV_SEED:MBEDTLS_FS_IO */
void func2()
{
}
/* END_CASE */
'''
        stream = StringIOWrapper('test_suite_ut.function', data)
        suite_dependencies, dispatch_code, func_code, func_info = \
            parse_functions(stream)
        self.assertEqual(stream.line_no, 23)
        self.assertEqual(suite_dependencies, ['MBEDTLS_ECP_C'])

        expected_dispatch_code = '''/* Function Id: 0 */

#if defined(MBEDTLS_ECP_C) && defined(MBEDTLS_ENTROPY_NV_SEED) && defined(MBEDTLS_FS_IO)
    test_func1_wrapper,
#else
    NULL,
#endif
/* Function Id: 1 */

#if defined(MBEDTLS_ECP_C) && defined(MBEDTLS_ENTROPY_NV_SEED) && defined(MBEDTLS_FS_IO)
    test_func2_wrapper,
#else
    NULL,
#endif
'''
        self.assertEqual(dispatch_code, expected_dispatch_code)
        expected_func_code = '''#if defined(MBEDTLS_ECP_C)
#line 2 "test_suite_ut.function"
#include "mbedtls/ecp.h"

#define ECP_PF_UNKNOWN     -1
#if defined(MBEDTLS_ENTROPY_NV_SEED)
#if defined(MBEDTLS_FS_IO)
#line 13 "test_suite_ut.function"
void test_func1()
{
exit:
    ;
}

void test_func1_wrapper( void ** params )
{
    (void)params;

    test_func1(  );
}
#endif /* MBEDTLS_FS_IO */
#endif /* MBEDTLS_ENTROPY_NV_SEED */
#if defined(MBEDTLS_ENTROPY_NV_SEED)
#if defined(MBEDTLS_FS_IO)
#line 19 "test_suite_ut.function"
void test_func2()
{
exit:
    ;
}

void test_func2_wrapper( void ** params )
{
    (void)params;

    test_func2(  );
}
#endif /* MBEDTLS_FS_IO */
#endif /* MBEDTLS_ENTROPY_NV_SEED */
#endif /* MBEDTLS_ECP_C */
'''
        self.assertEqual(func_code, expected_func_code)
        self.assertEqual(func_info, {'test_func1': (0, []),
                                     'test_func2': (1, [])})

    def test_same_function_name(self):
        """
        Test name conflict.
        :return:
        """
        data = '''/* BEGIN_HEADER */
#include "mbedtls/ecp.h"

#define ECP_PF_UNKNOWN     -1
/* END_HEADER */

/* BEGIN_DEPENDENCIES
 * depends_on:MBEDTLS_ECP_C
 * END_DEPENDENCIES
 */

/* BEGIN_CASE depends_on:MBEDTLS_ENTROPY_NV_SEED:MBEDTLS_FS_IO */
void func()
{
}
/* END_CASE */

/* BEGIN_CASE depends_on:MBEDTLS_ENTROPY_NV_SEED:MBEDTLS_FS_IO */
void func()
{
}
/* END_CASE */
'''
        stream = StringIOWrapper('test_suite_ut.function', data)
        self.assertRaises(GeneratorInputError, parse_functions, stream)


class EscapedSplit(TestCase):
    """
    Test suite for testing escaped_split().
    Note: Since escaped_split() output is used to write back to the
    intermediate data file. Any escape characters in the input are
    retained in the output.
    """

    def test_invalid_input(self):
        """
        Test when input split character is not a character.
        :return:
        """
        self.assertRaises(ValueError, escaped_split, '', 'string')

    def test_empty_string(self):
        """
        Test empty string input.
        :return:
        """
        splits = escaped_split('', ':')
        self.assertEqual(splits, [])

    def test_no_escape(self):
        """
        Test with no escape character. The behaviour should be same as
        str.split()
        :return:
        """
        test_str = 'yahoo:google'
        splits = escaped_split(test_str, ':')
        self.assertEqual(splits, test_str.split(':'))

    def test_escaped_input(self):
        """
        Test input that has escaped delimiter.
        :return:
        """
        test_str = r'yahoo\:google:facebook'
        splits = escaped_split(test_str, ':')
        self.assertEqual(splits, [r'yahoo\:google', 'facebook'])

    def test_escaped_escape(self):
        """
        Test input that has escaped delimiter.
        :return:
        """
        test_str = r'yahoo\\:google:facebook'
        splits = escaped_split(test_str, ':')
        self.assertEqual(splits, [r'yahoo\\', 'google', 'facebook'])

    def test_all_at_once(self):
        """
        Test input that has escaped delimiter.
        :return:
        """
        test_str = r'yahoo\\:google:facebook\:instagram\\:bbc\\:wikipedia'
        splits = escaped_split(test_str, ':')
        self.assertEqual(splits, [r'yahoo\\', r'google',
                                  r'facebook\:instagram\\',
                                  r'bbc\\', r'wikipedia'])


class ParseTestData(TestCase):
    """
    Test suite for parse test data.
    """

    def test_parser(self):
        """
        Test that tests are parsed correctly from data file.
        :return:
        """
        data = """
Diffie-Hellman full exchange #1
dhm_do_dhm:10:"23":10:"5"

Diffie-Hellman full exchange #2
dhm_do_dhm:10:"93450983094850938450983409623":10:"9345098304850938450983409622"

Diffie-Hellman full exchange #3
dhm_do_dhm:10:"9345098382739712938719287391879381271":10:"9345098792137312973297123912791271"

Diffie-Hellman selftest
dhm_selftest:
"""
        stream = StringIOWrapper('test_suite_ut.function', data)
        tests = [(name, test_function, dependencies, args)
                 for name, test_function, dependencies, args in
                 parse_test_data(stream)]
        test1, test2, test3, test4 = tests
        self.assertEqual(test1[0], 'Diffie-Hellman full exchange #1')
        self.assertEqual(test1[1], 'dhm_do_dhm')
        self.assertEqual(test1[2], [])
        self.assertEqual(test1[3], ['10', '"23"', '10', '"5"'])

        self.assertEqual(test2[0], 'Diffie-Hellman full exchange #2')
        self.assertEqual(test2[1], 'dhm_do_dhm')
        self.assertEqual(test2[2], [])
        self.assertEqual(test2[3], ['10', '"93450983094850938450983409623"',
                                    '10', '"9345098304850938450983409622"'])

        self.assertEqual(test3[0], 'Diffie-Hellman full exchange #3')
        self.assertEqual(test3[1], 'dhm_do_dhm')
        self.assertEqual(test3[2], [])
        self.assertEqual(test3[3], ['10',
                                    '"9345098382739712938719287391879381271"',
                                    '10',
                                    '"9345098792137312973297123912791271"'])

        self.assertEqual(test4[0], 'Diffie-Hellman selftest')
        self.assertEqual(test4[1], 'dhm_selftest')
        self.assertEqual(test4[2], [])
        self.assertEqual(test4[3], [])

    def test_with_dependencies(self):
        """
        Test that tests with dependencies are parsed.
        :return:
        """
        data = """
Diffie-Hellman full exchange #1
depends_on:YAHOO
dhm_do_dhm:10:"23":10:"5"

Diffie-Hellman full exchange #2
dhm_do_dhm:10:"93450983094850938450983409623":10:"9345098304850938450983409622"

"""
        stream = StringIOWrapper('test_suite_ut.function', data)
        tests = [(name, function_name, dependencies, args)
                 for name, function_name, dependencies, args in
                 parse_test_data(stream)]
        test1, test2 = tests
        self.assertEqual(test1[0], 'Diffie-Hellman full exchange #1')
        self.assertEqual(test1[1], 'dhm_do_dhm')
        self.assertEqual(test1[2], ['YAHOO'])
        self.assertEqual(test1[3], ['10', '"23"', '10', '"5"'])

        self.assertEqual(test2[0], 'Diffie-Hellman full exchange #2')
        self.assertEqual(test2[1], 'dhm_do_dhm')
        self.assertEqual(test2[2], [])
        self.assertEqual(test2[3], ['10', '"93450983094850938450983409623"',
                                    '10', '"9345098304850938450983409622"'])

    def test_no_args(self):
        """
        Test GeneratorInputError is raised when test function name and
        args line is missing.
        :return:
        """
        data = """
Diffie-Hellman full exchange #1
depends_on:YAHOO


Diffie-Hellman full exchange #2
dhm_do_dhm:10:"93450983094850938450983409623":10:"9345098304850938450983409622"

"""
        stream = StringIOWrapper('test_suite_ut.function', data)
        err = None
        try:
            for _, _, _, _ in parse_test_data(stream):
                pass
        except GeneratorInputError as err:
            self.assertEqual(type(err), GeneratorInputError)

    def test_incomplete_data(self):
        """
        Test GeneratorInputError is raised when test function name
        and args line is missing.
        :return:
        """
        data = """
Diffie-Hellman full exchange #1
depends_on:YAHOO
"""
        stream = StringIOWrapper('test_suite_ut.function', data)
        err = None
        try:
            for _, _, _, _ in parse_test_data(stream):
                pass
        except GeneratorInputError as err:
            self.assertEqual(type(err), GeneratorInputError)


class GenDepCheck(TestCase):
    """
    Test suite for gen_dep_check(). It is assumed this function is
    called with valid inputs.
    """

    def test_gen_dep_check(self):
        """
        Test that dependency check code generated correctly.
        :return:
        """
        expected = """
        case 5:
            {
#if defined(YAHOO)
                ret = DEPENDENCY_SUPPORTED;
#else
                ret = DEPENDENCY_NOT_SUPPORTED;
#endif
            }
            break;"""
        out = gen_dep_check(5, 'YAHOO')
        self.assertEqual(out, expected)

    def test_not_defined_dependency(self):
        """
        Test dependency with !.
        :return:
        """
        expected = """
        case 5:
            {
#if !defined(YAHOO)
                ret = DEPENDENCY_SUPPORTED;
#else
                ret = DEPENDENCY_NOT_SUPPORTED;
#endif
            }
            break;"""
        out = gen_dep_check(5, '!YAHOO')
        self.assertEqual(out, expected)

    def test_empty_dependency(self):
        """
        Test invalid dependency input.
        :return:
        """
        self.assertRaises(GeneratorInputError, gen_dep_check, 5, '!')

    def test_negative_dep_id(self):
        """
        Test invalid dependency input.
        :return:
        """
        self.assertRaises(GeneratorInputError, gen_dep_check, -1, 'YAHOO')


class GenExpCheck(TestCase):
    """
    Test suite for gen_expression_check(). It is assumed this function
    is called with valid inputs.
    """

    def test_gen_exp_check(self):
        """
        Test that expression check code generated correctly.
        :return:
        """
        expected = """
        case 5:
            {
                *out_value = YAHOO;
            }
            break;"""
        out = gen_expression_check(5, 'YAHOO')
        self.assertEqual(out, expected)

    def test_invalid_expression(self):
        """
        Test invalid expression input.
        :return:
        """
        self.assertRaises(GeneratorInputError, gen_expression_check, 5, '')

    def test_negative_exp_id(self):
        """
        Test invalid expression id.
        :return:
        """
        self.assertRaises(GeneratorInputError, gen_expression_check,
                          -1, 'YAHOO')


class WriteDependencies(TestCase):
    """
    Test suite for testing write_dependencies.
    """

    def test_no_test_dependencies(self):
        """
        Test when test dependencies input is empty.
        :return:
        """
        stream = StringIOWrapper('test_suite_ut.data', '')
        unique_dependencies = []
        dep_check_code = write_dependencies(stream, [], unique_dependencies)
        self.assertEqual(dep_check_code, '')
        self.assertEqual(len(unique_dependencies), 0)
        self.assertEqual(stream.getvalue(), '')

    def test_unique_dep_ids(self):
        """

        :return:
        """
        stream = StringIOWrapper('test_suite_ut.data', '')
        unique_dependencies = []
        dep_check_code = write_dependencies(stream, ['DEP3', 'DEP2', 'DEP1'],
                                            unique_dependencies)
        expect_dep_check_code = '''
        case 0:
            {
#if defined(DEP3)
                ret = DEPENDENCY_SUPPORTED;
#else
                ret = DEPENDENCY_NOT_SUPPORTED;
#endif
            }
            break;
        case 1:
            {
#if defined(DEP2)
                ret = DEPENDENCY_SUPPORTED;
#else
                ret = DEPENDENCY_NOT_SUPPORTED;
#endif
            }
            break;
        case 2:
            {
#if defined(DEP1)
                ret = DEPENDENCY_SUPPORTED;
#else
                ret = DEPENDENCY_NOT_SUPPORTED;
#endif
            }
            break;'''
        self.assertEqual(dep_check_code, expect_dep_check_code)
        self.assertEqual(len(unique_dependencies), 3)
        self.assertEqual(stream.getvalue(), 'depends_on:0:1:2\n')

    def test_dep_id_repeat(self):
        """

        :return:
        """
        stream = StringIOWrapper('test_suite_ut.data', '')
        unique_dependencies = []
        dep_check_code = ''
        dep_check_code += write_dependencies(stream, ['DEP3', 'DEP2'],
                                             unique_dependencies)
        dep_check_code += write_dependencies(stream, ['DEP2', 'DEP1'],
                                             unique_dependencies)
        dep_check_code += write_dependencies(stream, ['DEP1', 'DEP3'],
                                             unique_dependencies)
        expect_dep_check_code = '''
        case 0:
            {
#if defined(DEP3)
                ret = DEPENDENCY_SUPPORTED;
#else
                ret = DEPENDENCY_NOT_SUPPORTED;
#endif
            }
            break;
        case 1:
            {
#if defined(DEP2)
                ret = DEPENDENCY_SUPPORTED;
#else
                ret = DEPENDENCY_NOT_SUPPORTED;
#endif
            }
            break;
        case 2:
            {
#if defined(DEP1)
                ret = DEPENDENCY_SUPPORTED;
#else
                ret = DEPENDENCY_NOT_SUPPORTED;
#endif
            }
            break;'''
        self.assertEqual(dep_check_code, expect_dep_check_code)
        self.assertEqual(len(unique_dependencies), 3)
        self.assertEqual(stream.getvalue(),
                         'depends_on:0:1\ndepends_on:1:2\ndepends_on:2:0\n')


class WriteParams(TestCase):
    """
    Test Suite for testing write_parameters().
    """

    def test_no_params(self):
        """
        Test with empty test_args
        :return:
        """
        stream = StringIOWrapper('test_suite_ut.data', '')
        unique_expressions = []
        expression_code = write_parameters(stream, [], [], unique_expressions)
        self.assertEqual(len(unique_expressions), 0)
        self.assertEqual(expression_code, '')
        self.assertEqual(stream.getvalue(), '\n')

    def test_no_exp_param(self):
        """
        Test when there is no macro or expression in the params.
        :return:
        """
        stream = StringIOWrapper('test_suite_ut.data', '')
        unique_expressions = []
        expression_code = write_parameters(stream, ['"Yahoo"', '"abcdef00"',
                                                    '0'],
                                           ['char*', 'hex', 'int'],
                                           unique_expressions)
        self.assertEqual(len(unique_expressions), 0)
        self.assertEqual(expression_code, '')
        self.assertEqual(stream.getvalue(),
                         ':char*:"Yahoo":hex:"abcdef00":int:0\n')

    def test_hex_format_int_param(self):
        """
        Test int parameter in hex format.
        :return:
        """
        stream = StringIOWrapper('test_suite_ut.data', '')
        unique_expressions = []
        expression_code = write_parameters(stream,
                                           ['"Yahoo"', '"abcdef00"', '0xAA'],
                                           ['char*', 'hex', 'int'],
                                           unique_expressions)
        self.assertEqual(len(unique_expressions), 0)
        self.assertEqual(expression_code, '')
        self.assertEqual(stream.getvalue(),
                         ':char*:"Yahoo":hex:"abcdef00":int:0xAA\n')

    def test_with_exp_param(self):
        """
        Test when there is macro or expression in the params.
        :return:
        """
        stream = StringIOWrapper('test_suite_ut.data', '')
        unique_expressions = []
        expression_code = write_parameters(stream,
                                           ['"Yahoo"', '"abcdef00"', '0',
                                            'MACRO1', 'MACRO2', 'MACRO3'],
                                           ['char*', 'hex', 'int',
                                            'int', 'int', 'int'],
                                           unique_expressions)
        self.assertEqual(len(unique_expressions), 3)
        self.assertEqual(unique_expressions, ['MACRO1', 'MACRO2', 'MACRO3'])
        expected_expression_code = '''
        case 0:
            {
                *out_value = MACRO1;
            }
            break;
        case 1:
            {
                *out_value = MACRO2;
            }
            break;
        case 2:
            {
                *out_value = MACRO3;
            }
            break;'''
        self.assertEqual(expression_code, expected_expression_code)
        self.assertEqual(stream.getvalue(),
                         ':char*:"Yahoo":hex:"abcdef00":int:0:exp:0:exp:1'
                         ':exp:2\n')

    def test_with_repeat_calls(self):
        """
        Test when write_parameter() is called with same macro or expression.
        :return:
        """
        stream = StringIOWrapper('test_suite_ut.data', '')
        unique_expressions = []
        expression_code = ''
        expression_code += write_parameters(stream,
                                            ['"Yahoo"', 'MACRO1', 'MACRO2'],
                                            ['char*', 'int', 'int'],
                                            unique_expressions)
        expression_code += write_parameters(stream,
                                            ['"abcdef00"', 'MACRO2', 'MACRO3'],
                                            ['hex', 'int', 'int'],
                                            unique_expressions)
        expression_code += write_parameters(stream,
                                            ['0', 'MACRO3', 'MACRO1'],
                                            ['int', 'int', 'int'],
                                            unique_expressions)
        self.assertEqual(len(unique_expressions), 3)
        self.assertEqual(unique_expressions, ['MACRO1', 'MACRO2', 'MACRO3'])
        expected_expression_code = '''
        case 0:
            {
                *out_value = MACRO1;
            }
            break;
        case 1:
            {
                *out_value = MACRO2;
            }
            break;
        case 2:
            {
                *out_value = MACRO3;
            }
            break;'''
        self.assertEqual(expression_code, expected_expression_code)
        expected_data_file = ''':char*:"Yahoo":exp:0:exp:1
:hex:"abcdef00":exp:1:exp:2
:int:0:exp:2:exp:0
'''
        self.assertEqual(stream.getvalue(), expected_data_file)


class GenTestSuiteDependenciesChecks(TestCase):
    """
    Test suite for testing gen_suite_dep_checks()
    """
    def test_empty_suite_dependencies(self):
        """
        Test with empty suite_dependencies list.

        :return:
        """
        dep_check_code, expression_code = \
            gen_suite_dep_checks([], 'DEP_CHECK_CODE', 'EXPRESSION_CODE')
        self.assertEqual(dep_check_code, 'DEP_CHECK_CODE')
        self.assertEqual(expression_code, 'EXPRESSION_CODE')

    def test_suite_dependencies(self):
        """
        Test with suite_dependencies list.

        :return:
        """
        dep_check_code, expression_code = \
            gen_suite_dep_checks(['SUITE_DEP'], 'DEP_CHECK_CODE',
                                 'EXPRESSION_CODE')
        expected_dep_check_code = '''
#if defined(SUITE_DEP)
DEP_CHECK_CODE
#endif
'''
        expected_expression_code = '''
#if defined(SUITE_DEP)
EXPRESSION_CODE
#endif
'''
        self.assertEqual(dep_check_code, expected_dep_check_code)
        self.assertEqual(expression_code, expected_expression_code)

    def test_no_dep_no_exp(self):
        """
        Test when there are no dependency and expression code.
        :return:
        """
        dep_check_code, expression_code = gen_suite_dep_checks([], '', '')
        self.assertEqual(dep_check_code, '')
        self.assertEqual(expression_code, '')


class GenFromTestData(TestCase):
    """
    Test suite for gen_from_test_data()
    """

    @staticmethod
    @patch("generate_test_code.write_dependencies")
    @patch("generate_test_code.write_parameters")
    @patch("generate_test_code.gen_suite_dep_checks")
    def test_intermediate_data_file(func_mock1,
                                    write_parameters_mock,
                                    write_dependencies_mock):
        """
        Test that intermediate data file is written with expected data.
        :return:
        """
        data = '''
My test
depends_on:DEP1
func1:0
'''
        data_f = StringIOWrapper('test_suite_ut.data', data)
        out_data_f = StringIOWrapper('test_suite_ut.datax', '')
        func_info = {'test_func1': (1, ('int',))}
        suite_dependencies = []
        write_parameters_mock.side_effect = write_parameters
        write_dependencies_mock.side_effect = write_dependencies
        func_mock1.side_effect = gen_suite_dep_checks
        gen_from_test_data(data_f, out_data_f, func_info, suite_dependencies)
        write_dependencies_mock.assert_called_with(out_data_f,
                                                   ['DEP1'], ['DEP1'])
        write_parameters_mock.assert_called_with(out_data_f, ['0'],
                                                 ('int',), [])
        expected_dep_check_code = '''
        case 0:
            {
#if defined(DEP1)
                ret = DEPENDENCY_SUPPORTED;
#else
                ret = DEPENDENCY_NOT_SUPPORTED;
#endif
            }
            break;'''
        func_mock1.assert_called_with(
            suite_dependencies, expected_dep_check_code, '')

    def test_function_not_found(self):
        """
        Test that AssertError is raised when function info in not found.
        :return:
        """
        data = '''
My test
depends_on:DEP1
func1:0
'''
        data_f = StringIOWrapper('test_suite_ut.data', data)
        out_data_f = StringIOWrapper('test_suite_ut.datax', '')
        func_info = {'test_func2': (1, ('int',))}
        suite_dependencies = []
        self.assertRaises(GeneratorInputError, gen_from_test_data,
                          data_f, out_data_f, func_info, suite_dependencies)

    def test_different_func_args(self):
        """
        Test that AssertError is raised when no. of parameters and
        function args differ.
        :return:
        """
        data = '''
My test
depends_on:DEP1
func1:0
'''
        data_f = StringIOWrapper('test_suite_ut.data', data)
        out_data_f = StringIOWrapper('test_suite_ut.datax', '')
        func_info = {'test_func2': (1, ('int', 'hex'))}
        suite_dependencies = []
        self.assertRaises(GeneratorInputError, gen_from_test_data, data_f,
                          out_data_f, func_info, suite_dependencies)

    def test_output(self):
        """
        Test that intermediate data file is written with expected data.
        :return:
        """
        data = '''
My test 1
depends_on:DEP1
func1:0:0xfa:MACRO1:MACRO2

My test 2
depends_on:DEP1:DEP2
func2:"yahoo":88:MACRO1
'''
        data_f = StringIOWrapper('test_suite_ut.data', data)
        out_data_f = StringIOWrapper('test_suite_ut.datax', '')
        func_info = {'test_func1': (0, ('int', 'int', 'int', 'int')),
                     'test_func2': (1, ('char*', 'int', 'int'))}
        suite_dependencies = []
        dep_check_code, expression_code = \
            gen_from_test_data(data_f, out_data_f, func_info,
                               suite_dependencies)
        expected_dep_check_code = '''
        case 0:
            {
#if defined(DEP1)
                ret = DEPENDENCY_SUPPORTED;
#else
                ret = DEPENDENCY_NOT_SUPPORTED;
#endif
            }
            break;
        case 1:
            {
#if defined(DEP2)
                ret = DEPENDENCY_SUPPORTED;
#else
                ret = DEPENDENCY_NOT_SUPPORTED;
#endif
            }
            break;'''
        expected_data = '''My test 1
depends_on:0
0:int:0:int:0xfa:exp:0:exp:1

My test 2
depends_on:0:1
1:char*:"yahoo":int:88:exp:0

'''
        expected_expression_code = '''
        case 0:
            {
                *out_value = MACRO1;
            }
            break;
        case 1:
            {
                *out_value = MACRO2;
            }
            break;'''
        self.assertEqual(dep_check_code, expected_dep_check_code)
        self.assertEqual(out_data_f.getvalue(), expected_data)
        self.assertEqual(expression_code, expected_expression_code)


if __name__ == '__main__':
    unittest_main()
