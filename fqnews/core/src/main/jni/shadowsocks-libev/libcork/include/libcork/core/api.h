/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2012-2015, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#ifndef LIBCORK_CORE_API_H
#define LIBCORK_CORE_API_H

#include <libcork/config.h>
#include <libcork/core/attributes.h>


/*-----------------------------------------------------------------------
 * Calling conventions
 */

/* If you're using libcork as a shared library, you don't need to do anything
 * special; the following will automatically set things up so that libcork's
 * public symbols are imported from the library.  When we build the shared
 * library, we define this ourselves to export the symbols. */

#if !defined(CORK_API)
#define CORK_API  CORK_IMPORT
#endif


/*-----------------------------------------------------------------------
 * Library version
 */

#define CORK_VERSION_MAJOR  CORK_CONFIG_VERSION_MAJOR
#define CORK_VERSION_MINOR  CORK_CONFIG_VERSION_MINOR
#define CORK_VERSION_PATCH  CORK_CONFIG_VERSION_PATCH

#define CORK_MAKE_VERSION(major, minor, patch) \
    ((major * 1000000) + (minor * 1000) + patch)

#define CORK_VERSION  \
    CORK_MAKE_VERSION(CORK_VERSION_MAJOR, \
                      CORK_VERSION_MINOR, \
                      CORK_VERSION_PATCH)

CORK_API const char *
cork_version_string(void)
    CORK_ATTR_CONST;

CORK_API const char *
cork_revision_string(void)
    CORK_ATTR_CONST;


#endif /* LIBCORK_CORE_API_H */
