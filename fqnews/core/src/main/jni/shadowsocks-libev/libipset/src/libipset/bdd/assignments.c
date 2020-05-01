/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2010-2012, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <libcork/core.h>

#include "ipset/bdd/nodes.h"


struct ipset_assignment *
ipset_assignment_new()
{
    struct ipset_assignment  *assignment = cork_new(struct ipset_assignment);
    cork_array_init(&assignment->values);
    return assignment;
}


void
ipset_assignment_free(struct ipset_assignment *assignment)
{
    cork_array_done(&assignment->values);
    free(assignment);
}


bool
ipset_assignment_equal(const struct ipset_assignment *assignment1,
                       const struct ipset_assignment *assignment2)
{
    /* Identical pointers are trivially equal. */
    if (assignment1 == assignment2) {
        return true;
    }

    /* Otherwise we compare the assignments piecewise up through the end
     * of the smaller vector. */
    unsigned int  size1 = cork_array_size(&assignment1->values);
    unsigned int  size2 = cork_array_size(&assignment2->values);
    unsigned int  smaller_size = (size1 < size2)? size1: size2;

    unsigned int  i;
    for (i = 0; i < smaller_size; i++) {
        if (cork_array_at(&assignment1->values, i) !=
            cork_array_at(&assignment2->values, i)) {
            return false;
        }
    }

    /* If one of the assignment vectors is longer, any remaining
     * elements must be indeterminate. */
    if (size1 > smaller_size) {
        for (i = smaller_size; i < size1; i++) {
            if (cork_array_at(&assignment1->values, i) != IPSET_EITHER) {
                return false;
            }
        }
    }

    if (size2 > smaller_size) {
        for (i = smaller_size; i < size2; i++) {
            if (cork_array_at(&assignment2->values, i) != IPSET_EITHER) {
                return false;
            }
        }
    }

    /* If we make it through all of that, the two assignments are equal. */
    return true;
}


void
ipset_assignment_cut(struct ipset_assignment *assignment,
                     ipset_variable var)
{
    if (var < cork_array_size(&assignment->values)) {
        assignment->values.size = var;
    }
}


void
ipset_assignment_clear(struct ipset_assignment *assignment)
{
    ipset_assignment_cut(assignment, 0);
}


enum ipset_tribool
ipset_assignment_get(struct ipset_assignment *assignment, ipset_variable var)
{
    if (var < cork_array_size(&assignment->values)) {
        /* If the requested variable is in the range of the values
         * array, return whatever is stored there. */
        return cork_array_at(&assignment->values, var);
    } else {
        /* Variables htat aren't in the values array are always EITHER. */
        return IPSET_EITHER;
    }
}


void
ipset_assignment_set(struct ipset_assignment *assignment,
                     ipset_variable var, enum ipset_tribool value)
{
    /* Ensure that the vector is big enough to hold this variable
     * assignment, inserting new EITHERs if needed. */
    if (var >= cork_array_size(&assignment->values)) {
        unsigned int  old_len = cork_array_size(&assignment->values);

        /* Expand the array. */
        cork_array_ensure_size(&assignment->values, var+1);
        assignment->values.size = var+1;

        /* Fill in EITHERs in the newly allocated elements. */
        if (var != old_len) {
            unsigned int  i;
            for (i = old_len; i < var; i++) {
                cork_array_at(&assignment->values, i) = IPSET_EITHER;
            }
        }
    }

    /* Assign the desired value. */
    cork_array_at(&assignment->values, var) = value;
}
