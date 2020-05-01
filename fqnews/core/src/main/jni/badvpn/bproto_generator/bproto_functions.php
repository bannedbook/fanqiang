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
        else if (preg_match('/^include/', $str, $matches)) {
            $out[] = array('include', null);
            $str = substr($str, strlen($matches[0]));
        }
        else if (preg_match('/^message/', $str, $matches)) {
            $out[] = array('message', null);
            $str = substr($str, strlen($matches[0]));
        }
        else if (preg_match('/^repeated/', $str, $matches)) {
            $out[] = array('repeated', null);
            $str = substr($str, strlen($matches[0]));
        }
        else if (preg_match('/^required/', $str, $matches)) {
            $out[] = array('required', null);
            $str = substr($str, strlen($matches[0]));
        }
        else if (preg_match('/^optional/', $str, $matches)) {
            $out[] = array('optional', null);
            $str = substr($str, strlen($matches[0]));
        }
        else if (preg_match('/^{/', $str, $matches)) {
            $out[] = array('spar', null);
            $str = substr($str, strlen($matches[0]));
        }
        else if (preg_match('/^}/', $str, $matches)) {
            $out[] = array('epar', null);
            $str = substr($str, strlen($matches[0]));
        }
        else if (preg_match('/^\(/', $str, $matches)) {
            $out[] = array('srpar', null);
            $str = substr($str, strlen($matches[0]));
        }
        else if (preg_match('/^\)/', $str, $matches)) {
            $out[] = array('erpar', null);
            $str = substr($str, strlen($matches[0]));
        }
        else if (preg_match('/^=/', $str, $matches)) {
            $out[] = array('equals', null);
            $str = substr($str, strlen($matches[0]));
        }
        else if (preg_match('/^;/', $str, $matches)) {
            $out[] = array('semicolon', null);
            $str = substr($str, strlen($matches[0]));
        }
        else if (preg_match('/^uint(8|16|32|64)/', $str, $matches)) {
            $out[] = array('uint', $matches[1]);
            $str = substr($str, strlen($matches[0]));
        }
        else if (preg_match('/^data/', $str, $matches)) {
            $out[] = array('data', null);
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
        else if (preg_match('/^"([^"]*)"/', $str, $matches)) {
            $out[] = array('string', $matches[1]);
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

function make_writer_decl ($msg, $entry)
{
    switch ($entry["type"]["type"]) {
        case "uint":
            return "void {$msg["name"]}Writer_Add{$entry["name"]} ({$msg["name"]}Writer *o, uint{$entry["type"]["size"]}_t v)";
        case "data":
            return "uint8_t * {$msg["name"]}Writer_Add{$entry["name"]} ({$msg["name"]}Writer *o, int len)";
        case "constdata":
            return "uint8_t * {$msg["name"]}Writer_Add{$entry["name"]} ({$msg["name"]}Writer *o)";
        default:
            assert(0);
    }
}

function make_parser_decl ($msg, $entry)
{
    switch ($entry["type"]["type"]) {
        case "uint":
            return "int {$msg["name"]}Parser_Get{$entry["name"]} ({$msg["name"]}Parser *o, uint{$entry["type"]["size"]}_t *v)";
        case "data":
            return "int {$msg["name"]}Parser_Get{$entry["name"]} ({$msg["name"]}Parser *o, uint8_t **data, int *data_len)";
        case "constdata":
            return "int {$msg["name"]}Parser_Get{$entry["name"]} ({$msg["name"]}Parser *o, uint8_t **data)";
        default:
            assert(0);
    }
}

function make_parser_reset_decl ($msg, $entry)
{
    return "void {$msg["name"]}Parser_Reset{$entry["name"]} ({$msg["name"]}Parser *o)";
}

function make_parser_forward_decl ($msg, $entry)
{
    return "void {$msg["name"]}Parser_Forward{$entry["name"]} ({$msg["name"]}Parser *o)";
}

function make_type_name ($msg, $entry)
{
    switch ($entry["type"]["type"]) {
        case "uint":
            return "BPROTO_TYPE_UINT{$entry["type"]["size"]}";
        case "data":
            return "BPROTO_TYPE_DATA";
        case "constdata":
            return "BPROTO_TYPE_CONSTDATA";
        default:
            assert(0);
    }
}

function make_finish_assert ($msg, $entry)
{
    switch ($entry["cardinality"]) {
        case "repeated":
            return "ASSERT(o->{$entry["name"]}_count >= 0)";
        case "required repeated":
            return "ASSERT(o->{$entry["name"]}_count >= 1)";
        case "optional":
            return "ASSERT(o->{$entry["name"]}_count >= 0 && o->{$entry["name"]}_count <= 1)";
        case "required":
            return "ASSERT(o->{$entry["name"]}_count == 1)";
        default:
            assert(0);
    }
}

function make_add_count_assert ($msg, $entry)
{
    if (in_array($entry["cardinality"], array("optional", "required"))) {
        return "ASSERT(o->{$entry["name"]}_count == 0)";
    }
    return "";
}

function make_add_length_assert ($msg, $entry)
{
    if ($entry["type"]["type"] == "data") {
        return "ASSERT(len >= 0 && len <= UINT32_MAX)";
    }
    return "";
}

function make_size_define ($msg, $entry)
{
    switch ($entry["type"]["type"]) {
        case "uint":
            return "#define {$msg["name"]}_SIZE{$entry["name"]} (sizeof(struct BProto_header_s) + sizeof(struct BProto_uint{$entry["type"]["size"]}_s))";
        case "data":
            return "#define {$msg["name"]}_SIZE{$entry["name"]}(_len) (sizeof(struct BProto_header_s) + sizeof(struct BProto_data_header_s) + (_len))";
        case "constdata":
            return "#define {$msg["name"]}_SIZE{$entry["name"]} (sizeof(struct BProto_header_s) + sizeof(struct BProto_data_header_s) + ({$entry["type"]["size"]}))";
        default:
            assert(0);
    }
}

function generate_header ($name, $directives, $messages) {
    ob_start();

    echo <<<EOD
/*
    DO NOT EDIT THIS FILE!
    This file was automatically generated by the bproto generator.
*/

#include <stdint.h>
#include <string.h>

#include <misc/debug.h>
#include <misc/byteorder.h>
#include <bproto/BProto.h>


EOD;

    foreach ($directives as $directive) {
        if ($directive["type"] == "include") {
            echo <<<EOD
#include "{$directive["file"]}"

EOD;
        }
    }

    echo <<<EOD


EOD;

    foreach ($messages as $msg) {

        foreach ($msg["entries"] as $entry) {
            $def = make_size_define($msg, $entry);
            echo <<<EOD
{$def}

EOD;
        }

        echo <<<EOD

typedef struct {
    uint8_t *out;
    int used;

EOD;

        foreach ($msg["entries"] as $entry) {
            echo <<<EOD
    int {$entry["name"]}_count;

EOD;
        }

        echo <<<EOD
} {$msg["name"]}Writer;

static void {$msg["name"]}Writer_Init ({$msg["name"]}Writer *o, uint8_t *out);
static int {$msg["name"]}Writer_Finish ({$msg["name"]}Writer *o);

EOD;

        foreach ($msg["entries"] as $entry) {
            $decl = make_writer_decl($msg, $entry);
            echo <<<EOD
static {$decl};

EOD;
        }

        echo <<<EOD

typedef struct {
    uint8_t *buf;
    int buf_len;

EOD;
        foreach ($msg["entries"] as $entry) {
            echo <<<EOD
    int {$entry["name"]}_start;
    int {$entry["name"]}_span;
    int {$entry["name"]}_pos;

EOD;
        }

        echo <<<EOD
} {$msg["name"]}Parser;

static int {$msg["name"]}Parser_Init ({$msg["name"]}Parser *o, uint8_t *buf, int buf_len);
static int {$msg["name"]}Parser_GotEverything ({$msg["name"]}Parser *o);

EOD;

        foreach ($msg["entries"] as $entry) {
            $decl = make_parser_decl($msg, $entry);
            $reset_decl = make_parser_reset_decl($msg, $entry);
            $forward_decl = make_parser_forward_decl($msg, $entry);
            echo <<<EOD
static {$decl};
static {$reset_decl};
static {$forward_decl};

EOD;
        }

        echo <<<EOD

void {$msg["name"]}Writer_Init ({$msg["name"]}Writer *o, uint8_t *out)
{
    o->out = out;
    o->used = 0;

EOD;

        foreach ($msg["entries"] as $entry) {
            echo <<<EOD
    o->{$entry["name"]}_count = 0;

EOD;
        }

        echo <<<EOD
}

int {$msg["name"]}Writer_Finish ({$msg["name"]}Writer *o)
{
    ASSERT(o->used >= 0)

EOD;

        foreach ($msg["entries"] as $entry) {
            $ass = make_finish_assert($msg, $entry);
            echo <<<EOD
    {$ass}

EOD;
        }

        echo <<<EOD

    return o->used;
}


EOD;

        foreach ($msg["entries"] as $entry) {
            $decl = make_writer_decl($msg, $entry);
            $type = make_type_name($msg, $entry);
            $add_count_assert = make_add_count_assert($msg, $entry);
            $add_length_assert = make_add_length_assert($msg, $entry);

            echo <<<EOD
{$decl}
{
    ASSERT(o->used >= 0)
    {$add_count_assert}
    {$add_length_assert}

    struct BProto_header_s header;
    header.id = htol16({$entry["id"]});
    header.type = htol16({$type});
    memcpy(o->out + o->used, &header, sizeof(header));
    o->used += sizeof(struct BProto_header_s);


EOD;
            switch ($entry["type"]["type"]) {
                case "uint":
                    echo <<<EOD
    struct BProto_uint{$entry["type"]["size"]}_s data;
    data.v = htol{$entry["type"]["size"]}(v);
    memcpy(o->out + o->used, &data, sizeof(data));
    o->used += sizeof(struct BProto_uint{$entry["type"]["size"]}_s);

EOD;
                    break;
                case "data":
                    echo <<<EOD
    struct BProto_data_header_s data;
    data.len = htol32(len);
    memcpy(o->out + o->used, &data, sizeof(data));
    o->used += sizeof(struct BProto_data_header_s);

    uint8_t *dest = (o->out + o->used);
    o->used += len;

EOD;
                    break;
                case "constdata":
                    echo <<<EOD
    struct BProto_data_header_s data;
    data.len = htol32({$entry["type"]["size"]});
    memcpy(o->out + o->used, &data, sizeof(data));
    o->used += sizeof(struct BProto_data_header_s);

    uint8_t *dest = (o->out + o->used);
    o->used += ({$entry["type"]["size"]});

EOD;
                    break;
                default:
                    assert(0);
            }

            echo <<<EOD

    o->{$entry["name"]}_count++;

EOD;
            if (in_array($entry["type"]["type"], array("data", "constdata"))) {
                echo <<<EOD

    return dest;

EOD;
            }

            echo <<<EOD
}


EOD;
        }

        echo <<<EOD
int {$msg["name"]}Parser_Init ({$msg["name"]}Parser *o, uint8_t *buf, int buf_len)
{
    ASSERT(buf_len >= 0)

    o->buf = buf;
    o->buf_len = buf_len;

EOD;

        foreach ($msg["entries"] as $entry) {
            echo <<<EOD
    o->{$entry["name"]}_start = o->buf_len;
    o->{$entry["name"]}_span = 0;
    o->{$entry["name"]}_pos = 0;

EOD;
        }

        echo <<<EOD


EOD;

        foreach ($msg["entries"] as $entry) {
            echo <<<EOD
    int {$entry["name"]}_count = 0;

EOD;
        }

        echo <<<EOD

    int pos = 0;
    int left = o->buf_len;

    while (left > 0) {
        int entry_pos = pos;

        if (!(left >= sizeof(struct BProto_header_s))) {
            return 0;
        }
        struct BProto_header_s header;
        memcpy(&header, o->buf + pos, sizeof(header));
        pos += sizeof(struct BProto_header_s);
        left -= sizeof(struct BProto_header_s);
        uint16_t type = ltoh16(header.type);
        uint16_t id = ltoh16(header.id);

        switch (type) {

EOD;

        foreach (array(8, 16, 32, 64) as $bits) {
            echo <<<EOD
            case BPROTO_TYPE_UINT{$bits}: {
                if (!(left >= sizeof(struct BProto_uint{$bits}_s))) {
                    return 0;
                }
                pos += sizeof(struct BProto_uint{$bits}_s);
                left -= sizeof(struct BProto_uint{$bits}_s);

                switch (id) {

EOD;

            foreach ($msg["entries"] as $entry) {
                if (!($entry["type"]["type"] == "uint" && $entry["type"]["size"] == $bits)) {
                    continue;
                }
                $type = make_type_name($msg, $entry);
                echo <<<EOD
                    case {$entry["id"]}:
                        if (o->{$entry["name"]}_start == o->buf_len) {
                            o->{$entry["name"]}_start = entry_pos;
                        }
                        o->{$entry["name"]}_span = pos - o->{$entry["name"]}_start;
                        {$entry["name"]}_count++;
                        break;

EOD;
            }

            echo <<<EOD
                    default:
                        return 0;
                }
            } break;

EOD;
        }

        echo <<<EOD
            case BPROTO_TYPE_DATA:
            case BPROTO_TYPE_CONSTDATA:
            {
                if (!(left >= sizeof(struct BProto_data_header_s))) {
                    return 0;
                }
                struct BProto_data_header_s val;
                memcpy(&val, o->buf + pos, sizeof(val));
                pos += sizeof(struct BProto_data_header_s);
                left -= sizeof(struct BProto_data_header_s);

                uint32_t payload_len = ltoh32(val.len);
                if (!(left >= payload_len)) {
                    return 0;
                }
                pos += payload_len;
                left -= payload_len;

                switch (id) {

EOD;

        foreach ($msg["entries"] as $entry) {
            if (!in_array($entry["type"]["type"], array("data", "constdata"))) {
                continue;
            }
            $type = make_type_name($msg, $entry);
            echo <<<EOD
                    case {$entry["id"]}:
                        if (!(type == {$type})) {
                            return 0;
                        }

EOD;
            if ($entry["type"]["type"] == "constdata") {
                echo <<<EOD
                        if (!(payload_len == ({$entry["type"]["size"]}))) {
                            return 0;
                        }

EOD;
            }
            echo <<<EOD
                        if (o->{$entry["name"]}_start == o->buf_len) {
                            o->{$entry["name"]}_start = entry_pos;
                        }
                        o->{$entry["name"]}_span = pos - o->{$entry["name"]}_start;
                        {$entry["name"]}_count++;
                        break;

EOD;
        }

        echo <<<EOD
                    default:
                        return 0;
                }
            } break;
            default:
                return 0;
        }
    }


EOD;

        foreach ($msg["entries"] as $entry) {
            $cond = "";
            switch ($entry["cardinality"]) {
                case "repeated":
                    break;
                case "required repeated":
                    $cond = "{$entry["name"]}_count >= 1";
                    break;
                case "optional":
                    $cond = "{$entry["name"]}_count <= 1";
                    break;
                case "required":
                    $cond = "{$entry["name"]}_count == 1";
                    break;
                default:
                    assert(0);
            }
            if ($cond) {
                echo <<<EOD
    if (!({$cond})) {
        return 0;
    }

EOD;
            }
        }

        echo <<<EOD

    return 1;
}

int {$msg["name"]}Parser_GotEverything ({$msg["name"]}Parser *o)
{
    return (

EOD;

        $first = 1;
        foreach ($msg["entries"] as $entry) {
            if ($first) {
                $first = 0;
            } else {
                echo <<<EOD
        &&

EOD;
            }
            echo <<<EOD
        o->{$entry["name"]}_pos == o->{$entry["name"]}_span

EOD;
        }
    

        echo <<<EOD
    );
}


EOD;

        foreach ($msg["entries"] as $entry) {
            $decl = make_parser_decl($msg, $entry);
            $reset_decl = make_parser_reset_decl($msg, $entry);
            $forward_decl = make_parser_forward_decl($msg, $entry);
            $type = make_type_name($msg, $entry);

            echo <<<EOD
{$decl}
{
    ASSERT(o->{$entry["name"]}_pos >= 0)
    ASSERT(o->{$entry["name"]}_pos <= o->{$entry["name"]}_span)

    int left = o->{$entry["name"]}_span - o->{$entry["name"]}_pos;

    while (left > 0) {
        ASSERT(left >= sizeof(struct BProto_header_s))
        struct BProto_header_s header;
        memcpy(&header, o->buf + o->{$entry["name"]}_start + o->{$entry["name"]}_pos, sizeof(header));
        o->{$entry["name"]}_pos += sizeof(struct BProto_header_s);
        left -= sizeof(struct BProto_header_s);
        uint16_t type = ltoh16(header.type);
        uint16_t id = ltoh16(header.id);

        switch (type) {

EOD;

            foreach (array(8, 16, 32, 64) as $bits) {
                echo <<<EOD
            case BPROTO_TYPE_UINT{$bits}: {
                ASSERT(left >= sizeof(struct BProto_uint{$bits}_s))

EOD;
                if ($entry["type"]["type"] == "uint" && $entry["type"]["size"] == $bits) {
                    echo <<<EOD
                struct BProto_uint{$bits}_s val;
                memcpy(&val, o->buf + o->{$entry["name"]}_start + o->{$entry["name"]}_pos, sizeof(val));

EOD;
                }
                echo <<<EOD
                o->{$entry["name"]}_pos += sizeof(struct BProto_uint{$bits}_s);
                left -= sizeof(struct BProto_uint{$bits}_s);

EOD;
                if ($entry["type"]["type"] == "uint" && $entry["type"]["size"] == $bits) {
                    echo <<<EOD

                if (id == {$entry["id"]}) {
                    *v = ltoh{$bits}(val.v);
                    return 1;
                }

EOD;
                }

                echo <<<EOD
            } break;

EOD;
            }

            echo <<<EOD
            case BPROTO_TYPE_DATA:
            case BPROTO_TYPE_CONSTDATA:
            {
                ASSERT(left >= sizeof(struct BProto_data_header_s))
                struct BProto_data_header_s val;
                memcpy(&val, o->buf + o->{$entry["name"]}_start + o->{$entry["name"]}_pos, sizeof(val));
                o->{$entry["name"]}_pos += sizeof(struct BProto_data_header_s);
                left -= sizeof(struct BProto_data_header_s);

                uint32_t payload_len = ltoh32(val.len);
                ASSERT(left >= payload_len)

EOD;
            if ($entry["type"]["type"] == "data" || $entry["type"]["type"] == "constdata") {
                    echo <<<EOD
                uint8_t *payload = o->buf + o->{$entry["name"]}_start + o->{$entry["name"]}_pos;

EOD;
            }
            echo <<<EOD
                o->{$entry["name"]}_pos += payload_len;
                left -= payload_len;

EOD;
            if ($entry["type"]["type"] == "data") {
                echo <<<EOD

                if (type == BPROTO_TYPE_DATA && id == {$entry["id"]}) {
                    *data = payload;
                    *data_len = payload_len;
                    return 1;
                }

EOD;
            }
            else if ($entry["type"]["type"] == "constdata") {
                echo <<<EOD

                if (type == BPROTO_TYPE_CONSTDATA && id == {$entry["id"]}) {
                    *data = payload;
                    return 1;
                }

EOD;
            }

            echo <<<EOD
            } break;
            default:
                ASSERT(0);
        }
    }

    return 0;
}

{$reset_decl}
{
    o->{$entry["name"]}_pos = 0;
}

{$forward_decl}
{
    o->{$entry["name"]}_pos = o->{$entry["name"]}_span;
}


EOD;
        }
    }

    return ob_get_clean();
}
