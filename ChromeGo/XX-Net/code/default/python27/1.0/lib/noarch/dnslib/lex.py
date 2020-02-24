# -*- coding: utf-8 -*-

from __future__ import print_function

import collections,string

try:
    from StringIO import StringIO
except ImportError:
    from io import StringIO

class Lexer(object):

    """
        Simple Lexer base class. Provides basic lexer framework and
        helper functionality (read/peek/pushback etc)

        Each state is implemented using a method (lexXXXX) which should
        match a single token and return a (token,lexYYYY) tuple, with lexYYYY
        representing the next state. If token is None this is not emitted
        and if lexYYYY is None or the lexer reaches the end of the
        input stream the lexer exits.

        The 'parse' method returns a generator that will return tokens
        (the class also acts as an iterator)

        The default start state is 'lexStart'

        Input can either be a string/bytes or file object.

        The approach is based loosely on Rob Pike's Go lexer presentation
        (using generators rather than channels).

        >>> p = Lexer("a bcd efgh")
        >>> p.read()
        'a'
        >>> p.read()
        ' '
        >>> p.peek(3)
        'bcd'
        >>> p.read(5)
        'bcd e'
        >>> p.pushback('e')
        >>> p.read(4)
        'efgh'
    """

    escape_chars = '\\'
    escape = {'n':'\n','t':'\t','r':'\r'}

    def __init__(self,f,debug=False):
        if hasattr(f,'read'):
            self.f = f
        elif type(f) == str:
            self.f = StringIO(f)
        elif type(f) == bytes:
            self.f = StringIO(f.decode())
        else:
            raise ValueError("Invalid input")
        self.debug = debug
        self.q = collections.deque()
        self.state = self.lexStart
        self.escaped = False
        self.eof = False

    def __iter__(self):
        return self.parse()

    def next_token(self):
        if self.debug:
            print("STATE",self.state)
        (tok,self.state) = self.state()
        return tok

    def parse(self):
        while self.state is not None and not self.eof:
            tok = self.next_token()
            if tok:
                yield tok

    def read(self,n=1):
        s = ""
        while self.q and n > 0:
            s += self.q.popleft()
            n -= 1
        s += self.f.read(n)
        if s == '':
            self.eof = True
        if self.debug:
            print("Read: >%s<" % repr(s))
        return s

    def peek(self,n=1):
        s = ""
        i = 0
        while len(self.q) > i and n > 0:
            s += self.q[i]
            i += 1
            n -= 1
        r = self.f.read(n)
        if n > 0 and r == '':
            self.eof = True
        self.q.extend(r)
        if self.debug:
            print("Peek : >%s<" % repr(s + r))
        return s + r

    def pushback(self,s):
        p = collections.deque(s)
        p.extend(self.q)
        self.q = p

    def readescaped(self):
        c = self.read(1)
        if c in self.escape_chars:
            self.escaped = True
            n = self.peek(3)
            if n.isdigit():
                n = self.read(3)
                if self.debug:
                    print("Escape: >%s<" % n)
                return chr(int(n,8))
            elif n[0] in 'x':
                x = self.read(3)
                if self.debug:
                    print("Escape: >%s<" % x)
                return chr(int(x[1:],16))
            else:
                c = self.read(1)
                if self.debug:
                    print("Escape: >%s<" % c)
                return self.escape.get(c,c)
        else:
            self.escaped = False
            return c

    def lexStart(self):
        return (None,None)

