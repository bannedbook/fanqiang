/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2010-2012, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libcork/core.h>

#include "ipset/ipset.h"


static char  *output_filename = NULL;
static bool  loose_cidr = false;
static int  verbosity = 0;

struct removal {
    size_t  line;
    const char  *filename;
    struct cork_ip  address;
    unsigned int  cidr;
    bool  has_cidr;
};

static struct option longopts[] = {
    { "help", no_argument, NULL, 'h' },
    { "output", required_argument, NULL, 'o' },
    { "loose-cidr", 0, NULL, 'l' },
    { "verbose", 0, NULL, 'v' },
    { "quiet", 0, NULL, 'q' },
    { NULL, 0, NULL, 0 }
};

static bool
is_string_whitespace(const char *str)
{
    while (*str) {
        if (isspace(*str) == 0) {
            return false;
        }
        str++;
    }
    return true;
}

#define USAGE \
"Usage: ipsetbuild [options] <input file>...\n"

#define FULL_USAGE \
USAGE \
"\n" \
"Constructs a binary IP set file from a list of IP addresses and networks.\n" \
"\n" \
"Options:\n" \
"  <input file>...\n" \
"    A list of text files that contain the IP addresses and networks to add\n" \
"    to the set.  To read from stdin, use \"-\" as the filename.\n" \
"  --output=<filename>, -o <filename>\n" \
"    Writes the binary IP set file to <filename>.  If this option isn't\n" \
"    given, then the binary set will be written to standard output.\n" \
"  --loose-cidr, -l\n" \
"    Be more lenient about the address portion of any CIDR network blocks\n" \
"    found in the input file.\n" \
"  --verbose, -v\n" \
"    Show summary information about the IP set that's built, as well as\n" \
"    progress information about the files being read and written.  If this\n" \
"    option is not given, the only output will be any error, alert, or\n" \
"    warning messages that occur.\n" \
"  --quiet, -q\n" \
"    Show only error message for malformed input. All warnings, alerts,\n" \
"    and summary information about the IP set is suppressed.\n" \
"  --help\n" \
"    Display this help and exit.\n" \
"\n" \
"Input format:\n" \
"  Each input file must contain one IP address or network per line.  Lines\n" \
"  beginning with a \"#\" are considered comments and are ignored.  Each\n" \
"  IP address must have one of the following formats:\n" \
"\n" \
"    x.x.x.x\n" \
"    x.x.x.x/cidr\n" \
"    xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx\n" \
"    xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx/cidr\n" \
"\n" \
"  The first two are for IPv4 addresses and networks; the second two for\n" \
"  IPv6 addresses and networks.  For IPv6 addresses, you can use the \"::\"\n" \
"  shorthand notation to collapse consecutive \"0\" portions.\n" \
"\n" \
"  If an address contains a \"/cidr\" suffix, then the entire CIDR network\n" \
"  of addresses will be added to the set.  You must ensure that the low-\n" \
"  order bits of the address are set to 0; if not, we'll raise an error.\n" \
"  (If you pass in the \"--loose-cidr\" option, we won't perform this\n" \
"  sanity check.)\n" \
"\n" \
"  You can also prefix any input line with an exclamation point (\"!\").\n" \
"  This causes the given address or network to be REMOVED from the output\n" \
"  set.  This notation can be useful to define a set that contains most of\n" \
"  the addresses in a large CIDR block, except for addresses at certain\n" \
"  \"holes\".\n" \
"\n" \
"  The order of the addresses and networks given to ipsetbuild does not\n" \
"  matter.  If a particular address is added to the set more than once, or\n" \
"  removed from the set more than once, whether on its own or via a CIDR\n" \
"  network, then you will get a warning message.  (You can silence these\n" \
"  warnings with the --quiet option.)  If an address is both added to and\n" \
"  removed from the set, then the removal takes precedence, regardless of\n" \
"  where the relevant lines appear in the input file.\n"


