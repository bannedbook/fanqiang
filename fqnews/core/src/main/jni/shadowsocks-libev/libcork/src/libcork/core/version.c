/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2015, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include "libcork/config.h"
#include "libcork/core/api.h"


/*-----------------------------------------------------------------------
 * Library version
 */

const char *
cork_version_string(void)
{
    return CORK_CONFIG_VERSION_STRING;
}

const char *
cork_revision_string(void)
{
    return CORK_CONFIG_REVISION;
}
