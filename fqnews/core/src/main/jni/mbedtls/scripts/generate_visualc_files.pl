#!/usr/bin/env perl

# Generate main file, individual apps and solution files for MS Visual Studio
# 2010
#
# Must be run from mbedTLS root or scripts directory.
# Takes no argument.

use warnings;
use strict;
use Digest::MD5 'md5_hex';

my $vsx_dir = "visualc/VS2010";
my $vsx_ext = "vcxproj";
my $vsx_app_tpl_file = "scripts/data_files/vs2010-app-template.$vsx_ext";
my $vsx_main_tpl_file = "scripts/data_files/vs2010-main-template.$vsx_ext";
my $vsx_main_file = "$vsx_dir/mbedTLS.$vsx_ext";
my $vsx_sln_tpl_file = "scripts/data_files/vs2010-sln-template.sln";
my $vsx_sln_file = "$vsx_dir/mbedTLS.sln";

my $programs_dir = 'programs';
my $header_dir = 'include/mbedtls';
my $source_dir = 'library';

# Need windows line endings!
my $vsx_hdr_tpl = <<EOT;
    <ClInclude Include="..\\..\\{NAME}" />\r
EOT
my $vsx_src_tpl = <<EOT;
    <ClCompile Include="..\\..\\{NAME}" />\r
EOT

my $vsx_sln_app_entry_tpl = <<EOT;
Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "{APPNAME}", "{APPNAME}.vcxproj", "{GUID}"\r
	ProjectSection(ProjectDependencies) = postProject\r
		{46CF2D25-6A36-4189-B59C-E4815388E554} = {46CF2D25-6A36-4189-B59C-E4815388E554}\r
	EndProjectSection\r
EndProject\r
EOT

my $vsx_sln_conf_entry_tpl = <<EOT;
		{GUID}.Debug|Win32.ActiveCfg = Debug|Win32\r
		{GUID}.Debug|Win32.Build.0 = Debug|Win32\r
		{GUID}.Debug|x64.ActiveCfg = Debug|x64\r
		{GUID}.Debug|x64.Build.0 = Debug|x64\r
		{GUID}.Release|Win32.ActiveCfg = Release|Win32\r
		{GUID}.Release|Win32.Build.0 = Release|Win32\r
		{GUID}.Release|x64.ActiveCfg = Release|x64\r
		{GUID}.Release|x64.Build.0 = Release|x64\r
EOT

exit( main() );

sub check_dirs {
    return -d $vsx_dir
        && -d $header_dir
        && -d $source_dir
        && -d $programs_dir;
}

sub slurp_file {
    my ($filename) = @_;

    local $/ = undef;
    open my $fh, '<', $filename or die "Could not read $filename\n";
    my $content = <$fh>;
    close $fh;

    return $content;
}

sub content_to_file {
    my ($content, $filename) = @_;

    open my $fh, '>', $filename or die "Could not write to $filename\n";
    print $fh $content;
    close $fh;
}

sub gen_app_guid {
    my ($path) = @_;

    my $guid = md5_hex( "mbedTLS:$path" );
    $guid =~ s/(.{8})(.{4})(.{4})(.{4})(.{12})/\U{$1-$2-$3-$4-$5}/;

    return $guid;
}

sub gen_app {
    my ($path, $template, $dir, $ext) = @_;

    my $guid = gen_app_guid( $path );
    $path =~ s!/!\\!g;
    (my $appname = $path) =~ s/.*\\//;

    my $srcs = "\n    <ClCompile Include=\"..\\..\\programs\\$path.c\" \/>\r";
    if( $appname eq "ssl_client2" or $appname eq "ssl_server2" or
        $appname eq "query_compile_time_config" ) {
        $srcs .= "\n    <ClCompile Include=\"..\\..\\programs\\ssl\\query_config.c\" \/>\r";
    }

    my $content = $template;
    $content =~ s/<SOURCES>/$srcs/g;
    $content =~ s/<APPNAME>/$appname/g;
    $content =~ s/<GUID>/$guid/g;

    content_to_file( $content, "$dir/$appname.$ext" );
}

sub get_app_list {
    my $app_list = `cd $programs_dir && make list`;
    die "make list failed: $!\n" if $?;

    return split /\s+/, $app_list;
}

sub gen_app_files {
    my @app_list = @_;

    my $vsx_tpl = slurp_file( $vsx_app_tpl_file );

    for my $app ( @app_list ) {
        gen_app( $app, $vsx_tpl, $vsx_dir, $vsx_ext );
    }
}

sub gen_entry_list {
    my ($tpl, @names) = @_;

    my $entries;
    for my $name (@names) {
        (my $entry = $tpl) =~ s/{NAME}/$name/g;
        $entries .= $entry;
    }

    return $entries;
}

sub gen_main_file {
    my ($headers, $sources, $hdr_tpl, $src_tpl, $main_tpl, $main_out) = @_;

    my $header_entries = gen_entry_list( $hdr_tpl, @$headers );
    my $source_entries = gen_entry_list( $src_tpl, @$sources );

    my $out = slurp_file( $main_tpl );
    $out =~ s/SOURCE_ENTRIES\r\n/$source_entries/m;
    $out =~ s/HEADER_ENTRIES\r\n/$header_entries/m;

    content_to_file( $out, $main_out );
}

sub gen_vsx_solution {
    my (@app_names) = @_;

    my ($app_entries, $conf_entries);
    for my $path (@app_names) {
        my $guid = gen_app_guid( $path );
        (my $appname = $path) =~ s!.*/!!;

        my $app_entry = $vsx_sln_app_entry_tpl;
        $app_entry =~ s/{APPNAME}/$appname/g;
        $app_entry =~ s/{GUID}/$guid/g;

        $app_entries .= $app_entry;

        my $conf_entry = $vsx_sln_conf_entry_tpl;
        $conf_entry =~ s/{GUID}/$guid/g;

        $conf_entries .= $conf_entry;
    }

    my $out = slurp_file( $vsx_sln_tpl_file );
    $out =~ s/APP_ENTRIES\r\n/$app_entries/m;
    $out =~ s/CONF_ENTRIES\r\n/$conf_entries/m;

    content_to_file( $out, $vsx_sln_file );
}

sub del_vsx_files {
    unlink glob "'$vsx_dir/*.$vsx_ext'";
    unlink $vsx_main_file;
    unlink $vsx_sln_file;
}

sub main {
    if( ! check_dirs() ) {
        chdir '..' or die;
        check_dirs or die "Must but run from mbedTLS root or scripts dir\n";
    }

    # Remove old files to ensure that, for example, project files from deleted
    # apps are not kept
    del_vsx_files();

    my @app_list = get_app_list();
    my @headers = <$header_dir/*.h>;
    my @sources = <$source_dir/*.c>;
    map { s!/!\\!g } @headers;
    map { s!/!\\!g } @sources;

    gen_app_files( @app_list );

    gen_main_file( \@headers, \@sources,
                   $vsx_hdr_tpl, $vsx_src_tpl,
                   $vsx_main_tpl_file, $vsx_main_file );

    gen_vsx_solution( @app_list );

    return 0;
}
