#!/usr/bin/env bash

# TODO:
# - inline replace
# - clang-format-diff replacement
# - uncrustify for patches (not git refs)
# - maybe integrate into travis-ci?

function usage()
{
    cat <<EOL
$0 [ OPTS ] [ file-or-gitref [ ... ] ]

Example:
  # Chech HEAD git ref
  $ $0 -r
  $ $0 -r HEAD

  # Check patch
  $ git format-patch --stdout -1 | $0 -p
  $ git show -1 | $0 -p

  # Or via regular files
  $ git format-patch --stdout -2
  $ $0 *.patch

  # Over a file
  $ $0 -d event.c
  $ $0 -d < event.c

  # And print the whole file not only summary
  $ $0 -f event.c
  $ $0 -f < event.c

OPTS:
  -p   - treat as patch
  -f   - treat as regular file
  -f   - treat as regular file and print diff
  -r   - treat as git revision (default)
  -C   - check using clang-format (default)
  -U   - check with uncrustify
  -c   - config for clang-format/uncrustify
  -h   - print this message
EOL
}
function cfg()
{
    [ -z "${options[cfg]}" ] || {
        echo "${options[cfg]}"
        return
    }

    local dir="$(dirname "${BASH_SOURCE[0]}")"
    [ "${options[clang]}" -eq 0 ] || {
        echo "$dir/.clang-format"
        return
    }
    [ "${options[uncrustify]}" -eq 0 ] || {
        echo "$dir/.uncrustify"
        return
    }
}
function abort()
{
    local msg="$1"
    shift

    printf "$msg\n" "$@" >&2
    exit 1
}
function default_arg()
{
    if [ "${options[ref]}" -eq 1 ]; then
        echo "HEAD"
    else
        [ ! -t 0 ] || abort "<stdin> is a tty"
        echo "/dev/stdin"
    fi
}
function parse_options()
{
    options[patch]=0
    options[file]=0
    options[file_diff]=0
    options[ref]=1
    options[clang]=1
    options[uncrustify]=0
    options[cfg]=

    local OPTARG OPTIND c
    while getopts "pfrdCUc:h?" c; do
        case "$c" in
            p)
                options[patch]=1
                options[ref]=0
                options[file]=0
                options[file_diff]=0
                ;;
            f)
                options[file]=1
                options[ref]=0
                options[patch]=0
                options[file_diff]=0
                ;;
            r)
                options[ref]=1
                options[file]=0
                options[patch]=0
                options[file_diff]=0
                ;;
            d)
                options[file_diff]=1
                options[file]=0
                options[patch]=0
                options[ref]=0
                ;;
            C)
                options[clang]=1
                options[uncrustify]=0
                ;;
            U)
                options[uncrustify]=1
                options[clang]=0
                ;;
            c) options[cfg]="$OPTIND" ;;
            ?|h)
                usage
                exit 0
                ;;
            *)
                usage
                exit 1
                ;;
        esac
    done

    options[cfg]="$(cfg)"

    [ -f "${options[cfg]}" ] || \
        abort "Config '%s' does not exist" "${options[cfg]}"

    shift $((OPTIND - 1))
    args=( "$@" )

    if [ ${#args[@]} -eq 0 ]; then
        # exit on error globally, not only in subshell
        default_arg > /dev/null
        args=( "$(default_arg)" )
    fi

    if [ "${args[0]}" = "/dev/stdin" ]; then
        TMP_FILE="/tmp/libevent.checkpatch.$RANDOM"
        cat > "$TMP_FILE"
        trap "rm '$TMP_FILE'" EXIT

        args[0]="$TMP_FILE"
    fi
}

function diff() { command diff --color=always "$@"; }

function clang_style()
{
    local c="${options[cfg]}"
    echo "{ $(sed -e 's/#.*//' -e '/---/d' -e '/\.\.\./d' "$c" | tr $'\n' ,) }"
}
function clang_format() { clang-format --style="$(clang_style)" "$@"; }
function clang_format_diff() { clang-format-diff --style="$(clang_style)" "$@"; }
# for non-bare repo will work
function clang_format_git()
{ git format-patch --stdout "$@" -1 | clang_format_diff; }

function uncrustify() { command uncrustify -c "${options[cfg]}" "$@"; }
function uncrustify_frag() { uncrustify -l C --frag "$@"; }
function uncrustify_indent_off() { echo '/* *INDENT-OFF* */'; }
function uncrustify_indent_on() { echo '/* *INDENT-ON* */'; }
function git_hunk()
{
    local ref=$1 f=$2
    shift 2
    git cat-file -p $ref:$f
}
function uncrustify_git_indent_hunk()
{
    local start=$1 end=$2
    shift 2

    # Will be beatier with tee(1), but doh bash async substitution
    { uncrustify_indent_off; git_hunk "$@" | head -n$((start - 1)); }
    { uncrustify_indent_on;  git_hunk "$@" | head -n$((end - 1)) | tail -n+$start; }
    { uncrustify_indent_off; git_hunk "$@" | tail -n+$((end + 1)); }
}
function strip()
{
    local start=$1 end=$2
    shift 2

    # seek indent_{on,off}()
    let start+=2
    head -n$end | tail -n+$start
}
function patch_ranges()
{
    egrep -o '^@@ -[0-9]+(,[0-9]+|) \+[0-9]+(,[0-9]+|) @@' | \
        cut -d' ' -f3
}
function git_ranges()
{
    local ref=$1 f=$2
    shift 2

    git diff -W $ref^..$ref -- $f | patch_ranges
}
function diff_substitute()
{
    local f="$1"
    shift

    sed \
        -e "s#^--- /dev/fd.*\$#--- a/$f#" \
        -e "s#^+++ /dev/fd.*\$#+++ b/$f#"
}
function uncrustify_git()
{
    local ref=$1 r f start end length
    shift

    local files=( $(git diff --name-only $ref^..$ref | egrep "\.(c|h)$") )
    for f in "${files[@]}"; do
        local ranges=( $(git_ranges $ref "$f") )
        for r in "${ranges[@]}"; do
            [[ ! "$r" =~ ^\+([0-9]+)(,([0-9]+)|)$ ]] && continue
            start=${BASH_REMATCH[1]}
            [ -n "${BASH_REMATCH[3]}" ] && \
                length=${BASH_REMATCH[3]} || \
                length=1
            end=$((start + length))
            echo "Range: $start:$end ($length)" >&2

            diff -u \
                <(uncrustify_git_indent_hunk $start $end $ref "$f" | strip $start $end) \
                <(uncrustify_git_indent_hunk $start $end $ref "$f" | uncrustify_frag | strip $start $end) \
            | diff_substitute "$f"
        done
    done
}
function uncrustify_diff() { abort "Not implemented"; }
function uncrustify_file() { uncrustify -f "$@"; }

function checker()
{
    local c=$1 u=$2
    shift 2

    [ "${options[clang]}" -eq 0 ] || {
        $c "$@"
        return
    }
    [ "${options[uncrustify]}" -eq 0 ] || {
        $u "$@"
        return
    }
}
function check_patch() { checker clang_format_diff uncrustify_diff "$@"; }
function check_file() { checker clang_format uncrustify_file "$@"; }
function check_ref() { checker clang_format_git uncrustify_git "$@"; }

function check_arg()
{
    [ "${options[patch]}" -eq 0 ] || {
        check_patch "$@"
        return
    }
    [ "${options[file]}" -eq 0 ] || {
        check_file "$@"
        return
    }
    [ "${options[file_diff]}" -eq 0 ] || {
        diff -u "$@" <(check_file "$@") | diff_substitute "$@"
        return
    }
    [ "${options[ref]}" -eq 0 ] || {
        check_ref "$@"
        return
    }
}

function main()
{
    local a
    for a in "${args}"; do
        check_arg "$a"
    done
}

declare -A options
parse_options "$@"

main "$@" | less -FRSX
