#!/usr/bin/env perl
#
# This file is part of mbed TLS (https://tls.mbed.org)
#
# Copyright (c) 2014-2016, ARM Limited, All Rights Reserved
#
# Purpose
#
# Comments and uncomments #define lines in the given header file and optionally
# sets their value or can get the value. This is to provide scripting control of
# what preprocessor symbols, and therefore what build time configuration flags
# are set in the 'config.h' file.
#
# Usage: config.pl [-f <file> | --file <file>] [-o | --force]
#                   [set <symbol> <value> | unset <symbol> | get <symbol> |
#                       full | realfull]
#
# Full usage description provided below.
#
# The following options are disabled instead of enabled with "full".
#
#   MBEDTLS_TEST_NULL_ENTROPY
#   MBEDTLS_DEPRECATED_REMOVED
#   MBEDTLS_HAVE_SSE2
#   MBEDTLS_PLATFORM_NO_STD_FUNCTIONS
#   MBEDTLS_ECP_DP_M221_ENABLED
#   MBEDTLS_ECP_DP_M383_ENABLED
#   MBEDTLS_ECP_DP_M511_ENABLED
#   MBEDTLS_NO_DEFAULT_ENTROPY_SOURCES
#   MBEDTLS_NO_PLATFORM_ENTROPY
#   MBEDTLS_REMOVE_ARC4_CIPHERSUITES
#   MBEDTLS_REMOVE_3DES_CIPHERSUITES
#   MBEDTLS_SSL_HW_RECORD_ACCEL
#   MBEDTLS_RSA_NO_CRT
#   MBEDTLS_X509_ALLOW_EXTENSIONS_NON_V3
#   MBEDTLS_X509_ALLOW_UNSUPPORTED_CRITICAL_EXTENSION
#       - this could be enabled if the respective tests were adapted
#   MBEDTLS_ZLIB_SUPPORT
#   MBEDTLS_PKCS11_C
#   and any symbol beginning _ALT
#

use warnings;
use strict;

my $config_file = "include/mbedtls/config.h";
my $usage = <<EOU;
$0 [-f <file> | --file <file>] [-o | --force]
                   [set <symbol> <value> | unset <symbol> | get <symbol> |
                        full | realfull | baremetal]

Commands
    set <symbol> [<value>]  - Uncomments or adds a #define for the <symbol> to
                              the configuration file, and optionally making it
                              of <value>.
                              If the symbol isn't present in the file an error
                              is returned.
    unset <symbol>          - Comments out the #define for the given symbol if
                              present in the configuration file.
    get <symbol>            - Finds the #define for the given symbol, returning
                              an exitcode of 0 if the symbol is found, and 1 if
                              not. The value of the symbol is output if one is
                              specified in the configuration file.
    full                    - Uncomments all #define's in the configuration file
                              excluding some reserved symbols, until the
                              'Module configuration options' section
    realfull                - Uncomments all #define's with no exclusions
    baremetal               - Sets full configuration suitable for baremetal build.

Options
    -f | --file <filename>  - The file or file path for the configuration file
                              to edit. When omitted, the following default is
                              used:
                                $config_file
    -o | --force            - If the symbol isn't present in the configuration
                              file when setting its value, a #define is
                              appended to the end of the file.

EOU

my @excluded = qw(
MBEDTLS_TEST_NULL_ENTROPY
MBEDTLS_DEPRECATED_REMOVED
MBEDTLS_HAVE_SSE2
MBEDTLS_PLATFORM_NO_STD_FUNCTIONS
MBEDTLS_ECP_DP_M221_ENABLED
MBEDTLS_ECP_DP_M383_ENABLED
MBEDTLS_ECP_DP_M511_ENABLED
MBEDTLS_NO_DEFAULT_ENTROPY_SOURCES
MBEDTLS_NO_PLATFORM_ENTROPY
MBEDTLS_RSA_NO_CRT
MBEDTLS_REMOVE_ARC4_CIPHERSUITES
MBEDTLS_REMOVE_3DES_CIPHERSUITES
MBEDTLS_SSL_HW_RECORD_ACCEL
MBEDTLS_X509_ALLOW_EXTENSIONS_NON_V3
MBEDTLS_X509_ALLOW_UNSUPPORTED_CRITICAL_EXTENSION
MBEDTLS_ZLIB_SUPPORT
MBEDTLS_PKCS11_C
MBEDTLS_NO_UDBL_DIVISION
MBEDTLS_NO_64BIT_MULTIPLICATION
_ALT\s*$
);

# Things that should be disabled in "baremetal"
my @excluded_baremetal = qw(
MBEDTLS_NET_C
MBEDTLS_TIMING_C
MBEDTLS_FS_IO
MBEDTLS_ENTROPY_NV_SEED
MBEDTLS_HAVE_TIME
MBEDTLS_HAVE_TIME_DATE
MBEDTLS_DEPRECATED_WARNING
MBEDTLS_HAVEGE_C
MBEDTLS_THREADING_C
MBEDTLS_THREADING_PTHREAD
MBEDTLS_MEMORY_BACKTRACE
MBEDTLS_MEMORY_BUFFER_ALLOC_C
MBEDTLS_PLATFORM_TIME_ALT
MBEDTLS_PLATFORM_FPRINTF_ALT
);

