// http://kb.mozillazine.org/User.js_file

// The meek-http-helper extension uses dump to write its listening port number
// to stdout.
user_pref("browser.dom.window.dump.enabled", true);

// Enable TLS session tickets (disabled by default in Tor Browser). Otherwise
// there is a missing TLS extension.
// https://trac.torproject.org/projects/tor/ticket/13442#comment:1
user_pref("security.ssl.disable_session_identifiers", false);

// Enable SPDY and HTTP/2 as they are in Firefox 38, for a matching ALPN
// extension.
// https://trac.torproject.org/projects/tor/ticket/15512
user_pref("network.http.spdy.enabled", true);
user_pref("network.http.spdy.enabled.http2", true);
user_pref("network.http.spdy.enabled.http2draft", true);
user_pref("network.http.spdy.enabled.v3-1", true);

// Disable safe mode. In case of a crash, we don't want to prompt for a
// safe-mode browser that has extensions disabled.
// https://support.mozilla.org/en-US/questions/951221#answer-410562
user_pref("toolkit.startup.max_resumed_crashes", -1);

// Don't raise software update windows in this browser instance.
// https://trac.torproject.org/projects/tor/ticket/14203
user_pref("app.update.enabled", false);

// Set a failsafe blackhole proxy of 127.0.0.1:9, to prevent network interaction
// in case the user manages to open this profile with a normal browser UI (i.e.,
// not headless with the meek-http-helper extension running). Port 9 is
// "discard", so it should work as a blackhole whether the port is open or
// closed. network.proxy.type=1 means "Manual proxy configuration".
// http://kb.mozillazine.org/Network.proxy.type
user_pref("network.proxy.type", 1);
user_pref("network.proxy.socks", "127.0.0.1");
user_pref("network.proxy.socks_port", 9);
// Make sure DNS is also blackholed. network.proxy.socks_remote_dns is
// overridden by meek-http-helper at startup.
user_pref("network.proxy.socks_remote_dns", true);

user_pref("extensions.enabledAddons", "meek-http-helper@bamsoftware.com:1.0");

// Ensure that distribution extensions (e.g., Tor Launcher) are not copied
// into the meek-http-helper profile.
user_pref("extensions.installDistroAddons", false);
