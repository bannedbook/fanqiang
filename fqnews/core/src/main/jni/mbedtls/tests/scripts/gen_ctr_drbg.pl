#!/usr/bin/env perl
#
# Based on NIST CTR_DRBG.rsp validation file
# Only uses AES-256-CTR cases that use a Derivation function
# and concats nonce and personalization for initialization.

use strict;

my $file = shift;

open(TEST_DATA, "$file") or die "Opening test cases '$file': $!";

sub get_suite_val($)
{
    my $name = shift;
    my $val = "";

    my $line = <TEST_DATA>;
    ($val) = ($line =~ /\[$name\s\=\s(\w+)\]/);

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

my $cnt = 1;;
while (my $line = <TEST_DATA>)
{
    next if ($line !~ /^\[AES-256 use df/);

    my $PredictionResistanceStr = get_suite_val("PredictionResistance");
    my $PredictionResistance = 0;
    $PredictionResistance = 1 if ($PredictionResistanceStr eq 'True');
    my $EntropyInputLen = get_suite_val("EntropyInputLen");
    my $NonceLen = get_suite_val("NonceLen");
    my $PersonalizationStringLen = get_suite_val("PersonalizationStringLen");
    my $AdditionalInputLen = get_suite_val("AdditionalInputLen");

    for ($cnt = 0; $cnt < 15; $cnt++)
    {
        my $Count = get_val("COUNT");
        my $EntropyInput = get_val("EntropyInput");
        my $Nonce = get_val("Nonce");
        my $PersonalizationString = get_val("PersonalizationString");
        my $AdditionalInput1 = get_val("AdditionalInput");
        my $EntropyInputPR1 = get_val("EntropyInputPR") if ($PredictionResistance == 1);
        my $EntropyInputReseed = get_val("EntropyInputReseed") if ($PredictionResistance == 0);
        my $AdditionalInputReseed = get_val("AdditionalInputReseed") if ($PredictionResistance == 0);
        my $AdditionalInput2 = get_val("AdditionalInput");
        my $EntropyInputPR2 = get_val("EntropyInputPR") if ($PredictionResistance == 1);
        my $ReturnedBits = get_val("ReturnedBits");

        if ($PredictionResistance == 1)
        {
            print("CTR_DRBG NIST Validation (AES-256 use df,$PredictionResistanceStr,$EntropyInputLen,$NonceLen,$PersonalizationStringLen,$AdditionalInputLen) #$Count\n");
            print("ctr_drbg_validate_pr");
            print(":\"$Nonce$PersonalizationString\"");
            print(":\"$EntropyInput$EntropyInputPR1$EntropyInputPR2\"");
            print(":\"$AdditionalInput1\"");
            print(":\"$AdditionalInput2\"");
            print(":\"$ReturnedBits\"");
            print("\n\n");
        }
        else
        {
            print("CTR_DRBG NIST Validation (AES-256 use df,$PredictionResistanceStr,$EntropyInputLen,$NonceLen,$PersonalizationStringLen,$AdditionalInputLen) #$Count\n");
            print("ctr_drbg_validate_nopr");
            print(":\"$Nonce$PersonalizationString\"");
            print(":\"$EntropyInput$EntropyInputReseed\"");
            print(":\"$AdditionalInput1\"");
            print(":\"$AdditionalInputReseed\"");
            print(":\"$AdditionalInput2\"");
            print(":\"$ReturnedBits\"");
            print("\n\n");
        }
    }
}
close(TEST_DATA);
