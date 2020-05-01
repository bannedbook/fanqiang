/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2010-2012, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */


#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libcork/core.h>

#include "ipset/ipset.h"


static char  *input_filename = NULL;
static char  *output_filename = "-";
static bool  verbose = false;


static struct option longopts[] = {
    { "help", no_argument, NULL, 'h' },
    { "output", required_argument, NULL, 'o' },
    { "verbose", 0, NULL, 'v' },
    { NULL, 0, NULL, 0 }
};

#define USAGE \
"Usage: ipsetdot [options] <input filename>\n"

#define FULL_USAGE \
USAGE \
"\n" \
"Creates a GraphViz file showing the BDD structure of an IP set.\n" \
"\n" \
"Options:\n" \
"  <input filename>\n" \
"    The binary set file to read.  To read from stdin, use \"-\" as the\n" \
"    filename.\n" \
"  --output=<filename>, -o <filename>\n" \
"    Writes the GraphViz representation of the binary IP set file to\n" \
"    <filename>.  If this option isn't given, then the contents will be\n" \
"    written to standard output.\n" \
"  --verbose, -v\n" \
"    Show progress information about the files being read and written.  If\n" \
"    this option is not given, the only output will be any error messages\n" \
"    that occur.\n" \
"  --help\n" \
"    Display this help and exit.\n" \
"\n" \
"Output format:\n" \
"  Internally, IP sets are represented by a binary-decision diagram (BDD).\n" \
"  The ipsetdot program can be used to produce a GraphViz file that describes\n" \
"  the internal BDD structure for an IP set.  The GraphViz representation can\n" \
"  then be passed in to GraphViz's \"dot\" program, for instance, to generate\n" \
"  an image of the BDD's graph structure.\n"


int
main(int argc, char **argv)
{
    ipset_init_library();

    /* Parse the command-line options. */

    int  ch;
    while ((ch = getopt_long(argc, argv, "ho:", longopts, NULL)) != -1) {
        switch (ch) {
            case 'h':
                fprintf(stdout, FULL_USAGE);
                exit(0);

            case 'o':
                output_filename = optarg;
                break;

            case 'v':
                verbose = true;
                break;

            default:
                fprintf(stderr, USAGE);
                exit(1);
        }
    }

    argc -= optind;
    argv += optind;

    if (argc != 1) {
        fprintf(stderr, "ipsetdot: You must specify exactly one input file.\n");
        fprintf(stderr, USAGE);
        exit(1);
    }

    input_filename = argv[0];

    /* Read in the IP set files specified on the command line. */
    struct ip_set  *set = NULL;
    FILE  *stream;
    bool  close_stream;

    /* Create a FILE object for the file. */
    if (strcmp(input_filename, "-") == 0) {
        if (verbose) {
            fprintf(stderr, "Opening stdin...\n");
        }
        input_filename = "stdin";
        stream = stdin;
        close_stream = false;
    } else {
        if (verbose) {
            fprintf(stderr, "Opening file %s...\n", input_filename);
        }
        stream = fopen(input_filename, "rb");
        if (stream == NULL) {
            fprintf(stderr, "Cannot open file %s:\n  %s\n",
                    input_filename, strerror(errno));
            exit(1);
        }
        close_stream = true;
    }

    /* Read in the IP set from the specified file. */
    set = ipset_load(stream);
    if (set == NULL) {
        fprintf(stderr, "Error reading %s:\n  %s\n",
                input_filename, cork_error_message());
        exit(1);
    }

    if (close_stream) {
        fclose(stream);
    }

    /* Generate a GraphViz dot file for the set. */

    FILE  *ostream;
    bool  close_ostream;

    if ((output_filename == NULL) || (strcmp(output_filename, "-") == 0)) {
        if (verbose) {
            fprintf(stderr, "Writing to stdout...\n");
        }
        ostream = stdout;
        output_filename = "stdout";
        close_ostream = false;
    } else {
        if (verbose) {
            fprintf(stderr, "Writing to file %s...\n", output_filename);
        }
        ostream = fopen(output_filename, "wb");
        if (ostream == NULL) {
            fprintf(stderr, "Cannot open file %s:\n  %s\n",
                    output_filename, strerror(errno));
            exit(1);
        }
        close_ostream = true;
    }

    if (ipset_save_dot(ostream, set) != 0) {
        fprintf(stderr, "Error saving IP set:\n  %s\n",
                cork_error_message());
        exit(1);
    }

    ipset_free(set);

    /* Close the output stream for exiting. */
    if (close_ostream) {
        fclose(ostream);
    }

    return 0;
}
