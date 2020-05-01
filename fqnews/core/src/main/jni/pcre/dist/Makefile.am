## Process this file with automake to produce Makefile.in.

ACLOCAL_AMFLAGS = -I m4

dist_doc_DATA = \
  doc/pcre.txt \
  doc/pcre-config.txt \
  doc/pcregrep.txt \
  doc/pcretest.txt \
  AUTHORS \
  COPYING \
  ChangeLog \
  LICENCE \
  NEWS \
  README

# Note that pcrecpp.html is not in this list; it is listed separately below.

dist_html_DATA = \
  doc/html/NON-AUTOTOOLS-BUILD.txt \
  doc/html/README.txt \
  doc/html/index.html \
  doc/html/pcre-config.html \
  doc/html/pcre.html \
  doc/html/pcre16.html \
  doc/html/pcre32.html \
  doc/html/pcre_assign_jit_stack.html \
  doc/html/pcre_compile.html \
  doc/html/pcre_compile2.html \
  doc/html/pcre_config.html \
  doc/html/pcre_copy_named_substring.html \
  doc/html/pcre_copy_substring.html \
  doc/html/pcre_dfa_exec.html \
  doc/html/pcre_exec.html \
  doc/html/pcre_free_study.html \
  doc/html/pcre_free_substring.html \
  doc/html/pcre_free_substring_list.html \
  doc/html/pcre_fullinfo.html \
  doc/html/pcre_get_named_substring.html \
  doc/html/pcre_get_stringnumber.html \
  doc/html/pcre_get_stringtable_entries.html \
  doc/html/pcre_get_substring.html \
  doc/html/pcre_get_substring_list.html \
  doc/html/pcre_jit_exec.html \
  doc/html/pcre_jit_stack_alloc.html \
  doc/html/pcre_jit_stack_free.html \
  doc/html/pcre_maketables.html \
  doc/html/pcre_pattern_to_host_byte_order.html \
  doc/html/pcre_refcount.html \
  doc/html/pcre_study.html \
  doc/html/pcre_utf16_to_host_byte_order.html \
  doc/html/pcre_utf32_to_host_byte_order.html \
  doc/html/pcre_version.html \
  doc/html/pcreapi.html \
  doc/html/pcrebuild.html \
  doc/html/pcrecallout.html \
  doc/html/pcrecompat.html \
  doc/html/pcredemo.html \
  doc/html/pcregrep.html \
  doc/html/pcrejit.html \
  doc/html/pcrelimits.html \
  doc/html/pcrematching.html \
  doc/html/pcrepartial.html \
  doc/html/pcrepattern.html \
  doc/html/pcreperform.html \
  doc/html/pcreposix.html \
  doc/html/pcreprecompile.html \
  doc/html/pcresample.html \
  doc/html/pcrestack.html \
  doc/html/pcresyntax.html \
  doc/html/pcretest.html \
  doc/html/pcreunicode.html

pcrecpp_html = doc/html/pcrecpp.html
dist_noinst_DATA = $(pcrecpp_html)

if WITH_PCRE_CPP
html_DATA = $(pcrecpp_html)
endif

# The Libtool libraries to install.  We'll add to this later.
lib_LTLIBRARIES =

# Unit tests you want to run when people type 'make check'.
# TESTS is for binary unit tests, check_SCRIPTS for script-based tests
TESTS =
check_SCRIPTS =
dist_noinst_SCRIPTS =

# Some of the binaries we make are to be installed, and others are
# (non-user-visible) helper programs needed to build libpcre, libpcre16
# or libpcre32.
bin_PROGRAMS =
noinst_PROGRAMS =

# Additional files to delete on 'make clean' and 'make maintainer-clean'.
CLEANFILES =
MAINTAINERCLEANFILES =

# Additional files to bundle with the distribution, over and above what
# the Autotools include by default.
EXTRA_DIST =

# These files contain additional m4 macros that are used by autoconf.
EXTRA_DIST += \
  m4/ax_pthread.m4 m4/pcre_visibility.m4

# These files contain maintenance information
EXTRA_DIST += \
  doc/perltest.txt \
  NON-UNIX-USE \
  NON-AUTOTOOLS-BUILD \
  HACKING

# These files are used in the preparation of a release
EXTRA_DIST += \
  PrepareRelease \
  CheckMan \
  CleanTxt \
  Detrail \
  132html \
  doc/index.html.src

# These files are to do with building for Virtual Pascal
EXTRA_DIST += \
  makevp.bat \
  makevp_c.txt \
  makevp_l.txt \
  pcregexp.pas

# These files are usable versions of pcre.h and config.h that are distributed
# for the benefit of people who are building PCRE manually, without the
# Autotools support.
EXTRA_DIST += \
  pcre.h.generic \
  config.h.generic