int
main(int argc, char **argv)
{
    ipset_init_library();

    /* Parse the command-line options. */

    int  ch;
    while ((ch = getopt_long(argc, argv, "hlo:vq", longopts, NULL)) != -1) {
        switch (ch) {
            case 'h':
                fprintf(stdout, FULL_USAGE);
                exit(0);

            case 'l':
                loose_cidr = true;
                break;

            case 'o':
                output_filename = optarg;
                break;

            case 'v':
                verbosity++;
                break;

            case 'q':
                verbosity--;
                break;

            default:
                fprintf(stderr, USAGE);
                exit(1);
        }
    }

    argc -= optind;
    argv += optind;

    /* Verify that the user specified at least one text file to read. */
    if (argc == 0) {
        fprintf(stderr,
                "ipsetbuild: You need to specify at least one input file.\n");
        fprintf(stderr, USAGE);
        exit(1);
    }

    /* And an output file to write to. */
    if (output_filename == NULL) {
        fprintf(stderr,
                "ipsetbuild: You need to specify an output file.\n");
        fprintf(stderr, USAGE);
        exit(1);
    }

    /* Read in the IP set files specified on the command line. */

    size_t  total_ip_added = 0;
    size_t  total_ip_removed = 0;
    size_t  total_ip_v4 = 0;
    size_t  total_ip_v4_block = 0;
    size_t  total_ip_v6 = 0;
    size_t  total_ip_v6_block = 0;
    struct ip_set  set;
    struct removal  *entry;
    cork_array(struct removal)  removals;
    bool  read_from_stdin = false;
    bool  set_unchanged = false;

    ipset_init(&set);
    cork_array_init(&removals);

    int  i;
    for (i = 0; i < argc; i++) {
        const char  *filename = argv[i];
        FILE  *stream;
        bool  close_stream;

        /* Create a FILE object for the file. */
        if (strcmp(filename, "-") == 0) {
            if (read_from_stdin) {
                fprintf(stderr,
                        "ipsetbuild: Cannot read from stdin more than once.\n");
                exit(1);
            }
            if (verbosity > 0) {
                fprintf(stderr, "Opening stdin...\n\n");
            }
            filename = "stdin";
            stream = stdin;
            close_stream = false;
            read_from_stdin = true;
        } else {
            if (verbosity > 0) {
                fprintf(stderr, "Opening file %s...\n", filename);
            }
            stream = fopen(filename, "rb");
            if (stream == NULL) {
                fprintf(stderr, "ipsetbuild: Cannot open file %s:\n  %s\n",
                        filename, strerror(errno));
                exit(1);
            }
            close_stream = true;
        }

        /* Read in one IP address per line in the file. */
        size_t  ip_count = 0;
        size_t  ip_count_v4 = 0;
        size_t  ip_count_v4_block = 0;
        size_t  ip_count_v6 = 0;
        size_t  ip_count_v6_block = 0;
        size_t  line_num = 0;
        size_t  ip_error_num = 0;
        bool  ip_error = false;

#define MAX_LINELENGTH  4096
        char  line[MAX_LINELENGTH];
        char  *slash_pos;
        unsigned int  cidr = 0;

        while (fgets(line, MAX_LINELENGTH, stream) != NULL) {
            char  *address;
            struct cork_ip  addr;
            bool  remove_ip = false;

            line_num++;

            /* Skip empty lines and comments. Comments start with '#'
             * in the first column. */
            if ((line[0] == '#') || (is_string_whitespace(line))) {
                continue;
            }

            /* Check for a negating IP address.  If so, then the IP address
             * starts just after the '!'. */
            if (line[0] == '!') {
                remove_ip = true;
                address = line + 1;
            } else {
                address = line;
            }

            /* Chomp the trailing newline so we don't confuse our IP
             * address parser. */
            size_t  len = strlen(address);
            address[len-1] = '\0';

            /* Check for a / indicating a CIDR block.  If one is
             * present, split the string there and parse the trailing
             * part as a CIDR prefix integer. */
            if ((slash_pos = strchr(address, '/')) != NULL) {
                char  *endptr;
                *slash_pos = '\0';
                slash_pos++;
                cidr = (unsigned int) strtol(slash_pos, &endptr, 10);
                if (endptr == slash_pos) {
                    fprintf(stderr,
                            "Error: Line %zu: Missing CIDR prefix\n",
                            line_num);
                    ip_error_num++;
                    ip_error = true;
                    continue;
                } else if (*slash_pos == '\0' || *endptr != '\0') {
                    fprintf(stderr,
                            "Error: Line %zu: Invalid CIDR prefix \"%s\"\n",
                            line_num, slash_pos);
                    ip_error_num++;
                    ip_error = true;
                    continue;
                }
            }

            /* Try to parse the line as an IP address. */
            if (cork_ip_init(&addr, address) != 0) {
                fprintf(stderr, "Error: Line %zu: %s\n",
                        line_num, cork_error_message());
                cork_error_clear();
                ip_error_num++;
                ip_error = true;
                continue;
            }

            /* Add to address to the ipset and update the counters */
            if (slash_pos == NULL) {
                if (remove_ip) {
                    entry = cork_array_append_get(&removals);
                    entry->line = line_num;
                    entry->filename = filename;
                    entry->address = addr;
                    entry->cidr = 0;
                    entry->has_cidr = false;
                } else {
                    set_unchanged = ipset_ip_add(&set, &addr);
                    if (set_unchanged && verbosity >= 0) {
                        fprintf(stderr,
                                "Alert: %s, line %zu: %s is a duplicate\n",
                                filename, line_num, address);
                    } else {
                        if (addr.version == 4) {
                            ip_count_v4++;
                        } else {
                            ip_count_v6++;
                        }
                        ip_count++;
                    }
                }
            } else {
                /* If loose-cidr was not a command line option, then check the
                 * alignment of the IP address with the CIDR block. */
                if (!loose_cidr) {
                    if (!cork_ip_is_valid_network(&addr, cidr)) {
                        fprintf(stderr, "Error: %s, line %zu: Bad CIDR block: "
                                "\"%s/%u\"\n",
                                filename, line_num, address, cidr);
                        ip_error_num++;
                        ip_error = true;
                        continue;
                    }
                }
                if (remove_ip) {
                    entry = cork_array_append_get(&removals);
                    entry->line = line_num;
                    entry->filename = filename;
                    entry->address = addr;
                    entry->cidr = cidr;
                    entry->has_cidr = true;
                } else {
                    set_unchanged = ipset_ip_add_network(&set, &addr, cidr);
                }
                if (cork_error_occurred()) {
                    fprintf(stderr, "Error: %s, line %zu: Invalid IP address: "
                            "\"%s/%u\": %s\n", filename, line_num, address,
                            cidr, cork_error_message());
                    cork_error_clear();
                    ip_error_num++;
                    ip_error = true;
                    continue;
                }
                if (!remove_ip) {
                    if (set_unchanged && verbosity >= 0) {
                        fprintf(stderr,
                                "Alert: %s, line %zu: %s/%u is a duplicate\n",
                                filename, line_num, address, cidr);
                    } else {
                        if (addr.version == 4) {
                            ip_count_v4_block++;
                        } else {
                            ip_count_v6_block++;
                        }
                        ip_count++;
                    }
                }
            }
        }

        if (ferror(stream)) {
            /* There was an error reading from the stream. */
            fprintf(stderr, "Error reading from %s:\n  %s\n",
                    filename, strerror(errno));
            exit(1);
        }

        if (verbosity > 0) {
            fprintf(stderr,
                    "Summary: Read %zu valid IP address records from %s.\n",
                    ip_count, filename);
            fprintf(stderr, "  IPv4: %zu addresses, %zu block%s\n", ip_count_v4,
                    ip_count_v4_block, (ip_count_v4_block == 1)? "": "s");
            fprintf(stderr, "  IPv6: %zu addresses, %zu block%s\n", ip_count_v6,
                    ip_count_v6_block, (ip_count_v6_block == 1)? "": "s");
        }

        /* Update the total IP counters. */
        total_ip_added += ip_count;
        total_ip_v4 += ip_count_v4;
        total_ip_v4_block += ip_count_v4_block;
        total_ip_v6 += ip_count_v6;
        total_ip_v6_block += ip_count_v6_block;

        /* Free the streams before opening the next file. */
        if (close_stream) {
            fclose(stream);
        }

        /* If the input file has errors, then terminate the program. */
        if (ip_error) {
            fprintf(stderr, "The program halted on %s with %zu "
                    "input error%s.\n",
                    filename, ip_error_num, (ip_error_num == 1)? "": "s");
            exit(1);
        }
    }

    /* Combine the removals array with the set */
    size_t  removal_count = cork_array_size(&removals);
    for (i = 0; i < removal_count; i++) {
        entry = &cork_array_at(&removals, i);
        if (entry->has_cidr == true) {
            set_unchanged =
                ipset_ip_remove_network(&set, &entry->address, entry->cidr);
            if (set_unchanged) {
                if (verbosity >= 0) {
                    char  ip_buf[CORK_IP_STRING_LENGTH];
                    cork_ip_to_raw_string(&entry->address, ip_buf);
                    fprintf(stderr,
                            "Alert: %s, line %zu: %s/%u is not in the set\n",
                            entry->filename, entry->line, ip_buf, entry->cidr);
                }
            } else {
                if (entry->address.version == 4) {
                    total_ip_v4_block--;
                } else {
                    total_ip_v6_block--;
                }
                total_ip_removed++;
            }
        } else {
            set_unchanged = ipset_ip_remove(&set, &entry->address);
            if (set_unchanged) {
                if (verbosity >= 0) {
                    char  ip_buf[CORK_IP_STRING_LENGTH];
                    cork_ip_to_raw_string(&entry->address, ip_buf);
                    fprintf(stderr,
                            "Alert: %s, line %zu: %s is not in the set\n",
                            entry->filename, entry->line, ip_buf);
                }
            } else {
                if (entry->address.version == 4) {
                    total_ip_v4--;
                } else {
                    total_ip_v6--;
                }
                total_ip_removed++;
            }
        }
    }
    cork_array_done(&removals);

    /* Print the total counter values and set size. */
    if (verbosity > 0) {
        fprintf(stderr, "\nSummary: %zu valid IP address records found.\n",
                total_ip_added + total_ip_removed);
        fprintf(stderr, "  Total records added: %zu\n", total_ip_added);
        fprintf(stderr, "  Total records removed: %zu\n", total_ip_removed);
        fprintf(stderr, "  IPv4: %zu addresses, %zu complete block%s\n",
                total_ip_v4, total_ip_v4_block,
                (total_ip_v4_block == 1)? "": "s");
        fprintf(stderr, "  IPv6: %zu addresses, %zu complete block%s\n",
                total_ip_v6, total_ip_v6_block,
                (total_ip_v6_block == 1)? "": "s");
        fprintf(stderr, "Set uses %zu bytes of memory.\n",
                ipset_memory_size(&set));
    }

    /* Serialize the IP set to the desired output file. */
    FILE  *ostream;
    bool  close_ostream;

    if (strcmp(output_filename, "-") == 0) {
        if (verbosity > 0) {
            fprintf(stderr, "Writing to stdout...\n");
        }
        ostream = stdout;
        output_filename = "stdout";
        close_ostream = false;
    } else {
        if (verbosity > 0) {
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

    if (ipset_save(ostream, &set) != 0) {
        fprintf(stderr, "Error saving IP set:\n  %s\n",
                cork_error_message());
        exit(1);
    }

    /* Close the output stream for exiting. */
    if (close_ostream) {
        fclose(ostream);
    }

    return 0;
}
