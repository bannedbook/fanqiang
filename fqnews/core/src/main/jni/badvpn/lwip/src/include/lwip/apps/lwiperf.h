/**
 * @file
 * lwIP iPerf server implementation
 */

/*
 * Copyright (c) 2014 Simon Goldschmidt
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
 * Author: Simon Goldschmidt
 *
 */
#ifndef LWIP_HDR_APPS_LWIPERF_H
#define LWIP_HDR_APPS_LWIPERF_H

#include "lwip/opt.h"
#include "lwip/ip_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LWIPERF_TCP_PORT_DEFAULT  5001

/** lwIPerf test results */
enum lwiperf_report_type
{
  /** The server side test is done */
  LWIPERF_TCP_DONE_SERVER,
  /** The client side test is done */
  LWIPERF_TCP_DONE_CLIENT,
  /** Local error lead to test abort */
  LWIPERF_TCP_ABORTED_LOCAL,
  /** Data check error lead to test abort */
  LWIPERF_TCP_ABORTED_LOCAL_DATAERROR,
  /** Transmit error lead to test abort */
  LWIPERF_TCP_ABORTED_LOCAL_TXERROR,
  /** Remote side aborted the test */
  LWIPERF_TCP_ABORTED_REMOTE
};

/** Prototype of a report function that is called when a session is finished.
    This report function can show the test results.
    @param report_type contains the test result */
typedef void (*lwiperf_report_fn)(void *arg, enum lwiperf_report_type report_type,
  const ip_addr_t* local_addr, u16_t local_port, const ip_addr_t* remote_addr, u16_t remote_port,
  u32_t bytes_transferred, u32_t ms_duration, u32_t bandwidth_kbitpsec);


void* lwiperf_start_tcp_server(const ip_addr_t* local_addr, u16_t local_port,
                               lwiperf_report_fn report_fn, void* report_arg);
void* lwiperf_start_tcp_server_default(lwiperf_report_fn report_fn, void* report_arg);
void  lwiperf_abort(void* lwiperf_session);


#ifdef __cplusplus
}
#endif

#endif /* LWIP_HDR_APPS_LWIPERF_H */
