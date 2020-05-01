/**
 * @file modules.h
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
 */

#ifndef BADVPN_NCD_MODULES_MODULES_H
#define BADVPN_NCD_MODULES_MODULES_H

#include <stddef.h>

#include <ncd/NCDModule.h>

extern const struct NCDModuleGroup ncdmodule_var;
extern const struct NCDModuleGroup ncdmodule_list;
extern const struct NCDModuleGroup ncdmodule_depend;
extern const struct NCDModuleGroup ncdmodule_multidepend;
extern const struct NCDModuleGroup ncdmodule_dynamic_depend;
extern const struct NCDModuleGroup ncdmodule_concat;
extern const struct NCDModuleGroup ncdmodule_if;
extern const struct NCDModuleGroup ncdmodule_strcmp;
extern const struct NCDModuleGroup ncdmodule_logical;
extern const struct NCDModuleGroup ncdmodule_sleep;
extern const struct NCDModuleGroup ncdmodule_print;
extern const struct NCDModuleGroup ncdmodule_blocker;
extern const struct NCDModuleGroup ncdmodule_spawn;
extern const struct NCDModuleGroup ncdmodule_imperative;
extern const struct NCDModuleGroup ncdmodule_ref;
extern const struct NCDModuleGroup ncdmodule_index;
extern const struct NCDModuleGroup ncdmodule_alias;
extern const struct NCDModuleGroup ncdmodule_process_manager;
extern const struct NCDModuleGroup ncdmodule_ondemand;
extern const struct NCDModuleGroup ncdmodule_foreach;
extern const struct NCDModuleGroup ncdmodule_choose;
extern const struct NCDModuleGroup ncdmodule_from_string;
extern const struct NCDModuleGroup ncdmodule_to_string;
extern const struct NCDModuleGroup ncdmodule_value;
extern const struct NCDModuleGroup ncdmodule_try;
extern const struct NCDModuleGroup ncdmodule_exit;
extern const struct NCDModuleGroup ncdmodule_getargs;
extern const struct NCDModuleGroup ncdmodule_arithmetic;
extern const struct NCDModuleGroup ncdmodule_parse;
extern const struct NCDModuleGroup ncdmodule_valuemetic;
extern const struct NCDModuleGroup ncdmodule_file;
extern const struct NCDModuleGroup ncdmodule_netmask;
extern const struct NCDModuleGroup ncdmodule_implode;
extern const struct NCDModuleGroup ncdmodule_call2;
extern const struct NCDModuleGroup ncdmodule_assert;
extern const struct NCDModuleGroup ncdmodule_explode;
extern const struct NCDModuleGroup ncdmodule_net_ipv4_addr_in_network;
extern const struct NCDModuleGroup ncdmodule_net_ipv6_addr_in_network;
extern const struct NCDModuleGroup ncdmodule_timer;
extern const struct NCDModuleGroup ncdmodule_file_open;
extern const struct NCDModuleGroup ncdmodule_backtrack;
extern const struct NCDModuleGroup ncdmodule_depend_scope;
extern const struct NCDModuleGroup ncdmodule_substr;
extern const struct NCDModuleGroup ncdmodule_log;
extern const struct NCDModuleGroup ncdmodule_getenv;
extern const struct NCDModuleGroup ncdmodule_basic_functions;
extern const struct NCDModuleGroup ncdmodule_objref;
#ifndef BADVPN_EMSCRIPTEN
extern const struct NCDModuleGroup ncdmodule_regex_match;
extern const struct NCDModuleGroup ncdmodule_run;
extern const struct NCDModuleGroup ncdmodule_runonce;
extern const struct NCDModuleGroup ncdmodule_daemon;
extern const struct NCDModuleGroup ncdmodule_net_backend_waitdevice;
extern const struct NCDModuleGroup ncdmodule_net_backend_waitlink;
extern const struct NCDModuleGroup ncdmodule_net_backend_badvpn;
extern const struct NCDModuleGroup ncdmodule_net_backend_wpa_supplicant;
#ifdef BADVPN_USE_LINUX_RFKILL
extern const struct NCDModuleGroup ncdmodule_net_backend_rfkill;
#endif
extern const struct NCDModuleGroup ncdmodule_net_up;
extern const struct NCDModuleGroup ncdmodule_net_dns;
extern const struct NCDModuleGroup ncdmodule_net_iptables;
extern const struct NCDModuleGroup ncdmodule_net_ipv4_addr;
extern const struct NCDModuleGroup ncdmodule_net_ipv4_route;
extern const struct NCDModuleGroup ncdmodule_net_ipv4_dhcp;
extern const struct NCDModuleGroup ncdmodule_net_ipv4_arp_probe;
extern const struct NCDModuleGroup ncdmodule_net_watch_interfaces;
extern const struct NCDModuleGroup ncdmodule_sys_watch_input;
extern const struct NCDModuleGroup ncdmodule_sys_watch_usb;
#ifdef BADVPN_USE_LINUX_INPUT
extern const struct NCDModuleGroup ncdmodule_sys_evdev;
#endif
#ifdef BADVPN_USE_INOTIFY
extern const struct NCDModuleGroup ncdmodule_sys_watch_directory;
#endif
extern const struct NCDModuleGroup ncdmodule_sys_request_server;
extern const struct NCDModuleGroup ncdmodule_net_ipv6_wait_dynamic_addr;
extern const struct NCDModuleGroup ncdmodule_sys_request_client;
extern const struct NCDModuleGroup ncdmodule_reboot;
extern const struct NCDModuleGroup ncdmodule_net_ipv6_addr;
extern const struct NCDModuleGroup ncdmodule_net_ipv6_route;
extern const struct NCDModuleGroup ncdmodule_socket;
extern const struct NCDModuleGroup ncdmodule_sys_start_process;
extern const struct NCDModuleGroup ncdmodule_load_module;
#endif

