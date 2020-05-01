#!/usr/bin/env python
"""Functional testing framework for command line applications"""

import difflib
import itertools
import optparse
import os
import re
import signal
import subprocess
import sys
import shutil
import time
import tempfile

try:
    import configparser
except ImportError:
    import ConfigParser as configparser

__all__ = ['main', 'test']

def findtests(paths):
    """Yield tests in paths in sorted order"""
    for p in paths:
        if os.path.isdir(p):
            for root, dirs, files in os.walk(p):
                if os.path.basename(root).startswith('.'):
                    continue
                for f in sorted(files):
                    if not f.startswith('.') and f.endswith('.t'):
                        yield os.path.normpath(os.path.join(root, f))
        else:
            yield os.path.normpath(p)

def regex(pattern, s):
    """Match a regular expression or return False if invalid.

    >>> [bool(regex(r, 'foobar')) for r in ('foo.*', '***')]
    [True, False]
    """
    try:
        return re.match(pattern + r'\Z', s)
    except re.error:
        return False

def glob(el, l):
    r"""Match a glob-like pattern.

    The only supported special characters are * and ?. Escaping is
    supported.

    >>> bool(glob(r'\* \\ \? fo?b*', '* \\ ? foobar'))
    True
    """
    i, n = 0, len(el)
    res = ''
    while i < n:
        c = el[i]
        i += 1
        if c == '\\' and el[i] in '*?\\':
            res += el[i - 1:i + 1]
            i += 1
        elif c == '*':
            res += '.*'
        elif c == '?':
            res += '.'
        else:
            res += re.escape(c)
    return regex(res, l)

annotations = {'glob': glob, 're': regex}

def match(el, l):
    """Match patterns based on annotations"""
    for k in annotations:
        ann = ' (%s)\n' % k
        if el.endswith(ann) and annotations[k](el[:-len(ann)], l[:-1]):
            return True
    return False

class SequenceMatcher(difflib.SequenceMatcher, object):
    """Like difflib.SequenceMatcher, but matches globs and regexes"""

    def find_longest_match(self, alo, ahi, blo, bhi):
        """Find longest matching block in a[alo:ahi] and b[blo:bhi]"""
        # SequenceMatcher uses find_longest_match() to slowly whittle down
        # the differences between a and b until it has each matching block.
        # Because of this, we can end up doing the same matches many times.
        matches = []
        for n, (el, line) in enumerate(zip(self.a[alo:ahi], self.b[blo:bhi])):
            if el != line and match(el, line):
                # This fools the superclass's method into thinking that the
                # regex/glob in a is identical to b by replacing a's line (the
                # expected output) with b's line (the actual output).
                self.a[alo + n] = line
                matches.append((n, el))
        ret = super(SequenceMatcher, self).find_longest_match(alo, ahi,
                                                              blo, bhi)
        # Restore the lines replaced above. Otherwise, the diff output
        # would seem to imply that the tests never had any regexes/globs.
        for n, el in matches:
            self.a[alo + n] = el
        return ret

def unified_diff(a, b, fromfile='', tofile='', fromfiledate='',
                 tofiledate='', n=3, lineterm='\n', matcher=SequenceMatcher):
    """Compare two sequences of lines; generate the delta as a unified diff.

    This is like difflib.unified_diff(), but allows custom matchers.
    """
    started = False
    for group in matcher(None, a, b).get_grouped_opcodes(n):
        if not started:
            fromdate = fromfiledate and '\t%s' % fromfiledate or ''
            todate = fromfiledate and '\t%s' % tofiledate or ''
            yield '--- %s%s%s' % (fromfile, fromdate, lineterm)
            yield '+++ %s%s%s' % (tofile, todate, lineterm)
            started = True
        i1, i2, j1, j2 = group[0][1], group[-1][2], group[0][3], group[-1][4]
        yield "@@ -%d,%d +%d,%d @@%s" % (i1 + 1, i2 - i1, j1 + 1, j2 - j1,
                                         lineterm)
        for tag, i1, i2, j1, j2 in group:
            if tag == 'equal':
                for line in a[i1:i2]:
                    yield ' ' + line
                continue
            if tag == 'replace' or tag == 'delete':
                for line in a[i1:i2]:
                    yield '-' + line
            if tag == 'replace' or tag == 'insert':
                for line in b[j1:j2]:
                    yield '+' + line

