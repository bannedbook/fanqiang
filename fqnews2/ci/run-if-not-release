#!/bin/bash -eu

USAGE="Usage: $0 command arguments..."

if [ "$#" == "0" ]; then
  echo "$USAGE"
  exit 1
fi

cmd="$1"
shift

if git log --format=%B -n 1 | grep --quiet '^Releasing [[:digit:]]\+.[[:digit:]]\+.[[:digit:]]\+'; then
  echo >&2 "This is a release commit so will NOT execute: $cmd $*"
else
  "$cmd" "$@"
fi
