#!/usr/bin/env perl
#

use strict;

my $file = shift;

open(TEST_DATA, "$file") or die "Opening test cases '$file': $!";

sub get_val($$)
{
    my $str = shift;
    my $name = shift;
    my $val = "";

    while(my $line = <TEST_DATA>)
    {
        next if($line !~ /^# $str/);
        last;
    }

    while(my $line = <TEST_DATA>)
    {
        last if($line eq "\r\n");
        $val .= $line;
    }

    $val =~ s/[ \r\n]//g;

    return $val;
}

my $state = 0;
my $val_n = "";
my $val_e = "";
my $val_p = "";
my $val_q = "";
my $mod = 0;
my $cnt = 1;
while (my $line = <TEST_DATA>)
{
    next if ($line !~ /^# Example/);

    ( $mod ) = ($line =~ /A (\d+)/);
    $val_n = get_val("RSA modulus n", "N");
    $val_e = get_val("RSA public exponent e", "E");
    $val_p = get_val("Prime p", "P");
    $val_q = get_val("Prime q", "Q");

    for(my $i = 1; $i <= 6; $i++)
    {
        my $val_m = get_val("Message to be", "M");
        my $val_salt = get_val("Salt", "Salt");
        my $val_sig = get_val("Signature", "Sig");

        print("RSASSA-PSS Signature Example ${cnt}_${i}\n");
        print("pkcs1_rsassa_pss_sign:$mod:16:\"$val_p\":16:\"$val_q\":16:\"$val_n\":16:\"$val_e\":SIG_RSA_SHA1:MBEDTLS_MD_SHA1");
        print(":\"$val_m\"");
        print(":\"$val_salt\"");
        print(":\"$val_sig\":0");
        print("\n\n");

        print("RSASSA-PSS Signature Example ${cnt}_${i} (verify)\n");
        print("pkcs1_rsassa_pss_verify:$mod:16:\"$val_n\":16:\"$val_e\":SIG_RSA_SHA1:MBEDTLS_MD_SHA1");
        print(":\"$val_m\"");
        print(":\"$val_salt\"");
        print(":\"$val_sig\":0");
        print("\n\n");
    }
    $cnt++;
}
close(TEST_DATA);