needescape = re.compile(r'[\x00-\x09\x0b-\x1f\x7f-\xff]').search
escapesub = re.compile(r'[\x00-\x09\x0b-\x1f\\\x7f-\xff]').sub
escapemap = dict((chr(i), r'\x%02x' % i) for i in range(256))
escapemap.update({'\\': '\\\\', '\r': r'\r', '\t': r'\t'})

def escape(s):
    """Like the string-escape codec, but doesn't escape quotes"""
    return escapesub(lambda m: escapemap[m.group(0)], s[:-1]) + ' (esc)\n'

def makeresetsigpipe():
    """Make a function to reset SIGPIPE to SIG_DFL (for use in subprocesses).

    Doing subprocess.Popen(..., preexec_fn=makeresetsigpipe()) will prevent
    Python's SIGPIPE handler (SIG_IGN) from being inherited by the
    child process.
    """
    if sys.platform == 'win32' or getattr(signal, 'SIGPIPE', None) is None:
        return None
    return lambda: signal.signal(signal.SIGPIPE, signal.SIG_DFL)

def test(path, shell, indent=2):
    """Run test at path and return input, output, and diff.

    This returns a 3-tuple containing the following:

        (list of lines in test, same list with actual output, diff)

    diff is a generator that yields the diff between the two lists.

    If a test exits with return code 80, the actual output is set to
    None and diff is set to [].
    """
    indent = ' ' * indent
    cmdline = '%s$ ' % indent
    conline = '%s> ' % indent

    f = open(path)
    abspath = os.path.abspath(path)
    env = os.environ.copy()
    env['TESTDIR'] = os.path.dirname(abspath)
    env['TESTFILE'] = os.path.basename(abspath)
    p = subprocess.Popen([shell, '-'], bufsize=-1, stdin=subprocess.PIPE,
                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                         universal_newlines=True, env=env,
                         preexec_fn=makeresetsigpipe(),
                         close_fds=os.name == 'posix')
    salt = 'CRAM%s' % time.time()

    after = {}
    refout, postout = [], []
    i = pos = prepos = -1
    stdin = []
    for i, line in enumerate(f):
        refout.append(line)
        if line.startswith(cmdline):
            after.setdefault(pos, []).append(line)
            prepos = pos
            pos = i
            stdin.append('echo "\n%s %s $?"\n' % (salt, i))
            stdin.append(line[len(cmdline):])
        elif line.startswith(conline):
            after.setdefault(prepos, []).append(line)
            stdin.append(line[len(conline):])
        elif not line.startswith(indent):
            after.setdefault(pos, []).append(line)
    stdin.append('echo "\n%s %s $?"\n' % (salt, i + 1))

    output = p.communicate(input=''.join(stdin))[0]
    if p.returncode == 80:
        return (refout, None, [])

    # Add a trailing newline to the input script if it's missing.
    if refout and not refout[-1].endswith('\n'):
        refout[-1] += '\n'

    # We use str.split instead of splitlines to get consistent
    # behavior between Python 2 and 3. In 3, we use unicode strings,
    # which has more line breaks than \n and \r.
    pos = -1
    ret = 0
    for i, line in enumerate(output[:-1].split('\n')):
        line += '\n'
        if line.startswith(salt):
            presalt = postout.pop()
            if presalt != '%s\n' % indent:
                postout.append(presalt[:-1] + ' (no-eol)\n')
            ret = int(line.split()[2])
            if ret != 0:
                postout.append('%s[%s]\n' % (indent, ret))
            postout += after.pop(pos, [])
            pos = int(line.split()[1])
        else:
            if needescape(line):
                line = escape(line)
            postout.append(indent + line)
    postout += after.pop(pos, [])

    diffpath = os.path.basename(abspath)
    diff = unified_diff(refout, postout, diffpath, diffpath + '.err')
    for firstline in diff:
        return refout, postout, itertools.chain([firstline], diff)
    return refout, postout, []

def prompt(question, answers, auto=None):
    """Write a prompt to stdout and ask for answer in stdin.

    answers should be a string, with each character a single
    answer. An uppercase letter is considered the default answer.

    If an invalid answer is given, this asks again until it gets a
    valid one.

    If auto is set, the question is answered automatically with the
    specified value.
    """
    default = [c for c in answers if c.isupper()]
    while True:
        sys.stdout.write('%s [%s] ' % (question, answers))
        sys.stdout.flush()
        if auto is not None:
            sys.stdout.write(auto + '\n')
            sys.stdout.flush()
            return auto

        answer = sys.stdin.readline().strip().lower()
        if not answer and default:
            return default[0]
        elif answer and answer in answers.lower():
            return answer

