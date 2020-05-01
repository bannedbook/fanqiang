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

ConfigVariablePtr configVariables = NULL;

static ConfigVariablePtr
findConfigVariable(AtomPtr name)
{
    ConfigVariablePtr var;
    var = configVariables;
    while(var != NULL) {
        if(var->name == name)
            break;
        var = var->next;
    }
    return var;
}

void
declareConfigVariable(AtomPtr name, int type, void *value, 
                      int (*setter)(ConfigVariablePtr, void*), char *help)
{
    ConfigVariablePtr var, previous, next;

    var = findConfigVariable(name);

    if(var) {
        do_log(L_ERROR, 
               "Configuration variable %s declared multiple times.\n",
               name->string);
        if(var->type != type) {
            exit(1);
        }
    }

    var = malloc(sizeof(ConfigVariableRec));
    if(var == NULL) {
        do_log(L_ERROR, "Couldn't allocate config variable.\n");
        exit(1);
    }

    var->name = retainAtom(name);
    var->type = type;
    switch(type) {
    case CONFIG_INT: case CONFIG_OCTAL: case CONFIG_HEX: case CONFIG_TIME:
    case CONFIG_BOOLEAN: case CONFIG_TRISTATE: case CONFIG_TETRASTATE:
    case CONFIG_PENTASTATE:
        var->value.i = value; break;
    case CONFIG_FLOAT: var->value.f = value; break;
    case CONFIG_ATOM: case CONFIG_ATOM_LOWER: case CONFIG_PASSWORD:
        var->value.a = value; break;
    case CONFIG_INT_LIST:
        var->value.il = value; break;
    case CONFIG_ATOM_LIST: case CONFIG_ATOM_LIST_LOWER: 
        var->value.al = value; break;
    default: abort();
    }
    var->setter = setter;
    var->help = help;

    previous = NULL;
    next = configVariables;
    while(next && strcmp(next->name->string, var->name->string) < 0) {
        previous = next;
        next = next->next;
    }
    if(next && strcmp(next->name->string, var->name->string) == 0) {
        do_log(L_ERROR, "Variable %s declared multiple times.\n",
               next->name->string);
        abort();
    }
    if(previous == NULL) {
        var->next = configVariables;
        configVariables = var;
    } else {
        var->next = next;
        previous->next = var;
    }
}

static void
printString(FILE *out, char *string, int html)
{
    if(html) {
        char buf[512];
        int i;
        i = htmlString(buf, 0, 512, string, strlen(string));
        if(i < 0) {
            fprintf(out, "(overflow)");
            return;
        }
        fwrite(buf, 1, i, out);
    } else {
        fprintf(out, "%s", string);
    }
}

