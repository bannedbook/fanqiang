<?php

function tokenize ($str, &$out) {
    $out = array();

    while (strlen($str) > 0) {
        if (preg_match('/^\\/\\/.*/', $str, $matches)) {
            $str = substr($str, strlen($matches[0]));
        }
        else if (preg_match('/^\\s+/', $str, $matches)) {
            $str = substr($str, strlen($matches[0]));
        }
        else if (preg_match('/^[0-9]+/', $str, $matches)) {
            $out[] = array('number', $matches[0]);
            $str = substr($str, strlen($matches[0]));
        }
        else if (preg_match('/^[a-zA-Z_][a-zA-Z0-9_]*/', $str, $matches)) {
            $out[] = array('name', $matches[0]);
            $str = substr($str, strlen($matches[0]));
        }
        else {
            return FALSE;
        }
    }

    return TRUE;
}

function fatal_error ($message)
{
    fwrite(STDERR, "Fatal error: $message\n");

    ob_get_clean();
    exit(1);
}