# The only difference between pcre.h.in and pcre.h is the setting of the PCRE
# version number. Therefore, we can create the generic version just by copying.
pcre.h.generic: pcre.h.in configure.ac
	rm -f $@
	cp -p pcre.h $@

# It is more complicated for config.h.generic. We need the version that results
# from a default configuration so as to get all the default values for PCRE
# configuration macros such as MATCH_LIMIT and NEWLINE. We can get this by
# doing a configure in a temporary directory. However, some trickery is needed,
# because the source directory may already be configured. If you just try
# running configure in a new directory, it complains. For this reason, we move
# config.status out of the way while doing the default configuration. The
# resulting config.h is munged by perl to put #ifdefs round any #defines for
# macros with values, and to #undef all boolean macros such as HAVE_xxx and
# SUPPORT_xxx. We also get rid of any gcc-specific visibility settings. Make
# sure that PCRE_EXP_DEFN is unset (in case it has visibility settings).
config.h.generic: configure.ac
	rm -rf $@ _generic
	mkdir _generic
	cs=$(srcdir)/config.status; test ! -f $$cs || mv -f $$cs $$cs.aside
	cd _generic && $(abs_top_srcdir)/configure || :
	cs=$(srcdir)/config.status; test ! -f $$cs.aside || mv -f $$cs.aside $$cs
	test -f _generic/config.h
	perl -n \
	  -e 'BEGIN{$$blank=0;}' \
	  -e 'if(/PCRE_EXP_DEFN/){print"/* #undef PCRE_EXP_DEFN */\n";$$blank=0;next;}' \
	  -e 'if(/to make a symbol visible/){next;}' \
	  -e 'if(/__attribute__ \(\(visibility/){next;}' \
	  -e 'if(/LT_OBJDIR/){print"/* This is ignored unless you are using libtool. */\n";}' \
	  -e 'if(/^#define\s((?:HAVE|SUPPORT|STDC)_\w+)/){print"/* #undef $$1 */\n";$$blank=0;next;}' \
	  -e 'if(/^#define\s(?!PACKAGE|VERSION)(\w+)/){print"#ifndef $$1\n$$_#endif\n";$$blank=0;next;}' \
	  -e 'if(/^\s*$$/){print unless $$blank; $$blank=1;} else{print;$$blank=0;}' \
	  _generic/config.h >$@
	rm -rf _generic

MAINTAINERCLEANFILES += pcre.h.generic config.h.generic

# These are the header files we'll install. We do not distribute pcre.h because
# it is generated from pcre.h.in.
nodist_include_HEADERS = \
  pcre.h
include_HEADERS = \
  pcreposix.h

# These additional headers will be be installed if C++ support is enabled. We
# do not distribute pcrecpparg.h or pcre_stringpiece.h, as these are generated
# from corresponding .h.in files (which we do distribute).
if WITH_PCRE_CPP
nodist_include_HEADERS += \
  pcrecpparg.h \
  pcre_stringpiece.h
include_HEADERS += \
  pcrecpp.h \
  pcre_scanner.h
endif # WITH_PCRE_CPP

bin_SCRIPTS = pcre-config

## ---------------------------------------------------------------
## The dftables program is used to rebuild character tables before compiling
## PCRE, if --enable-rebuild-chartables is specified. It is not a user-visible
## program. The default (when --enable-rebuild-chartables is not specified) is
## to copy a distributed set of tables that are defined for ASCII code. In this
## case, dftables is not needed.

if WITH_REBUILD_CHARTABLES

noinst_PROGRAMS += dftables
dftables_SOURCES = dftables.c

pcre_chartables.c: dftables$(EXEEXT)
	./dftables$(EXEEXT) $@
else

pcre_chartables.c: $(srcdir)/pcre_chartables.c.dist
	rm -f $@
	$(LN_S) $(srcdir)/pcre_chartables.c.dist $@

endif # WITH_REBUILD_CHARTABLES

BUILT_SOURCES = pcre_chartables.c

## The main pcre library

# Build the 8 bit library if it is enabled.
if WITH_PCRE8
lib_LTLIBRARIES += libpcre.la

libpcre_la_SOURCES = \
  pcre_byte_order.c \
  pcre_compile.c \
  pcre_config.c \
  pcre_dfa_exec.c \
  pcre_exec.c \
  pcre_fullinfo.c \
  pcre_get.c \
  pcre_globals.c \
  pcre_internal.h \
  pcre_jit_compile.c \
  pcre_maketables.c \
  pcre_newline.c \
  pcre_ord2utf8.c \
  pcre_refcount.c \
  pcre_string_utils.c \
  pcre_study.c \
  pcre_tables.c \
  pcre_ucd.c \
  pcre_valid_utf8.c \
  pcre_version.c \
  pcre_xclass.c \
  ucp.h

libpcre_la_CFLAGS = \
  $(VISIBILITY_CFLAGS) \
  $(AM_CFLAGS)