static void
printVariable(FILE *out, ConfigVariablePtr var, int html, int parseable)
{
    int i;

    switch(var->type) {
    case CONFIG_INT: fprintf(out, "%d", *var->value.i); break;
    case CONFIG_OCTAL: fprintf(out, "0%o", *var->value.i); break;
    case CONFIG_HEX: fprintf(out, "0x%x", *var->value.i); break;
    case CONFIG_TIME:
        {
            int v = *var->value.i;
            if(v == 0) {
                fprintf(out, "0s");
            } else {
                if(v >= 3600 * 24) fprintf(out, "%dd", v/(3600*24));
                v = v % (3600 * 24);
                if(v >= 3600) fprintf(out, "%dh", v / 3600);
                v = v % 3600;
                if(v >= 60) fprintf(out, "%dm", v / 60);
                v = v % 60;
                if(v > 0) fprintf(out, "%ds", v);
            }
        }
        break;
    case CONFIG_BOOLEAN:
        switch(*var->value.i) {
        case 0: fprintf(out, "false"); break;
        case 1: fprintf(out, "true"); break;
        default: fprintf(out, "???"); break;
        }
        break;
    case CONFIG_TRISTATE:
        switch(*var->value.i) {
        case 0: fprintf(out, "false"); break;
        case 1: fprintf(out, "maybe"); break;
        case 2: fprintf(out, "true"); break;
        default: fprintf(out, "???"); break;
        }
        break;
    case CONFIG_TETRASTATE:
        switch(*var->value.i) {
        case 0: fprintf(out, "false"); break;
        case 1: fprintf(out, "reluctantly"); break;
        case 2: fprintf(out, "happily"); break;
        case 3: fprintf(out, "true"); break;
        default: fprintf(out, "???"); break;
        }
        break;
    case CONFIG_PENTASTATE:
        switch(*var->value.i) {
        case 0: fprintf(out, "no"); break;
        case 1: fprintf(out, "reluctantly"); break;
        case 2: fprintf(out, "maybe"); break;
        case 3: fprintf(out, "happily"); break;
        case 4: fprintf(out, "true"); break;
        default: fprintf(out, "???"); break;
        }
        break;
    case CONFIG_FLOAT: fprintf(out, "%f", *var->value.f); break;
    case CONFIG_ATOM: case CONFIG_ATOM_LOWER:
        if(*var->value.a) {
            if((*var->value.a)->length > 0) {
                printString(out, (*var->value.a)->string, html);
            } else {
                if(!parseable)
                    fprintf(out, "(empty)");
            }
        } else {
            if(!parseable)
                fprintf(out, "(none)");
        }
        break;
    case CONFIG_PASSWORD:
        if(!parseable)
            fprintf(out, "(hidden)");
        break;
    case CONFIG_INT_LIST:
        if((*var->value.il) == NULL) {
            if(!parseable)
                fprintf(out, "(not set)");
        } else if((*var->value.il)->length == 0) {
            if(!parseable)
                fprintf(out, "(empty list)");
        } else {
            for(i = 0; i < (*var->value.il)->length; i++) {
                int from = (*var->value.il)->ranges[i].from;
                int to = (*var->value.il)->ranges[i].to;
                assert(from <= to);
                if(from == to)
                    fprintf(out, "%d", from);
                else
                    fprintf(out, "%d-%d", from, to);
                if(i < (*var->value.il)->length - 1)
                    fprintf(out, ", ");
            }
        }
        break;
    case CONFIG_ATOM_LIST: case CONFIG_ATOM_LIST_LOWER:
        if((*var->value.al) == NULL) {
            if(!parseable)
                fprintf(out, "(not set)");
        } else if((*var->value.al)->length == 0) {
            if(!parseable)
                fprintf(out, "(empty list)");
        } else {
            for(i = 0; i < (*var->value.al)->length; i++) {
                AtomPtr atom = (*var->value.al)->list[i];
                if(atom) {
                    if(atom->length > 0)
                        printString(out, atom->string, html);
                    else {
                        if(!parseable)
                            fprintf(out, "(empty)");
                    }
                } else {
                    if(!parseable)
                        fprintf(out, "(none)");
                }
                if(i < (*var->value.al)->length - 1)
                    fprintf(out, ", ");
            }
        }
        break;
    default: abort();
    }
}

