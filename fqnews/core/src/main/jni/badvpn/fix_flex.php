<?php

$filename = $argv[1];
$contents = file_get_contents($filename);
if ($contents === FALSE) exit(1);
$search = array("<inttypes.h>", "#if defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L");
$replace = array("<stdint.h>", "#if 1");
$contents = str_replace($search, $replace, $contents);
$res = file_put_contents($filename, $contents);
if ($res === FALSE) exit(1);
