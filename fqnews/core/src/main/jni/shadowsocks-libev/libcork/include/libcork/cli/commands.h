/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2012, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef LIBCORK_COMMANDS_H
#define LIBCORK_COMMANDS_H

#include <libcork/core/api.h>


typedef void
(*cork_leaf_command_run)(int argc, char **argv);

typedef int
(*cork_option_parser)(int argc, char **argv);

enum cork_command_type {
    CORK_COMMAND_SET,
    CORK_LEAF_COMMAND
};

struct cork_command {
    enum cork_command_type  type;
    const char  *name;
    const char  *short_desc;
    const char  *usage_suffix;
    const char  *full_help;

    int
    (*parse_options)(int argc, char **argv);

    struct cork_command  **set;
    cork_leaf_command_run  run;
};

#define cork_command_set(name, sd, parse_options, set) \
{ \
    CORK_COMMAND_SET, name, sd, NULL, NULL, \
    parse_options, set, NULL \
}

#define cork_leaf_command(name, sd, us, fh, parse_options, run) \
{ \
    CORK_LEAF_COMMAND, name, sd, us, fh, \
    parse_options, NULL, run \
}

CORK_API void
cork_command_show_help(struct cork_command *command, const char *message);

CORK_API int
cork_command_main(struct cork_command *root, int argc, char **argv);


#endif /* LIBCORK_COMMANDS_H */