static void
printVariableForm(FILE *out, ConfigVariablePtr var)
{
    char *disabled = "";
    int i;
    
    if(disableConfiguration || !var->setter) disabled = "disabled=true";

    fprintf(out, "<form method=POST action=\"config?\">");
  
    switch(var->type) {
    case CONFIG_INT: case CONFIG_OCTAL: case CONFIG_HEX:
    case CONFIG_TIME: case CONFIG_FLOAT: case CONFIG_ATOM:
    case CONFIG_ATOM_LOWER: case CONFIG_PASSWORD:
    case CONFIG_INT_LIST: case CONFIG_ATOM_LIST: case CONFIG_ATOM_LIST_LOWER:
        fprintf(out, "<input value=\"");
        printVariable(out, var, 1, 1);
        fprintf(out, "\"%s size=14 name=%s %s>\n",
                var->type == CONFIG_PASSWORD ? " type=password" : "",
                var->name->string, disabled);
        break;

    case CONFIG_BOOLEAN:
        {
            static char *states[] = {"false", "true"};
            
            fprintf(out, "<select name=%s %s>", var->name->string, disabled);
            for(i=0; i < sizeof(states) / sizeof(states[0]); i++) {
                if(*var->value.i == i) {
                    fprintf(out, "<option selected>%s</option>", states[i]);
                } else {
                    fprintf(out, "<option>%s</option>", states[i]);
                }
            }
            fprintf(out, "</select>");
            if(var->setter)
                fprintf(out, "<input type=\"submit\" value=\"set\"\n>");
            break;
        }
    
    case CONFIG_TRISTATE:
        {
            static char *states[] = {"false", "maybe", "true"};
            
            fprintf(out, "<select name=%s %s>", var->name->string, disabled);
            for(i=0; i < sizeof(states) / sizeof(states[0]); i++) {
                if(*var->value.i == i) {
                    fprintf(out, "<option selected>%s</option>", states[i]);
                } else {
                    fprintf(out, "<option>%s</option>", states[i]);
                }
            }
            fprintf(out, "</select>");
            if(var->setter)
                fprintf(out, "<input type=\"submit\" value=\"set\"\n>");
            break;
        }

    case CONFIG_TETRASTATE:
        {
            static char *states[] =
                {"false", "reluctantly", "happily", "true"};
            
            fprintf(out, "<select name=%s %s>", var->name->string, disabled);
            for(i=0; i <sizeof(states) / sizeof(states[0]); i++) {
                if(*var->value.i == i) {
                    fprintf(out, "<option selected>%s</option>", states[i]);
                } else {
                    fprintf(out, "<option>%s</option>", states[i]);
                }
            }
            fprintf(out, "</select>");
            if(var->setter)
                fprintf(out, "<input type=\"submit\" value=\"set\"\n>");
            break;
        }

    case CONFIG_PENTASTATE:
        {
            static char *states[] =
                {"no", "reluctantly", "maybe", "happily", "true"};

            fprintf(out, "<select name=%s %s>", var->name->string, disabled);
            for(i=0; i < sizeof(states) / sizeof(states[0]); i++) {
                if(*var->value.i == i) {
                    fprintf(out, "<option selected>%s</option>", states[i]);
                } else {
                    fprintf(out, "<option>%s</option>", states[i]);
                }
            }
            fprintf(out, "</select>");
            if(var->setter)
                fprintf(out,"<input type=\"submit\" value=\"set\"\n>");
            break;
        }
    default: abort();
    }
    fprintf(out, "</form>");
}





void
printConfigVariables(FILE *out, int html)
{
    ConfigVariablePtr var;
    int entryno = 0;

#define PRINT_SEP() \
    do {if(html) fprintf(out, "</td><td>"); else fprintf(out, " ");} while(0)

    if(html) {
        fprintf(out, "<table>\n");
        fprintf(out, "<tbody>\n");
    }

    if(html) {
        alternatingHttpStyle(out, "configlist");
        fprintf(out,
                "<table id=configlist>\n"
                "<thead>\n"
                "<tr><th>variable name</th>"
                "<th>current value</th>"
                "<th>new value</th>"
                "<th>description</th>\n"
                "</thead><tbody>\n"
);
    }

    /* configFile is not a config variable, for obvious bootstrapping reasons.
       CHUNK_SIZE is hardwired for now. */

    fprintf(out,
	    html ?
	    "<tr class=\"even\"><td>configFile</td><td>%s</td><td></td><td>"
	    "Configuration file.</td></tr>\n" :
	    "configFile %s Configuration file.\n",
	    configFile && configFile->length > 0 ?
	    configFile->string : "(none)");
    fprintf(out,
	    html ?
	    "<tr class=\"odd\"><td>CHUNK_SIZE</td><td>%d</td><td></td><td>"
	    "Unit of chunk memory allocation.</td></tr>\n" :
	    "CHUNK_SIZE %d Unit of chunk memory allocation.\n", CHUNK_SIZE);
    
    var = configVariables;
    while(var != NULL) {
      if(html) {
          if(entryno % 2)
              fprintf(out, "<tr class=odd>");
          else
              fprintf(out, "<tr class=even>");
          fprintf(out, "<td>");
      }

      fprintf(out, "%s", var->name->string);

      fprintf(out, html ? "<br/>" : " "); 
      
      fprintf(out, html ? "<i>" : "");    
      
      switch(var->type) {
      case CONFIG_INT: case CONFIG_OCTAL: case CONFIG_HEX:
	  fprintf(out, "integer"); break;
      case CONFIG_TIME: fprintf(out, "time"); break;
      case CONFIG_BOOLEAN: fprintf(out, "boolean"); break;
      case CONFIG_TRISTATE: fprintf(out, "tristate"); break;
      case CONFIG_TETRASTATE: fprintf(out, "4-state"); break;
      case CONFIG_PENTASTATE: fprintf(out, "5-state"); break;
      case CONFIG_FLOAT: fprintf(out, "float"); break;
      case CONFIG_ATOM: case CONFIG_ATOM_LOWER: case CONFIG_PASSWORD:
          fprintf(out, "atom"); break;
      case CONFIG_INT_LIST: fprintf(out, "intlist"); break;
      case CONFIG_ATOM_LIST: case CONFIG_ATOM_LIST_LOWER:
	  fprintf(out, "list"); break;
      default: abort();
      }
        
      fprintf(out, html ? "</i>" : "");

      PRINT_SEP();

      printVariable(out, var, html, 0);

      PRINT_SEP();
	
      if(html) {
	printVariableForm(out, var);
	PRINT_SEP();
      }

      fprintf(out, "%s", var->help?var->help:"");
      if(html)
	fprintf(out, "</td></tr>\n");
      else
	fprintf(out, "\n");

      entryno++;
      var = var->next;
    }
    if(html) {
        fprintf(out, "</tbody>\n");
        fprintf(out, "</table>\n");
    }
    return;
#undef PRINT_SEP
}