class WordLexer(Lexer):
    """
        Example lexer which will split input stream into words (respecting
        quotes)

        To emit SPACE tokens: self.spacetok = ('SPACE',None)
        To emit NL tokens: self.nltok = ('NL',None)

        >>> l = WordLexer(r'abc "def\100\x3d\. ghi" jkl')
        >>> list(l)
        [('ATOM', 'abc'), ('ATOM', 'def@=. ghi'), ('ATOM', 'jkl')]
        >>> l = WordLexer(r"1 '2 3 4' 5")
        >>> list(l)
        [('ATOM', '1'), ('ATOM', '2 3 4'), ('ATOM', '5')]
        >>> l = WordLexer("abc# a comment")
        >>> list(l)
        [('ATOM', 'abc'), ('COMMENT', 'a comment')]
    """

    wordchars = set(string.ascii_letters) | set(string.digits) | \
                set(string.punctuation)
    quotechars = set('"\'')
    commentchars = set('#')
    spacechars = set(' \t\x0b\x0c')
    nlchars = set('\r\n')
    spacetok = None
    nltok = None

    def lexStart(self):
        return (None,self.lexSpace)

    def lexSpace(self):
        s = []
        if self.spacetok:
            tok = lambda n : (self.spacetok,n) if s else (None,n)
        else:
            tok = lambda n : (None,n)
        while not self.eof:
            c = self.peek()
            if c in self.spacechars:
                s.append(self.read())
            elif c in self.nlchars:
                return tok(self.lexNL)
            elif c in self.commentchars:
                return tok(self.lexComment)
            elif c in self.quotechars:
                return tok(self.lexQuote)
            elif c in self.wordchars:
                return tok(self.lexWord)
                return (self.spacetok,self.lexWord)
            elif c:
                raise ValueError("Invalid input [%d]: %s" % (
                                        self.f.tell(),c))
        return (None,None)

    def lexNL(self):
        while True:
            c = self.read()
            if c not in self.nlchars:
                self.pushback(c)
                return (self.nltok,self.lexSpace)

    def lexComment(self):
        s = []
        tok = lambda n : (('COMMENT',''.join(s)),n) if s else (None,n)
        start = False
        _ = self.read()
        while not self.eof:
            c = self.read()
            if c == '\n':
                self.pushback(c)
                return tok(self.lexNL)
            elif start or c not in string.whitespace:
                start = True
                s.append(c)
        return tok(None)

    def lexWord(self):
        s = []
        tok = lambda n : (('ATOM',''.join(s)),n) if s else (None,n)
        while not self.eof:
            c = self.peek()
            if c == '"':
                return tok(self.lexQuote)
            elif c in self.commentchars:
                return tok(self.lexComment)
            elif c.isspace():
                return tok(self.lexSpace)
            elif c in self.wordchars:
                s.append(self.read())
            elif c:
                raise ValueError('Invalid input [%d]: %s' % (
                                    self.f.tell(),c))
        return tok(None)

    def lexQuote(self):
        s = []
        tok = lambda n : (('ATOM',''.join(s)),n)
        q = self.read(1)
        while not self.eof:
            c = self.readescaped()
            if c == q and not self.escaped:
                break
            else:
                s.append(c)
        return tok(self.lexSpace)

class RandomLexer(Lexer):

    """
        Test lexing from infinite stream.

        Extract strings of letters/numbers from /dev/urandom

        >>> import itertools,sys
        >>> if sys.version[0] == '2':
        ...     f = open("/dev/urandom")
        ... else:
        ...     f = open("/dev/urandom",encoding="ascii",errors="replace")
        >>> r = RandomLexer(f)
        >>> i = iter(r)
        >>> len(list(itertools.islice(i,10)))
        10
    """

    minalpha = 4
    mindigits = 3

    def lexStart(self):
        return (None,self.lexRandom)

    def lexRandom(self):
        n = 0
        c = self.peek(1)
        while not self.eof:
            if c.isalpha():
                return (None,self.lexAlpha)
            elif c.isdigit():
                return (None,self.lexDigits)
            else:
                n += 1
                _ = self.read(1)
                c = self.peek(1)
        return (None,None)

    def lexDigits(self):
        s = []
        c = self.read(1)
        while c.isdigit():
            s.append(c)
            c = self.read(1)
        self.pushback(c)
        if len(s) >= self.mindigits:
            return (('NUMBER',"".join(s)),self.lexRandom)
        else:
            return (None,self.lexRandom)

    def lexAlpha(self):
        s = []
        c = self.read(1)
        while c.isalpha():
            s.append(c)
            c = self.read(1)
        self.pushback(c)
        if len(s) >= self.minalpha:
            return (('STRING',"".join(s)),self.lexRandom)
        else:
            return (None,self.lexRandom)

if __name__ == '__main__':

    import argparse,doctest,sys

    p = argparse.ArgumentParser(description="Lex Tester")
    p.add_argument("--lex","-l",action='store_true',default=False,
                    help="Lex input (stdin)")
    p.add_argument("--nl",action='store_true',default=False,
                    help="Output NL tokens")
    p.add_argument("--space",action='store_true',default=False,
                    help="Output Whitespace tokens")
    p.add_argument("--wordchars",help="Wordchars")
    p.add_argument("--quotechars",help="Quotechars")
    p.add_argument("--commentchars",help="Commentchars")
    p.add_argument("--spacechars",help="Spacechars")
    p.add_argument("--nlchars",help="NLchars")

    args = p.parse_args()

    if args.lex:
        l = WordLexer(sys.stdin)
        if args.wordchars:
            l.wordchars = set(args.wordchars)
        if args.quotechars:
            l.quotechars = set(args.quotechars)
        if args.commentchars:
            l.commentchars = set(args.commentchars)
        if args.spacechars:
            l.spacechars = set(args.spacechars)
        if args.nlchars:
            l.nlchars = set(args.nlchars)
        if args.space:
            l.spacetok = ('SPACE',)
        if args.nl:
            l.nltok = ('NL',)

        for tok in l:
            print(tok)
    else:
        try:
            # Test if we have /dev/urandom
            open("/dev/urandom")
            doctest.testmod()
        except IOError:
            # Don't run stream test
            doctest.run_docstring_examples(Lexer, globals())
            doctest.run_docstring_examples(WordLexer, globals())

