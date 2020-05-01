#!/usr/bin/env bash

root=$(pwd)
source="$root"/src

function format() {
  filelist=$(ls "$1")
  pushd "$1"
  for file in $filelist; do
    if test -d "$file"; then
      echo "format directory $file"
      format "$file"
    else
      if ([ "${file%%.*}" != "base64" ] &&
        [ "${file%%.*}" != "json" ] &&
        [ "${file%%.*}" != "uthash" ]) &&
        ([ "${file##*.}" = "h" ] || [ "${file##*.}" = "c" ]); then
        echo "format file $file"
        uncrustify -c "$root"/.uncrustify.cfg -l C --replace --no-backup "$file"
        rm ./*.uncrustify >/dev/null 2>&1
      fi
    fi
  done
  popd
}

format "$source"