static int
skipWhitespace(char *buf, int i)
{
    while(buf[i] == ' ' || buf[i] == '\t' || buf[i] == '\r')
        i++;
    return i;
}

static int
parseInt(char *buf, int offset, int *value_return)
{
    char *p;
    int value;

    value = strtol(buf + offset, &p, 0);
    if(p <= buf + offset)
        return -1;

    *value_return = value;
    return p - buf;
}

static struct config_state { char *name; int value; }
states[] = 
    { { "false", 0 }, 
      { "no", 0 },
      { "reluctantly", 1 },
      { "seldom", 1 },
      { "rarely", 1 },
      { "lazily", 1 },
      { "maybe", 2 },
      { "perhaps", 2 },
      { "happily", 3 },
      { "often", 3 },
      { "eagerly", 3 },
      { "true", 4 },
      { "yes", 4 } };

static int
parseState(char *buf, int offset, int kind)
{
    int i = offset;
    int n;
    int state = -1;

    while(letter(buf[i]))
        i++;
    for(n = 0; n < sizeof(states) / sizeof(states[0]); n++) {
        if(strlen(states[n].name) == i - offset &&
           lwrcmp(buf + offset, states[n].name, i - offset) == 0) {
            state = states[n].value;
            break;
        }
    }
    if(state < 0)
        return -1;

    switch(kind) {
    case CONFIG_BOOLEAN:
        if(state == 0) return 0;
        else if(state == 4) return 1;
        else return -1;
        break;
    case CONFIG_TRISTATE:
        if(state == 0) return 0;
        else if(state == 2) return 1;
        else if(state == 4) return 2;
        else return -1;
        break;
    case CONFIG_TETRASTATE:
        if(state == 0) return 0;
        else if(state == 1) return 1;
        else if(state == 3) return 2;
        else if(state == 4) return 3;
        else return -1;
        break;
    case CONFIG_PENTASTATE:
        return state;
        break;
    default:
        abort();
    }
}