libpcre_la_LIBADD =

## This file is generated as part of the building process, so don't distribute.
nodist_libpcre_la_SOURCES = \
  pcre_chartables.c

endif # WITH_PCRE8

# Build the 16 bit library if it is enabled.
if WITH_PCRE16
lib_LTLIBRARIES += libpcre16.la
libpcre16_la_SOURCES = \
  pcre16_byte_order.c \
  pcre16_chartables.c \
  pcre16_compile.c \
  pcre16_config.c \
  pcre16_dfa_exec.c \
  pcre16_exec.c \
  pcre16_fullinfo.c \
  pcre16_get.c \
  pcre16_globals.c \
  pcre16_jit_compile.c \
  pcre16_maketables.c \
  pcre16_newline.c \
  pcre16_ord2utf16.c \
  pcre16_refcount.c \
  pcre16_string_utils.c \
  pcre16_study.c \
  pcre16_tables.c \
  pcre16_ucd.c \
  pcre16_utf16_utils.c \
  pcre16_valid_utf16.c \
  pcre16_version.c \
  pcre16_xclass.c

libpcre16_la_CFLAGS = \
  $(VISIBILITY_CFLAGS) \
  $(AM_CFLAGS)

libpcre16_la_LIBADD =

## This file is generated as part of the building process, so don't distribute.
nodist_libpcre16_la_SOURCES = \
  pcre_chartables.c

endif # WITH_PCRE16

# Build the 32 bit library if it is enabled.
if WITH_PCRE32
lib_LTLIBRARIES += libpcre32.la
libpcre32_la_SOURCES = \
  pcre32_byte_order.c \
  pcre32_chartables.c \
  pcre32_compile.c \
  pcre32_config.c \
  pcre32_dfa_exec.c \
  pcre32_exec.c \
  pcre32_fullinfo.c \
  pcre32_get.c \
  pcre32_globals.c \
  pcre32_jit_compile.c \
  pcre32_maketables.c \
  pcre32_newline.c \
  pcre32_ord2utf32.c \
  pcre32_refcount.c \
  pcre32_string_utils.c \
  pcre32_study.c \
  pcre32_tables.c \
  pcre32_ucd.c \
  pcre32_utf32_utils.c \
  pcre32_valid_utf32.c \
  pcre32_version.c \
  pcre32_xclass.c

libpcre32_la_CFLAGS = \
  $(VISIBILITY_CFLAGS) \
  $(AM_CFLAGS)

libpcre32_la_LIBADD =

## This file is generated as part of the building process, so don't distribute.
nodist_libpcre32_la_SOURCES = \
  pcre_chartables.c

endif # WITH_PCRE32

# The pcre_chartables.c.dist file is the default version of pcre_chartables.c,
# used unless --enable-rebuild-chartables is specified.
EXTRA_DIST += pcre_chartables.c.dist

# The JIT compiler lives in a separate directory, but its files are #included
# when pcre_jit_compile.c is processed, so they must be distributed.
EXTRA_DIST += \
  sljit/sljitConfig.h \
  sljit/sljitConfigInternal.h \
  sljit/sljitExecAllocator.c \
  sljit/sljitLir.c \
  sljit/sljitLir.h \
  sljit/sljitNativeARM_32.c \
  sljit/sljitNativeARM_64.c \
  sljit/sljitNativeARM_T2_32.c \
  sljit/sljitNativeMIPS_32.c \
  sljit/sljitNativeMIPS_64.c \
  sljit/sljitNativeMIPS_common.c \
  sljit/sljitNativePPC_32.c \
  sljit/sljitNativePPC_64.c \
  sljit/sljitNativePPC_common.c \
  sljit/sljitNativeSPARC_32.c \
  sljit/sljitNativeSPARC_common.c \
  sljit/sljitNativeTILEGX_64.c \
  sljit/sljitNativeTILEGX-encoder.c \
  sljit/sljitNativeX86_32.c \
  sljit/sljitNativeX86_64.c \
  sljit/sljitNativeX86_common.c \
  sljit/sljitUtils.c

if WITH_PCRE8
libpcre_la_LDFLAGS = $(EXTRA_LIBPCRE_LDFLAGS)
endif # WITH_PCRE8
if WITH_PCRE16
libpcre16_la_LDFLAGS = $(EXTRA_LIBPCRE16_LDFLAGS)
endif # WITH_PCRE16
if WITH_PCRE32
libpcre32_la_LDFLAGS = $(EXTRA_LIBPCRE32_LDFLAGS)
endif # WITH_PCRE32

if WITH_VALGRIND
if WITH_PCRE8
libpcre_la_CFLAGS += $(VALGRIND_CFLAGS)
endif # WITH_PCRE8
if WITH_PCRE16
libpcre16_la_CFLAGS += $(VALGRIND_CFLAGS)
endif # WITH_PCRE16
if WITH_PCRE32
libpcre32_la_CFLAGS += $(VALGRIND_CFLAGS)
endif # WITH_PCRE32
endif # WITH_VALGRIND

