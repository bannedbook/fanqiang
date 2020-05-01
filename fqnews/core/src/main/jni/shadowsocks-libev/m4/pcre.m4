dnl -------------------------------------------------------- -*- autoconf -*-
dnl Licensed to the Apache Software Foundation (ASF) under one or more
dnl contributor license agreements.  See the NOTICE file distributed with
dnl this work for additional information regarding copyright ownership.
dnl The ASF licenses this file to You under the Apache License, Version 2.0
dnl (the "License"); you may not use this file except in compliance with
dnl the License.  You may obtain a copy of the License at
dnl
dnl     http://www.apache.org/licenses/LICENSE-2.0
dnl
dnl Unless required by applicable law or agreed to in writing, software
dnl distributed under the License is distributed on an "AS IS" BASIS,
dnl WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
dnl See the License for the specific language governing permissions and
dnl limitations under the License.

dnl
dnl TS_ADDTO(variable, value)
dnl
dnl  Add value to variable
dnl
AC_DEFUN([TS_ADDTO], [
  if test "x$$1" = "x"; then
    test "x$verbose" = "xyes" && echo "  setting $1 to \"$2\""
    $1="$2"
  else
    ats_addto_bugger="$2"
    for i in $ats_addto_bugger; do
      ats_addto_duplicate="0"
      for j in $$1; do
        if test "x$i" = "x$j"; then
          ats_addto_duplicate="1"
          break
        fi
      done
      if test $ats_addto_duplicate = "0"; then
        test "x$verbose" = "xyes" && echo "  adding \"$i\" to $1"
        $1="$$1 $i"
      fi
    done
  fi
])dnl

dnl
dnl TS_ADDTO_RPATH(path)
dnl
dnl   Adds path to variable with the '-rpath' directive.
dnl
AC_DEFUN([TS_ADDTO_RPATH], [
  AC_MSG_NOTICE([adding $1 to RPATH])
  TS_ADDTO(LIBTOOL_LINK_FLAGS, [-R$1])
])dnl

dnl
dnl pcre.m4: Trafficserver's pcre autoconf macros
dnl

dnl
dnl TS_CHECK_PCRE: look for pcre libraries and headers
dnl
AC_DEFUN([TS_CHECK_PCRE], [
enable_pcre=no
AC_ARG_WITH(pcre, [AC_HELP_STRING([--with-pcre=DIR],[use a specific pcre library])],
[
  if test "x$withval" != "xyes" && test "x$withval" != "x"; then
    pcre_base_dir="$withval"
    if test "$withval" != "no"; then
      enable_pcre=yes
      case "$withval" in
      *":"*)
        pcre_include="`echo $withval |sed -e 's/:.*$//'`"
        pcre_ldflags="`echo $withval |sed -e 's/^.*://'`"
        AC_MSG_CHECKING(checking for pcre includes in $pcre_include libs in $pcre_ldflags )
        ;;
      *)
        pcre_include="$withval/include"
        pcre_ldflags="$withval/lib"
        AC_MSG_CHECKING(checking for pcre includes in $withval)
        ;;
      esac
    fi
  fi
],
[
  AC_CHECK_PROG(PCRE_CONFIG, pcre-config, pcre-config)
  if test "x$PCRE_CONFIG" != "x"; then
    enable_pcre=yes
    pcre_base_dir="`$PCRE_CONFIG --prefix`"
    pcre_include="`$PCRE_CONFIG --cflags | sed -es/-I//`"
    pcre_ldflags="`$PCRE_CONFIG --libs | sed -es/-lpcre// -es/-L//`"
  fi
])

if test "x$pcre_base_dir" = "x"; then
  AC_MSG_CHECKING([for pcre location])
  AC_CACHE_VAL(ats_cv_pcre_dir,[
  for dir in /usr/local /usr ; do
    if test -d $dir && ( test -f $dir/include/pcre.h || test -f $dir/include/pcre/pcre.h ); then
      ats_cv_pcre_dir=$dir
      break
    fi
  done
  ])
  pcre_base_dir=$ats_cv_pcre_dir
  if test "x$pcre_base_dir" = "x"; then
    enable_pcre=no
    AC_MSG_RESULT([not found])
  else
    enable_pcre=yes
    pcre_include="$pcre_base_dir/include"
    pcre_ldflags="$pcre_base_dir/lib"
    AC_MSG_RESULT([$pcre_base_dir])
  fi
else
  AC_MSG_CHECKING(for pcre headers in $pcre_include)
  if test -d $pcre_include && test -d $pcre_ldflags && ( test -f $pcre_include/pcre.h || test -f $pcre_include/pcre/pcre.h ); then
    AC_MSG_RESULT([ok])
  else
    AC_MSG_RESULT([not found])
  fi
fi

pcreh=0
pcre_pcreh=0
if test "$enable_pcre" != "no"; then
  saved_ldflags=$LDFLAGS
  saved_cppflags=$CFLAGS
  pcre_have_headers=0
  pcre_have_libs=0
  if test "$pcre_base_dir" != "/usr"; then
    TS_ADDTO(CFLAGS, [-I${pcre_include}])
    TS_ADDTO(CFLAGS, [-DPCRE_STATIC])
    TS_ADDTO(LDFLAGS, [-L${pcre_ldflags}])
    TS_ADDTO_RPATH(${pcre_ldflags})
  fi
  AC_SEARCH_LIBS([pcre_exec], [pcre], [pcre_have_libs=1])
  if test "$pcre_have_libs" != "0"; then
    AC_CHECK_HEADERS(pcre.h, [pcre_have_headers=1])
    AC_CHECK_HEADERS(pcre/pcre.h, [pcre_have_headers=1])
  fi
  if test "$pcre_have_headers" != "0"; then
    AC_DEFINE(HAVE_LIBPCRE,1,[Compiling with pcre support])
    AC_SUBST(LIBPCRE, [-lpcre])
  else
    enable_pcre=no
    CFLAGS=$saved_cppflags
    LDFLAGS=$saved_ldflags
  fi
fi
AC_SUBST(pcreh)
AC_SUBST(pcre_pcreh)
])
