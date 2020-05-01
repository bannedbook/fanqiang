#!/usr/bin/env perl
## ArchLinux install package via pacman: perl-net-cidr-lite
use strict;
use warnings;
use Net::CIDR::Lite;
my $cidr = Net::CIDR::Lite->new;
while (my $line=<>) {
    $cidr->add($line);
}
foreach my $line( @{$cidr->list} ) {
    print "<item>$line</item>\n";
}
