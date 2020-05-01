#!/usr/bin/env perl

# depends-hashes.pl
#
# Copyright (c) 2017, ARM Limited, All Rights Reserved
#
# Purpose
#
# To test the code dependencies on individual hashes in each test suite. This
# is a verification step to ensure we don't ship test suites that do not work
# for some build options.
#
# The process is:
#       for each possible hash
#           build the library and test suites with the hash disabled
#           execute the test suites
#
# And any test suite with the wrong dependencies will fail.
#
# Usage: tests/scripts/depends-hashes.pl
#
# This script should be executed from the root of the project directory.
#
# For best effect, run either with cmake disabled, or cmake enabled in a mode
# that includes -Werror.

use warnings;
use strict;

-d 'library' && -d 'include' && -d 'tests' or die "Must be run from root\n";

my $config_h = 'include/mbedtls/config.h';

# as many SSL options depend on specific hashes,
# and SSL is not in the test suites anyways,
# disable it to avoid dependcies issues
my $ssl_sed_cmd = 's/^#define \(MBEDTLS_SSL.*\)/\1/p';
my @ssl = split( /\s+/, `sed -n -e '$ssl_sed_cmd' $config_h` );

# for md we want to catch MD5_C but not MD_C, hence the extra dot
my $mdx_sed_cmd = 's/^#define \(MBEDTLS_MD..*_C\)/\1/p';
my $sha_sed_cmd = 's/^#define \(MBEDTLS_SHA.*_C\)/\1/p';
my @hashes = split( /\s+/,
                    `sed -n -e '$mdx_sed_cmd' -e '$sha_sed_cmd' $config_h` );
system( "cp $config_h $config_h.bak" ) and die;
sub abort {
    system( "mv $config_h.bak $config_h" ) and warn "$config_h not restored\n";
    # use an exit code between 1 and 124 for git bisect (die returns 255)
    warn $_[0];
    exit 1;
}

for my $hash (@hashes) {
    system( "cp $config_h.bak $config_h" ) and die "$config_h not restored\n";
    system( "make clean" ) and die;

    print "\n******************************************\n";
    print "* Testing without hash: $hash\n";
    print "******************************************\n";

    system( "scripts/config.pl unset $hash" )
        and abort "Failed to disable $hash\n";

    for my $opt (@ssl) {
        system( "scripts/config.pl unset $opt" )
            and abort "Failed to disable $opt\n";
    }

    system( "CFLAGS='-Werror -Wall -Wextra' make lib" )
        and abort "Failed to build lib: $hash\n";
    system( "cd tests && make" ) and abort "Failed to build tests: $hash\n";
    system( "make test" ) and abort "Failed test suite: $hash\n";
}

system( "mv $config_h.bak $config_h" ) and die "$config_h not restored\n";
system( "make clean" ) and die;
exit 0;