if WITH_GCOV
if WITH_PCRE8
libpcre_la_CFLAGS += $(GCOV_CFLAGS)
endif # WITH_PCRE8
if WITH_PCRE16
libpcre16_la_CFLAGS += $(GCOV_CFLAGS)
endif # WITH_PCRE16
if WITH_PCRE32
libpcre32_la_CFLAGS += $(GCOV_CFLAGS)
endif # WITH_PCRE32
endif # WITH_GCOV

CLEANFILES += pcre_chartables.c

## If JIT support is enabled, arrange for the JIT test program to run.
if WITH_JIT
TESTS += pcre_jit_test
noinst_PROGRAMS += pcre_jit_test
pcre_jit_test_SOURCES = pcre_jit_test.c
pcre_jit_test_CFLAGS = $(AM_CFLAGS)
pcre_jit_test_LDADD =
if WITH_PCRE8
pcre_jit_test_LDADD += libpcre.la
endif # WITH_PCRE8
if WITH_PCRE16
pcre_jit_test_LDADD += libpcre16.la
endif # WITH_PCRE16
if WITH_PCRE32
pcre_jit_test_LDADD += libpcre32.la
endif # WITH_PCRE32
if WITH_GCOV
pcre_jit_test_CFLAGS += $(GCOV_CFLAGS)
pcre_jit_test_LDADD += $(GCOV_LIBS)
endif # WITH_GCOV
endif # WITH_JIT

## A version of the main pcre library that has a posix re API.
if WITH_PCRE8

lib_LTLIBRARIES += libpcreposix.la
libpcreposix_la_SOURCES = \
  pcreposix.c
libpcreposix_la_CFLAGS = $(VISIBILITY_CFLAGS) $(AM_CFLAGS)
libpcreposix_la_LDFLAGS = $(EXTRA_LIBPCREPOSIX_LDFLAGS)
libpcreposix_la_LIBADD = libpcre.la

if WITH_GCOV
libpcreposix_la_CFLAGS += $(GCOV_CFLAGS)
endif # WITH_GCOV

endif # WITH_PCRE8

## There's a C++ library as well.
if WITH_PCRE_CPP

lib_LTLIBRARIES += libpcrecpp.la
libpcrecpp_la_SOURCES = \
  pcrecpp_internal.h \
  pcrecpp.cc \
  pcre_scanner.cc \
  pcre_stringpiece.cc
libpcrecpp_la_CXXFLAGS = $(VISIBILITY_CXXFLAGS) $(AM_CXXFLAGS)
libpcrecpp_la_LDFLAGS = $(EXTRA_LIBPCRECPP_LDFLAGS)
libpcrecpp_la_LIBADD = libpcre.la

TESTS += pcrecpp_unittest
noinst_PROGRAMS += pcrecpp_unittest
pcrecpp_unittest_SOURCES = pcrecpp_unittest.cc
pcrecpp_unittest_CXXFLAGS = $(AM_CXXFLAGS)
pcrecpp_unittest_LDADD = libpcrecpp.la

TESTS += pcre_scanner_unittest
noinst_PROGRAMS += pcre_scanner_unittest
pcre_scanner_unittest_SOURCES = pcre_scanner_unittest.cc
pcre_scanner_unittest_CXXFLAGS = $(AM_CXXFLAGS)
pcre_scanner_unittest_LDADD = libpcrecpp.la

TESTS += pcre_stringpiece_unittest
noinst_PROGRAMS += pcre_stringpiece_unittest
pcre_stringpiece_unittest_SOURCES = pcre_stringpiece_unittest.cc
pcre_stringpiece_unittest_CXXFLAGS = $(AM_CXXFLAGS)
pcre_stringpiece_unittest_LDADD = libpcrecpp.la

if WITH_GCOV
libpcrecpp_la_CXXFLAGS += $(GCOV_CXXFLAGS)
pcrecpp_unittest_LDADD += $(GCOV_LIBS)
pcre_scanner_unittest_LDADD += $(GCOV_LIBS)
pcre_stringpiece_unittest_LDADD += $(GCOV_LIBS)
endif # WITH_GCOV

endif # WITH_PCRE_CPP

## The main unit tests

# Each unit test is a binary plus a script that runs that binary in various
# ways. We install these test binaries in case folks find it helpful.

