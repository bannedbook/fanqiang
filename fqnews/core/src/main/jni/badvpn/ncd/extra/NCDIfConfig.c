/**
 * @file NCDIfConfig.c
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

#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/if_tun.h>
#include <pwd.h>
  
#include <misc/debug.h>
#include <base/BLog.h>

#include "NCDIfConfig.h"

#include <generated/blog_channel_NCDIfConfig.h>

#define IP_CMD "ip"
#define MODPROBE_CMD "modprobe"
#define RESOLVCONF_FILE "/etc/resolv.conf"
#define RESOLVCONF_TEMP_FILE "/etc/resolv.conf-ncd-temp"
#define TUN_DEVNODE "/dev/net/tun"

static int run_command (const char *cmd)
{
    BLog(BLOG_INFO, "run: %s", cmd);
    
    return system(cmd);
}

static int write_to_file (uint8_t *data, size_t data_len, FILE *f)
{
    while (data_len > 0) {
        size_t bytes = fwrite(data, 1, data_len, f);
        if (bytes == 0) {
            return 0;
        }
        data += bytes;
        data_len -= bytes;
    }
    
    return 1;
}

int NCDIfConfig_query (const char *ifname)
{
    struct ifreq ifr;
    
    int flags = 0;
    
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (!s) {
        BLog(BLOG_ERROR, "socket failed");
        goto fail0;
    }
    
    memset(&ifr, 0, sizeof(ifr));
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", ifname);
    if (ioctl(s, SIOCGIFFLAGS, &ifr)) {
        BLog(BLOG_ERROR, "SIOCGIFFLAGS failed");
        goto fail1;
    }
    
    flags |= NCDIFCONFIG_FLAG_EXISTS;
    
    if ((ifr.ifr_flags&IFF_UP)) {
        flags |= NCDIFCONFIG_FLAG_UP;
        
        if ((ifr.ifr_flags&IFF_RUNNING)) {
            flags |= NCDIFCONFIG_FLAG_RUNNING;
        }
    }
    
fail1:
    close(s);
fail0:
    return flags;
}

int NCDIfConfig_set_up (const char *ifname)
{
    if (strlen(ifname) >= IFNAMSIZ) {
        BLog(BLOG_ERROR, "ifname too long");
        return 0;
    }
    
    char cmd[50 + IFNAMSIZ];
    sprintf(cmd, IP_CMD" link set %s up", ifname);
    
    return !run_command(cmd);
}

int NCDIfConfig_set_down (const char *ifname)
{
    if (strlen(ifname) >= IFNAMSIZ) {
        BLog(BLOG_ERROR, "ifname too long");
        return 0;
    }
    
    char cmd[50 + IFNAMSIZ];
    sprintf(cmd, IP_CMD" link set %s down", ifname);
    
    return !run_command(cmd);
}

int NCDIfConfig_add_ipv4_addr (const char *ifname, struct ipv4_ifaddr ifaddr)
{
    ASSERT(ifaddr.prefix >= 0)
    ASSERT(ifaddr.prefix <= 32)
    
    if (strlen(ifname) >= IFNAMSIZ) {
        BLog(BLOG_ERROR, "ifname too long");
        return 0;
    }
    
    uint8_t *addr = (uint8_t *)&ifaddr.addr;
    
    char cmd[50 + IFNAMSIZ];
    sprintf(cmd, IP_CMD" addr add %"PRIu8".%"PRIu8".%"PRIu8".%"PRIu8"/%d dev %s", addr[0], addr[1], addr[2], addr[3], ifaddr.prefix, ifname);
    
    return !run_command(cmd);
}

int NCDIfConfig_remove_ipv4_addr (const char *ifname, struct ipv4_ifaddr ifaddr)
{
    ASSERT(ifaddr.prefix >= 0)
    ASSERT(ifaddr.prefix <= 32)
    
    if (strlen(ifname) >= IFNAMSIZ) {
        BLog(BLOG_ERROR, "ifname too long");
        return 0;
    }
    
    uint8_t *addr = (uint8_t *)&ifaddr.addr;
    
    char cmd[50 + IFNAMSIZ];
    sprintf(cmd, IP_CMD" addr del %"PRIu8".%"PRIu8".%"PRIu8".%"PRIu8"/%d dev %s", addr[0], addr[1], addr[2], addr[3], ifaddr.prefix, ifname);
    
    return !run_command(cmd);
}

int NCDIfConfig_add_ipv6_addr (const char *ifname, struct ipv6_ifaddr ifaddr)
{
    ASSERT(ifaddr.prefix >= 0)
    ASSERT(ifaddr.prefix <= 128)
    
    if (strlen(ifname) >= IFNAMSIZ) {
        BLog(BLOG_ERROR, "ifname too long");
        return 0;
    }
    
    char addr_str[IPADDR6_PRINT_MAX];
    ipaddr6_print_addr(ifaddr.addr, addr_str);
    
    char cmd[40 + IPADDR6_PRINT_MAX + IFNAMSIZ];
    sprintf(cmd, IP_CMD" addr add %s/%d dev %s", addr_str, ifaddr.prefix, ifname);
    
    return !run_command(cmd);
}

int NCDIfConfig_remove_ipv6_addr (const char *ifname, struct ipv6_ifaddr ifaddr)
{
    ASSERT(ifaddr.prefix >= 0)
    ASSERT(ifaddr.prefix <= 128)
    
    if (strlen(ifname) >= IFNAMSIZ) {
        BLog(BLOG_ERROR, "ifname too long");
        return 0;
    }
    
    char addr_str[IPADDR6_PRINT_MAX];
    ipaddr6_print_addr(ifaddr.addr, addr_str);
    
    char cmd[40 + IPADDR6_PRINT_MAX + IFNAMSIZ];
    sprintf(cmd, IP_CMD" addr del %s/%d dev %s", addr_str, ifaddr.prefix, ifname);
    
    return !run_command(cmd);
}

static int route_cmd (const char *cmdtype, struct ipv4_ifaddr dest, const uint32_t *gateway, int metric, const char *ifname)
{
    ASSERT(!strcmp(cmdtype, "add") || !strcmp(cmdtype, "del"))
    ASSERT(dest.prefix >= 0)
    ASSERT(dest.prefix <= 32)
    
    if (strlen(ifname) >= IFNAMSIZ) {
        BLog(BLOG_ERROR, "ifname too long");
        return 0;
    }
    
    uint8_t *d_addr = (uint8_t *)&dest.addr;
    
    char gwstr[30];
    if (gateway) {
        const uint8_t *g_addr = (uint8_t *)gateway;
        sprintf(gwstr, " via %"PRIu8".%"PRIu8".%"PRIu8".%"PRIu8, g_addr[0], g_addr[1], g_addr[2], g_addr[3]);
    } else {
        gwstr[0] = '\0';
    }
    
    char cmd[120 + IFNAMSIZ];
    sprintf(cmd, IP_CMD" route %s %"PRIu8".%"PRIu8".%"PRIu8".%"PRIu8"/%d%s metric %d dev %s",
            cmdtype, d_addr[0], d_addr[1], d_addr[2], d_addr[3], dest.prefix, gwstr, metric, ifname);
    
    return !run_command(cmd);
}

int NCDIfConfig_add_ipv4_route (struct ipv4_ifaddr dest, const uint32_t *gateway, int metric, const char *device)
{
    return route_cmd("add", dest, gateway, metric, device);
}

int NCDIfConfig_remove_ipv4_route (struct ipv4_ifaddr dest, const uint32_t *gateway, int metric, const char *device)
{
    return route_cmd("del", dest, gateway, metric, device);
}

static int route_cmd6 (const char *cmdtype, struct ipv6_ifaddr dest, const struct ipv6_addr *gateway, int metric, const char *ifname)
{
    ASSERT(!strcmp(cmdtype, "add") || !strcmp(cmdtype, "del"))
    ASSERT(dest.prefix >= 0)
    ASSERT(dest.prefix <= 128)
    
    if (strlen(ifname) >= IFNAMSIZ) {
        BLog(BLOG_ERROR, "ifname too long");
        return 0;
    }
    
    char dest_str[IPADDR6_PRINT_MAX];
    ipaddr6_print_addr(dest.addr, dest_str);
    
    char gwstr[10 + IPADDR6_PRINT_MAX];
    if (gateway) {
        strcpy(gwstr, " via ");
        ipaddr6_print_addr(*gateway, gwstr + strlen(gwstr));
    } else {
        gwstr[0] = '\0';
    }
    
    char cmd[70 + IPADDR6_PRINT_MAX + IPADDR6_PRINT_MAX + IFNAMSIZ];
    sprintf(cmd, IP_CMD" route %s %s/%d%s metric %d dev %s",
            cmdtype, dest_str, dest.prefix, gwstr, metric, ifname);
    
    return !run_command(cmd);
}

int NCDIfConfig_add_ipv6_route (struct ipv6_ifaddr dest, const struct ipv6_addr *gateway, int metric, const char *device)
{
    return route_cmd6("add", dest, gateway, metric, device);
}

int NCDIfConfig_remove_ipv6_route (struct ipv6_ifaddr dest, const struct ipv6_addr *gateway, int metric, const char *device)
{
    return route_cmd6("del", dest, gateway, metric, device);
}

static int blackhole_route_cmd (const char *cmdtype, struct ipv4_ifaddr dest, int metric)
{
    ASSERT(!strcmp(cmdtype, "add") || !strcmp(cmdtype, "del"))
    ASSERT(dest.prefix >= 0)
    ASSERT(dest.prefix <= 32)
    
    uint8_t *d_addr = (uint8_t *)&dest.addr;
    
    char cmd[120];
    sprintf(cmd, IP_CMD" route %s blackhole %"PRIu8".%"PRIu8".%"PRIu8".%"PRIu8"/%d metric %d",
            cmdtype, d_addr[0], d_addr[1], d_addr[2], d_addr[3], dest.prefix, metric);
    
    return !run_command(cmd);
}

int NCDIfConfig_add_ipv4_blackhole_route (struct ipv4_ifaddr dest, int metric)
{
    return blackhole_route_cmd("add", dest, metric);
}

int NCDIfConfig_remove_ipv4_blackhole_route (struct ipv4_ifaddr dest, int metric)
{
    return blackhole_route_cmd("del", dest, metric);
}

static int blackhole_route_cmd6 (const char *cmdtype, struct ipv6_ifaddr dest, int metric)
{
    ASSERT(!strcmp(cmdtype, "add") || !strcmp(cmdtype, "del"))
    ASSERT(dest.prefix >= 0)
    ASSERT(dest.prefix <= 128)
    
    char dest_str[IPADDR6_PRINT_MAX];
    ipaddr6_print_addr(dest.addr, dest_str);
    
    char cmd[70 + IPADDR6_PRINT_MAX];
    sprintf(cmd, IP_CMD" route %s blackhole %s/%d metric %d",
            cmdtype, dest_str, dest.prefix, metric);
    
    return !run_command(cmd);
}

int NCDIfConfig_add_ipv6_blackhole_route (struct ipv6_ifaddr dest, int metric)
{
    return blackhole_route_cmd6("add", dest, metric);
}

int NCDIfConfig_remove_ipv6_blackhole_route (struct ipv6_ifaddr dest, int metric)
{
    return blackhole_route_cmd6("del", dest, metric);
}

int NCDIfConfig_set_resolv_conf (const char *data, size_t data_len)
{
    FILE *temp_file = fopen(RESOLVCONF_TEMP_FILE, "w");
    if (!temp_file) {
        BLog(BLOG_ERROR, "failed to open resolvconf temp file");
        goto fail0;
    }
    
    char line[] = "# generated by badvpn-ncd\n";
    if (!write_to_file((uint8_t *)line, strlen(line), temp_file) ||
        !write_to_file((uint8_t *)data, data_len, temp_file)
    ) {
        BLog(BLOG_ERROR, "failed to write to resolvconf temp file");
        goto fail1;
    }
    
    if (fclose(temp_file) != 0) {
        BLog(BLOG_ERROR, "failed to close resolvconf temp file");
        return 0;
    }
    
    if (rename(RESOLVCONF_TEMP_FILE, RESOLVCONF_FILE) < 0) {
        BLog(BLOG_ERROR, "failed to rename resolvconf temp file to resolvconf file");
        return 0;
    }
    
    return 1;
    
fail1:
    fclose(temp_file);
fail0:
    return 0;
}

static int open_tuntap (const char *ifname, int flags)
{
    if (strlen(ifname) >= IFNAMSIZ) {
        BLog(BLOG_ERROR, "ifname too long");
        return -1;
    }
    
    int fd = open(TUN_DEVNODE, O_RDWR);
    if (fd < 0) {
         BLog(BLOG_ERROR, "open tun failed");
         return -1;
    }
    
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = flags;
    snprintf(ifr.ifr_name, IFNAMSIZ, "%s", ifname);
    
    if (ioctl(fd, TUNSETIFF, (void *)&ifr) < 0) {
        BLog(BLOG_ERROR, "TUNSETIFF failed");
        close(fd);
        return -1;
    }
    
    return fd;
}

int NCDIfConfig_make_tuntap (const char *ifname, const char *owner, int tun)
{
    // load tun module if needed
    if (access(TUN_DEVNODE, F_OK) < 0) {
        if (run_command(MODPROBE_CMD" tun") != 0) {
            BLog(BLOG_ERROR, "modprobe tun failed");
        }
    }
    
    int fd;
    if ((fd = open_tuntap(ifname, (tun ? IFF_TUN : IFF_TAP))) < 0) {
        goto fail0;
    }
    
    if (owner) {
        long bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
        if (bufsize < 0) {
            bufsize = 16384;
        }
        
        char *buf = malloc(bufsize);
        if (!buf) {
            BLog(BLOG_ERROR, "malloc failed");
            goto fail1;
        }
        
        struct passwd pwd;
        struct passwd *res;
        getpwnam_r(owner, &pwd, buf, bufsize, &res);
        if (!res) {
            BLog(BLOG_ERROR, "getpwnam_r failed");
            free(buf);
            goto fail1;
        }
        
        int uid = pwd.pw_uid;
        
        free(buf);
        
        if (ioctl(fd, TUNSETOWNER, uid) < 0) {
            BLog(BLOG_ERROR, "TUNSETOWNER failed");
            goto fail1;
        }
    }
    
    if (ioctl(fd, TUNSETPERSIST, (void *)1) < 0) {
        BLog(BLOG_ERROR, "TUNSETPERSIST failed");
        goto fail1;
    }
    
    close(fd);
    
    return 1;
    
fail1:
    close(fd);
fail0:
    return 0;
}

int NCDIfConfig_remove_tuntap (const char *ifname, int tun)
{
    int fd;
    if ((fd = open_tuntap(ifname, (tun ? IFF_TUN : IFF_TAP))) < 0) {
        goto fail0;
    }
    
    if (ioctl(fd, TUNSETPERSIST, (void *)0) < 0) {
        BLog(BLOG_ERROR, "TUNSETPERSIST failed");
        goto fail1;
    }
    
    close(fd);
    
    return 1;
    
fail1:
    close(fd);
fail0:
    return 0;
}
