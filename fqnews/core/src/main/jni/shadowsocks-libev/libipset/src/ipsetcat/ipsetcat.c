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
static bool  want_networks = false;


static struct option longopts[] = {
    { "help", no_argument, NULL, 'h' },
    { "output", required_argument, NULL, 'o' },
    { "networks", no_argument, NULL, 'n' },
    { "verbose", 0, NULL, 'v' },
    { NULL, 0, NULL, 0 }
};

#define USAGE \
"Usage: ipsetcat [options] <input filename>\n"

#define FULL_USAGE \
USAGE \
"\n" \
"Prints out the (non-sorted) contents of a binary IP set file.\n" \
"\n" \
"Options:\n" \
"  <input filename>\n" \
"    The binary set file to read.  To read from stdin, use \"-\" as the\n" \
"    filename.\n" \
"  --output=<filename>, -o <filename>\n" \
"    Writes the contents of the binary IP set file to <filename>.  If this\n" \
"    option isn't given, then the contents will be written to standard\n" \
"    output.\n" \
"  --networks, -n\n" \
"    Where possible, we group the IP addresses in the set into CIDR network\n" \
"    blocks.  For dense sets, this can greatly reduce the amount of output\n" \
"    that's generated.\n" \
"  --verbose, -v\n" \
"    Show progress information about the files being read and written.  If\n" \
"    this option is not given, the only output will be any error messages\n" \
"    that occur.\n" \
"  --help\n" \
"    Display this help and exit.\n" \
"\n" \
"Output format:\n" \
"  The output will contain one IP address or network per line.  If you give\n" \
"  the \"--networks\" option, then we will collapse addresses into CIDR\n" \
"  networks where possible.  CIDR network blocks will have one of the\n" \
"  following formats:\n" \
"\n" \
"    x.x.x.x/cidr\n" \
"    xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx/cidr\n" \
"\n" \
"  Individual IP addresses will have one of the following formats:\n" \
"\n" \
"    x.x.x.x\n" \
"    xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx\n" \
"\n" \
"  Note that we never include a /32 or /128 suffix for individual addresses,\n" \
"  even if you've requested CIDR networks via the \"--networks\" option.\n" \
"\n" \
"  Please note that the output is UNSORTED.  There are no guarantees made\n" \
"  about the order of the IP addresses and networks in the output.\n"

int
main(int argc, char **argv)
{
    ipset_init_library();

    /* Parse the command-line options. */

    int  ch;
    while ((ch = getopt_long(argc, argv, "hno:", longopts, NULL)) != -1) {
        switch (ch) {
            case 'h':
                fprintf(stdout, FULL_USAGE);
                exit(0);

            case 'n':
                want_networks = true;
                break;

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
        fprintf(stderr, "ipsetcat: You must specify exactly one input file.\n");
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

    /* Print out the IP addresses in the set. */
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

    char  ip_buf[CORK_IP_STRING_LENGTH];
    struct cork_buffer  buf = CORK_BUFFER_INIT();

    struct ipset_iterator  *it;
    if (want_networks) {
        /* If requested, iterate through network blocks instead of
         * individual IP addresses. */
        it = ipset_iterate_networks(set, true);
    } else {
        /* The user wants individual IP addresses.  Hope they know what
         * they're doing! */
        it = ipset_iterate(set, true);
    }

    for (/* nothing */; !it->finished; ipset_iterator_advance(it)) {
        cork_ip_to_raw_string(&it->addr, ip_buf);
        if ((it->addr.version == 4 && it->cidr_prefix == 32) ||
            (it->addr.version == 6 && it->cidr_prefix == 128)) {
            cork_buffer_printf(&buf, "%s\n", ip_buf);
        } else {
            cork_buffer_printf(&buf, "%s/%u\n", ip_buf, it->cidr_prefix);
        }

        if (fputs(buf.buf, ostream) == EOF) {
            fprintf(stderr, "Cannot write to file %s:\n  %s\n",
                    output_filename, strerror(errno));
            exit(1);
        }
    }

    cork_buffer_done(&buf);
    ipset_free(set);

    /* Close the output stream for exiting. */
    if (close_ostream) {
        fclose(ostream);
    }

    return 0;
}
