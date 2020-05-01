#!/usr/bin/env perl
#
# Based on NIST gcmDecryptxxx.rsp validation files
# Only first 3 of every set used for compile time saving

use strict;

my $file = shift;

open(TEST_DATA, "$file") or die "Opening test cases '$file': $!";

sub get_suite_val($)
{
    my $name = shift;
    my $val = "";

    while(my $line = <TEST_DATA>)
    {
        next if ($line !~ /^\[/);
        ($val) = ($line =~ /\[$name\s\=\s(\w+)\]/);
        last;
    }

    return $val;
}

sub get_val($)
{
    my $name = shift;
    my $val = "";
    my $line;

    while($line = <TEST_DATA>)
    {
        next if($line !~ /=/);
        last;
    }

    ($val) = ($line =~ /^$name = (\w+)/);

    return $val;
}

sub get_val_or_fail($)
{
    my $name = shift;
    my $val = "FAIL";
    my $line;

    while($line = <TEST_DATA>)
    {
        next if($line !~ /=/ && $line !~ /FAIL/);
        last;
    }

    ($val) = ($line =~ /^$name = (\w+)/) if ($line =~ /=/);

    return $val;
}

my $cnt = 1;;
while (my $line = <TEST_DATA>)
{
    my $key_len = get_suite_val("Keylen");
    next if ($key_len !~ /\d+/);
    my $iv_len = get_suite_val("IVlen");
    my $pt_len = get_suite_val("PTlen");
    my $add_len = get_suite_val("AADlen");
    my $tag_len = get_suite_val("Taglen");

    for ($cnt = 0; $cnt < 3; $cnt++)
    {
        my $Count = get_val("Count");
        my $key = get_val("Key");
        my $iv = get_val("IV");
        my $ct = get_val("CT");
        my $add = get_val("AAD");
        my $tag = get_val("Tag");
        my $pt = get_val_or_fail("PT");

        print("GCM NIST Validation (AES-$key_len,$iv_len,$pt_len,$add_len,$tag_len) #$Count\n");
        print("gcm_decrypt_and_verify");
        print(":\"$key\"");
        print(":\"$ct\"");
        print(":\"$iv\"");
        print(":\"$add\"");
        print(":$tag_len");
        print(":\"$tag\"");
        print(":\"$pt\"");
        print(":0");
        print("\n\n");
    }
}

print("GCM Selftest\n");
print("gcm_selftest:\n\n");

close(TEST_DATA);
