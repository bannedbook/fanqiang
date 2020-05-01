/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2012, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libcork/cli.h"
#include "libcork/core.h"
#include "libcork/ds.h"
#include "libcork/os.h"


#define streq(s1, s2)  (strcmp((s1), (s2)) == 0)

#define ri_check_exit(call) \
    do { \
        if ((call) != 0) { \
            fprintf(stderr, "%s\n", cork_error_message()); \
            exit(EXIT_FAILURE); \
        } \
    } while (0)

#define rp_check_exit(call) \
    do { \
        if ((call) == NULL) { \
            fprintf(stderr, "%s\n", cork_error_message()); \
            exit(EXIT_FAILURE); \
        } \
    } while (0)


/*-----------------------------------------------------------------------
 * Command list
 */

static bool  test_option = false;
static const char  *file_option = NULL;

/* cork-test c1 s1 */

static int
c1_s1_options(int argc, char **argv);

static void
c1_s1_run(int argc, char **argv);

static struct cork_command  c1_s1 =
    cork_leaf_command("s1", "Subcommand 1", "[<options>] <filename>",
                      "This is a pretty cool command.\n",
                      c1_s1_options, c1_s1_run);

static int
c1_s1_options(int argc, char **argv)
{
    if (argc >= 2 && (streq(argv[1], "-t") || streq(argv[1], "--test"))) {
        test_option = true;
        return 2;
    } else {
        return 1;
    }
}

static void
c1_s1_run(int argc, char **argv)
{
    printf("You chose command \"c1 s1\".  Good for you!\n");
    if (test_option) {
        printf("And you gave the --test option!  Look at that.\n");
    }
    if (file_option != NULL) {
        printf("And you want the file to be %s.  Sure thing.\n", file_option);
    }
    exit(EXIT_SUCCESS);
}


/* cork-test c1 s2 */

static void
c1_s2_run(int argc, char **argv)
{
    printf("You chose command \"c1 s2\".  Fantastico!\n");
    if (file_option != NULL) {
        struct cork_stream_consumer  *consumer;
        printf("And you want the file to be %s.  Sure thing.\n", file_option);

        /* Print the contents of the file to stdout. */
        rp_check_exit(consumer = cork_file_consumer_new(stdout));
        ri_check_exit(cork_consume_file_from_path
                      (consumer, file_option, O_RDONLY));
        cork_stream_consumer_free(consumer);
    }

    exit(EXIT_SUCCESS);
}

static struct cork_command  c1_s2 =
    cork_leaf_command("s2", "Subcommand 2", "[<options>] <filename>",
                      "This is an excellent command.\n",
                      NULL, c1_s2_run);


/* cork-test c1 */

static int
c1_options(int argc, char **argv);

static struct cork_command  *c1_subcommands[] = {
    &c1_s1, &c1_s2, NULL
};

static struct cork_command  c1 =
    cork_command_set("c1", "Command 1 (now with subcommands)",
                     c1_options, c1_subcommands);

static int
c1_options(int argc, char **argv)
{
    if (argc >= 3) {
        if (streq(argv[1], "-f") || streq(argv[1], "--file")) {
            file_option = argv[2];
            return 3;
        }
    }

    if (argc >= 2) {
        if (memcmp(argv[1], "--file=", 7) == 0) {
            file_option = argv[1] + 7;
            return 2;
        }
    }

    return 1;
}


/* cork-test c2 */

static void
c2_run(int argc, char **argv)
{
    printf("You chose command \"c2\".  That's pretty good.\n");
    exit(EXIT_SUCCESS);
}

static struct cork_command  c2 =
    cork_leaf_command("c2", "Command 2", "[<options>] <filename>",
                      "This command is pretty decent.\n",
                      NULL, c2_run);


/*-----------------------------------------------------------------------
 * Forking subprocesses
 */

static const char  *sub_cwd = NULL;
static const char  *sub_stdin = NULL;

static int
sub_options(int argc, char **argv);

static void
sub_run(int argc, char **argv);

static struct cork_command  sub =
    cork_leaf_command("sub", "Run a subcommand", "<program> [<options>]",
                      "Runs a subcommand.\n",
                      sub_options, sub_run);

