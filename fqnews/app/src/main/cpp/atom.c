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

#include "polipo.h"

/* Atoms are interned, read-only reference-counted strings.

   Interned means that equality of atoms is equivalent to structural
   equality -- you don't need to strcmp, you just compare the AtomPtrs.
   This property is used throughout Polipo, e.g. to speed up the HTTP
   parser.

   Polipo's atoms may contain NUL bytes -- you can use internAtomN to
   store any random binary data within an atom.  However, Polipo always
   terminates your data, so if you store textual data in an atom, you
   may use the result of atomString as though it were a (read-only)
   C string.

*/

static AtomPtr *atomHashTable;
int used_atoms;

void
initAtoms()
{
    atomHashTable = calloc((1 << LOG2_ATOM_HASH_TABLE_SIZE),
                           sizeof(AtomPtr));

    if(atomHashTable == NULL) {
        do_log(L_ERROR, "Couldn't allocate atom hash table.\n");
        exit(1);
    }
    used_atoms = 0;
}

AtomPtr
internAtomN(const char *string, int n)
{
    AtomPtr atom;
    int h;

    if(n < 0 || n >= (1 << (8 * sizeof(unsigned short))))
        return NULL;

    h = hash(0, string, n, LOG2_ATOM_HASH_TABLE_SIZE);
    atom = atomHashTable[h];
    while(atom) {
        if(atom->length == n &&
           (n == 0 || memcmp(atom->string, string, n) == 0))
            break;
        atom = atom->next;
    }

    if(!atom) {
        atom = malloc(sizeof(AtomRec) - 1 + n + 1);
        if(atom == NULL) {
            return NULL;
        }
        atom->refcount = 0;
        atom->length = n;
        /* Atoms are used both for binary data and strings.  To make
           their use as strings more convenient, atoms are always
           NUL-terminated. */
        memcpy(atom->string, string, n);
        atom->string[n] = '\0';
        atom->next = atomHashTable[h];
        atomHashTable[h] = atom;
        used_atoms++;
    }
    do_log(D_ATOM_REFCOUNT, "A 0x%lx %d++\n",
           (unsigned long)atom, atom->refcount);
    atom->refcount++;
    return atom;
}

AtomPtr
internAtom(const char *string)
{
    return internAtomN(string, strlen(string));
}

AtomPtr
atomCat(AtomPtr atom, const char *string)
{
    char buf[128];
    char *s = buf;
    AtomPtr newAtom;
    int n = strlen(string);
    if(atom->length + n > 128) {
        s = malloc(atom->length + n + 1);
        if(s == NULL)
            return NULL;
    }
    memcpy(s, atom->string, atom->length);
    memcpy(s + atom->length, string, n);
    newAtom = internAtomN(s, atom->length + n);
    if(s != buf) free(s);
    return newAtom;
}

int
atomSplit(AtomPtr atom, char c, AtomPtr *return1, AtomPtr *return2)
{
    char *p;
    AtomPtr atom1, atom2;
    p = memchr(atom->string, c, atom->length);
    if(p == NULL)
        return 0;
    atom1 = internAtomN(atom->string, p - atom->string);
    if(atom1 == NULL)
        return -ENOMEM;
    atom2 = internAtomN(p + 1, atom->length - (p + 1 - atom->string));
    if(atom2 == NULL) {
        releaseAtom(atom1);
        return -ENOMEM;
    }
    *return1 = atom1;
    *return2 = atom2;
    return 1;
}

AtomPtr
internAtomLowerN(const char *string, int n)
{
    char *s;
    char buf[100];
    AtomPtr atom;

    if(n < 0 || n >= 50000)
        return NULL;

    if(n < 100) {
        s = buf;
    } else {
        s = malloc(n);
        if(s == NULL)
            return NULL;
    }

    lwrcpy(s, string, n);
    atom = internAtomN(s, n);
    if(s != buf) free(s);
    return atom;
}

AtomPtr
retainAtom(AtomPtr atom)
{
    if(atom == NULL)
        return NULL;

    do_log(D_ATOM_REFCOUNT, "A 0x%lx %d++\n",
           (unsigned long)atom, atom->refcount);
    assert(atom->refcount >= 1 && atom->refcount < LARGE_ATOM_REFCOUNT);
    atom->refcount++;
    return atom;
}