static int
parseAtom(char *buf, int offset, AtomPtr *value_return, int insensitive)
{
    int y0, i, j, k;
    AtomPtr atom;
    int escape = 0;
    char *s;

    i = offset;
    if(buf[i] == '\"') {
        i++;
        y0 = i;
        while(buf[i] != '\"' && buf[i] != '\n' && buf[i] != '\0') {
            if(buf[i] == '\\' && buf[i + 1] != '\0') {
                escape = 1;
                i += 2;
            } else
                i++;
        }
        if(buf[i] != '\"')
            return -1;
        j = i + 1;
    } else {
        y0 = i;
        while(letter(buf[i]) || digit(buf[i]) || 
              buf[i] == '_' || buf[i] == '-' || buf[i] == '~' ||
              buf[i] == '.' || buf[i] == ':' || buf[i] == '/')
            i++;
        j = i;
    }

    if(escape) {
        s = malloc(i - y0);
        if(s == NULL) return -1;
        k = 0;
        j = y0;
        while(j < i) {
            if(buf[j] == '\\' && j <= i - 2) {
                s[k++] = buf[j + 1];
                j += 2;
            } else
                s[k++] = buf[j++];
        }
        if(insensitive)
            atom = internAtomLowerN(s, k);
        else
            atom = internAtomN(s, k);
        free(s);
        j++;
    } else {
        if(insensitive)
            atom = internAtomLowerN(buf + y0, i - y0);
        else
            atom = internAtomN(buf + y0, i - y0);
    }
    *value_return = atom;
    return j;
}

static int
parseTime(char *line, int i, int *value_return)
{
    int v = 0, w;
    while(1) {
        if(!digit(line[i]))
            break;
        w = atoi(line + i);
        while(digit(line[i])) i++;
        switch(line[i]) {
        case 'd': v += w * 24 * 3600; i++; break;
        case 'h': v += w * 3600; i++; break;
        case 'm': v += w * 60; i++; break;
        case 's': v += w; i++; break;
        default: v += w; goto done;
        }
    }
 done:
    *value_return = v;
    return i;
}

int
parseConfigLine(char *line, char *filename, int lineno, int set)
{
    int x0, x1;
    int i, from, to;
    AtomPtr name, value;
    ConfigVariablePtr var;
    int iv;
    float fv;
    AtomPtr av;
    AtomListPtr alv;
    IntListPtr ilv;

    i = skipWhitespace(line, 0);
    if(line[i] == '\n' || line[i] == '\0' || line[i] == '#')
        return 0;

    x0 = i;
    while(letter(line[i]) || digit(line[i]))
        i++;
    x1 = i;

    i = skipWhitespace(line, i);
    if(line[i] != '=') {
        goto syntax;
    }
    i++;
    i = skipWhitespace(line, i);

    name = internAtomN(line + x0, x1 - x0);
    var = findConfigVariable(name);
    releaseAtom(name);

    if(set && var->setter == NULL)
        return -2;
 
    if(var == NULL) {
        if(!set) {
            do_log(L_ERROR, "%s:%d: unknown config variable ",
                   filename, lineno);
            do_log_n(L_ERROR, line + x0, x1 - x0);
            do_log(L_ERROR, "\n");
        }
        return -1;
    }
    
    i = skipWhitespace(line, i);
    switch(var->type) {
    case CONFIG_INT: case CONFIG_OCTAL: case CONFIG_HEX:
        i = parseInt(line, i, &iv);
        if(i < 0) goto syntax;
        if(set)
            var->setter(var, &iv);
        else
            *var->value.i = iv;
    break;
    case CONFIG_TIME:
        i = parseTime(line, i, &iv);
        if(i < 0) goto syntax;
        i = skipWhitespace(line, i);
        if(line[i] != '\n' && line[i] != '\0' && line[i] != '#')
            goto syntax;
        if(set)
            var->setter(var, &iv);
        else
            *var->value.i = iv;
        break;
    case CONFIG_BOOLEAN:
    case CONFIG_TRISTATE:
    case CONFIG_TETRASTATE:
    case CONFIG_PENTASTATE:
        iv = parseState(line, i, var->type);
        if(iv < 0)
            goto syntax;
        if(set)
            var->setter(var, &iv);
        else
            *var->value.i = iv;
        break;
    case CONFIG_FLOAT: 
        if(!digit(line[i]) && line[i] != '.')
            goto syntax;
        fv = atof(line + i);
        if(set)
            var->setter(var, &fv);
        else
            *var->value.f = fv;
        break;
    case CONFIG_ATOM: case CONFIG_ATOM_LOWER: case CONFIG_PASSWORD:
        i = parseAtom(line, i, &av, (var->type == CONFIG_ATOM_LOWER));
        if(i < 0) goto syntax;
        if(!av) {
            if(!set)
                do_log(L_ERROR, "%s:%d: couldn't allocate atom.\n",
                       filename, lineno);
            return -1;
        }
        i = skipWhitespace(line, i);
        if(line[i] != '\n' && line[i] != '\0' && line[i] != '#') {
            releaseAtom(av);
            goto syntax;
        }
        if(set)
            var->setter(var, &av);
        else {
            if(*var->value.a) releaseAtom(*var->value.a);
            *var->value.a = av;
        }
        break;
    case CONFIG_INT_LIST:
        ilv = makeIntList(0);
        if(ilv == NULL) {
            if(!set)
                do_log(L_ERROR, "%s:%d: couldn't allocate int list.\n",
                       filename, lineno);
            return -1;
        }
        while(1) {
            i = parseInt(line, i, &from);
            if(i < 0) goto syntax;
            to = from;
            i = skipWhitespace(line, i);
            if(line[i] == '-') {
                i = skipWhitespace(line, i + 1);
                i = parseInt(line, i, &to);
                if(i < 0) {
                    destroyIntList(ilv);
                    goto syntax;
                }
                i = skipWhitespace(line, i);
            }
            intListCons(from, to, ilv);
            if(line[i] == '\n' || line[i] == '\0' || line[i] == '#')
                break;
            if(line[i] != ',') {
                destroyIntList(ilv);
                goto syntax;
            }
            i = skipWhitespace(line, i + 1);
        }
        if(set)
            var->setter(var, &ilv);
        else {
            if(*var->value.il) destroyIntList(*var->value.il);
            *var->value.il = ilv;
        }
        break;
    case CONFIG_ATOM_LIST: case CONFIG_ATOM_LIST_LOWER:
        alv = makeAtomList(NULL, 0);
        if(alv == NULL) {
            if(!set)
                do_log(L_ERROR, "%s:%d: couldn't allocate atom list.\n",
                       filename, lineno);
            return -1;
        }
        while(1) {
            i = parseAtom(line, i, &value, 
                          (var->type == CONFIG_ATOM_LIST_LOWER));
            if(i < 0) goto syntax;
            if(!value) {
                if(!set)
                    do_log(L_ERROR, "%s:%d: couldn't allocate atom.\n",
                           filename, lineno);
                return -1;
            }
            atomListCons(value, alv);
            i = skipWhitespace(line, i);
            if(line[i] == '\n' || line[i] == '\0' || line[i] == '#')
                break;
            if(line[i] != ',') {
                destroyAtomList(alv);
                goto syntax;
            }
            i = skipWhitespace(line, i + 1);
        }
        if(set)
            var->setter(var, &alv);
        else {
            if(*var->value.al) destroyAtomList(*var->value.al);
            *var->value.al = alv;
        }
        break;
    default: abort();
    }
    return 1;

 syntax:
    if(!set)
        do_log(L_ERROR, "%s:%d: parse error.\n", filename, lineno);
    return -1;
}