TESTS += RunTest
dist_noinst_SCRIPTS += RunTest
EXTRA_DIST += RunTest.bat
bin_PROGRAMS += pcretest
pcretest_SOURCES = pcretest.c
pcretest_CFLAGS = $(AM_CFLAGS)
pcretest_LDADD = $(LIBREADLINE)
if WITH_PCRE8
pcretest_SOURCES += pcre_printint.c
pcretest_LDADD += libpcre.la libpcreposix.la
endif # WITH_PCRE8
if WITH_PCRE16
pcretest_SOURCES += pcre16_printint.c
pcretest_LDADD += libpcre16.la
endif # WITH_PCRE16
if WITH_PCRE32
pcretest_SOURCES += pcre32_printint.c
pcretest_LDADD += libpcre32.la
endif # WITH_PCRE32
if WITH_VALGRIND
pcretest_CFLAGS += $(VALGRIND_CFLAGS)
endif # WITH_VALGRIND
if WITH_GCOV
pcretest_CFLAGS += $(GCOV_CFLAGS)
pcretest_LDADD += $(GCOV_LIBS)
endif # WITH_GCOV

if WITH_PCRE8
TESTS += RunGrepTest
dist_noinst_SCRIPTS += RunGrepTest
bin_PROGRAMS += pcregrep
pcregrep_SOURCES = pcregrep.c
pcregrep_CFLAGS = $(AM_CFLAGS)
pcregrep_LDADD = $(LIBZ) $(LIBBZ2)
pcregrep_LDADD += libpcre.la libpcreposix.la
if WITH_GCOV
pcregrep_CFLAGS += $(GCOV_CFLAGS)
pcregrep_LDADD += $(GCOV_LIBS)
endif # WITH_GCOV
endif # WITH_PCRE8

EXTRA_DIST += \
  testdata/grepbinary \
  testdata/grepfilelist \
  testdata/grepinput \
  testdata/grepinput3 \
  testdata/grepinput8 \
  testdata/grepinputv \
  testdata/grepinputx \
  testdata/greplist \
  testdata/grepoutput \
  testdata/grepoutput8 \
  testdata/grepoutputN \
  testdata/greppatN4 \
  testdata/saved16 \
  testdata/saved16BE-1 \
  testdata/saved16BE-2 \
  testdata/saved16LE-1 \
  testdata/saved16LE-2 \
  testdata/saved32 \
  testdata/saved32BE-1 \
  testdata/saved32BE-2 \
  testdata/saved32LE-1 \
  testdata/saved32LE-2 \
  testdata/saved8 \
  testdata/testinput1 \
  testdata/testinput2 \
  testdata/testinput3 \
  testdata/testinput4 \
  testdata/testinput5 \
  testdata/testinput6 \
  testdata/testinput7 \
  testdata/testinput8 \
  testdata/testinput9 \
  testdata/testinput10 \
  testdata/testinput11 \
  testdata/testinput12 \
  testdata/testinput13 \
  testdata/testinput14 \
  testdata/testinput15 \
  testdata/testinput16 \
  testdata/testinput17 \
  testdata/testinput18 \
  testdata/testinput19 \
  testdata/testinput20 \
  testdata/testinput21 \
  testdata/testinput22 \
  testdata/testinput23 \
  testdata/testinput24 \
  testdata/testinput25 \
  testdata/testinput26 \
  testdata/testinputEBC \
  testdata/testoutput1 \
  testdata/testoutput2 \
  testdata/testoutput3 \
  testdata/testoutput3A \
  testdata/testoutput3B \
  testdata/testoutput4 \
  testdata/testoutput5 \
  testdata/testoutput6 \
  testdata/testoutput7 \
  testdata/testoutput8 \
  testdata/testoutput9 \
  testdata/testoutput10 \
  testdata/testoutput11-8 \
  testdata/testoutput11-16 \
  testdata/testoutput11-32 \
  testdata/testoutput12 \
  testdata/testoutput13 \
  testdata/testoutput14 \
  testdata/testoutput15 \
  testdata/testoutput16 \
  testdata/testoutput17 \
  testdata/testoutput18-16 \
  testdata/testoutput18-32 \
  testdata/testoutput19 \
  testdata/testoutput20 \
  testdata/testoutput21-16 \
  testdata/testoutput21-32 \
  testdata/testoutput22-16 \
  testdata/testoutput22-32 \
  testdata/testoutput23 \
  testdata/testoutput24 \
  testdata/testoutput25 \
  testdata/testoutput26 \
  testdata/testoutputEBC \
  testdata/wintestinput3 \
  testdata/wintestoutput3 \
  perltest.pl

CLEANFILES += \
	testsavedregex \
	teststderr \
        testtemp* \
	testtry \
        testNinput \
        testtrygrep \
        teststderrgrep \
        testNinputgrep

# PCRE demonstration program. No longer built automatcally. The point is that
# the users should build it themselves. So just distribute the source.
# noinst_PROGRAMS += pcredemo
# pcredemo_SOURCES = pcredemo.c
# pcredemo_LDADD = libpcre.la

EXTRA_DIST += pcredemo.c


## Utility rules, documentation, etc.

# A compatibility line, the old build system worked with 'make test'
test: check ;


