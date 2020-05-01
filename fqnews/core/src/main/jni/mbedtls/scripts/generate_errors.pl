#!/usr/bin/env perl

# Generate error.c
#
# Usage: ./generate_errors.pl or scripts/generate_errors.pl without arguments,
# or generate_errors.pl include_dir data_dir error_file

use strict;

my ($include_dir, $data_dir, $error_file);

if( @ARGV ) {
    die "Invalid number of arguments" if scalar @ARGV != 3;
    ($include_dir, $data_dir, $error_file) = @ARGV;

    -d $include_dir or die "No such directory: $include_dir\n";
    -d $data_dir or die "No such directory: $data_dir\n";
} else {
    $include_dir = 'include/mbedtls';
    $data_dir = 'scripts/data_files';
    $error_file = 'library/error.c';

    unless( -d $include_dir && -d $data_dir ) {
        chdir '..' or die;
        -d $include_dir && -d $data_dir
            or die "Without arguments, must be run from root or scripts\n"
    }
}

my $error_format_file = $data_dir.'/error.fmt';

my @low_level_modules = qw( AES ARC4 ARIA ASN1 BASE64 BIGNUM BLOWFISH
                            CAMELLIA CCM CHACHA20 CHACHAPOLY CMAC CTR_DRBG DES
                            ENTROPY GCM HKDF HMAC_DRBG MD2 MD4 MD5
                            NET OID PADLOCK PBKDF2 PLATFORM POLY1305 RIPEMD160
                            SHA1 SHA256 SHA512 THREADING XTEA );
my @high_level_modules = qw( CIPHER DHM ECP MD
                             PEM PK PKCS12 PKCS5
                             RSA SSL X509 );

my $line_separator = $/;
undef $/;

open(FORMAT_FILE, "$error_format_file") or die "Opening error format file '$error_format_file': $!";
my $error_format = <FORMAT_FILE>;
close(FORMAT_FILE);

$/ = $line_separator;

my @files = <$include_dir/*.h>;
my @matches;
foreach my $file (@files) {
    open(FILE, "$file");
    my @grep_res = grep(/^\s*#define\s+MBEDTLS_ERR_\w+\s+\-0x[0-9A-Fa-f]+/, <FILE>);
    push(@matches, @grep_res);
    close FILE;
}

my $ll_old_define = "";
my $hl_old_define = "";

my $ll_code_check = "";
my $hl_code_check = "";

my $headers = "";

my %error_codes_seen;


foreach my $line (@matches)
{
    next if ($line =~ /compat-1.2.h/);
    my ($error_name, $error_code) = $line =~ /(MBEDTLS_ERR_\w+)\s+\-(0x\w+)/;
    my ($description) = $line =~ /\/\*\*< (.*?)\.? \*\//;

    die "Duplicated error code: $error_code ($error_name)\n"
        if( $error_codes_seen{$error_code}++ );

    $description =~ s/\\/\\\\/g;
    if ($description eq "") {
        $description = "DESCRIPTION MISSING";
        warn "Missing description for $error_name\n";
    }

    my ($module_name) = $error_name =~ /^MBEDTLS_ERR_([^_]+)/;

    # Fix faulty ones
    $module_name = "BIGNUM" if ($module_name eq "MPI");
    $module_name = "CTR_DRBG" if ($module_name eq "CTR");
    $module_name = "HMAC_DRBG" if ($module_name eq "HMAC");

    my $define_name = $module_name;
    $define_name = "X509_USE,X509_CREATE" if ($define_name eq "X509");
    $define_name = "ASN1_PARSE" if ($define_name eq "ASN1");
    $define_name = "SSL_TLS" if ($define_name eq "SSL");
    $define_name = "PEM_PARSE,PEM_WRITE" if ($define_name eq "PEM");

    my $include_name = $module_name;
    $include_name =~ tr/A-Z/a-z/;
    $include_name = "" if ($include_name eq "asn1");

    # Fix faulty ones
    $include_name = "net_sockets" if ($module_name eq "NET");

    my $found_ll = grep $_ eq $module_name, @low_level_modules;
    my $found_hl = grep $_ eq $module_name, @high_level_modules;
    if (!$found_ll && !$found_hl)
    {
        printf("Error: Do not know how to handle: $module_name\n");
        exit 1;
    }

    my $code_check;
    my $old_define;
    my $white_space;
    my $first;

    if ($found_ll)
    {
        $code_check = \$ll_code_check;
        $old_define = \$ll_old_define;
        $white_space = '    ';
    }
    else
    {
        $code_check = \$hl_code_check;
        $old_define = \$hl_old_define;
        $white_space = '        ';
    }

    if ($define_name ne ${$old_define})
    {
        if (${$old_define} ne "")
        {
            ${$code_check} .= "#endif /* ";
            $first = 0;
            foreach my $dep (split(/,/, ${$old_define}))
            {
                ${$code_check} .= " || " if ($first++);
                ${$code_check} .= "MBEDTLS_${dep}_C";
            }
            ${$code_check} .= " */\n\n";
        }

        ${$code_check} .= "#if ";
        $headers .= "#if " if ($include_name ne "");
        $first = 0;
        foreach my $dep (split(/,/, ${define_name}))
        {
            ${$code_check} .= " || " if ($first);
            $headers       .= " || " if ($first++);

            ${$code_check} .= "defined(MBEDTLS_${dep}_C)";
            $headers       .= "defined(MBEDTLS_${dep}_C)" if
                                                    ($include_name ne "");
        }
        ${$code_check} .= "\n";
        $headers .= "\n#include \"mbedtls/${include_name}.h\"\n".
                    "#endif\n\n" if ($include_name ne "");
        ${$old_define} = $define_name;
    }

    if ($error_name eq "MBEDTLS_ERR_SSL_FATAL_ALERT_MESSAGE")
    {
        ${$code_check} .= "${white_space}if( use_ret == -($error_name) )\n".
                          "${white_space}\{\n".
                          "${white_space}    mbedtls_snprintf( buf, buflen, \"$module_name - $description\" );\n".
                          "${white_space}    return;\n".
                          "${white_space}}\n"
    }
    else
    {
        ${$code_check} .= "${white_space}if( use_ret == -($error_name) )\n".
                          "${white_space}    mbedtls_snprintf( buf, buflen, \"$module_name - $description\" );\n"
    }
};

if ($ll_old_define ne "")
{
    $ll_code_check .= "#endif /* ";
    my $first = 0;
    foreach my $dep (split(/,/, $ll_old_define))
    {
        $ll_code_check .= " || " if ($first++);
        $ll_code_check .= "MBEDTLS_${dep}_C";
    }
    $ll_code_check .= " */\n";
}
if ($hl_old_define ne "")
{
    $hl_code_check .= "#endif /* ";
    my $first = 0;
    foreach my $dep (split(/,/, $hl_old_define))
    {
        $hl_code_check .= " || " if ($first++);
        $hl_code_check .= "MBEDTLS_${dep}_C";
    }
    $hl_code_check .= " */\n";
}

$error_format =~ s/HEADER_INCLUDED\n/$headers/g;
$error_format =~ s/LOW_LEVEL_CODE_CHECKS\n/$ll_code_check/g;
$error_format =~ s/HIGH_LEVEL_CODE_CHECKS\n/$hl_code_check/g;

open(ERROR_FILE, ">$error_file") or die "Opening destination file '$error_file': $!";
print ERROR_FILE $error_format;
close(ERROR_FILE);
