/**
 * @file
 * lwIP Operating System abstraction
 *
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/**
 * @defgroup sys_layer Porting (system abstraction layer)
 * @ingroup lwip
 * @verbinclude "sys_arch.txt"
 *
 * @defgroup sys_os OS abstraction layer
 * @ingroup sys_layer
 * No need to implement functions in this section in NO_SYS mode.
 *
 * @defgroup sys_sem Semaphores
 * @ingroup sys_os
 *
 * @defgroup sys_mutex Mutexes
 * @ingroup sys_os
 * Mutexes are recommended to correctly handle priority inversion,
 * especially if you use LWIP_CORE_LOCKING .
 *
 * @defgroup sys_mbox Mailboxes
 * @ingroup sys_os
 *
 * @defgroup sys_time Time
 * @ingroup sys_layer
 *
 * @defgroup sys_prot Critical sections
 * @ingroup sys_layer
 * Used to protect short regions of code against concurrent access.
 * - Your system is a bare-metal system (probably with an RTOS)
 *   and interrupts are under your control:
 *   Implement this as LockInterrupts() / UnlockInterrupts()
 * - Your system uses an RTOS with deferred interrupt handling from a
 *   worker thread: Implement as a global mutex or lock/unlock scheduler
 * - Your system uses a high-level OS with e.g. POSIX signals:
 *   Implement as a global mutex
 *
 * @defgroup sys_misc Misc
 * @ingroup sys_os
 */

#include "lwip/opt.h"

#include "lwip/sys.h"

/* Most of the functions defined in sys.h must be implemented in the
 * architecture-dependent file sys_arch.c */

#if !NO_SYS

#ifndef sys_msleep
/**
 * Sleep for some ms. Timeouts are NOT processed while sleeping.
 *
 * @param ms number of milliseconds to sleep
 */
void
sys_msleep(u32_t ms)
{
  if (ms > 0) {
    sys_sem_t delaysem;
    err_t err = sys_sem_new(&delaysem, 0);
    if (err == ERR_OK) {
      sys_arch_sem_wait(&delaysem, ms);
      sys_sem_free(&delaysem);
    }
  }
}
#endif /* sys_msleep */

#endif /* !NO_SYS */