# A PCRE user submitted the following addition, saying that it "will allow
# anyone using the 'mingw32' compiler to simply type 'make pcre.dll' and get a
# nice DLL for Windows use". (It is used by the pcre.dll target.)
DLL_OBJS= pcre_byte_order.o pcre_compile.o pcre_config.o \
	pcre_dfa_exec.o pcre_exec.o pcre_fullinfo.o pcre_get.o \
	pcre_globals.o pcre_jit_compile.o pcre_maketables.o \
	pcre_newline.o pcre_ord2utf8.o pcre_refcount.o \
	pcre_study.o pcre_tables.o pcre_ucd.o \
	pcre_valid_utf8.o pcre_version.o pcre_chartables.o \
	pcre_xclass.o

# A PCRE user submitted the following addition, saying that it "will allow
# anyone using the 'mingw32' compiler to simply type 'make pcre.dll' and get a
# nice DLL for Windows use".
pcre.dll: $(DLL_OBJS)
	$(CC) -shared -o pcre.dll -Wl,"--strip-all" -Wl,"--export-all-symbols" $(DLL_OBJS)


# We have .pc files for pkg-config users.
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libpcre.pc libpcreposix.pc
if WITH_PCRE16
pkgconfig_DATA += libpcre16.pc
endif
if WITH_PCRE32
pkgconfig_DATA += libpcre32.pc
endif
if WITH_PCRE_CPP
pkgconfig_DATA += libpcrecpp.pc
endif

# Note that pcrecpp.3 is not in this list, but is included separately below.

dist_man_MANS = \
  doc/pcre-config.1 \
  doc/pcre.3 \
  doc/pcre16.3 \
  doc/pcre32.3 \
  doc/pcre_assign_jit_stack.3 \
  doc/pcre_compile.3 \
  doc/pcre_compile2.3 \
  doc/pcre_config.3 \
  doc/pcre_copy_named_substring.3 \
  doc/pcre_copy_substring.3 \
  doc/pcre_dfa_exec.3 \
  doc/pcre_exec.3 \
  doc/pcre_free_study.3 \
  doc/pcre_free_substring.3 \
  doc/pcre_free_substring_list.3 \
  doc/pcre_fullinfo.3 \
  doc/pcre_get_named_substring.3 \
  doc/pcre_get_stringnumber.3 \
  doc/pcre_get_stringtable_entries.3 \
  doc/pcre_get_substring.3 \
  doc/pcre_get_substring_list.3 \
  doc/pcre_jit_exec.3 \
  doc/pcre_jit_stack_alloc.3 \
  doc/pcre_jit_stack_free.3 \
  doc/pcre_maketables.3 \
  doc/pcre_pattern_to_host_byte_order.3 \
  doc/pcre_refcount.3 \
  doc/pcre_study.3 \
  doc/pcre_utf16_to_host_byte_order.3 \
  doc/pcre_utf32_to_host_byte_order.3 \
  doc/pcre_version.3 \
  doc/pcreapi.3 \
  doc/pcrebuild.3 \
  doc/pcrecallout.3 \
  doc/pcrecompat.3 \
  doc/pcredemo.3 \
  doc/pcregrep.1 \
  doc/pcrejit.3 \
  doc/pcrelimits.3 \
  doc/pcrematching.3 \
  doc/pcrepartial.3 \
  doc/pcrepattern.3 \
  doc/pcreperform.3 \
  doc/pcreposix.3 \
  doc/pcreprecompile.3 \
  doc/pcresample.3 \
  doc/pcrestack.3 \
  doc/pcresyntax.3 \
  doc/pcretest.1 \
  doc/pcreunicode.3