static int
sub_options(int argc, char **argv)
{
    int  processed = 1;
    for (argc--, argv++; argc >= 1; argc--, argv++, processed++) {
        if ((streq(argv[0], "-d") || streq(argv[0], "--cwd"))) {
            if (argc >= 2) {
                sub_cwd = argv[1];
                argc--, argv++, processed++;
            } else {
                cork_command_show_help(&sub, "Missing directory for --cwd");
                exit(EXIT_FAILURE);
            }
        } else if ((streq(argv[0], "-i") || streq(argv[0], "--stdin"))) {
            if (argc >= 2) {
                sub_stdin = argv[1];
                argc--, argv++, processed++;
            } else {
                cork_command_show_help(&sub, "Missing content for --stdin");
                exit(EXIT_FAILURE);
            }
        } else {
            return processed;
        }
    }
    return processed;
}

static void
sub_run(int argc, char **argv)
{
    struct cork_env  *env;
    struct cork_exec  *exec;
    struct cork_subprocess_group  *group;
    struct cork_subprocess  *sp;
    struct cork_stream_consumer  *stdin_consumer;

    if (argc == 0) {
        cork_command_show_help(&sub, "Missing command");
        exit(EXIT_FAILURE);
    }

    rp_check_exit(env = cork_env_clone_current());
    rp_check_exit(exec = cork_exec_new_with_param_array(argv[0], argv));
    cork_exec_set_env(exec, env);
    if (sub_cwd != NULL) {
        cork_exec_set_cwd(exec, sub_cwd);
    }
    fprintf(stderr, "%s\n", cork_exec_description(exec));
    rp_check_exit(group = cork_subprocess_group_new());
    rp_check_exit(sp = cork_subprocess_new_exec(exec, NULL, NULL, NULL));
    cork_subprocess_group_add(group, sp);
    ri_check_exit(cork_subprocess_group_start(group));
    stdin_consumer = cork_subprocess_stdin(sp);
    if (sub_stdin != NULL) {
        size_t  stdin_length = strlen(sub_stdin);
        cork_stream_consumer_data
            (stdin_consumer, sub_stdin, stdin_length, true);
        cork_stream_consumer_data(stdin_consumer, "\n", 1, false);
    }
    cork_stream_consumer_eof(stdin_consumer);
    ri_check_exit(cork_subprocess_group_wait(group));
    cork_subprocess_group_free(group);
}


/*-----------------------------------------------------------------------
 * pwd
 */

/* cork-test pwd */

static void
pwd_run(int argc, char **argv);

static struct cork_command  pwd =
    cork_leaf_command("pwd", "Print working directory",
                      "",
                      "Prints out the current working directory.\n",
                      NULL, pwd_run);

static void
pwd_run(int argc, char **argv)
{
    struct cork_path  *path;
    rp_check_exit(path = cork_path_cwd());
    printf("%s\n", cork_path_get(path));
    cork_path_free(path);
    exit(EXIT_SUCCESS);
}


/*-----------------------------------------------------------------------
 * mkdir
 */

static unsigned int  mkdir_flags = CORK_FILE_PERMISSIVE;

/* cork-test mkdir */

static int
mkdir_options(int argc, char **argv);

static void
mkdir_run(int argc, char **argv);

static struct cork_command  mkdir_cmd =
    cork_leaf_command("mkdir", "Create a directory",
                      "[<options>] <path>",
                      "Create a new directory.\n",
                      mkdir_options, mkdir_run);

static int
mkdir_options(int argc, char **argv)
{
    int  count = 1;

    while (count < argc) {
        if (streq(argv[count], "--recursive")) {
            mkdir_flags |= CORK_FILE_RECURSIVE;
            count++;
        } else if (streq(argv[count], "--require")) {
            mkdir_flags &= ~CORK_FILE_PERMISSIVE;
            count++;
        } else {
            return count;
        }
    }

    return count;
}

static void
mkdir_run(int argc, char **argv)
{
    struct cork_file  *file;

    if (argc < 1) {
        cork_command_show_help(&mkdir_cmd, "Missing file");
        exit(EXIT_FAILURE);
    } else if (argc > 1) {
        cork_command_show_help(&mkdir_cmd, "Too many directories");
        exit(EXIT_FAILURE);
    }

    file = cork_file_new(argv[0]);
    ri_check_exit(cork_file_mkdir(file, 0755, mkdir_flags));
    cork_file_free(file);
    exit(EXIT_SUCCESS);
}


