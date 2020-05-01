#!/usr/bin/env perl
#

use strict;

my ($include_dir, $data_dir, $feature_file);

if( @ARGV ) {
    die "Invalid number of arguments" if scalar @ARGV != 3;
    ($include_dir, $data_dir, $feature_file) = @ARGV;

    -d $include_dir or die "No such directory: $include_dir\n";
    -d $data_dir or die "No such directory: $data_dir\n";
} else {
    $include_dir = 'include/mbedtls';
    $data_dir = 'scripts/data_files';
    $feature_file = 'library/version_features.c';

    unless( -d $include_dir && -d $data_dir ) {
        chdir '..' or die;
        -d $include_dir && -d $data_dir
            or die "Without arguments, must be run from root or scripts\n"
    }
}

my $feature_format_file = $data_dir.'/version_features.fmt';

my @sections = ( "System support", "mbed TLS modules",
                 "mbed TLS feature support" );

my $line_separator = $/;
undef $/;

open(FORMAT_FILE, "$feature_format_file") or die "Opening feature format file '$feature_format_file': $!";
my $feature_format = <FORMAT_FILE>;
close(FORMAT_FILE);

$/ = $line_separator;

open(CONFIG_H, "$include_dir/config.h") || die("Failure when opening config.h: $!");

my $feature_defines = "";
my $in_section = 0;

while (my $line = <CONFIG_H>)
{
    next if ($in_section && $line !~ /#define/ && $line !~ /SECTION/);
    next if (!$in_section && $line !~ /SECTION/);

    if ($in_section) {
        if ($line =~ /SECTION/) {
            $in_section = 0;
            next;
        }

        my ($define) = $line =~ /#define (\w+)/;
        $feature_defines .= "#if defined(${define})\n";
        $feature_defines .= "    \"${define}\",\n";
        $feature_defines .= "#endif /* ${define} */\n";
    }

    if (!$in_section) {
        my ($section_name) = $line =~ /SECTION: ([\w ]+)/;
        my $found_section = grep $_ eq $section_name, @sections;

        $in_section = 1 if ($found_section);
    }
};

$feature_format =~ s/FEATURE_DEFINES\n/$feature_defines/g;

open(ERROR_FILE, ">$feature_file") or die "Opening destination file '$feature_file': $!";
print ERROR_FILE $feature_format;
close(ERROR_FILE);
