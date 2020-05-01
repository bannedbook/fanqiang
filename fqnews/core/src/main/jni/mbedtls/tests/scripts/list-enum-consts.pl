#!/usr/bin/env perl

use warnings;
use strict;

use utf8;
use open qw(:std utf8);

-d 'include/mbedtls' or die "$0: must be run from root\n";

@ARGV = grep { ! /compat-1\.3\.h/ } <include/mbedtls/*.h>;

my @consts;
my $state = 'out';
while (<>)
{
    if( $state eq 'out' and /^(typedef )?enum \{/ ) {
        $state = 'in';
    } elsif( $state eq 'out' and /^(typedef )?enum/ ) {
        $state = 'start';
    } elsif( $state eq 'start' and /{/ ) {
        $state = 'in';
    } elsif( $state eq 'in' and /}/ ) {
        $state = 'out';
    } elsif( $state eq 'in' ) {
        s/=.*//; s!/\*.*!!; s/,.*//; s/\s+//g; chomp;
        push @consts, $_ if $_;
    }
}

open my $fh, '>', 'enum-consts' or die;
print $fh "$_\n" for sort @consts;
close $fh or die;

printf "%8d enum-consts\n", scalar @consts;
