/**
 * @file dead.h
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
 * Dead mechanism definitions.
 * 
 * The dead mechanism is a way for a piece of code to detect whether some
 * specific event has occured during some operation (usually during calling
 * a user-provided handler function), without requiring access to memory
 * that might no longer be available because of the event.
 * 
 * It works somehow like that:
 * 
 * First a dead variable ({@link dead_t}) is allocated somewhere, and
 * initialized with {@link DEAD_INIT}, e.g.:
 *   DEAD_INIT(dead);
 * 
 * When the event that needs to be caught occurs, {@link DEAD_KILL} is
 * called, e.g.:
 *   DEAD_KILL(dead);
 * The memory used by the dead variable is no longer needed after
 * that.
 * 
 * If a piece of code needs to know whether the event occured during some
 * operation (but it must not have occured before!), it puts {@link DEAD_ENTER}}
 * in front of the operation, and does {@link DEAD_LEAVE} at the end. If
 * {@link DEAD_LEAVE} returned nonzero, the event occured, otherwise it did
 * not. Example:
 *   DEAD_ENTER(dead)
 *   HandlerFunction();
 *   if (DEAD_LEAVE(dead)) {
 *     (event occured)
 *   }
 * 
 * If is is needed to check for the event more than once in a single block,
 * {@link DEAD_DECLARE} should be put somewhere before, and {@link DEAD_ENTER2}
 * should be used instead of {@link DEAD_ENTER}.
 * 
 * If it is needed to check for multiple events (dead variables) at the same
 * time, DEAD_*_N macros should be used instead, specifying different
 * identiers as the first argument for different dead variables.
 */

#ifndef BADVPN_MISC_DEAD_H
#define BADVPN_MISC_DEAD_H

#include <stdlib.h>

/**
 * Dead variable.
 */
typedef int *dead_t;

/**
 * Initializes a dead variable.
 */
#define DEAD_INIT(ptr) { ptr = NULL; }

/**
 * Kills the dead variable,
 */
#define DEAD_KILL(ptr) { if (ptr) *(ptr) = 1; }

/**
 * Kills the dead variable with the given value, or does nothing
 * if the value is 0. The value will seen by {@link DEAD_KILLED}.
 */
#define DEAD_KILL_WITH(ptr, val) { if (ptr) *(ptr) = (val); }

/**
 * Declares dead catching variables.
 */
#define DEAD_DECLARE int badvpn__dead; dead_t badvpn__prev_ptr;

/**
 * Enters a dead catching using already declared dead catching variables.
 * The dead variable must have been initialized with {@link DEAD_INIT},
 * and {@link DEAD_KILL} must not have been called yet.
 * {@link DEAD_LEAVE2} must be called before the current scope is left.
 */
#define DEAD_ENTER2(ptr) { badvpn__dead = 0; badvpn__prev_ptr = ptr; ptr = &badvpn__dead; }

/**
 * Declares dead catching variables and enters a dead catching.
 * The dead variable must have been initialized with {@link DEAD_INIT},
 * and {@link DEAD_KILL} must not have been called yet.
 * {@link DEAD_LEAVE2} must be called before the current scope is left.
 */
#define DEAD_ENTER(ptr) DEAD_DECLARE DEAD_ENTER2(ptr)

/**
 * Leaves a dead catching.
 */
#define DEAD_LEAVE2(ptr) { if (!badvpn__dead) ptr = badvpn__prev_ptr; if (badvpn__prev_ptr) *badvpn__prev_ptr = badvpn__dead; }

/**
 * Returns 1 if {@link DEAD_KILL} was called for the dead variable, 0 otherwise.
 * Must be called after entering a dead catching.
 */
#define DEAD_KILLED (badvpn__dead)

#define DEAD_DECLARE_N(n) int badvpn__dead##n; dead_t badvpn__prev_ptr##n;
#define DEAD_ENTER2_N(n, ptr) { badvpn__dead##n = 0; badvpn__prev_ptr##n = ptr; ptr = &badvpn__dead##n;}
#define DEAD_ENTER_N(n, ptr) DEAD_DECLARE_N(n) DEAD_ENTER2_N(n, ptr)
#define DEAD_LEAVE2_N(n, ptr) { if (!badvpn__dead##n) ptr = badvpn__prev_ptr##n; if (badvpn__prev_ptr##n) *badvpn__prev_ptr##n = badvpn__dead##n; }
#define DEAD_KILLED_N(n) (badvpn__dead##n)

#endif
