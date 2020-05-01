<?php

require_once "blog_functions.php";

function assert_failure ($script, $line, $message)
{
    if ($message == "") {
        fatal_error("Assertion failure at {$script}:{$line}");
    } else {
        fatal_error("Assertion failure at {$script}:{$line}: {$message}");
    }
}

assert_options(ASSERT_CALLBACK, "assert_failure");

function print_help ($name)
{
    echo <<<EOD
Usage: {$name}
    --input-file <file>         Input channels file.
    --output-dir <dir>          Destination directory for generated files.

EOD;
}

$input_file = "";
$output_dir = "";

for ($i = 1; $i < $argc;) {
    $arg = $argv[$i++];
    switch ($arg) {
        case "--input-file":
            $input_file = $argv[$i++];
            break;
        case "--output-dir":
            $output_dir = $argv[$i++];
            break;
        case "--help":
            print_help($argv[0]);
            exit(0);
        default:
            fatal_error("Unknown option: {$arg}");
    }
}

if ($input_file == "") {
    fatal_error("--input-file missing");
}

if ($output_dir == "") {
    fatal_error("--output-dir missing");
}

if (($data = file_get_contents($input_file)) === FALSE) {
    fatal_error("Failed to read input file");
}

if (!tokenize($data, $tokens)) {
    fatal_error("Failed to tokenize");
}

$i = 0;
$channels_defines = "";
$channels_list = "";

reset($tokens);

while (1) {
    if (($ch_name = current($tokens)) === FALSE) {
        break;
    }
    next($tokens);
    if (($ch_priority = current($tokens)) === FALSE) {
        fatal_error("missing priority");
    }
    next($tokens);
    if ($ch_name[0] != "name") {
        fatal_error("name is not a name");
    }
    if ($ch_priority[0] != "number") {
        fatal_error("priority is not a number");
    }

    $channel_file = <<<EOD
#ifdef BLOG_CURRENT_CHANNEL
#undef BLOG_CURRENT_CHANNEL
#endif
#define BLOG_CURRENT_CHANNEL BLOG_CHANNEL_{$ch_name[1]}

EOD;

    $channels_defines .= <<<EOD
#define BLOG_CHANNEL_{$ch_name[1]} {$i}

EOD;

    $channels_list .= <<<EOD
{"{$ch_name[1]}", {$ch_priority[1]}},

EOD;

    if (file_put_contents("{$output_dir}/blog_channel_{$ch_name[1]}.h", $channel_file) === NULL) {
        fatal_error("{$input_file}: Failed to write channel file");
    }

    $i++;
}

$channels_defines .= <<<EOD
#define BLOG_NUM_CHANNELS {$i}

EOD;

if (file_put_contents("{$output_dir}/blog_channels_defines.h", $channels_defines) === NULL) {
    fatal_error("{$input_file}: Failed to write channels defines file");
}

if (file_put_contents("{$output_dir}/blog_channels_list.h", $channels_list) === NULL) {
    fatal_error("{$input_file}: Failed to write channels list file");
}