static const struct NCDModuleGroup *ncd_modules[] = {
    &ncdmodule_var,
    &ncdmodule_list,
    &ncdmodule_depend,
    &ncdmodule_multidepend,
    &ncdmodule_dynamic_depend,
    &ncdmodule_concat,
    &ncdmodule_if,
    &ncdmodule_strcmp,
    &ncdmodule_logical,
    &ncdmodule_sleep,
    &ncdmodule_print,
    &ncdmodule_blocker,
    &ncdmodule_spawn,
    &ncdmodule_imperative,
    &ncdmodule_ref,
    &ncdmodule_index,
    &ncdmodule_alias,
    &ncdmodule_process_manager,
    &ncdmodule_ondemand,
    &ncdmodule_foreach,
    &ncdmodule_choose,
    &ncdmodule_from_string,
    &ncdmodule_to_string,
    &ncdmodule_value,
    &ncdmodule_try,
    &ncdmodule_exit,
    &ncdmodule_getargs,
    &ncdmodule_arithmetic,
    &ncdmodule_parse,
    &ncdmodule_valuemetic,
    &ncdmodule_file,
    &ncdmodule_netmask,
    &ncdmodule_implode,
    &ncdmodule_call2,
    &ncdmodule_assert,
    &ncdmodule_explode,
    &ncdmodule_net_ipv4_addr_in_network,
    &ncdmodule_net_ipv6_addr_in_network,
    &ncdmodule_timer,
    &ncdmodule_file_open,
    &ncdmodule_backtrack,
    &ncdmodule_depend_scope,
    &ncdmodule_substr,
    &ncdmodule_log,
    &ncdmodule_getenv,
    &ncdmodule_basic_functions,
    &ncdmodule_objref,
#ifndef BADVPN_EMSCRIPTEN
    &ncdmodule_regex_match,
    &ncdmodule_run,
    &ncdmodule_runonce,
    &ncdmodule_daemon,
    &ncdmodule_net_backend_waitdevice,
    &ncdmodule_net_backend_waitlink,
    &ncdmodule_net_backend_badvpn,
    &ncdmodule_net_backend_wpa_supplicant,
#ifdef BADVPN_USE_LINUX_RFKILL
    &ncdmodule_net_backend_rfkill,
#endif
    &ncdmodule_net_up,
    &ncdmodule_net_dns,
    &ncdmodule_net_iptables,
    &ncdmodule_net_ipv4_addr,
    &ncdmodule_net_ipv4_route,
    &ncdmodule_net_ipv4_dhcp,
    &ncdmodule_net_ipv4_arp_probe,
    &ncdmodule_net_watch_interfaces,
    &ncdmodule_sys_watch_input,
    &ncdmodule_sys_watch_usb,
#ifdef BADVPN_USE_LINUX_INPUT
    &ncdmodule_sys_evdev,
#endif
#ifdef BADVPN_USE_INOTIFY
    &ncdmodule_sys_watch_directory,
#endif
    &ncdmodule_sys_request_server,
    &ncdmodule_net_ipv6_wait_dynamic_addr,
    &ncdmodule_sys_request_client,
    &ncdmodule_reboot,
    &ncdmodule_net_ipv6_addr,
    &ncdmodule_net_ipv6_route,
    &ncdmodule_socket,
    &ncdmodule_sys_start_process,
    &ncdmodule_load_module,
#endif
    NULL
};

#endif