def log(msg=None, verbosemsg=None, verbose=False):
    """Write msg to standard out and flush.

    If verbose is True, write verbosemsg instead.
    """
    if verbose:
        msg = verbosemsg
    if msg:
        sys.stdout.write(msg)
        sys.stdout.flush()

def patch(cmd, diff, path):
    """Run echo [lines from diff] | cmd -p0"""
    p = subprocess.Popen([cmd, '-p0'], bufsize=-1, stdin=subprocess.PIPE,
                         universal_newlines=True,
                         preexec_fn=makeresetsigpipe(),
                         cwd=path,
                         close_fds=os.name == 'posix')
    p.communicate(''.join(diff))
    return p.returncode == 0

def run(paths, tmpdir, shell, quiet=False, verbose=False, patchcmd=None,
        answer=None, indent=2):
    """Run tests in paths in tmpdir.

    If quiet is True, diffs aren't printed. If verbose is True,
    filenames and status information are printed.

    If patchcmd is set, a prompt is written to stdout asking if
    changed output should be merged back into the original test. The
    answer is read from stdin. If 'y', the test is patched using patch
    based on the changed output.
    """
    cwd = os.getcwd()
    seen = set()
    basenames = set()
    skipped = failed = 0
    for i, path in enumerate(findtests(paths)):
        abspath = os.path.abspath(path)
        if abspath in seen:
            continue
        seen.add(abspath)

        log(None, '%s: ' % path, verbose)
        if not os.stat(abspath).st_size:
            skipped += 1
            log('s', 'empty\n', verbose)
        else:
            basename = os.path.basename(path)
            if basename in basenames:
                basename = '%s-%s' % (basename, i)
            else:
                basenames.add(basename)
            testdir = os.path.join(tmpdir, basename)
            os.mkdir(testdir)
            try:
                os.chdir(testdir)
                refout, postout, diff = test(abspath, shell, indent)
            finally:
                os.chdir(cwd)

            errpath = abspath + '.err'
            if postout is None:
                skipped += 1
                log('s', 'skipped\n', verbose)
            elif not diff:
                log('.', 'passed\n', verbose)
                if os.path.exists(errpath):
                    os.remove(errpath)
            else:
                failed += 1
                log('!', 'failed\n', verbose)
                if not quiet:
                    log('\n', None, verbose)
                errfile = open(errpath, 'w')
                try:
                    for line in postout:
                        errfile.write(line)
                finally:
                    errfile.close()
                if not quiet:
                    if patchcmd:
                        diff = list(diff)
                    for line in diff:
                        log(line)
                    if (patchcmd and
                        prompt('Accept this change?', 'yN', answer) == 'y'):
                        if patch(patchcmd, diff, os.path.dirname(abspath)):
                            log(None, '%s: merged output\n' % path, verbose)
                            os.remove(errpath)
                        else:
                            log('%s: merge failed\n' % path)
    log('\n', None, verbose)
    log('# Ran %s tests, %s skipped, %s failed.\n'
        % (len(seen), skipped, failed))
    return bool(failed)

def which(cmd):
    """Return the patch to cmd or None if not found"""
    for p in os.environ['PATH'].split(os.pathsep):
        path = os.path.join(p, cmd)
        if os.path.isfile(path) and os.access(path, os.X_OK):
            return os.path.abspath(path)
    return None

def expandpath(path):
    """Expands ~ and environment variables in path"""
    return os.path.expanduser(os.path.expandvars(path))

