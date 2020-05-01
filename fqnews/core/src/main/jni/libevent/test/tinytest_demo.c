/* tinytest_demo.c -- Copyright 2009-2012 Nick Mathewson
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/* Welcome to the example file for tinytest!  I'll show you how to set up
 * some simple and not-so-simple testcases. */

/* Make sure you include these headers. */
#include "tinytest.h"
#include "tinytest_macros.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

/* ============================================================ */

/* First, let's see if strcmp is working.  (All your test cases should be
 * functions declared to take a single void * as an argument.) */
void
test_strcmp(void *data)
{
	(void)data; /* This testcase takes no data. */

	/* Let's make sure the empty string is equal to itself */
	if (strcmp("","")) {
		/* This macro tells tinytest to stop the current test
		 * and go straight to the "end" label. */
		tt_abort_msg("The empty string was not equal to itself");
	}

	/* Pretty often, calling tt_abort_msg to indicate failure is more
	   heavy-weight than you want.	Instead, just say: */
	tt_assert(strcmp("testcase", "testcase") == 0);

	/* Occasionally, you don't want to stop the current testcase just
	   because a single assertion has failed.  In that case, use
	   tt_want: */
	tt_want(strcmp("tinytest", "testcase") > 0);

	/* You can use the tt_*_op family of macros to compare values and to
	   fail unless they have the relationship you want.  They produce
	   more useful output than tt_assert, since they display the actual
	   values of the failing things.

	   Fail unless strcmp("abc, "abc") == 0 */
	tt_int_op(strcmp("abc", "abc"), ==, 0);

	/* Fail unless strcmp("abc, "abcd") is less than 0 */
	tt_int_op(strcmp("abc", "abcd"), < , 0);

	/* Incidentally, there's a test_str_op that uses strcmp internally. */
	tt_str_op("abc", <, "abcd");


	/* Every test-case function needs to finish with an "end:"
	   label and (optionally) code to clean up local variables. */
 end:
	;
}

/* ============================================================ */

/* Now let's mess with setup and teardown functions!  These are handy if
   you have a bunch of tests that all need a similar environment, and you
   want to reconstruct that environment freshly for each one. */

/* First you declare a type to hold the environment info, and functions to
   set it up and tear it down. */
struct data_buffer {
	/* We're just going to have couple of character buffer.	 Using
	   setup/teardown functions is probably overkill for this case.

	   You could also do file descriptors, complicated handles, temporary
	   files, etc. */
	char buffer1[512];
	char buffer2[512];
};
/* The setup function needs to take a const struct testcase_t and return
   void* */
void *
setup_data_buffer(const struct testcase_t *testcase)
{
	struct data_buffer *db = malloc(sizeof(struct data_buffer));

	/* If you had a complicated set of setup rules, you might behave
	   differently here depending on testcase->flags or
	   testcase->setup_data or even or testcase->name. */

	/* Returning a NULL here would mean that we couldn't set up for this
	   test, so we don't need to test db for null. */
	return db;
}
/* The clean function deallocates storage carefully and returns true on
   success. */
int
clean_data_buffer(const struct testcase_t *testcase, void *ptr)
{
	struct data_buffer *db = ptr;

	if (db) {
		free(db);
		return 1;
	}
	return 0;
}
/* Finally, declare a testcase_setup_t with these functions. */
struct testcase_setup_t data_buffer_setup = {
	setup_data_buffer, clean_data_buffer
};


/* Now let's write our test. */
void
test_memcpy(void *ptr)
{
	/* This time, we use the argument. */
	struct data_buffer *db = ptr;

	/* We'll also introduce a local variable that might need cleaning up. */
	char *mem = NULL;

	/* Let's make sure that memcpy does what we'd like. */
	strcpy(db->buffer1, "String 0");
	memcpy(db->buffer2, db->buffer1, sizeof(db->buffer1));
	tt_str_op(db->buffer1, ==, db->buffer2);

        /* This one works if there's an internal NUL. */
        tt_mem_op(db->buffer1, <, db->buffer2, sizeof(db->buffer1));

	/* Now we've allocated memory that's referenced by a local variable.
	   The end block of the function will clean it up. */
	mem = strdup("Hello world.");
	tt_assert(mem);

	/* Another rather trivial test. */
	tt_str_op(db->buffer1, !=, mem);

 end:
	/* This time our end block has something to do. */
	if (mem)
		free(mem);
}

void
test_timeout(void *ptr)
{
	time_t t1, t2;
	(void)ptr;
	t1 = time(NULL);
#ifdef _WIN32
	Sleep(5000);
#else
	sleep(5);
#endif
	t2 = time(NULL);

	tt_int_op(t2-t1, >=, 4);

	tt_int_op(t2-t1, <=, 6);

 end:
	;
}

/* ============================================================ */

/* Now we need to make sure that our tests get invoked.	  First, you take
   a bunch of related tests and put them into an array of struct testcase_t.
*/

struct testcase_t demo_tests[] = {
	/* Here's a really simple test: it has a name you can refer to it
	   with, and a function to invoke it. */
	{ "strcmp", test_strcmp, },

	/* The second test has a flag, "TT_FORK", to make it run in a
	   subprocess, and a pointer to the testcase_setup_t that configures
	   its environment. */
	{ "memcpy", test_memcpy, TT_FORK, &data_buffer_setup },

	/* This flag is off-by-default, since it takes a while to run.	You
	 * can enable it manually by passing +demo/timeout at the command line.*/
	{ "timeout", test_timeout, TT_OFF_BY_DEFAULT },

	/* The array has to end with END_OF_TESTCASES. */
	END_OF_TESTCASES
};

/* Next, we make an array of testgroups.  This is mandatory.  Unlike more
   heavy-duty testing frameworks, groups can't nest. */
struct testgroup_t groups[] = {

	/* Every group has a 'prefix', and an array of tests.  That's it. */
	{ "demo/", demo_tests },

	END_OF_GROUPS
};

/* We can also define test aliases. These can be used for types of tests that
 * cut across groups. */
const char *alltests[] = { "+..", NULL };
const char *slowtests[] = { "+demo/timeout", NULL };
struct testlist_alias_t aliases[] = {

	{ "ALL", alltests },
	{ "SLOW", slowtests },

	END_OF_ALIASES
};


int
main(int c, const char **v)
{
	/* Finally, just call tinytest_main().	It lets you specify verbose
	   or quiet output with --verbose and --quiet.	You can list
	   specific tests:

	       tinytest-demo demo/memcpy

	   or use a ..-wildcard to select multiple tests with a common
	   prefix:

	       tinytest-demo demo/..

	   If you list no tests, you get them all by default, so that
	   "tinytest-demo" and "tinytest-demo .." mean the same thing.

	*/
	tinytest_set_aliases(aliases);
	return tinytest_main(c, v, groups);
}
