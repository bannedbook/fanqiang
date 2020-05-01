/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2013-2015, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#if defined(__linux)
/* This is needed on Linux to get the pthread_setname_np function. */
#if !defined(_GNU_SOURCE)
#define _GNU_SOURCE 1
#endif
#endif

#include <assert.h>
#include <string.h>

#include <pthread.h>

#include "libcork/core/allocator.h"
#include "libcork/core/error.h"
#include "libcork/core/types.h"
#include "libcork/ds/buffer.h"
#include "libcork/threads/basics.h"


/*-----------------------------------------------------------------------
 * Current thread
 */

static volatile cork_thread_id  last_thread_descriptor = 0;

struct cork_thread {
    const char  *name;
    cork_thread_id  id;
    pthread_t  thread_id;
    void  *user_data;
    cork_free_f  free_user_data;
    cork_run_f  run;
    cork_error  error_code;
    struct cork_buffer  error_message;
    bool  started;
    bool  joined;
};

struct cork_thread_descriptor {
    struct cork_thread  *current_thread;
    cork_thread_id  id;
};

cork_tls(struct cork_thread_descriptor, cork_thread_descriptor);

struct cork_thread *
cork_current_thread_get(void)
{
    struct cork_thread_descriptor  *desc = cork_thread_descriptor_get();
    return desc->current_thread;
}

cork_thread_id
cork_current_thread_get_id(void)
{
    struct cork_thread_descriptor  *desc = cork_thread_descriptor_get();
    if (CORK_UNLIKELY(desc->id == 0)) {
        if (desc->current_thread == NULL) {
            desc->id = cork_uint_atomic_add(&last_thread_descriptor, 1);
        } else {
            desc->id = desc->current_thread->id;
        }
    }
    return desc->id;
}


/*-----------------------------------------------------------------------
 * Threads
 */

struct cork_thread *
cork_thread_new(const char *name,
                void *user_data, cork_free_f free_user_data,
                cork_run_f run)
{
    struct cork_thread  *self = cork_new(struct cork_thread);
    self->name = cork_strdup(name);
    self->id = cork_uint_atomic_add(&last_thread_descriptor, 1);
    self->user_data = user_data;
    self->free_user_data = free_user_data;
    self->run = run;
    self->error_code = CORK_ERROR_NONE;
    cork_buffer_init(&self->error_message);
    self->started = false;
    self->joined = false;
    return self;
}

static void
cork_thread_free_private(struct cork_thread *self)
{
    cork_strfree(self->name);
    cork_free_user_data(self);
    cork_buffer_done(&self->error_message);
    cork_delete(struct cork_thread, self);
}

void
cork_thread_free(struct cork_thread *self)
{
    assert(!self->started);
    cork_thread_free_private(self);
}

const char *
cork_thread_get_name(struct cork_thread *self)
{
    return self->name;
}

cork_thread_id
cork_thread_get_id(struct cork_thread *self)
{
    return self->id;
}

#define PTHREADS_MAX_THREAD_NAME_LENGTH  16

static void *
cork_thread_pthread_run(void *vself)
{
    int  rc;
    struct cork_thread  *self = vself;
    struct cork_thread_descriptor  *desc = cork_thread_descriptor_get();
#if defined(__APPLE__) && defined(__MACH__)
    char  thread_name[PTHREADS_MAX_THREAD_NAME_LENGTH];
#endif

    desc->current_thread = self;
    desc->id = self->id;
    rc = self->run(self->user_data);

#if defined(__APPLE__) && defined(__MACH__)
    /* On Mac OS X, we set the name of the current thread, not of an arbitrary
     * thread of our choosing. */
    strncpy(thread_name, self->name, PTHREADS_MAX_THREAD_NAME_LENGTH);
    thread_name[PTHREADS_MAX_THREAD_NAME_LENGTH - 1] = '\0';
    pthread_setname_np(thread_name);
#endif

    /* If an error occurred in the body of the thread, save the error into the
     * cork_thread object so that we can propagate that error when some calls
     * cork_thread_join. */
    if (CORK_UNLIKELY(rc != 0)) {
        if (CORK_LIKELY(cork_error_occurred())) {
            self->error_code = cork_error_code();
            cork_buffer_set_string(&self->error_message, cork_error_message());
        } else {
            self->error_code = CORK_UNKNOWN_ERROR;
            cork_buffer_set_string(&self->error_message, "Unknown error");
        }
    }

    return NULL;
}

int
cork_thread_start(struct cork_thread *self)
{
    int  rc;
    pthread_t  thread_id;
#if defined(__linux) && ((__GLIBC__ >= 2) && (__GLIBC_MINOR__ >= 12))
    char  thread_name[PTHREADS_MAX_THREAD_NAME_LENGTH];
#endif

    assert(!self->started);

    rc = pthread_create(&thread_id, NULL, cork_thread_pthread_run, self);
    if (CORK_UNLIKELY(rc != 0)) {
        cork_system_error_set_explicit(rc);
        return -1;
    }

#if defined(__linux) && ((__GLIBC__ >= 2) && (__GLIBC_MINOR__ >= 12))
    /* On Linux we choose which thread to name via an explicit thread ID.
     * However, pthread_setname_np() isn't supported on versions of glibc
     * earlier than 2.12. So we need to check for a MINOR version of 12 or
     * higher. */
    strncpy(thread_name, self->name, PTHREADS_MAX_THREAD_NAME_LENGTH);
    thread_name[PTHREADS_MAX_THREAD_NAME_LENGTH - 1] = '\0';
    pthread_setname_np(thread_id, thread_name);
#endif

    self->thread_id = thread_id;
    self->started = true;
    return 0;
}

int
cork_thread_join(struct cork_thread *self)
{
    int  rc;

    assert(self->started && !self->joined);

    rc = pthread_join(self->thread_id, NULL);
    if (CORK_UNLIKELY(rc != 0)) {
        cork_system_error_set_explicit(rc);
        cork_thread_free_private(self);
        return -1;
    }

    if (CORK_UNLIKELY(self->error_code != CORK_ERROR_NONE)) {
        cork_error_set_printf
            (self->error_code, "Error from thread %s: %s",
             self->name, (char *) self->error_message.buf);
        cork_thread_free_private(self);
        return -1;
    }

    cork_thread_free_private(self);
    return 0;
}
