/**
 * @file balign.h
 * @author Ambroz Bizjak <ambrop7@gmail.com>
 * 
 * @section LICENSE
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * @section DESCRIPTION
 * 
 * Integer alignment macros.
 */

#ifndef BADVPN_MISC_BALIGN_H
#define BADVPN_MISC_BALIGN_H

#include <stddef.h>
#include <stdint.h>

/**
 * Checks if aligning x up to n would overflow.
 */
static int balign_up_overflows (size_t x, size_t n)
{
    size_t r = x % n;
    
    return (r && x > SIZE_MAX - (n - r));
}

/**
 * Aligns x up to n.
 */
static size_t balign_up (size_t x, size_t n)
{
    size_t r = x % n;
    return (r ? x + (n - r) : x);
}

/**
 * Aligns x down to n.
 */
static size_t balign_down (size_t x, size_t n)
{
    return (x - (x % n));
}

/**
 * Calculates the quotient of a and b, rounded up.
 */
static size_t bdivide_up (size_t a, size_t b)
{
    size_t r = a % b;
    return (r > 0 ? a / b + 1 : a / b);
}

#endif