# Things that should be enabled in "full" even if they match @excluded
my @non_excluded = qw(
PLATFORM_[A-Z0-9]+_ALT
);

# Things that should be enabled in "baremetal"
my @non_excluded_baremetal = qw(
MBEDTLS_NO_PLATFORM_ENTROPY
);

# Process the command line arguments

my $force_option = 0;

my ($arg, $name, $value, $action);

while ($arg = shift) {

    # Check if the argument is an option
    if ($arg eq "-f" || $arg eq "--file") {
        $config_file = shift;

        -f $config_file or die "No such file: $config_file\n";

    }
    elsif ($arg eq "-o" || $arg eq "--force") {
        $force_option = 1;

    }
    else
    {
        # ...else assume it's a command
        $action = $arg;

        if ($action eq "full" || $action eq "realfull" || $action eq "baremetal" ) {
            # No additional parameters
            die $usage if @ARGV;

        }
        elsif ($action eq "unset" || $action eq "get") {
            die $usage unless @ARGV;
            $name = shift;

        }
        elsif ($action eq "set") {
            die $usage unless @ARGV;
            $name = shift;
            $value = shift if @ARGV;

        }
        else {
            die "Command '$action' not recognised.\n\n".$usage;
        }
    }
}

# If no command was specified, exit...
if ( not defined($action) ){ die $usage; }

# Check the config file is present
if (! -f $config_file)  {

    chdir '..' or die;

    # Confirm this is the project root directory and try again
    if ( !(-d 'scripts' && -d 'include' && -d 'library' && -f $config_file) ) {
        die "If no file specified, must be run from the project root or scripts directory.\n";
    }
}


# Now read the file and process the contents

open my $config_read, '<', $config_file or die "read $config_file: $!\n";
my @config_lines = <$config_read>;
close $config_read;

# Add required baremetal symbols to the list that is included.
if ( $action eq "baremetal" ) {
    @non_excluded = ( @non_excluded, @non_excluded_baremetal );
}

my ($exclude_re, $no_exclude_re, $exclude_baremetal_re);
if ($action eq "realfull") {
    $exclude_re = qr/^$/;
    $no_exclude_re = qr/./;
} else {
    $exclude_re = join '|', @excluded;
    $no_exclude_re = join '|', @non_excluded;
}
if ( $action eq "baremetal" ) {
    $exclude_baremetal_re = join '|', @excluded_baremetal;
}

my $config_write = undef;
if ($action ne "get") {
    open $config_write, '>', $config_file or die "write $config_file: $!\n";
}

my $done;
for my $line (@config_lines) {
    if ($action eq "full" || $action eq "realfull" || $action eq "baremetal" ) {
        if ($line =~ /name SECTION: Module configuration options/) {
            $done = 1;
        }

        if (!$done && $line =~ m!^//\s?#define! &&
                ( $line !~ /$exclude_re/ || $line =~ /$no_exclude_re/ ) &&
                ( $action ne "baremetal" || ( $line !~ /$exclude_baremetal_re/ ) ) ) {
            $line =~ s!^//\s?!!;
        }
        if (!$done && $line =~ m!^\s?#define! &&
                ! ( ( $line !~ /$exclude_re/ || $line =~ /$no_exclude_re/ ) &&
                    ( $action ne "baremetal" || ( $line !~ /$exclude_baremetal_re/ ) ) ) ) {
            $line =~ s!^!//!;
        }
    } elsif ($action eq "unset") {
        if (!$done && $line =~ /^\s*#define\s*$name\b/) {
            $line = '//' . $line;
            $done = 1;
        }
    } elsif (!$done && $action eq "set") {
        if ($line =~ m!^(?://)?\s*#define\s*$name\b!) {
            $line = "#define $name";
            $line .= " $value" if defined $value && $value ne "";
            $line .= "\n";
            $done = 1;
        }
    } elsif (!$done && $action eq "get") {
        if ($line =~ /^\s*#define\s*$name(?:\s+(.*?))\s*(?:$|\/\*|\/\/)/) {
            $value = $1;
            $done = 1;
        }
    }

    if (defined $config_write) {
        print $config_write $line or die "write $config_file: $!\n";
    }
}

# Did the set command work?
if ($action eq "set" && $force_option && !$done) {

    # If the force option was set, append the symbol to the end of the file
    my $line = "#define $name";
    $line .= " $value" if defined $value && $value ne "";
    $line .= "\n";
    $done = 1;

    print $config_write $line or die "write $config_file: $!\n";
}

if (defined $config_write) {
    close $config_write or die "close $config_file: $!\n";
}

if ($action eq "get") {
    if ($done) {
        if ($value ne '') {
            print "$value\n";
        }
        exit 0;
    } else {
        # If the symbol was not found, return an error
        exit 1;
    }
}

if ($action eq "full" && !$done) {
    die "Configuration section was not found in $config_file\n";

}

if ($action ne "full" && $action ne "unset" && !$done) {
    die "A #define for the symbol $name was not found in $config_file\n";
}

__END__
