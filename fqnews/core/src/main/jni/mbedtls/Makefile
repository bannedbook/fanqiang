
DESTDIR=/usr/local
PREFIX=mbedtls_

.SILENT:

.PHONY: all no_test programs lib tests install uninstall clean test check covtest lcov apidoc apidoc_clean

all: programs tests
	$(MAKE) post_build

no_test: programs

programs: lib
	$(MAKE) -C programs

lib:
	$(MAKE) -C library

tests: lib
	$(MAKE) -C tests

ifndef WINDOWS
install: no_test
	mkdir -p $(DESTDIR)/include/mbedtls
	cp -rp include/mbedtls $(DESTDIR)/include

	mkdir -p $(DESTDIR)/lib
	cp -RP library/libmbedtls.*    $(DESTDIR)/lib
	cp -RP library/libmbedx509.*   $(DESTDIR)/lib
	cp -RP library/libmbedcrypto.* $(DESTDIR)/lib

	mkdir -p $(DESTDIR)/bin
	for p in programs/*/* ; do              \
	    if [ -x $$p ] && [ ! -d $$p ] ;     \
	    then                                \
	        f=$(PREFIX)`basename $$p` ;     \
	        cp $$p $(DESTDIR)/bin/$$f ;     \
	    fi                                  \
	done

uninstall:
	rm -rf $(DESTDIR)/include/mbedtls
	rm -f $(DESTDIR)/lib/libmbedtls.*
	rm -f $(DESTDIR)/lib/libmbedx509.*
	rm -f $(DESTDIR)/lib/libmbedcrypto.*

	for p in programs/*/* ; do              \
	    if [ -x $$p ] && [ ! -d $$p ] ;     \
	    then                                \
	        f=$(PREFIX)`basename $$p` ;     \
	        rm -f $(DESTDIR)/bin/$$f ;      \
	    fi                                  \
	done
endif

WARNING_BORDER      =*******************************************************\n
NULL_ENTROPY_WARN_L1=****  WARNING!  MBEDTLS_TEST_NULL_ENTROPY defined! ****\n
NULL_ENTROPY_WARN_L2=****  THIS BUILD HAS NO DEFINED ENTROPY SOURCES    ****\n
NULL_ENTROPY_WARN_L3=****  AND IS *NOT* SUITABLE FOR PRODUCTION USE     ****\n

NULL_ENTROPY_WARNING=\n$(WARNING_BORDER)$(NULL_ENTROPY_WARN_L1)$(NULL_ENTROPY_WARN_L2)$(NULL_ENTROPY_WARN_L3)$(WARNING_BORDER)

WARNING_BORDER_LONG      =**********************************************************************************\n
CTR_DRBG_128_BIT_KEY_WARN_L1=****  WARNING!  MBEDTLS_CTR_DRBG_USE_128_BIT_KEY defined!                      ****\n
CTR_DRBG_128_BIT_KEY_WARN_L2=****  Using 128-bit keys for CTR_DRBG limits the security of generated         ****\n
CTR_DRBG_128_BIT_KEY_WARN_L3=****  keys and operations that use random values generated to 128-bit security ****\n

CTR_DRBG_128_BIT_KEY_WARNING=\n$(WARNING_BORDER_LONG)$(CTR_DRBG_128_BIT_KEY_WARN_L1)$(CTR_DRBG_128_BIT_KEY_WARN_L2)$(CTR_DRBG_128_BIT_KEY_WARN_L3)$(WARNING_BORDER_LONG)

# Post build steps
post_build:
ifndef WINDOWS

	# If 128-bit keys are configured for CTR_DRBG, display an appropriate warning
	-scripts/config.pl get MBEDTLS_CTR_DRBG_USE_128_BIT_KEY && ([ $$? -eq 0 ]) && \
	    echo '$(CTR_DRBG_128_BIT_KEY_WARNING)'

	# If NULL Entropy is configured, display an appropriate warning
	-scripts/config.pl get MBEDTLS_TEST_NULL_ENTROPY && ([ $$? -eq 0 ]) && \
	    echo '$(NULL_ENTROPY_WARNING)'
endif

clean:
	$(MAKE) -C library clean
	$(MAKE) -C programs clean
	$(MAKE) -C tests clean
ifndef WINDOWS
	find . \( -name \*.gcno -o -name \*.gcda -o -name \*.info \) -exec rm {} +
endif

check: lib tests
	$(MAKE) -C tests check

test: check

ifndef WINDOWS
# note: for coverage testing, build with:
# make CFLAGS='--coverage -g3 -O0'
covtest:
	$(MAKE) check
	programs/test/selftest
	tests/compat.sh
	tests/ssl-opt.sh

lcov:
	rm -rf Coverage
	lcov --capture --initial --directory library -o files.info
	lcov --capture --directory library -o tests.info
	lcov --add-tracefile files.info --add-tracefile tests.info -o all.info
	lcov --remove all.info -o final.info '*.h'
	gendesc tests/Descriptions.txt -o descriptions
	genhtml --title "mbed TLS" --description-file descriptions --keep-descriptions --legend --no-branch-coverage -o Coverage final.info
	rm -f files.info tests.info all.info final.info descriptions

apidoc:
	mkdir -p apidoc
	cd doxygen && doxygen mbedtls.doxyfile

apidoc_clean:
	rm -rf apidoc
endif
