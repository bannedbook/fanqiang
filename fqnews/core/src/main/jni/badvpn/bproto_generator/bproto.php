<?php
/**
 * @file bproto.php
 * @author Ambroz Bizjak <ambrop7@gmail.com>
 * 
 * @section LICENSE
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

require_once "lime/parse_engine.php";
require_once "ProtoParser.php";
require_once "bproto_functions.php";

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
    --name <string>             Output file prefix.
    --input-file <file>         Message file to generate source for.
    --output-dir <dir>          Destination directory for generated files.

EOD;
}

$name = "";
$input_file = "";
$output_dir = "";

for ($i = 1; $i < $argc;) {
    $arg = $argv[$i++];
    switch ($arg) {
        case "--name":
            $name = $argv[$i++];
            break;
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

if ($name == "") {
    fatal_error("--name missing");
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

$parser = new parse_engine(new ProtoParser());

try {
    foreach ($tokens as $token) {
        $parser->eat($token[0], $token[1]);
    }
    $parser->eat_eof();
} catch (parse_error $e) {
    fatal_error("$input_file: Parse error: ".$e->getMessage());
}

$data = generate_header($name, $parser->semantic["directives"], $parser->semantic["messages"]);
if (file_put_contents("{$output_dir}/{$name}.h", $data) === NULL) {
    fatal_error("{$input_file}: Failed to write .h file");
}