void
releaseAtom(AtomPtr atom)
{
    if(atom == NULL)
        return;

    do_log(D_ATOM_REFCOUNT, "A 0x%lx %d--\n",
           (unsigned long)atom, atom->refcount);
    assert(atom->refcount >= 1 && atom->refcount < LARGE_ATOM_REFCOUNT);

    atom->refcount--;

    if(atom->refcount == 0) {
        int h = hash(0, atom->string, atom->length, LOG2_ATOM_HASH_TABLE_SIZE);
        assert(atomHashTable[h] != NULL);

        if(atom == atomHashTable[h]) {
            atomHashTable[h] = atom->next;
            free(atom);
        } else {
            AtomPtr previous = atomHashTable[h];
            while(previous->next) {
                if(previous->next == atom)
                    break;
                previous = previous->next;
            }
            assert(previous->next != NULL);
            previous->next = atom->next;
            free(atom);
        }
        used_atoms--;
    }
}

AtomPtr
internAtomF(const char *format, ...)
{
    char *s;
    char buf[150];
    int n;
    va_list args;
    AtomPtr atom = NULL;

    va_start(args, format);
    n = vsnprintf(buf, 150, format, args);
    va_end(args);
    if(n >= 0 && n < 150) {
        atom = internAtomN(buf, n);
    } else {
        va_start(args, format);
        s = vsprintf_a(format, args);
        va_end(args);
        if(s != NULL) {
            atom = internAtom(s);
            free(s);
        }
    }
    return atom;
}

static AtomPtr
internAtomErrorV(int e, const char *f, va_list args)
{
    
    char *es = pstrerror(e);
    AtomPtr atom;
    char *s1, *s2;
    int n, rc;
    va_list args_copy;

    if(f) {
        va_copy(args_copy, args);
        s1 = vsprintf_a(f, args_copy);
        va_end(args_copy);
        if(s1 == NULL)
            return NULL;
        n = strlen(s1);
    } else {
        s1 = NULL;
        n = 0;
    }

    s2 = malloc(n + 70);
    if(s2 == NULL) {
        free(s1);
        return NULL;
    }
    if(s1) {
        strcpy(s2, s1);
        free(s1);
    }

    rc = snprintf(s2 + n, 69, f ? ": %s" : "%s", es);
    if(rc < 0 || rc >= 69) {
        free(s2);
        return NULL;
    }

    atom = internAtomN(s2, n + rc);
    free(s2);
    return atom;
}

AtomPtr
internAtomError(int e, const char *f, ...)
{
    AtomPtr atom;
    va_list args;
    va_start(args, f);
    atom = internAtomErrorV(e, f, args);
    va_end(args);
    return atom;
}

char *
atomString(AtomPtr atom)
{
    if(atom)
        return atom->string;
    else
        return "(null)";
}

AtomListPtr
makeAtomList(AtomPtr *atoms, int n)
{
    AtomListPtr list;
    list = malloc(sizeof(AtomListRec));
    if(list == NULL) return NULL;
    list->length = 0;
    list->size = 0;
    list->list = NULL;
    if(n > 0) {
        int i;
        list->list = malloc(n * sizeof(AtomPtr));
        if(list->list == NULL) {
            free(list);
            return NULL;
        }
        list->size = n;
        for(i = 0; i < n; i++)
            list->list[i] = atoms[i];
        list->length = n;
    }
    return list;
}

void
destroyAtomList(AtomListPtr list)
{
    int i;
    if(list->list) {
        for(i = 0; i < list->length; i++)
            releaseAtom(list->list[i]);
        list->length = 0;
        free(list->list);
        list->list = NULL;
        list->size = 0;
    }
    assert(list->size == 0);
    free(list);
}

int
atomListMember(AtomPtr atom, AtomListPtr list)
{
    int i;
    for(i = 0; i < list->length; i++) {
        if(atom == list->list[i])
            return 1;
    }
    return 0;
}

void
atomListCons(AtomPtr atom, AtomListPtr list)
{
    if(list->list == NULL) {
        assert(list->size == 0);
        list->list = malloc(5 * sizeof(AtomPtr));
        if(list->list == NULL) {
            do_log(L_ERROR, "Couldn't allocate AtomList\n");
            return;
        }
        list->size = 5;
    }
    if(list->size <= list->length) {
        AtomPtr *new_list;
        int n = (2 * list->length + 1);
        new_list = realloc(list->list, n * sizeof(AtomPtr));
        if(new_list == NULL) {
            do_log(L_ERROR, "Couldn't realloc AtomList\n");
            return;
        }
        list->list = new_list;
        list->size = n;
    }
    list->list[list->length] = atom;
    list->length++;
}

