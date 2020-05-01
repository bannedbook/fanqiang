#!/usr/bin/env perl

# depends-pkalgs.pl
#
# Copyright (c) 2017, ARM Limited, All Rights Reserved
#
# Purpose
#
# To test the code dependencies on individual PK algs (those that can be used
# from the PK layer, so currently signature and encryption but not key
# exchange) in each test suite. This is a verification step to ensure we don't
# ship test suites that do not work for some build options.
#
# The process is:
#       for each possible PK alg
#           build the library and test suites with that alg disabled
#           execute the test suites
#
# And any test suite with the wrong dependencies will fail.
#
# Usage: tests/scripts/depends-pkalgs.pl
#
# This script should be executed from the root of the project directory.
#
# For best effect, run either with cmake disabled, or cmake enabled in a mode
# that includes -Werror.

use warnings;
use strict;

-d 'library' && -d 'include' && -d 'tests' or die "Must be run from root\n";

my $config_h = 'include/mbedtls/config.h';

# Some algorithms can't be disabled on their own as others depend on them, so
# we list those reverse-dependencies here to keep check_config.h happy.
my %algs = (
    'MBEDTLS_ECDSA_C'   => ['MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED'],
    'MBEDTLS_ECP_C'     => ['MBEDTLS_ECDSA_C',
                            'MBEDTLS_ECDH_C',
                            'MBEDTLS_ECJPAKE_C',
                            'MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA_ENABLED',
                            'MBEDTLS_KEY_EXCHANGE_ECDH_RSA_ENABLED',
                            'MBEDTLS_KEY_EXCHANGE_ECDHE_PSK_ENABLED',
                            'MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED',
                            'MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED'],
    'MBEDTLS_X509_RSASSA_PSS_SUPPORT'   => [],
    'MBEDTLS_PKCS1_V21' => ['MBEDTLS_X509_RSASSA_PSS_SUPPORT'],
    'MBEDTLS_PKCS1_V15' => ['MBEDTLS_KEY_EXCHANGE_DHE_RSA_ENABLED',
                            'MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED',
                            'MBEDTLS_KEY_EXCHANGE_RSA_PSK_ENABLED',
                            'MBEDTLS_KEY_EXCHANGE_RSA_ENABLED'],
    'MBEDTLS_RSA_C'     => ['MBEDTLS_X509_RSASSA_PSS_SUPPORT',
                            'MBEDTLS_KEY_EXCHANGE_DHE_RSA_ENABLED',
                            'MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED',
                            'MBEDTLS_KEY_EXCHANGE_RSA_PSK_ENABLED',
                            'MBEDTLS_KEY_EXCHANGE_RSA_ENABLED'],
);

system( "cp $config_h $config_h.bak" ) and die;
sub abort {
    system( "mv $config_h.bak $config_h" ) and warn "$config_h not restored\n";
    # use an exit code between 1 and 124 for git bisect (die returns 255)
    warn $_[0];
    exit 1;
}

while( my ($alg, $extras) = each %algs ) {
    system( "cp $config_h.bak $config_h" ) and die "$config_h not restored\n";
    system( "make clean" ) and die;

    print "\n******************************************\n";
    print "* Testing without alg: $alg\n";
    print "******************************************\n";

    system( "scripts/config.pl unset $alg" )
        and abort "Failed to disable $alg\n";
    for my $opt (@$extras) {
        system( "scripts/config.pl unset $opt" )
            and abort "Failed to disable $opt\n";
    }

    system( "CFLAGS='-Werror -Wall -Wextra' make lib" )
        and abort "Failed to build lib: $alg\n";
    system( "cd tests && make" ) and abort "Failed to build tests: $alg\n";
    system( "make test" ) and abort "Failed test suite: $alg\n";
}

system( "mv $config_h.bak $config_h" ) and die "$config_h not restored\n";
system( "make clean" ) and die;
exit 0;
