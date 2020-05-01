/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2012, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef IPSET_ERRORS_H
#define IPSET_ERRORS_H


#include <libcork/core.h>


/*-----------------------------------------------------------------------
 * Error reporting
 */

/* Hash of "ipset.h" */
#define IPSET_ERROR  0xf2000181

enum ipset_error {
    IPSET_IO_ERROR,
    IPSET_PARSE_ERROR
};


#endif  /* IPSET_ERRORS_H */
