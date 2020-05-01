#!/bin/bash
set -eux

cd git/re2

case "${KOKORO_JOB_NAME}" in
  */windows-*)
    choco upgrade bazel -y -i
    # Pin to Visual Studio 2015, which is the minimum that we support.
    export BAZEL_VC='C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC'
    ;;
  *)
    # Use the script provided by Kokoro.
    use_bazel.sh latest
    ;;
esac

bazel clean
bazel build --compilation_mode=dbg -- //:all
bazel test  --compilation_mode=dbg --test_output=errors -- //:all \
  -//:dfa_test \
  -//:exhaustive1_test \
  -//:exhaustive2_test \
  -//:exhaustive3_test \
  -//:exhaustive_test \
  -//:random_test

bazel clean
bazel build --compilation_mode=opt -- //:all
bazel test  --compilation_mode=opt --test_output=errors -- //:all \
  -//:dfa_test \
  -//:exhaustive1_test \
  -//:exhaustive2_test \
  -//:exhaustive3_test \
  -//:exhaustive_test \
  -//:random_test

exit 0