int
parseConfigFile(AtomPtr filename)
{
    char buf[512];
    int lineno;
    FILE *f;

    if(!filename || filename->length == 0)
        return 0;
    f = fopen(filename->string, "r");
    if(f == NULL) {
        do_log_error(L_ERROR, errno, "Couldn't open config file %s",
                     filename->string);
        return -1;
    }

    lineno = 1;
    while(1) {
        char *s;
        s = fgets(buf, 512, f);
        if(s == NULL) {
            fclose(f);
            return 1;
        }
        parseConfigLine(buf, filename->string, lineno, 0);
        lineno++;
    }
}

int
configIntSetter(ConfigVariablePtr var, void* value)
{
    assert(var->type <= CONFIG_PENTASTATE);
    *var->value.i = *(int*)value;
    return 1;
}

int
configFloatSetter(ConfigVariablePtr var, void* value)
{
    assert(var->type == CONFIG_FLOAT);
    *var->value.i = *(float*)value;
    return 1;
}


int
configAtomSetter(ConfigVariablePtr var, void* value)
{
    assert(var->type == CONFIG_ATOM || var->type == CONFIG_ATOM_LOWER ||
           var->type == CONFIG_PASSWORD);
    if(*var->value.a)
        releaseAtom(*var->value.a);
    *var->value.a = *(AtomPtr*)value;
    return 1;
}
