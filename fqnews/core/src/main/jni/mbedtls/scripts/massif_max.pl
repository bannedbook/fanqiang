#!/usr/bin/env perl

# Parse a massif.out.xxx file and output peak total memory usage

use warnings;
use strict;

use utf8;
use open qw(:std utf8);

die unless @ARGV == 1;

my @snaps;
open my $fh, '<', $ARGV[0] or die;
{ local $/ = 'snapshot='; @snaps = <$fh>; }
close $fh or die;

my ($max, $max_heap, $max_he, $max_stack) = (0, 0, 0, 0);
for (@snaps)
{
    my ($heap, $heap_extra, $stack) = m{
        mem_heap_B=(\d+)\n
        mem_heap_extra_B=(\d+)\n
        mem_stacks_B=(\d+)
    }xm;
    next unless defined $heap;
    my $total = $heap + $heap_extra + $stack;
    if( $total > $max ) {
        ($max, $max_heap, $max_he, $max_stack) = ($total, $heap, $heap_extra, $stack);
    }
}

printf "$max (heap $max_heap+$max_he, stack $max_stack)\n";
