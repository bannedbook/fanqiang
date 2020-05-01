#!/usr/bin/env perl
#
# This file is part of mbed TLS (https://tls.mbed.org)
#
# Copyright (c) 2015-2016, ARM Limited, All Rights Reserved
#
# Purpose
#
# This script migrates application source code from the mbed TLS 1.3 API to the
# mbed TLS 2.0 API.
#
# The script processes the given source code and renames identifiers - functions
# types, enums etc, as
#
# Usage:  rename.pl [-f datafile] [-s] [--] [filenames...]
#

use warnings;
use strict;

use utf8;
use Path::Class;
use open qw(:std utf8);

my $usage = "Usage: $0 [-f datafile] [-s] [--] [filenames...]\n";

(my $datafile = $0) =~ s/rename.pl$/data_files\/rename-1.3-2.0.txt/;
my $do_strings = 0;

while( @ARGV && $ARGV[0] =~ /^-/ ) {
    my $opt = shift;
    if( $opt eq '--' ) {
        last;
    } elsif( $opt eq '-f' ) {
        $datafile = shift;
    } elsif( $opt eq '-s' ) {
        $do_strings = 1; shift;
    } else {
        die $usage;
    }
}

my %subst;
open my $nfh, '<', $datafile or die "Could not read $datafile\n";
my $ident = qr/[_A-Za-z][_A-Za-z0-9]*/;
while( my $line = <$nfh> ) {
    chomp $line;
    my ( $old, $new ) = ( $line =~ /^($ident)\s+($ident)$/ );
    if( ! $old || ! $new ) {
        die "$0: $datafile:$.: bad input '$line'\n";
    }
    $subst{$old} = $new;
}
close $nfh or die;

my $string = qr/"(?:\\.|[^\\"])*"/;
my $space = qr/\s+/;
my $idnum = qr/[a-zA-Z0-9_]+/;
my $symbols = qr/[-!#\$%&'()*+,.\/:;<=>?@[\\\]^_`{|}~]+|"/;

my $lib_include_dir = dir($0)->parent->parent->subdir('include', 'mbedtls');
my $lib_source_dir = dir($0)->parent->parent->subdir('library');

# if we replace inside strings, we don't consider them a token
my $token = $do_strings ?         qr/$space|$idnum|$symbols/
                        : qr/$string|$space|$idnum|$symbols/;

my %warnings;

# If no files were passed, exit...
if ( not defined($ARGV[0]) ){ die $usage; }

while( my $filename = shift )
{
    print STDERR "$filename... ";

    if( dir($filename)->parent eq $lib_include_dir ||
         dir($filename)->parent eq $lib_source_dir )
    {
        die "Script cannot be executed on the mbed TLS library itself.";
    }

    if( -d $filename ) { print STDERR "skip (directory)\n"; next }

    open my $rfh, '<', $filename or die;
    my @lines = <$rfh>;
    close $rfh or die;

    my @out;
    for my $line (@lines) {
        if( $line =~ /#include/ ) {
            $line =~ s/polarssl/mbedtls/;
            $line =~ s/POLARSSL/MBEDTLS/;
            push( @out, $line );
            next;
        }

        my @words = ($line =~ /$token/g);
        my $checkline = join '', @words;
        if( $checkline eq $line ) {
            my @new = map { exists $subst{$_} ? $subst{$_} : $_ } @words;
            push( @out, join '', @new );
        } else {
            $warnings{$filename} = [] unless $warnings{$filename};
            push @{ $warnings{$filename} }, $line;
            push( @out, $line );
        }
    }

    open my $wfh, '>', $filename or die;
    print $wfh $_ for @out;
    close $wfh or die;
    print STDERR "done\n";
}

if( %warnings ) {
    print "\nWarning: lines skipped due to unexpected characters:\n";
    for my $filename (sort keys %warnings) {
        print "in $filename:\n";
        print for @{ $warnings{$filename} };
    }
}
