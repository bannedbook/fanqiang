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

#define CONFIG_INT 0
#define CONFIG_OCTAL 1
#define CONFIG_HEX 2
#define CONFIG_TIME 3
#define CONFIG_BOOLEAN 4
#define CONFIG_TRISTATE 5
#define CONFIG_TETRASTATE 6
#define CONFIG_PENTASTATE 7
#define CONFIG_FLOAT 8
#define CONFIG_ATOM 9
#define CONFIG_ATOM_LOWER 10
#define CONFIG_PASSWORD 11
#define CONFIG_INT_LIST 12
#define CONFIG_ATOM_LIST 13
#define CONFIG_ATOM_LIST_LOWER 14

typedef struct _ConfigVariable {
    AtomPtr name;
    int type;
    union {
        int *i;
        float *f;
        struct _Atom **a;
        struct _AtomList **al;
        struct _IntList **il;
    } value;
    int (*setter)(struct _ConfigVariable*, void*);
    char *help;
    struct _ConfigVariable *next;
} ConfigVariableRec, *ConfigVariablePtr;

#define CONFIG_VARIABLE(name, type, help) \
    CONFIG_VARIABLE_SETTABLE(name, type, NULL, help)

#define CONFIG_VARIABLE_SETTABLE(name, type, setter, help) \
    declareConfigVariable(internAtom(#name), type, &name, setter, help)

void declareConfigVariable(AtomPtr name, int type, void *value, 
                           int (*setter)(ConfigVariablePtr, void*),
                           char *help);
void printConfigVariables(FILE *out, int html);
int parseConfigLine(char *line, char *filename, int lineno, int set);
int parseConfigFile(AtomPtr);
int configIntSetter(ConfigVariablePtr, void*);
int configFloatSetter(ConfigVariablePtr, void*);
int configAtomSetter(ConfigVariablePtr, void*);