class OptionParser(optparse.OptionParser):
    """Like optparse.OptionParser, but supports setting values through
    CRAM= and .cramrc."""

    def __init__(self, *args, **kwargs):
        self._config_opts = {}
        optparse.OptionParser.__init__(self, *args, **kwargs)

    def add_option(self, *args, **kwargs):
        option = optparse.OptionParser.add_option(self, *args, **kwargs)
        if option.dest and option.dest != 'version':
            key = option.dest.replace('_', '-')
            self._config_opts[key] = option.action == 'store_true'
        return option

    def parse_args(self, args=None, values=None):
        config = configparser.RawConfigParser()
        config.read(expandpath(os.environ.get('CRAMRC', '.cramrc')))
        defaults = {}
        for key, isbool in self._config_opts.items():
            try:
                if isbool:
                    try:
                        value = config.getboolean('cram', key)
                    except ValueError:
                        value = config.get('cram', key)
                        self.error('--%s: invalid boolean value: %r'
                                   % (key, value))
                else:
                    value = config.get('cram', key)
            except (configparser.NoSectionError, configparser.NoOptionError):
                pass
            else:
                defaults[key] = value
        self.set_defaults(**defaults)

        eargs = os.environ.get('CRAM', '').strip()
        if eargs:
            import shlex
            args = args or []
            args += shlex.split(eargs)

        try:
            return optparse.OptionParser.parse_args(self, args, values)
        except optparse.OptionValueError:
            self.error(str(sys.exc_info()[1]))

def main(args):
    """Main entry point.

    args should not contain the script name.
    """
    p = OptionParser(usage='cram [OPTIONS] TESTS...', prog='cram')
    p.add_option('-V', '--version', action='store_true',
                 help='show version information and exit')
    p.add_option('-q', '--quiet', action='store_true',
                 help="don't print diffs")
    p.add_option('-v', '--verbose', action='store_true',
                 help='show filenames and test status')
    p.add_option('-i', '--interactive', action='store_true',
                 help='interactively merge changed test output')
    p.add_option('-y', '--yes', action='store_true',
                 help='answer yes to all questions')
    p.add_option('-n', '--no', action='store_true',
                 help='answer no to all questions')
    p.add_option('-E', '--preserve-env', action='store_true',
                 help="don't reset common environment variables")
    p.add_option('--keep-tmpdir', action='store_true',
                 help='keep temporary directories')
    p.add_option('--shell', action='store', default='/bin/sh', metavar='PATH',
                 help='shell to use for running tests')
    p.add_option('--indent', action='store', default=2, metavar='NUM',
                 type='int', help='number of spaces to use for indentation')
    opts, paths = p.parse_args(args)

    if opts.version:
        sys.stdout.write("""Cram CLI testing framework (version 0.6)

Copyright (C) 2010-2011 Brodie Rao <brodie@bitheap.org> and others
This is free software; see the source for copying conditions. There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
""")
        return

    conflicts = [('-y', opts.yes, '-n', opts.no),
                 ('-q', opts.quiet, '-i', opts.interactive)]
    for s1, o1, s2, o2 in conflicts:
        if o1 and o2:
            sys.stderr.write('options %s and %s are mutually exclusive\n'
                             % (s1, s2))
            return 2

    patchcmd = None
    if opts.interactive:
        patchcmd = which('patch')
        if not patchcmd:
            sys.stderr.write('patch(1) required for -i\n')
            return 2

    if not paths:
        sys.stdout.write(p.get_usage())
        return 2

    badpaths = [path for path in paths if not os.path.exists(path)]
    if badpaths:
        sys.stderr.write('no such file: %s\n' % badpaths[0])
        return 2

    tmpdir = os.environ['CRAMTMP'] = tempfile.mkdtemp('', 'cramtests-')
    proctmp = os.path.join(tmpdir, 'tmp')
    os.mkdir(proctmp)
    for s in ('TMPDIR', 'TEMP', 'TMP'):
        os.environ[s] = proctmp

    if not opts.preserve_env:
        for s in ('LANG', 'LC_ALL', 'LANGUAGE'):
            os.environ[s] = 'C'
        os.environ['TZ'] = 'GMT'
        os.environ['CDPATH'] = ''
        os.environ['COLUMNS'] = '80'
        os.environ['GREP_OPTIONS'] = ''

    if opts.yes:
        answer = 'y'
    elif opts.no:
        answer = 'n'
    else:
        answer = None

    try:
        return run(paths, tmpdir, opts.shell, opts.quiet, opts.verbose,
                   patchcmd, answer, opts.indent)
    finally:
        if opts.keep_tmpdir:
            log('# Kept temporary directory: %s\n' % tmpdir)
        else:
            shutil.rmtree(tmpdir)

if __name__ == '__main__':
    try:
        sys.exit(main(sys.argv[1:]))
    except KeyboardInterrupt:
        pass
