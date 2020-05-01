/*
 * acl.h - Define the ACL interface
 *
 * Copyright (C) 2013 - 2019, Max Lv <max.c.lv@gmail.com>
 *
 * This file is part of the shadowsocks-libev.
 *
 * shadowsocks-libev is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * shadowsocks-libev is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with shadowsocks-libev; see the file COPYING. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef _PLUGIN_H
#define _PLUGIN_H

#define PLUGIN_EXIT_ERROR  -2
#define PLUGIN_EXIT_NORMAL -1
#define PLUGIN_RUNNING      0

enum plugin_mode {
    MODE_CLIENT,
    MODE_SERVER
};

/*
 * XXX: Since we have SS plugins and obfsproxy support, for now we will
 *      do extra check against the plugin name.
 *      For obfsproxy, we will not follow the SS specified protocol and
 *      do special routine for obfsproxy.
 *      This may change when the protocol is finally settled
 *
 * Main function to start a plugin.
 *
 * @plugin: name of the plugin
 *          search from PATH and current directory.
 * @plugin_opts: Special options for plugin
 * @remote_host:
 *   CLIENT mode:
 *     The remote server address, which also runs corresponding plugin
 *   SERVER mode:
 *     The real listen address, which plugin will listen to
 * @remote_port:
 *   CLIENT mode:
 *     The remote server port, which corresponding plugin is listening to
 *   SERVER mode:
 *     The real listen port, which plugin will listen to
 * @local_host:
 *   Where ss-libev will connect/listen to.
 *   Normally localhost for both modes.
 * @local_port:
 *   Where ss-libev will connect/listen to.
 *   Internal user port.
 * @mode:
 *   Indicates which mode the plugin should run at.
 */
int start_plugin(const char *plugin,
                 const char *plugin_opts,
                 const char *remote_host,
                 const char *remote_port,
                 const char *local_host,
                 const char *local_port,
#ifdef __MINGW32__
                 uint16_t control_port,
#endif
                 enum plugin_mode mode);
uint16_t get_local_port();
void stop_plugin();
int is_plugin_running();

#endif // _PLUGIN_H
