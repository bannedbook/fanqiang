#!/usr/bin/env perl
use strict;
use warnings;

if (!@ARGV || $ARGV[0] == '--help') {
    print <<EOF;
Usage: $0 mbedtls_test_foo <file.pem
       $0 TEST_FOO mbedtls_test_foo <file.pem
Print out a PEM file as C code defining a string constant.

Used to include some of the test data in /library/certs.c for
self-tests and sample programs.
EOF
    exit;
}

my $pp_name = @ARGV > 1 ? shift @ARGV : undef;
my $name = shift @ARGV;

my @lines = map {chomp; s/([\\"])/\\$1/g; "\"$_\\r\\n\""} <STDIN>;

if (defined $pp_name) {
    foreach ("#define $pp_name", @lines[0..@lines-2]) {
        printf "%-72s\\\n", $_;
    }
    print "$lines[@lines-1]\n";
    print "const char $name\[\] = $pp_name;\n";
} else {
    print "const char $name\[\] =";
    foreach (@lines) {
        print "\n$_";
    }
    print ";\n";
}