/*-----------------------------------------------------------------------
 * rm
 */

static unsigned int  rm_flags = CORK_FILE_PERMISSIVE;

/* cork-test rm */

static int
rm_options(int argc, char **argv);

static void
rm_run(int argc, char **argv);

static struct cork_command  rm_cmd =
    cork_leaf_command("rm", "Remove a file or directory",
                      "[<options>] <path>",
                      "Remove a file or directory.\n",
                      rm_options, rm_run);

static int
rm_options(int argc, char **argv)
{
    int  count = 1;

    while (count < argc) {
        if (streq(argv[count], "--recursive")) {
            rm_flags |= CORK_FILE_RECURSIVE;
            count++;
        } else if (streq(argv[count], "--require")) {
            rm_flags &= ~CORK_FILE_PERMISSIVE;
            count++;
        } else {
            return count;
        }
    }

    return count;
}

static void
rm_run(int argc, char **argv)
{
    struct cork_file  *file;

    if (argc < 1) {
        cork_command_show_help(&rm_cmd, "Missing file");
        exit(EXIT_FAILURE);
    } else if (argc > 1) {
        cork_command_show_help(&rm_cmd, "Too many directories");
        exit(EXIT_FAILURE);
    }

    file = cork_file_new(argv[0]);
    ri_check_exit(cork_file_remove(file, rm_flags));
    cork_file_free(file);
    exit(EXIT_SUCCESS);
}


/*-----------------------------------------------------------------------
 * find
 */

static bool  find_all = false;

/* cork-test find */

static int
find_options(int argc, char **argv);

static void
find_run(int argc, char **argv);

static struct cork_command  find =
    cork_leaf_command("find", "Search for a file in a list of directories",
                      "<file> <path list>",
                      "Search for a file in a list of directories.\n",
                      find_options, find_run);

static int
find_options(int argc, char **argv)
{
    if (argc >= 2 && streq(argv[1], "--all")) {
        find_all = true;
        return 2;
    }
    return 1;
}

static void
find_run(int argc, char **argv)
{
    struct cork_path_list  *list;

    if (argc < 1) {
        cork_command_show_help(&find, "Missing file");
        exit(EXIT_FAILURE);
    } else if (argc < 2) {
        cork_command_show_help(&find, "Missing path");
        exit(EXIT_FAILURE);
    } else if (argc < 2) {
        cork_command_show_help(&find, "Too many parameters");
        exit(EXIT_FAILURE);
    }

    list = cork_path_list_new(argv[1]);

    if (find_all) {
        struct cork_file_list  *file_list;
        size_t  i;
        size_t  count;
        rp_check_exit(file_list = cork_path_list_find_files(list, argv[0]));
        count = cork_file_list_size(file_list);
        for (i = 0; i < count; i++) {
            struct cork_file  *file = cork_file_list_get(file_list, i);
            printf("%s\n", cork_path_get(cork_file_path(file)));
        }
        cork_file_list_free(file_list);
    } else {
        struct cork_file  *file;
        rp_check_exit(file = cork_path_list_find_file(list, argv[0]));
        printf("%s\n", cork_path_get(cork_file_path(file)));
        cork_file_free(file);
    }

    cork_path_list_free(list);
    exit(EXIT_SUCCESS);
}


/*-----------------------------------------------------------------------
 * paths
 */

/* cork-test paths */

static void
paths_run(int argc, char **argv);

static struct cork_command  paths =
    cork_leaf_command("paths", "Print out standard paths for the current user",
                      "",
                      "Print out standard paths for the current user.\n",
                      NULL, paths_run);

static void
print_path(const char *prefix, struct cork_path *path)
{
    rp_check_exit(path);
    printf("%s %s\n", prefix, cork_path_get(path));
    cork_path_free(path);
}

static void
print_path_list(const char *prefix, struct cork_path_list *list)
{
    rp_check_exit(list);
    printf("%s %s\n", prefix, cork_path_list_to_string(list));
    cork_path_list_free(list);
}

