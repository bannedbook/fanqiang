#
# Copyright 2007 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# GGL_CHECK_STACK_PROTECTOR([ACTION-IF-OK], [ACTION-IF-NOT-OK])
# Check if c compiler supports -fstack-protector and -fstack-protector-all
# options.

AC_DEFUN([GGL_CHECK_STACK_PROTECTOR], [
ggl_check_stack_protector_save_CXXFLAGS="$CXXFLAGS"
ggl_check_stack_protector_save_CFLAGS="$CFLAGS"

AC_MSG_CHECKING([if -fstack-protector and -fstack-protector-all are supported.])

CXXFLAGS="$CXXFLAGS -fstack-protector"
CFLAGS="$CFLAGS -fstack-protector"
AC_COMPILE_IFELSE([AC_LANG_SOURCE([
int main() {
  return 0;
}
])],
[ggl_check_stack_protector_ok=yes],
[ggl_check_stack_protector_ok=no])

CXXFLAGS="$ggl_check_stack_protector_save_CXXFLAGS -fstack-protector-all"
CFLAGS="$ggl_check_stack_protector_save_CFLAGS -fstack-protector-all"
AC_COMPILE_IFELSE([AC_LANG_SOURCE([
int main() {
  return 0;
}
])],
[ggl_check_stack_protector_all_ok=yes],
[ggl_check_stack_protector_all_ok=no])

if test "x$ggl_check_stack_protector_ok" = "xyes" -a \
        "x$ggl_check_stack_protector_all_ok" = "xyes"; then
  AC_MSG_RESULT([yes])
  ifelse([$1], , :, [$1])
else
  AC_MSG_RESULT([no])
  ifelse([$2], , :, [$2])
fi

CXXFLAGS="$ggl_check_stack_protector_save_CXXFLAGS"
CFLAGS="$ggl_check_stack_protector_save_CFLAGS"

]) # GGL_CHECK_STACK_PROTECTOR
