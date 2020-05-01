/**
 * @file
 *
 * SLIP netif API
 */

/*
 * Copyright (c) 2001, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
#ifndef LWIP_HDR_NETIF_SLIPIF_H
#define LWIP_HDR_NETIF_SLIPIF_H

#include "lwip/opt.h"
#include "lwip/netif.h"

/** Set this to 1 to start a thread that blocks reading on the serial line
 * (using sio_read()).
 */
#ifndef SLIP_USE_RX_THREAD
#define SLIP_USE_RX_THREAD !NO_SYS
#endif

/** Set this to 1 to enable functions to pass in RX bytes from ISR context.
 * If enabled, slipif_received_byte[s]() process incoming bytes and put assembled
 * packets on a queue, which is fed into lwIP from slipif_poll().
 * If disabled, slipif_poll() polls the serial line (using sio_tryread()).
 */
#ifndef SLIP_RX_FROM_ISR
#define SLIP_RX_FROM_ISR 0
#endif

/** Set this to 1 (default for SLIP_RX_FROM_ISR) to queue incoming packets
 * received by slipif_received_byte[s]() as long as PBUF_POOL pbufs are available.
 * If disabled, packets will be dropped if more than one packet is received.
 */
#ifndef SLIP_RX_QUEUE
#define SLIP_RX_QUEUE SLIP_RX_FROM_ISR
#endif

#ifdef __cplusplus
extern "C" {
#endif

err_t slipif_init(struct netif * netif);
void slipif_poll(struct netif *netif);
#if SLIP_RX_FROM_ISR
void slipif_process_rxqueue(struct netif *netif);
void slipif_received_byte(struct netif *netif, u8_t data);
void slipif_received_bytes(struct netif *netif, u8_t *data, u8_t len);
#endif /* SLIP_RX_FROM_ISR */

#ifdef __cplusplus
}
#endif

#endif /* LWIP_HDR_NETIF_SLIPIF_H */

