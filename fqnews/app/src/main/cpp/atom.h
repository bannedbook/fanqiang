/*
Copyright (c) 2003-2006 by Juliusz Chroboczek

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

typedef struct _Atom {
    unsigned int refcount;
    struct _Atom *next;
    unsigned short length;
    char string[1];
} AtomRec, *AtomPtr;

typedef struct _AtomList {
    int length;
    int size;
    AtomPtr *list;
} AtomListRec, *AtomListPtr;

#define LOG2_ATOM_HASH_TABLE_SIZE 10
#define LARGE_ATOM_REFCOUNT 0xFFFFFF00U

extern int used_atoms;

void initAtoms(void);
AtomPtr internAtom(const char *string);
AtomPtr internAtomN(const char *string, int n);
AtomPtr internAtomLowerN(const char *string, int n);
AtomPtr atomCat(AtomPtr atom, const char *string);
int atomSplit(AtomPtr atom, char c, AtomPtr *return1, AtomPtr *return2);
AtomPtr retainAtom(AtomPtr atom);
void releaseAtom(AtomPtr atom);
AtomPtr internAtomError(int e, const char *f, ...)
     ATTRIBUTE ((format (printf, 2, 3)));
AtomPtr internAtomF(const char *format, ...)
     ATTRIBUTE ((format (printf, 1, 2)));
char *atomString(AtomPtr) ATTRIBUTE ((pure));
AtomListPtr makeAtomList(AtomPtr *atoms, int n);
void destroyAtomList(AtomListPtr list);
int atomListMember(AtomPtr atom, AtomListPtr list)
    ATTRIBUTE ((pure));
void atomListCons(AtomPtr atom, AtomListPtr list);
