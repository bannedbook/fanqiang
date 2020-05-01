/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2012, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libcork/cli.h"
#include "libcork/core.h"
#include "libcork/ds.h"


#define streq(a,b) (strcmp((a), (b)) == 0)

static struct cork_buffer  breadcrumbs_buf = CORK_BUFFER_INIT();

static void
cork_command_add_breadcrumb(struct cork_command *command)
{
    cork_buffer_append_printf(&breadcrumbs_buf, " %s", command->name);
}

#define cork_command_breadcrumbs() ((char *) breadcrumbs_buf.buf)

static void
cork_command_run(struct cork_command *command, int argc, char **argv);

static struct cork_command *
cork_command_set_get_subcommand(struct cork_command *command,
                                const char *command_name)
{
    struct cork_command  **curr;
    for (curr = command->set; *curr != NULL; curr++) {
        if (streq(command_name, (*curr)->name)) {
            return *curr;
        }
    }
    return NULL;
}

static void
cork_command_set_show_help(struct cork_command *command)
{
    size_t  max_length = 0;
    struct cork_command  **curr;

    /* Calculate the length of the longest command name. */
    for (curr = command->set; *curr != NULL; curr++) {
        size_t  len = strlen((*curr)->name);
        if (len > max_length) {
            max_length = len;
        }
    }

    /* Then print out the available commands. */
    printf("Usage:%s <command> [<options>]\n"
           "\nAvailable commands:\n",
           cork_command_breadcrumbs());

    for (curr = command->set; *curr != NULL; curr++) {
        printf("  %*s", (int) -max_length, (*curr)->name);
        if ((*curr)->short_desc != NULL) {
            printf("  %s\n", (*curr)->short_desc);
        } else {
            printf("\n");
        }
    }
}

static void
cork_command_leaf_show_help(struct cork_command *command)
{
    printf("Usage:%s", cork_command_breadcrumbs());
    if (command->usage_suffix != NULL) {
        printf(" %s", command->usage_suffix);
    }
    if (command->full_help != NULL) {
        printf("\n\n%s", command->full_help);
    } else {
        printf("\n");
    }
}

void
cork_command_show_help(struct cork_command *command, const char *message)
{
    if (message != NULL) {
        printf("%s\n", message);
    }

    if (command->type == CORK_COMMAND_SET) {
        cork_command_set_show_help(command);
    } else if (command->type == CORK_LEAF_COMMAND) {
        cork_command_leaf_show_help(command);
    }
}

static void
cork_command_set_run_help(struct cork_command *command, int argc, char **argv)
{
    /* When we see the help command when processing a command set, we use any
     * remaining arguments to identifity which subcommand the user wants help
     * with. */

    /* Skip over the name of the command set */
    argc--;
    argv++;

    while (argc > 0 && command->type == CORK_COMMAND_SET) {
        struct cork_command  *subcommand =
            cork_command_set_get_subcommand(command, argv[0]);
        if (subcommand == NULL) {
            printf("Unknown command \"%s\".\n"
                   "Usage:%s <command> [<options>]\n",
                   argv[0], cork_command_breadcrumbs());
            exit(EXIT_FAILURE);
        }

        cork_command_add_breadcrumb(subcommand);
        command = subcommand;
        argc--;
        argv++;
    }

    cork_command_show_help(command, NULL);
}

static void
cork_command_set_run(struct cork_command *command, int argc, char **argv)
{
    const char  *command_name;
    struct cork_command  *subcommand;

    if (argc == 0) {
        printf("No command given.\n");
        cork_command_set_show_help(command);
        exit(EXIT_FAILURE);
    }

    command_name = argv[0];

    /* The "help" command is special. */
    if (streq(command_name, "help")) {
        cork_command_set_run_help(command, argc, argv);
        return;
    }

    /* Otherwise look for a real subcommand with this name. */
    subcommand = cork_command_set_get_subcommand(command, command_name);
    if (subcommand == NULL) {
        printf("Unknown command \"%s\".\n"
               "Usage:%s <command> [<options>]\n",
               command_name, cork_command_breadcrumbs());
        exit(EXIT_FAILURE);
    } else {
        cork_command_run(subcommand, argc, argv);
    }
}

static void
cork_command_leaf_run(struct cork_command *command, int argc, char **argv)
{
    command->run(argc, argv);
}

static void
cork_command_cleanup(void)
{
    cork_buffer_done(&breadcrumbs_buf);
}

static void
cork_command_run(struct cork_command *command, int argc, char **argv)
{
    cork_command_add_breadcrumb(command);

    /* If the gives the --help option at this point, describe the current
     * command. */
    if (argc >= 2 && (streq(argv[1], "--help") || streq(argv[1], "-h"))) {
        cork_command_show_help(command, NULL);
        return;
    }

    /* Otherwise let the command parse any options that occur here. */
    if (command->parse_options != NULL) {
        int  option_count = command->parse_options(argc, argv);
        argc -= option_count;
        argv += option_count;
    } else {
        argc--;
        argv++;
    }

    switch (command->type) {
        case CORK_COMMAND_SET:
            cork_command_set_run(command, argc, argv);
            return;

        case CORK_LEAF_COMMAND:
            cork_command_leaf_run(command, argc, argv);
            return;

        default:
            cork_unreachable();
    }
}


int
cork_command_main(struct cork_command *root, int argc, char **argv)
{
    /* Clean up after ourselves when the command finishes. */
    atexit(cork_command_cleanup);

    /* Run the root command. */
    cork_command_run(root, argc, argv);
    return EXIT_SUCCESS;
}
