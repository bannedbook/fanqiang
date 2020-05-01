#!/bin/sh

FAILED=no

if test "x$TEST_OUTPUT_FILE" = "x"
then
	TEST_OUTPUT_FILE=/dev/null
fi

# /bin/echo is a little more likely to support -n than sh's builtin echo.
if test -x /bin/echo
then
	ECHO=/bin/echo
else
	ECHO=echo
fi

if test "$TEST_OUTPUT_FILE" != "/dev/null"
then
	touch "$TEST_OUTPUT_FILE" || exit 1
fi

TEST_DIR=.

T=`echo "$0" | sed -e 's/test-ratelim.sh$//'`
if test -x "$T/test-ratelim"
then
	TEST_DIR="$T"
fi

announce () {
	echo $@
	echo $@ >>"$TEST_OUTPUT_FILE"
}

announce_n () {
	$ECHO -n $@
	echo $@ >>"$TEST_OUTPUT_FILE"
}


run_tests () {
	announce_n "  Group limits, no connection limit:"
	if $TEST_DIR/test-ratelim -g 30000 -n 30 -t 100 --check-grouplimit 1000 --check-stddev 100 >>"$TEST_OUTPUT_FILE"
	then
		announce OKAY
	else
		announce FAILED
		FAILED=yes
	fi

	announce_n "  Connection limit, no group limit:"
	if $TEST_DIR/test-ratelim -c 1000 -n 30 -t 100 --check-connlimit 50 --check-stddev 50 >>"$TEST_OUTPUT_FILE"
	then
		announce OKAY ;
	else
		announce FAILED ;
		FAILED=yes
	fi

	announce_n "  Connection limit and group limit:"
	if $TEST_DIR/test-ratelim -c 1000 -g 30000 -n 30 -t 100 --check-grouplimit 1000 --check-connlimit 50 --check-stddev 50 >>"$TEST_OUTPUT_FILE"
	then
		announce OKAY ;
	else
		announce FAILED ;
		FAILED=yes
	fi

	announce_n "  Connection limit and group limit with independent drain:"
	if $TEST_DIR/test-ratelim -c 1000 -g 35000 -n 30 -t 100 -G 500 --check-grouplimit 1000 --check-connlimit 50 --check-stddev 50 >>"$TEST_OUTPUT_FILE"
	then
		announce OKAY ;
	else
		announce FAILED ;
		FAILED=yes
	fi


}

announce "Running rate-limiting tests:"

run_tests

if test "$FAILED" = "yes"; then
	exit 1
fi
