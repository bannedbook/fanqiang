#!/usr/bin/env perl

# key-exchanges.pl
#
# Copyright (c) 2015-2017, ARM Limited, All Rights Reserved
#
# Purpose
#
# To test the code dependencies on individual key exchanges in the SSL module.
# is a verification step to ensure we don't ship SSL code that do not work
# for some build options.
#
# The process is:
#       for each possible key exchange
#           build the library with all but that key exchange disabled
#
# Usage: tests/scripts/key-exchanges.pl
#
# This script should be executed from the root of the project directory.
#
# For best effect, run either with cmake disabled, or cmake enabled in a mode
# that includes -Werror.

use warnings;
use strict;

-d 'library' && -d 'include' && -d 'tests' or die "Must be run from root\n";

my $sed_cmd = 's/^#define \(MBEDTLS_KEY_EXCHANGE_.*_ENABLED\)/\1/p';
my $config_h = 'include/mbedtls/config.h';
my @kexes = split( /\s+/, `sed -n -e '$sed_cmd' $config_h` );

system( "cp $config_h $config_h.bak" ) and die;
sub abort {
    system( "mv $config_h.bak $config_h" ) and warn "$config_h not restored\n";
    # use an exit code between 1 and 124 for git bisect (die returns 255)
    warn $_[0];
    exit 1;
}

for my $kex (@kexes) {
    system( "cp $config_h.bak $config_h" ) and die "$config_h not restored\n";
    system( "make clean" ) and die;

    print "\n******************************************\n";
    print "* Testing with key exchange: $kex\n";
    print "******************************************\n";

    # full config with all key exchanges disabled except one
    system( "scripts/config.pl full" ) and abort "Failed config full\n";
    for my $k (@kexes) {
        next if $k eq $kex;
        system( "scripts/config.pl unset $k" )
            and abort "Failed to disable $k\n";
    }

    system( "make lib CFLAGS='-Os -Werror'" ) and abort "Failed to build lib: $kex\n";
}

system( "mv $config_h.bak $config_h" ) and die "$config_h not restored\n";
system( "make clean" ) and die;
exit 0;
