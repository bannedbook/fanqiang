/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2010-2012, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <string.h>

#include <libcork/core.h>

#include "ipset/bdd/nodes.h"
#include "ipset/bits.h"
#include "ipset/logging.h"


static void
initialize(struct ipset_expanded_assignment *exp,
           const struct ipset_assignment *assignment,
           ipset_variable var_count)
{
    /* First loop through all of the variables in the assignment vector,
     * making sure not to go further than the caller requested. */

    ipset_variable  last_assignment = cork_array_size(&assignment->values);
    if (var_count < last_assignment) {
        last_assignment = var_count;
    }

    ipset_variable  var;
    for (var = 0; var < last_assignment; var++) {
        enum ipset_tribool  curr_value =
            cork_array_at(&assignment->values, var);

        if (curr_value == IPSET_EITHER) {
            /* If this variable is EITHER, start it off as FALSE, and
             * add it to the eithers list. */
            DEBUG("Variable %u is EITHER", var);

            IPSET_BIT_SET(exp->values.buf, var, false);
            cork_array_append(&exp->eithers, var);
        } else {
            /* Otherwise set the variable to the same value in the
             * expanded assignment as it is in the non-expanded one. */

            DEBUG("Variable %u is %s", var, curr_value? "true": "false");
            IPSET_BIT_SET(exp->values.buf, var, curr_value);
        }
    }

    /* If the caller requested more variables than there are in the
     * assignment vector, add them to the eithers list. */
    for (var = last_assignment; var < var_count; var++) {
        DEBUG("Variable %u is implicitly EITHER", var);
        cork_array_append(&exp->eithers, var);
    }
}


struct ipset_expanded_assignment *
ipset_assignment_expand(const struct ipset_assignment *assignment,
                        ipset_variable var_count)
{
    /* First allocate the iterator itself, and all of its contained
     * fields. */

    struct ipset_expanded_assignment  *exp;
    unsigned int  values_size = (var_count / 8) + ((var_count % 8) != 0);

    exp = cork_new(struct ipset_expanded_assignment);
    exp->finished = false;
    cork_buffer_init(&exp->values);
    cork_buffer_ensure_size(&exp->values, values_size);
    memset(exp->values.buf, 0, values_size);
    cork_array_init(&exp->eithers);

    /* Then initialize the values and eithers fields. */
    initialize(exp, assignment, var_count);
    return exp;
}


void
ipset_expanded_assignment_free(struct ipset_expanded_assignment *exp)
{
    if (exp == NULL) {
        return;
    }

    cork_buffer_done(&exp->values);
    cork_array_done(&exp->eithers);
    free(exp);
}


void
ipset_expanded_assignment_advance(struct ipset_expanded_assignment *exp)
{
    /* If we're already at the end of the iterator, don't do anything. */
    if (CORK_UNLIKELY(exp->finished)) {
        return;
    }

    DEBUG("Advancing iterator");

    /* Look at the last EITHER bit in the assignment.  If it's 0, then
     * set it to 1 and return.  Otherwise we set it to 0 and carry up to
     * the previous indeterminate bit. */

    size_t  i;
    for (i = cork_array_size(&exp->eithers); i > 0; i--) {
        size_t  idx = i - 1;
        ipset_variable  either_var = cork_array_at(&exp->eithers, idx);
        DEBUG("Checking EITHER variable %u", either_var);

        if (IPSET_BIT_GET(exp->values.buf, either_var)) {
            /* This variable is currently true, so set it back to false
             * and carry. */
            DEBUG("  Variable %u is true, changing to false and carrying",
                  either_var);
            IPSET_BIT_SET(exp->values.buf, either_var, false);
        } else {
            /* This variable is currently false, so set it to true and
             * return. */
            DEBUG("  Variable %u is false, changing to true",
                  either_var);
            IPSET_BIT_SET(exp->values.buf, either_var, true);
            return;
        }
    }

    /* If we fall through then we've made it through all of the expanded
     * assignments. */
    exp->finished = true;
}