# Arrange for the per-function man pages to have 16- and 32-bit names as well.
install-data-hook:
	ln -sf pcre_assign_jit_stack.3		 $(DESTDIR)$(man3dir)/pcre16_assign_jit_stack.3
	ln -sf pcre_compile.3			 $(DESTDIR)$(man3dir)/pcre16_compile.3
	ln -sf pcre_compile2.3			 $(DESTDIR)$(man3dir)/pcre16_compile2.3
	ln -sf pcre_config.3			 $(DESTDIR)$(man3dir)/pcre16_config.3
	ln -sf pcre_copy_named_substring.3	 $(DESTDIR)$(man3dir)/pcre16_copy_named_substring.3
	ln -sf pcre_copy_substring.3		 $(DESTDIR)$(man3dir)/pcre16_copy_substring.3
	ln -sf pcre_dfa_exec.3			 $(DESTDIR)$(man3dir)/pcre16_dfa_exec.3
	ln -sf pcre_exec.3			 $(DESTDIR)$(man3dir)/pcre16_exec.3
	ln -sf pcre_free_study.3		 $(DESTDIR)$(man3dir)/pcre16_free_study.3
	ln -sf pcre_free_substring.3		 $(DESTDIR)$(man3dir)/pcre16_free_substring.3
	ln -sf pcre_free_substring_list.3	 $(DESTDIR)$(man3dir)/pcre16_free_substring_list.3
	ln -sf pcre_fullinfo.3			 $(DESTDIR)$(man3dir)/pcre16_fullinfo.3
	ln -sf pcre_get_named_substring.3	 $(DESTDIR)$(man3dir)/pcre16_get_named_substring.3
	ln -sf pcre_get_stringnumber.3		 $(DESTDIR)$(man3dir)/pcre16_get_stringnumber.3
	ln -sf pcre_get_stringtable_entries.3	 $(DESTDIR)$(man3dir)/pcre16_get_stringtable_entries.3
	ln -sf pcre_get_substring.3		 $(DESTDIR)$(man3dir)/pcre16_get_substring.3
	ln -sf pcre_get_substring_list.3	 $(DESTDIR)$(man3dir)/pcre16_get_substring_list.3
	ln -sf pcre_jit_exec.3			 $(DESTDIR)$(man3dir)/pcre16_jit_exec.3
	ln -sf pcre_jit_stack_alloc.3		 $(DESTDIR)$(man3dir)/pcre16_jit_stack_alloc.3
	ln -sf pcre_jit_stack_free.3		 $(DESTDIR)$(man3dir)/pcre16_jit_stack_free.3
	ln -sf pcre_maketables.3		 $(DESTDIR)$(man3dir)/pcre16_maketables.3
	ln -sf pcre_pattern_to_host_byte_order.3 $(DESTDIR)$(man3dir)/pcre16_pattern_to_host_byte_order.3
	ln -sf pcre_refcount.3			 $(DESTDIR)$(man3dir)/pcre16_refcount.3
	ln -sf pcre_study.3			 $(DESTDIR)$(man3dir)/pcre16_study.3
	ln -sf pcre_utf16_to_host_byte_order.3	 $(DESTDIR)$(man3dir)/pcre16_utf16_to_host_byte_order.3
	ln -sf pcre_version.3			 $(DESTDIR)$(man3dir)/pcre16_version.3
	ln -sf pcre_assign_jit_stack.3		 $(DESTDIR)$(man3dir)/pcre32_assign_jit_stack.3
	ln -sf pcre_compile.3			 $(DESTDIR)$(man3dir)/pcre32_compile.3
	ln -sf pcre_compile2.3			 $(DESTDIR)$(man3dir)/pcre32_compile2.3
	ln -sf pcre_config.3			 $(DESTDIR)$(man3dir)/pcre32_config.3
	ln -sf pcre_copy_named_substring.3	 $(DESTDIR)$(man3dir)/pcre32_copy_named_substring.3
	ln -sf pcre_copy_substring.3		 $(DESTDIR)$(man3dir)/pcre32_copy_substring.3
	ln -sf pcre_dfa_exec.3			 $(DESTDIR)$(man3dir)/pcre32_dfa_exec.3
	ln -sf pcre_exec.3			 $(DESTDIR)$(man3dir)/pcre32_exec.3
	ln -sf pcre_free_study.3		 $(DESTDIR)$(man3dir)/pcre32_free_study.3
	ln -sf pcre_free_substring.3		 $(DESTDIR)$(man3dir)/pcre32_free_substring.3
	ln -sf pcre_free_substring_list.3	 $(DESTDIR)$(man3dir)/pcre32_free_substring_list.3
	ln -sf pcre_fullinfo.3			 $(DESTDIR)$(man3dir)/pcre32_fullinfo.3
	ln -sf pcre_get_named_substring.3	 $(DESTDIR)$(man3dir)/pcre32_get_named_substring.3
	ln -sf pcre_get_stringnumber.3		 $(DESTDIR)$(man3dir)/pcre32_get_stringnumber.3
	ln -sf pcre_get_stringtable_entries.3	 $(DESTDIR)$(man3dir)/pcre32_get_stringtable_entries.3
	ln -sf pcre_get_substring.3		 $(DESTDIR)$(man3dir)/pcre32_get_substring.3
	ln -sf pcre_get_substring_list.3	 $(DESTDIR)$(man3dir)/pcre32_get_substring_list.3
	ln -sf pcre_jit_exec.3			 $(DESTDIR)$(man3dir)/pcre32_jit_exec.3
	ln -sf pcre_jit_stack_alloc.3		 $(DESTDIR)$(man3dir)/pcre32_jit_stack_alloc.3
	ln -sf pcre_jit_stack_free.3		 $(DESTDIR)$(man3dir)/pcre32_jit_stack_free.3
	ln -sf pcre_maketables.3		 $(DESTDIR)$(man3dir)/pcre32_maketables.3
	ln -sf pcre_pattern_to_host_byte_order.3 $(DESTDIR)$(man3dir)/pcre32_pattern_to_host_byte_order.3
	ln -sf pcre_refcount.3			 $(DESTDIR)$(man3dir)/pcre32_refcount.3
	ln -sf pcre_study.3			 $(DESTDIR)$(man3dir)/pcre32_study.3
	ln -sf pcre_utf32_to_host_byte_order.3	 $(DESTDIR)$(man3dir)/pcre32_utf32_to_host_byte_order.3
	ln -sf pcre_version.3			 $(DESTDIR)$(man3dir)/pcre32_version.3

