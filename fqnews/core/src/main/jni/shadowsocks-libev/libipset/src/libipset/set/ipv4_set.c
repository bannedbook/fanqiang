/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009-2012, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

/*
 * The IPv4 and IPv6 set types are basically identical, except for the
 * names of the functions, and the size of the values that are being
 * stored.  Rather than having two mostly duplicate definitions of each
 * function, we define “template functions” where anything that depends
 * on the size of the IP address is defined using the following macros.
 */


/* The name of the cork_ipvX type. */
#define CORK_IP  struct cork_ipv4

/* The number of bits in an IPvX address. */
#define IP_BIT_SIZE  32

/* The value of the discriminator variable for an IPvX address. */
#define IP_DISCRIMINATOR_VALUE  true

/* Creates a identifier of the form “ipset_ipv4_<basename>”. */
#define IPSET_NAME(basename) ipset_ipv4_##basename

/* Creates a identifier of the form “ipset_<basename>_ipv4”. */
#define IPSET_PRENAME(basename) ipset_##basename##_ipv4


/* Now include all of the templates. */
#include "inspection-template.c.in"
