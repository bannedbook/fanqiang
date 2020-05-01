#! /usr/bin/env perl

# Generate query_config.c
#
# The file query_config.c contains a C function that can be used to check if
# a configuration macro is defined and to retrieve its expansion in string
# form (if any). This facilitates querying the compile time configuration of
# the library, for example, for testing.
#
# The query_config.c is generated from the current configuration at
# include/mbedtls/config.h. The idea is that the config.h contains ALL the
# compile time configurations available in Mbed TLS (commented or uncommented).
# This script extracts the configuration macros from the config.h and this
# information is used to automatically generate the body of the query_config()
# function by using the template in scripts/data_files/query_config.fmt.
#
# Usage: ./scripts/generate_query_config.pl without arguments

use strict;

my $config_file = "./include/mbedtls/config.h";

my $query_config_format_file = "./scripts/data_files/query_config.fmt";
my $query_config_file = "./programs/ssl/query_config.c";

# Excluded macros from the generated query_config.c. For example, macros that
# have commas or function-like macros cannot be transformed into strings easily
# using the preprocessor, so they should be excluded or the preprocessor will
# throw errors.
my @excluded = qw(
MBEDTLS_SSL_CIPHERSUITES
MBEDTLS_PARAM_FAILED
);
my $excluded_re = join '|', @excluded;

open(CONFIG_FILE, "$config_file") or die "Opening config file '$config_file': $!";

# This variable will contain the string to replace in the CHECK_CONFIG of the
# format file
my $config_check = "";

while (my $line = <CONFIG_FILE>) {
    if ($line =~ /^(\/\/)?\s*#\s*define\s+(MBEDTLS_\w+).*/) {
        my $name = $2;

        # Skip over the macro that prevents multiple inclusion
        next if "MBEDTLS_CONFIG_H" eq $name;

        # Skip over the macro if it is in the ecluded list
        next if $name =~ /$excluded_re/;

        $config_check .= "#if defined($name)\n";
        $config_check .= "    if( strcmp( \"$name\", config ) == 0 )\n";
        $config_check .= "    {\n";
        $config_check .= "        MACRO_EXPANSION_TO_STR( $name );\n";
        $config_check .= "        return( 0 );\n";
        $config_check .= "    }\n";
        $config_check .= "#endif /* $name */\n";
        $config_check .= "\n";
    }
}

# Read the full format file into a string
local $/;
open(FORMAT_FILE, "$query_config_format_file") or die "Opening query config format file '$query_config_format_file': $!";
my $query_config_format = <FORMAT_FILE>;
close(FORMAT_FILE);

# Replace the body of the query_config() function with the code we just wrote
$query_config_format =~ s/CHECK_CONFIG/$config_check/g;

# Rewrite the query_config.c file
open(QUERY_CONFIG_FILE, ">$query_config_file") or die "Opening destination file '$query_config_file': $!";
print QUERY_CONFIG_FILE $query_config_format;
close(QUERY_CONFIG_FILE);