pcrecpp_man = doc/pcrecpp.3
EXTRA_DIST += $(pcrecpp_man)

if WITH_PCRE_CPP
man_MANS = $(pcrecpp_man)
endif

# gcov/lcov code coverage reporting

if WITH_GCOV

# Coverage reporting targets:
#
# coverage: Create a coverage report from 'make check'
# coverage-baseline: Capture baseline coverage information
# coverage-reset: This zeros the coverage counters only
# coverage-report: This creates the coverage report only
# coverage-clean-report: This removes the generated coverage report
#   without cleaning the coverage data itself
# coverage-clean-data: This removes the captured coverage data without
#   removing the coverage files created at compile time (*.gcno)
# coverage-clean: This cleans all coverage data including the generated
#   coverage report.

COVERAGE_TEST_NAME = $(PACKAGE)
COVERAGE_NAME = $(PACKAGE)-$(VERSION)
COVERAGE_OUTPUT_FILE = $(COVERAGE_NAME)-coverage.info
COVERAGE_OUTPUT_DIR = $(COVERAGE_NAME)-coverage
COVERAGE_LCOV_EXTRA_FLAGS =
COVERAGE_GENHTML_EXTRA_FLAGS =

coverage_quiet = $(coverage_quiet_$(V))
coverage_quiet_ = $(coverage_quiet_$(AM_DEFAULT_VERBOSITY))
coverage_quiet_0 = --quiet

coverage-check: all
	-$(MAKE) $(AM_MAKEFLAGS) -k check

coverage-baseline:
	$(LCOV) $(coverage_quiet) \
		--directory $(top_builddir) \
		--output-file "$(COVERAGE_OUTPUT_FILE)" \
		--capture \
		--initial

coverage-report:
	$(LCOV) $(coverage_quiet) \
		--directory $(top_builddir) \
		--capture \
		--output-file "$(COVERAGE_OUTPUT_FILE).tmp" \
		--test-name "$(COVERAGE_TEST_NAME)" \
		--no-checksum \
		--compat-libtool \
		$(COVERAGE_LCOV_EXTRA_FLAGS)
	$(LCOV) $(coverage_quiet) \
		--directory $(top_builddir) \
		--output-file "$(COVERAGE_OUTPUT_FILE)" \
		--remove "$(COVERAGE_OUTPUT_FILE).tmp" \
		"/tmp/*" \
		"/usr/include/*" \
		"$(includedir)/*"
	-@rm -f "$(COVERAGE_OUTPUT_FILE).tmp"
	LANG=C $(GENHTML) $(coverage_quiet) \
		--prefix $(top_builddir) \
		--output-directory "$(COVERAGE_OUTPUT_DIR)" \
		--title "$(PACKAGE) $(VERSION) Code Coverage Report" \
		--show-details "$(COVERAGE_OUTPUT_FILE)" \
		--legend \
		$(COVERAGE_GENHTML_EXTRA_FLAGS)
	@echo "Code coverage report written to file://$(abs_builddir)/$(COVERAGE_OUTPUT_DIR)/index.html"

coverage-reset:
	-$(LCOV) $(coverage_quiet) --zerocounters --directory $(top_builddir)

coverage-clean-report:
	-rm -f "$(COVERAGE_OUTPUT_FILE)" "$(COVERAGE_OUTPUT_FILE).tmp"
	-rm -rf "$(COVERAGE_OUTPUT_DIR)"

coverage-clean-data:
	-find $(top_builddir) -name "*.gcda" -delete

coverage-clean: coverage-reset coverage-clean-report coverage-clean-data
	-find $(top_builddir) -name "*.gcno" -delete

coverage-distclean: coverage-clean

coverage: coverage-reset coverage-baseline coverage-check coverage-report
clean-local: coverage-clean
distclean-local: coverage-distclean

.PHONY: coverage coverage-baseline coverage-check coverage-report coverage-reset coverage-clean-report coverage-clean-data coverage-clean coverage-distclean

else

coverage:
	@echo "Configuring with --enable-coverage required to generate code coverage report."

endif # WITH_GCOV

## CMake support

EXTRA_DIST += \
  cmake/COPYING-CMAKE-SCRIPTS \
  cmake/FindPackageHandleStandardArgs.cmake \
  cmake/FindReadline.cmake \
  cmake/FindEditline.cmake \
  CMakeLists.txt \
  config-cmake.h.in

## end Makefile.am