static void
paths_run(int argc, char **argv)
{
    print_path     ("Home:   ", cork_path_home());
    print_path_list("Config: ", cork_path_config_paths());
    print_path_list("Data:   ", cork_path_data_paths());
    print_path     ("Cache:  ", cork_path_user_cache_path());
    print_path     ("Runtime:", cork_path_user_runtime_path());
    exit(EXIT_SUCCESS);
}


/*-----------------------------------------------------------------------
 * Directory walker
 */

static bool  only_files = false;
static bool  shallow = false;
static const char  *dir_path = NULL;

static int
dir_options(int argc, char **argv)
{
    if (argc == 3) {
        if (streq(argv[1], "--shallow")) {
            shallow = true;
            dir_path = argv[2];
            return 3;
        } else if (streq(argv[1], "--only-files")) {
            only_files = true;
            dir_path = argv[2];
            return 3;
        }
    }

    else if (argc == 2) {
        dir_path = argv[1];
        return 2;
    }

    fprintf(stderr, "Invalid usage.\n");
    exit(EXIT_FAILURE);
}

static size_t  indent = 0;

static void
print_indent(void)
{
    size_t  i;
    for (i = 0; i < indent; i++) {
        printf("  ");
    }
}

static int
enter_directory(struct cork_dir_walker *walker, const char *full_path,
                const char *rel_path, const char *base_name)
{
    print_indent();
    if (shallow) {
        printf("Skipping %s\n", rel_path);
        return CORK_SKIP_DIRECTORY;
    } else if (only_files) {
        return 0;
    } else {
        printf("Entering %s (%s)\n", base_name, rel_path);
        indent++;
        return 0;
    }
}

static int
print_file(struct cork_dir_walker *walker, const char *full_path,
           const char *rel_path, const char *base_name)
{
    if (only_files) {
        printf("%s\n", rel_path);
    } else {
        print_indent();
        printf("%s (%s) (%s)\n", base_name, rel_path, full_path);
    }
    return 0;
}

static int
leave_directory(struct cork_dir_walker *walker, const char *full_path,
                const char *rel_path, const char *base_name)
{
    if (!only_files) {
        indent--;
        print_indent();
        printf("Leaving %s\n", rel_path);
    }
    return 0;
}

static struct cork_dir_walker  walker = {
    enter_directory,
    print_file,
    leave_directory
};

static void
dir_run(int argc, char **argv)
{
    ri_check_exit(cork_walk_directory(dir_path, &walker));
    exit(EXIT_SUCCESS);
}

static struct cork_command  dir =
    cork_leaf_command("dir", "Print the contents of a directory",
                      "[--shallow] <path>",
                      "Prints the contents of a directory.\n",
                      dir_options, dir_run);


/*-----------------------------------------------------------------------
 * Cleanup functions
 */

#define define_cleanup_function(id) \
static void \
cleanup_##id(void) \
{ \
    printf("Cleanup function " #id "\n"); \
}

define_cleanup_function(0);
define_cleanup_function(1);
define_cleanup_function(2);
define_cleanup_function(3);
define_cleanup_function(4);
define_cleanup_function(5);

static void
cleanup_run(int argc, char **argv)
{
    cork_cleanup_at_exit(10, cleanup_1);
    cork_cleanup_at_exit( 0, cleanup_0);
    cork_cleanup_at_exit(50, cleanup_5);
    cork_cleanup_at_exit(20, cleanup_2);
    cork_cleanup_at_exit(40, cleanup_4);
    cork_cleanup_at_exit(30, cleanup_3);
}

static struct cork_command  cleanup =
    cork_leaf_command("cleanup", "Test process cleanup functions", "",
                      "Test process cleanup functions.\n",
                      NULL, cleanup_run);


/*-----------------------------------------------------------------------
 * Root command
 */

/* [root] cork-test */

static struct cork_command  *root_subcommands[] = {
    &c1, &c2,
    &pwd,
    &mkdir_cmd,
    &rm_cmd,
    &find,
    &paths,
    &dir,
    &sub,
    &cleanup,
    NULL
};

static struct cork_command  root_command =
    cork_command_set("cork-test", NULL, NULL, root_subcommands);


/*-----------------------------------------------------------------------
 * Entry point
 */

int
main(int argc, char **argv)
{
    return cork_command_main(&root_command, argc, argv);
}
