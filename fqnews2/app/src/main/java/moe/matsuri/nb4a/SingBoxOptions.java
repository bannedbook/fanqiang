package moe.matsuri.nb4a;

import static moe.matsuri.nb4a.utils.JavaUtil.gson;

import com.google.gson.annotations.SerializedName;

import java.util.List;
import java.util.Map;

public class SingBoxOptions {

    // base

    public static class SingBoxOption {
        public Map<String, Object> asMap() {
            return gson.fromJson(gson.toJson(this), Map.class);
        }
    }

    // custom classes

    public static class User {
        public String username;
        public String password;
    }

    public static class MyOptions extends SingBoxOption {
        public LogOptions log;

        public DNSOptions dns;

        public NTPOptions ntp;

        public List<Inbound> inbounds;

        public List<Map<String, Object>> outbounds;

        public RouteOptions route;

        public ExperimentalOptions experimental;

    }

    // paste generate output here

    public static class ClashAPIOptions extends SingBoxOption {

        public String external_controller;

        public String external_ui;

        public String external_ui_download_url;

        public String external_ui_download_detour;

        public String secret;

        public String default_mode;

        // Generate note: option type:  public List<String> ModeList;

    }

    public static class SelectorOutboundOptions extends SingBoxOption {

        public List<String> outbounds;

        @SerializedName("default")
        public String default_;

    }

    public static class URLTestOutboundOptions extends SingBoxOption {

        public List<String> outbounds;

        public String url;

        public Long interval;

        public Integer tolerance;

    }


    public static class Options extends SingBoxOption {

        public String $schema;

        public LogOptions log;

        public DNSOptions dns;

        public NTPOptions ntp;

        public List<Inbound> inbounds;

        public List<Outbound> outbounds;

        public RouteOptions route;

        public ExperimentalOptions experimental;

    }

    public static class LogOptions extends SingBoxOption {

        public Boolean disabled;

        public String level;

        public String output;

        public Boolean timestamp;

        // Generate note: option type:  public Boolean DisableColor;

    }

    public static class DebugOptions extends SingBoxOption {

        public String listen;

        public Integer gc_percent;

        public Integer max_stack;

        public Integer max_threads;

        public Boolean panic_on_fault;

        public String trace_back;

        public Long memory_limit;

        public Boolean oom_killer;

    }


    public static class DirectInboundOptions extends SingBoxOption {

        // Generate note: nested type ListenOptions
        public String listen;

        public Integer listen_port;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;

        // Generate note: option type:  public Boolean UDPFragmentDefault;

        public Long udp_timeout;

        public Boolean proxy_protocol;

        public Boolean proxy_protocol_accept_no_header;

        public String detour;

        // Generate note: nested type InboundOptions
        public Boolean sniff;

        public Boolean sniff_override_destination;

        public Long sniff_timeout;

        public String domain_strategy;

        // End of public InboundOptions ;

        // End of public ListenOptions ;

        public String network;

        public String override_address;

        public Integer override_port;

    }

    public static class DirectOutboundOptions extends SingBoxOption {

        // Generate note: nested type DialerOptions
        public String detour;

        public String bind_interface;

        public String inet4_bind_address;

        public String inet6_bind_address;

        public String protect_path;

        public Integer routing_mark;

        public Boolean reuse_addr;

        public Long connect_timeout;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;

        // Generate note: option type:  public Boolean UDPFragmentDefault;

        public String domain_strategy;

        public Long fallback_delay;

        // End of public DialerOptions ;

        public String override_address;

        public Integer override_port;

        public Integer proxy_protocol;

    }

    public static class DNSOptions extends SingBoxOption {

        public List<DNSServerOptions> servers;

        public List<DNSRule> rules;

        @SerializedName("final")
        public String final_;

        public Boolean reverse_mapping;

        public DNSFakeIPOptions fakeip;

        // Generate note: nested type DNSClientOptions
        public String strategy;

        public Boolean disable_cache;

        public Boolean disable_expire;

        public Boolean independent_cache;

        // End of public DNSClientOptions ;

    }

    public static class DNSServerOptions extends SingBoxOption {

        public String tag;

        public String address;

        public String address_resolver;

        public String address_strategy;

        public Long address_fallback_delay;

        public String strategy;

        public String detour;

    }

    public static class DNSClientOptions extends SingBoxOption {

        public String strategy;

        public Boolean disable_cache;

        public Boolean disable_expire;

        public Boolean independent_cache;

    }

    public static class DNSFakeIPOptions extends SingBoxOption {

        public Boolean enabled;

        public String inet4_range;

        public String inet6_range;

    }

    public static class ExperimentalOptions extends SingBoxOption {

        public ClashAPIOptions clash_api;

        public V2RayAPIOptions v2ray_api;

        public CacheFile cache_file;

        public DebugOptions debug;

    }

    public static class CacheFile extends SingBoxOption {

        public Boolean enabled;

        public Boolean store_fakeip;

        public String path;

        public String cache_id;

    }

    public static class HysteriaInboundOptions extends SingBoxOption {

        // Generate note: nested type ListenOptions
        public String listen;

        public Integer listen_port;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;

        // Generate note: option type:  public Boolean UDPFragmentDefault;

        public Long udp_timeout;

        public Boolean proxy_protocol;

        public Boolean proxy_protocol_accept_no_header;

        public String detour;

        // Generate note: nested type InboundOptions
        public Boolean sniff;

        public Boolean sniff_override_destination;

        public Long sniff_timeout;

        public String domain_strategy;

        // End of public InboundOptions ;

        // End of public ListenOptions ;

        public String up;

        public Integer up_mbps;

        public String down;

        public Integer down_mbps;

        public String obfs;

        public List<HysteriaUser> users;

        public Long recv_window_conn;

        public Long recv_window_client;

        public Integer max_conn_client;

        public Boolean disable_mtu_discovery;

        public InboundTLSOptions tls;

    }

    public static class HysteriaUser extends SingBoxOption {

        public String name;

        // Generate note: Base64 String
        public String auth;

        public String auth_str;

    }

    public static class HysteriaOutboundOptions extends SingBoxOption {

        // Generate note: nested type DialerOptions
        public String detour;

        public String bind_interface;

        public String inet4_bind_address;

        public String inet6_bind_address;

        public String protect_path;

        public Integer routing_mark;

        public Boolean reuse_addr;

        public Long connect_timeout;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;

        // Generate note: option type:  public Boolean UDPFragmentDefault;

        public String domain_strategy;

        public Long fallback_delay;

        // End of public DialerOptions ;

        // Generate note: nested type ServerOptions
        public String server;

        public Integer server_port;

        // End of public ServerOptions ;

        public String up;

        public Integer up_mbps;

        public String down;

        public Integer down_mbps;

        public String obfs;

        // Generate note: Base64 String
        public String auth;

        public String auth_str;

        public Long recv_window_conn;

        public Long recv_window;

        public Boolean disable_mtu_discovery;

        public String network;

        public OutboundTLSOptions tls;

        public String hop_ports;

        public Integer hop_interval;

    }

    public static class Hysteria2InboundOptions extends SingBoxOption {

        // Generate note: nested type ListenOptions
        public String listen;

        public Integer listen_port;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;

        // Generate note: option type:  public Boolean UDPFragmentDefault;

        public Long udp_timeout;

        public Boolean proxy_protocol;

        public Boolean proxy_protocol_accept_no_header;

        public String detour;

        // Generate note: nested type InboundOptions
        public Boolean sniff;

        public Boolean sniff_override_destination;

        public Long sniff_timeout;

        public String domain_strategy;

        // End of public InboundOptions ;

        // End of public ListenOptions ;

        public Integer up_mbps;

        public Integer down_mbps;

        public Hysteria2Obfs obfs;

        public List<Hysteria2User> users;

        public Boolean ignore_client_bandwidth;

        public InboundTLSOptions tls;

        public String masquerade;

    }

    public static class Hysteria2Obfs extends SingBoxOption {

        public String type;

        public String password;

    }

    public static class Hysteria2User extends SingBoxOption {

        public String name;

        public String password;

    }

    public static class Hysteria2OutboundOptions extends SingBoxOption {

        // Generate note: nested type DialerOptions
        public String detour;

        public String bind_interface;

        public String inet4_bind_address;

        public String inet6_bind_address;

        public String protect_path;

        public Integer routing_mark;

        public Boolean reuse_addr;

        public Long connect_timeout;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;

        // Generate note: option type:  public Boolean UDPFragmentDefault;

        public String domain_strategy;

        public Long fallback_delay;

        // End of public DialerOptions ;

        // Generate note: nested type ServerOptions
        public String server;

        public Integer server_port;

        // End of public ServerOptions ;

        public Integer up_mbps;

        public Integer down_mbps;

        public Hysteria2Obfs obfs;

        public String password;

        public String network;

        public OutboundTLSOptions tls;

        public String hop_ports;

        public Integer hop_interval;

    }


    public static class Inbound extends SingBoxOption {

        public String type;

        public String tag;

        // Generate note: option type:  public TunInboundOptions TunOptions;

        // Generate note: option type:  public RedirectInboundOptions RedirectOptions;

        // Generate note: option type:  public TProxyInboundOptions TProxyOptions;

        // Generate note: option type:  public DirectInboundOptions DirectOptions;

        // Generate note: option type:  public SocksInboundOptions SocksOptions;

        // Generate note: option type:  public HTTPMixedInboundOptions HTTPOptions;

        // Generate note: option type:  public HTTPMixedInboundOptions MixedOptions;

        // Generate note: option type:  public ShadowsocksInboundOptions ShadowsocksOptions;

        // Generate note: option type:  public VMessInboundOptions VMessOptions;

        // Generate note: option type:  public TrojanInboundOptions TrojanOptions;

        // Generate note: option type:  public NaiveInboundOptions NaiveOptions;

        // Generate note: option type:  public HysteriaInboundOptions HysteriaOptions;

        // Generate note: option type:  public ShadowTLSInboundOptions ShadowTLSOptions;

        // Generate note: option type:  public VLESSInboundOptions VLESSOptions;

        // Generate note: option type:  public TUICInboundOptions TUICOptions;

        // Generate note: option type:  public Hysteria2InboundOptions Hysteria2Options;

    }

    public static class InboundOptions extends SingBoxOption {

        public Boolean sniff;

        public Boolean sniff_override_destination;

        public Long sniff_timeout;

        public String domain_strategy;

    }

    public static class ListenOptions extends SingBoxOption {

        public String listen;

        public Integer listen_port;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;

        // Generate note: option type:  public Boolean UDPFragmentDefault;

        public Long udp_timeout;

        public Boolean proxy_protocol;

        public Boolean proxy_protocol_accept_no_header;

        public String detour;

        // Generate note: nested type InboundOptions
        public Boolean sniff;

        public Boolean sniff_override_destination;

        public Long sniff_timeout;

        public String domain_strategy;

        // End of public InboundOptions ;

    }

    public static class NaiveInboundOptions extends SingBoxOption {

        // Generate note: nested type ListenOptions
        public String listen;

        public Integer listen_port;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;

        // Generate note: option type:  public Boolean UDPFragmentDefault;

        public Long udp_timeout;

        public Boolean proxy_protocol;

        public Boolean proxy_protocol_accept_no_header;

        public String detour;

        // Generate note: nested type InboundOptions
        public Boolean sniff;

        public Boolean sniff_override_destination;

        public Long sniff_timeout;

        public String domain_strategy;

        // End of public InboundOptions ;

        // End of public ListenOptions ;

        public List<User> users;

        public String network;

        public InboundTLSOptions tls;

    }

    public static class NTPOptions extends SingBoxOption {

        public Boolean enabled;

        public Long interval;

        public Boolean write_to_system;

        // Generate note: nested type ServerOptions
        public String server;

        public Integer server_port;

        // End of public ServerOptions ;

        // Generate note: nested type DialerOptions
        public String detour;

        public String bind_interface;

        public String inet4_bind_address;

        public String inet6_bind_address;

        public String protect_path;

        public Integer routing_mark;

        public Boolean reuse_addr;

        public Long connect_timeout;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;

        // Generate note: option type:  public Boolean UDPFragmentDefault;

        public String domain_strategy;

        public Long fallback_delay;

        // End of public DialerOptions ;

    }


    public static class Outbound extends SingBoxOption {

        public String type;

        public String tag;

        // Generate note: option type:  public DirectOutboundOptions DirectOptions;

        // Generate note: option type:  public SocksOutboundOptions SocksOptions;

        // Generate note: option type:  public HTTPOutboundOptions HTTPOptions;

        // Generate note: option type:  public ShadowsocksOutboundOptions ShadowsocksOptions;

        // Generate note: option type:  public VMessOutboundOptions VMessOptions;

        // Generate note: option type:  public TrojanOutboundOptions TrojanOptions;

        // Generate note: option type:  public WireGuardOutboundOptions WireGuardOptions;

        // Generate note: option type:  public HysteriaOutboundOptions HysteriaOptions;

        // Generate note: option type:  public TorOutboundOptions TorOptions;

        // Generate note: option type:  public SSHOutboundOptions SSHOptions;

        // Generate note: option type:  public ShadowTLSOutboundOptions ShadowTLSOptions;

        // Generate note: option type:  public ShadowsocksROutboundOptions ShadowsocksROptions;

        // Generate note: option type:  public VLESSOutboundOptions VLESSOptions;

        // Generate note: option type:  public TUICOutboundOptions TUICOptions;

        // Generate note: option type:  public Hysteria2OutboundOptions Hysteria2Options;

        // Generate note: option type:  public SelectorOutboundOptions SelectorOptions;

        // Generate note: option type:  public URLTestOutboundOptions URLTestOptions;

    }

    public static class DialerOptions extends SingBoxOption {

        public String detour;

        public String bind_interface;

        public String inet4_bind_address;

        public String inet6_bind_address;

        public String protect_path;

        public Integer routing_mark;

        public Boolean reuse_addr;

        public Long connect_timeout;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;

        // Generate note: option type:  public Boolean UDPFragmentDefault;

        public String domain_strategy;

        public Long fallback_delay;

    }

    public static class ServerOptions extends SingBoxOption {

        public String server;

        public Integer server_port;

    }

    public static class MultiplexOptions extends SingBoxOption {

        public Boolean enabled;

        public String protocol;

        public Integer max_connections;

        public Integer min_streams;

        public Integer max_streams;

        public Boolean padding;

    }

    public static class OnDemandOptions extends SingBoxOption {

        public Boolean enabled;

        public List<OnDemandRule> rules;

    }

    public static class OnDemandRule extends SingBoxOption {

        public String action;

        // Generate note: Listable
        public List<String> dns_search_domain_match;

        // Generate note: Listable
        public List<String> dns_server_address_match;

        public String interface_type_match;

        // Generate note: Listable
        public List<String> ssid_match;

        public String probe_url;

    }


    public static class RedirectInboundOptions extends SingBoxOption {

        // Generate note: nested type ListenOptions
        public String listen;

        public Integer listen_port;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;

        // Generate note: option type:  public Boolean UDPFragmentDefault;

        public Long udp_timeout;

        public Boolean proxy_protocol;

        public Boolean proxy_protocol_accept_no_header;

        public String detour;

        // Generate note: nested type InboundOptions
        public Boolean sniff;

        public Boolean sniff_override_destination;

        public Long sniff_timeout;

        public String domain_strategy;

        // End of public InboundOptions ;

        // End of public ListenOptions ;

    }

    public static class TProxyInboundOptions extends SingBoxOption {

        // Generate note: nested type ListenOptions
        public String listen;

        public Integer listen_port;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;

        // Generate note: option type:  public Boolean UDPFragmentDefault;

        public Long udp_timeout;

        public Boolean proxy_protocol;

        public Boolean proxy_protocol_accept_no_header;

        public String detour;

        // Generate note: nested type InboundOptions
        public Boolean sniff;

        public Boolean sniff_override_destination;

        public Long sniff_timeout;

        public String domain_strategy;

        // End of public InboundOptions ;

        // End of public ListenOptions ;

        public String network;

    }

    public static class RouteOptions extends SingBoxOption {

        public List<Rule> rules;

        public List<RuleSet> rule_set;

        @SerializedName("final")
        public String final_;

        public Boolean find_process;

        public Boolean auto_detect_interface;

        public Boolean override_android_vpn;

        public String default_interface;

        public Integer default_mark;

    }


    public static class Rule extends SingBoxOption {

        public String type;

        // Generate note: option type:  public DefaultRule DefaultOptions;

        // Generate note: option type:  public LogicalRule LogicalOptions;

    }

    public static class RuleSet extends SingBoxOption {

        public String type;

        public String tag;

        public String format;

        public String path;

        public String url;

    }

    public static class DefaultRule extends SingBoxOption {

        // Generate note: Listable
        public List<String> inbound;

        public Integer ip_version;

        // Generate note: Listable
        public List<String> network;

        // Generate note: Listable
        public List<String> auth_user;

        // Generate note: Listable
        public List<String> protocol;

        // Generate note: Listable
        public List<String> domain;

        // Generate note: Listable
        public List<String> domain_suffix;

        // Generate note: Listable
        public List<String> domain_keyword;

        // Generate note: Listable
        public List<String> domain_regex;

        // Generate note: Listable
        public List<String> source_ip_cidr;

        // Generate note: Listable
        public List<String> ip_cidr;

        // Generate note: Listable
        public List<Integer> source_port;

        // Generate note: Listable
        public List<String> source_port_range;

        // Generate note: Listable
        public List<Integer> port;

        // Generate note: Listable
        public List<String> port_range;

        // Generate note: Listable
        public List<String> process_name;

        // Generate note: Listable
        public List<String> process_path;

        // Generate note: Listable
        public List<String> package_name;

        // Generate note: Listable
        public List<String> user;

        // Generate note: Listable
        public List<Integer> user_id;

        public String clash_mode;

        public Boolean invert;

        public String outbound;

    }


    public static class DNSRule extends SingBoxOption {

        public String type;

        // Generate note: option type:  public DefaultDNSRule DefaultOptions;

        // Generate note: option type:  public LogicalDNSRule LogicalOptions;

    }

    public static class DefaultDNSRule extends SingBoxOption {

        // Generate note: Listable
        public List<String> inbound;

        public Integer ip_version;

        // Generate note: Listable
        public List<String> query_type;

        // Generate note: Listable
        public List<String> network;

        // Generate note: Listable
        public List<String> auth_user;

        // Generate note: Listable
        public List<String> protocol;

        // Generate note: Listable
        public List<String> domain;

        // Generate note: Listable
        public List<String> domain_suffix;

        // Generate note: Listable
        public List<String> domain_keyword;

        // Generate note: Listable
        public List<String> domain_regex;

        // Generate note: Listable
        public List<String> geosite;

        // Generate note: Listable
        public List<String> source_ip_cidr;

        // Generate note: Listable
        public List<Integer> source_port;

        // Generate note: Listable
        public List<String> source_port_range;

        // Generate note: Listable
        public List<Integer> port;

        // Generate note: Listable
        public List<String> port_range;

        // Generate note: Listable
        public List<String> process_name;

        // Generate note: Listable
        public List<String> process_path;

        // Generate note: Listable
        public List<String> package_name;

        // Generate note: Listable
        public List<String> user;

        // Generate note: Listable
        public List<Integer> user_id;

        // Generate note: Listable
        public List<String> outbound;

        public String clash_mode;

        public Boolean invert;

        public String server;

        public Boolean disable_cache;

        public Integer rewrite_ttl;

    }

    public static class ShadowsocksInboundOptions extends SingBoxOption {

        // Generate note: nested type ListenOptions
        public String listen;

        public Integer listen_port;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;

        // Generate note: option type:  public Boolean UDPFragmentDefault;

        public Long udp_timeout;

        public Boolean proxy_protocol;

        public Boolean proxy_protocol_accept_no_header;

        public String detour;

        // Generate note: nested type InboundOptions
        public Boolean sniff;

        public Boolean sniff_override_destination;

        public Long sniff_timeout;

        public String domain_strategy;

        // End of public InboundOptions ;

        // End of public ListenOptions ;

        public String network;

        public String method;

        public String password;

        public List<ShadowsocksUser> users;

        public List<ShadowsocksDestination> destinations;

    }

    public static class ShadowsocksUser extends SingBoxOption {

        public String name;

        public String password;

    }

    public static class ShadowsocksDestination extends SingBoxOption {

        public String name;

        public String password;

        // Generate note: nested type ServerOptions
        public String server;

        public Integer server_port;

        // End of public ServerOptions ;

    }

    public static class ShadowsocksOutboundOptions extends SingBoxOption {

        // Generate note: nested type DialerOptions
        public String detour;

        public String bind_interface;

        public String inet4_bind_address;

        public String inet6_bind_address;

        public String protect_path;

        public Integer routing_mark;

        public Boolean reuse_addr;

        public Long connect_timeout;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;

        // Generate note: option type:  public Boolean UDPFragmentDefault;

        public String domain_strategy;

        public Long fallback_delay;

        // End of public DialerOptions ;

        // Generate note: nested type ServerOptions
        public String server;

        public Integer server_port;

        // End of public ServerOptions ;

        public String method;

        public String password;

        public String plugin;

        public String plugin_opts;

        public String network;

        public UDPOverTCPOptions udp_over_tcp;

        public MultiplexOptions multiplex;

    }

    public static class ShadowsocksROutboundOptions extends SingBoxOption {

        // Generate note: nested type DialerOptions
        public String detour;

        public String bind_interface;

        public String inet4_bind_address;

        public String inet6_bind_address;

        public String protect_path;

        public Integer routing_mark;

        public Boolean reuse_addr;

        public Long connect_timeout;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;

        // Generate note: option type:  public Boolean UDPFragmentDefault;

        public String domain_strategy;

        public Long fallback_delay;

        // End of public DialerOptions ;

        // Generate note: nested type ServerOptions
        public String server;

        public Integer server_port;

        // End of public ServerOptions ;

        public String method;

        public String password;

        public String obfs;

        public String obfs_param;

        public String protocol;

        public String protocol_param;

        public String network;

    }

    public static class ShadowTLSInboundOptions extends SingBoxOption {

        // Generate note: nested type ListenOptions
        public String listen;

        public Integer listen_port;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;

        // Generate note: option type:  public Boolean UDPFragmentDefault;

        public Long udp_timeout;

        public Boolean proxy_protocol;

        public Boolean proxy_protocol_accept_no_header;

        public String detour;

        // Generate note: nested type InboundOptions
        public Boolean sniff;

        public Boolean sniff_override_destination;

        public Long sniff_timeout;

        public String domain_strategy;

        // End of public InboundOptions ;

        // End of public ListenOptions ;

        public Integer version;

        public String password;

        public List<ShadowTLSUser> users;

        public ShadowTLSHandshakeOptions handshake;

        public Map<String, ShadowTLSHandshakeOptions> handshake_for_server_name;

        public Boolean strict_mode;

    }

    public static class ShadowTLSUser extends SingBoxOption {

        public String name;

        public String password;

    }

    public static class ShadowTLSHandshakeOptions extends SingBoxOption {

        // Generate note: nested type ServerOptions
        public String server;

        public Integer server_port;

        // End of public ServerOptions ;

        // Generate note: nested type DialerOptions
        public String detour;

        public String bind_interface;

        public String inet4_bind_address;

        public String inet6_bind_address;

        public String protect_path;

        public Integer routing_mark;

        public Boolean reuse_addr;

        public Long connect_timeout;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;

        // Generate note: option type:  public Boolean UDPFragmentDefault;

        public String domain_strategy;

        public Long fallback_delay;

        // End of public DialerOptions ;

    }

    public static class ShadowTLSOutboundOptions extends SingBoxOption {

        // Generate note: nested type DialerOptions
        public String detour;

        public String bind_interface;

        public String inet4_bind_address;

        public String inet6_bind_address;

        public String protect_path;

        public Integer routing_mark;

        public Boolean reuse_addr;

        public Long connect_timeout;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;

        // Generate note: option type:  public Boolean UDPFragmentDefault;

        public String domain_strategy;

        public Long fallback_delay;

        // End of public DialerOptions ;

        // Generate note: nested type ServerOptions
        public String server;

        public Integer server_port;

        // End of public ServerOptions ;

        public Integer version;

        public String password;

        public OutboundTLSOptions tls;

    }

    public static class SocksInboundOptions extends SingBoxOption {

        // Generate note: nested type ListenOptions
        public String listen;

        public Integer listen_port;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;

        // Generate note: option type:  public Boolean UDPFragmentDefault;

        public Long udp_timeout;

        public Boolean proxy_protocol;

        public Boolean proxy_protocol_accept_no_header;

        public String detour;

        // Generate note: nested type InboundOptions
        public Boolean sniff;

        public Boolean sniff_override_destination;

        public Long sniff_timeout;

        public String domain_strategy;

        // End of public InboundOptions ;

        // End of public ListenOptions ;

        public List<User> users;

    }

    public static class HTTPMixedInboundOptions extends SingBoxOption {

        // Generate note: nested type ListenOptions
        public String listen;

        public Integer listen_port;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;

        // Generate note: option type:  public Boolean UDPFragmentDefault;

        public Long udp_timeout;

        public Boolean proxy_protocol;

        public Boolean proxy_protocol_accept_no_header;

        public String detour;

        // Generate note: nested type InboundOptions
        public Boolean sniff;

        public Boolean sniff_override_destination;

        public Long sniff_timeout;

        public String domain_strategy;

        // End of public InboundOptions ;

        // End of public ListenOptions ;

        public List<User> users;

        public Boolean set_system_proxy;

        public InboundTLSOptions tls;

    }

    public static class SocksOutboundOptions extends SingBoxOption {

        // Generate note: nested type DialerOptions
        public String detour;

        public String bind_interface;

        public String inet4_bind_address;

        public String inet6_bind_address;

        public String protect_path;

        public Integer routing_mark;

        public Boolean reuse_addr;

        public Long connect_timeout;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;

        // Generate note: option type:  public Boolean UDPFragmentDefault;

        public String domain_strategy;

        public Long fallback_delay;

        // End of public DialerOptions ;

        // Generate note: nested type ServerOptions
        public String server;

        public Integer server_port;

        // End of public ServerOptions ;

        public String version;

        public String username;

        public String password;

        public String network;

        public UDPOverTCPOptions udp_over_tcp;

    }

    public static class HTTPOutboundOptions extends SingBoxOption {

        // Generate note: nested type DialerOptions
        public String detour;

        public String bind_interface;

        public String inet4_bind_address;

        public String inet6_bind_address;

        public String protect_path;

        public Integer routing_mark;

        public Boolean reuse_addr;

        public Long connect_timeout;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;

        // Generate note: option type:  public Boolean UDPFragmentDefault;

        public String domain_strategy;

        public Long fallback_delay;

        // End of public DialerOptions ;

        // Generate note: nested type ServerOptions
        public String server;

        public Integer server_port;

        // End of public ServerOptions ;

        public String username;

        public String password;

        public OutboundTLSOptions tls;

        public String path;

        public Map<String, String> headers;

    }

    public static class SSHOutboundOptions extends SingBoxOption {

        // Generate note: nested type DialerOptions
        public String detour;

        public String bind_interface;

        public String inet4_bind_address;

        public String inet6_bind_address;

        public String protect_path;

        public Integer routing_mark;

        public Boolean reuse_addr;

        public Long connect_timeout;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;

        // Generate note: option type:  public Boolean UDPFragmentDefault;

        public String domain_strategy;

        public Long fallback_delay;

        // End of public DialerOptions ;

        // Generate note: nested type ServerOptions
        public String server;

        public Integer server_port;

        // End of public ServerOptions ;

        public String user;

        public String password;

        public String private_key;

        public String private_key_path;

        public String private_key_passphrase;

        // Generate note: Listable
        public List<String> host_key;

        // Generate note: Listable
        public List<String> host_key_algorithms;

        public String client_version;

    }

    public static class InboundTLSOptions extends SingBoxOption {

        public Boolean enabled;

        public String server_name;

        public Boolean insecure;

        // Generate note: Listable
        public List<String> alpn;

        public String min_version;

        public String max_version;

        // Generate note: Listable
        public List<String> cipher_suites;

        // Generate note: Listable
        public List<String> certificate;

        public String certificate_path;

        // Generate note: Listable
        public List<String> key;

        public String key_path;

        public InboundACMEOptions acme;

        public InboundECHOptions ech;

        public InboundRealityOptions reality;

    }

    public static class OutboundTLSOptions extends SingBoxOption {

        public Boolean enabled;

        public Boolean disable_sni;

        public String server_name;

        public Boolean insecure;

        // Generate note: Listable
        public List<String> alpn;

        public String min_version;

        public String max_version;

        // Generate note: Listable
        public List<String> cipher_suites;

        public String certificate;

        public String certificate_path;

        public OutboundECHOptions ech;

        public OutboundUTLSOptions utls;

        public OutboundRealityOptions reality;

    }

    public static class InboundRealityOptions extends SingBoxOption {

        public Boolean enabled;

        public InboundRealityHandshakeOptions handshake;

        public String private_key;

        // Generate note: Listable
        public List<String> short_id;

        public Long max_time_difference;

    }

    public static class InboundRealityHandshakeOptions extends SingBoxOption {

        // Generate note: nested type ServerOptions
        public String server;

        public Integer server_port;

        // End of public ServerOptions ;

        // Generate note: nested type DialerOptions
        public String detour;

        public String bind_interface;

        public String inet4_bind_address;

        public String inet6_bind_address;

        public String protect_path;

        public Integer routing_mark;

        public Boolean reuse_addr;

        public Long connect_timeout;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;

        // Generate note: option type:  public Boolean UDPFragmentDefault;

        public String domain_strategy;

        public Long fallback_delay;

        // End of public DialerOptions ;

    }

    public static class InboundECHOptions extends SingBoxOption {

        public Boolean enabled;

        public Boolean pq_signature_schemes_enabled;

        public Boolean dynamic_record_sizing_disabled;

        // Generate note: Listable
        public List<String> key;

        public String key_path;

    }

    public static class OutboundECHOptions extends SingBoxOption {

        public Boolean enabled;

        public Boolean pq_signature_schemes_enabled;

        public Boolean dynamic_record_sizing_disabled;

        // Generate note: Listable
        public List<String> config;

        public String config_path;

    }

    public static class OutboundUTLSOptions extends SingBoxOption {

        public Boolean enabled;

        public String fingerprint;

    }

    public static class OutboundRealityOptions extends SingBoxOption {

        public Boolean enabled;

        public String public_key;

        public String short_id;

    }

    public static class InboundACMEOptions extends SingBoxOption {

        // Generate note: Listable
        public List<String> domain;

        public String data_directory;

        public String default_server_name;

        public String email;

        public String provider;

        public Boolean disable_http_challenge;

        public Boolean disable_tls_alpn_challenge;

        public Integer alternative_http_port;

        public Integer alternative_tls_port;

        public ACMEExternalAccountOptions external_account;

    }

    public static class ACMEExternalAccountOptions extends SingBoxOption {

        public String key_id;

        public String mac_key;

    }

    public static class TorOutboundOptions extends SingBoxOption {

        // Generate note: nested type DialerOptions
        public String detour;

        public String bind_interface;

        public String inet4_bind_address;

        public String inet6_bind_address;

        public String protect_path;

        public Integer routing_mark;

        public Boolean reuse_addr;

        public Long connect_timeout;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;

        // Generate note: option type:  public Boolean UDPFragmentDefault;

        public String domain_strategy;

        public Long fallback_delay;

        // End of public DialerOptions ;

        public String executable_path;

        public List<String> extra_args;

        public String data_directory;

        public Map<String, String> torrc;

    }

    public static class TrojanInboundOptions extends SingBoxOption {

        // Generate note: nested type ListenOptions
        public String listen;

        public Integer listen_port;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;

        // Generate note: option type:  public Boolean UDPFragmentDefault;

        public Long udp_timeout;

        public Boolean proxy_protocol;

        public Boolean proxy_protocol_accept_no_header;

        public String detour;

        // Generate note: nested type InboundOptions
        public Boolean sniff;

        public Boolean sniff_override_destination;

        public Long sniff_timeout;

        public String domain_strategy;

        // End of public InboundOptions ;

        // End of public ListenOptions ;

        public List<TrojanUser> users;

        public InboundTLSOptions tls;

        public ServerOptions fallback;

        public Map<String, ServerOptions> fallback_for_alpn;

        public V2RayTransportOptions transport;

    }

    public static class TrojanUser extends SingBoxOption {

        public String name;

        public String password;

    }

    public static class TrojanOutboundOptions extends SingBoxOption {

        // Generate note: nested type DialerOptions
        public String detour;

        public String bind_interface;

        public String inet4_bind_address;

        public String inet6_bind_address;

        public String protect_path;

        public Integer routing_mark;

        public Boolean reuse_addr;

        public Long connect_timeout;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;

        // Generate note: option type:  public Boolean UDPFragmentDefault;

        public String domain_strategy;

        public Long fallback_delay;

        // End of public DialerOptions ;

        // Generate note: nested type ServerOptions
        public String server;

        public Integer server_port;

        // End of public ServerOptions ;

        public String password;

        public String network;

        public OutboundTLSOptions tls;

        public MultiplexOptions multiplex;

        public V2RayTransportOptions transport;

    }

    public static class TUICInboundOptions extends SingBoxOption {

        // Generate note: nested type ListenOptions
        public String listen;

        public Integer listen_port;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;

        // Generate note: option type:  public Boolean UDPFragmentDefault;

        public Long udp_timeout;

        public Boolean proxy_protocol;

        public Boolean proxy_protocol_accept_no_header;

        public String detour;

        // Generate note: nested type InboundOptions
        public Boolean sniff;

        public Boolean sniff_override_destination;

        public Long sniff_timeout;

        public String domain_strategy;

        // End of public InboundOptions ;

        // End of public ListenOptions ;

        public List<TUICUser> users;

        public String congestion_control;

        public Long auth_timeout;

        public Boolean zero_rtt_handshake;

        public Long heartbeat;

        public InboundTLSOptions tls;

    }

    public static class TUICUser extends SingBoxOption {

        public String name;

        public String uuid;

        public String password;

    }

    public static class TUICOutboundOptions extends SingBoxOption {

        // Generate note: nested type DialerOptions
        public String detour;

        public String bind_interface;

        public String inet4_bind_address;

        public String inet6_bind_address;

        public String protect_path;

        public Integer routing_mark;

        public Boolean reuse_addr;

        public Long connect_timeout;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;

        // Generate note: option type:  public Boolean UDPFragmentDefault;

        public String domain_strategy;

        public Long fallback_delay;

        // End of public DialerOptions ;

        // Generate note: nested type ServerOptions
        public String server;

        public Integer server_port;

        // End of public ServerOptions ;

        public String uuid;

        public String password;

        public String congestion_control;

        public String udp_relay_mode;

        public Boolean udp_over_stream;

        public Boolean zero_rtt_handshake;

        public Long heartbeat;

        public String network;

        public OutboundTLSOptions tls;

    }

    public static class TunInboundOptions extends SingBoxOption {

        public String interface_name;

        public Integer mtu;

        // Generate note: Listable
        public List<String> inet4_address;

        // Generate note: Listable
        public List<String> inet6_address;

        public Boolean auto_route;

        public Boolean strict_route;

        // Generate note: Listable
        public List<String> inet4_route_address;

        // Generate note: Listable
        public List<String> inet6_route_address;

        // Generate note: Listable
        public List<String> include_interface;

        // Generate note: Listable
        public List<String> exclude_interface;

        // Generate note: Listable
        public List<Integer> include_uid;

        // Generate note: Listable
        public List<String> include_uid_range;

        // Generate note: Listable
        public List<Integer> exclude_uid;

        // Generate note: Listable
        public List<String> exclude_uid_range;

        // Generate note: Listable
        public List<Integer> include_android_user;

        // Generate note: Listable
        public List<String> include_package;

        // Generate note: Listable
        public List<String> exclude_package;

        public Boolean endpoint_independent_nat;

        public Long udp_timeout;

        public String stack;

        public TunPlatformOptions platform;

        // Generate note: nested type InboundOptions
        public Boolean sniff;

        public Boolean sniff_override_destination;

        public Long sniff_timeout;

        public String domain_strategy;

        // End of public InboundOptions ;

    }

    public static class TunPlatformOptions extends SingBoxOption {

        public HTTPProxyOptions http_proxy;

    }

    public static class HTTPProxyOptions extends SingBoxOption {

        public Boolean enabled;

        // Generate note: nested type ServerOptions
        public String server;

        public Integer server_port;

        // End of public ServerOptions ;

    }


    public static class UDPOverTCPOptions extends SingBoxOption {

        public Boolean enabled;

        public Integer version;

    }

    public static class V2RayAPIOptions extends SingBoxOption {

        public String listen;

        public V2RayStatsServiceOptions stats;

    }

    public static class V2RayStatsServiceOptions extends SingBoxOption {

        public Boolean enabled;

        public List<String> inbounds;

        public List<String> outbounds;

        public List<String> users;

    }


    public static class V2RayTransportOptions extends SingBoxOption {

        public String type;

        // Generate note: option type:  public V2RayHTTPOptions HTTPOptions;

        // Generate note: option type:  public V2RayWebsocketOptions WebsocketOptions;

        // Generate note: option type:  public V2RayQUICOptions QUICOptions;

        // Generate note: option type:  public V2RayGRPCOptions GRPCOptions;

    }

    public static class V2RayHTTPOptions extends SingBoxOption {

        // Generate note: Listable
        public List<String> host;

        public String path;

        public String method;

        public Map<String, String> headers;

        public Long idle_timeout;

        public Long ping_timeout;

    }

    public static class V2RayWebsocketOptions extends SingBoxOption {

        public String path;

        public Map<String, String> headers;

        public Integer max_early_data;

        public String early_data_header_name;

    }


    public static class V2RayGRPCOptions extends SingBoxOption {

        public String service_name;

        public Long idle_timeout;

        public Long ping_timeout;

        public Boolean permit_without_stream;

        // Generate note: option type:  public Boolean ForceLite;

    }

    public static class V2RayHTTPUpgradeOptions extends SingBoxOption {

        public String host;

        public String path;

        public Map<String, String> headers;

    }

    public static class VLESSInboundOptions extends SingBoxOption {

        // Generate note: nested type ListenOptions
        public String listen;

        public Integer listen_port;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;

        // Generate note: option type:  public Boolean UDPFragmentDefault;

        public Long udp_timeout;

        public Boolean proxy_protocol;

        public Boolean proxy_protocol_accept_no_header;

        public String detour;

        // Generate note: nested type InboundOptions
        public Boolean sniff;

        public Boolean sniff_override_destination;

        public Long sniff_timeout;

        public String domain_strategy;

        // End of public InboundOptions ;

        // End of public ListenOptions ;

        public List<VLESSUser> users;

        public InboundTLSOptions tls;

        public V2RayTransportOptions transport;

    }

    public static class VLESSUser extends SingBoxOption {

        public String name;

        public String uuid;

        public String flow;

    }

    public static class VLESSOutboundOptions extends SingBoxOption {

        // Generate note: nested type DialerOptions
        public String detour;

        public String bind_interface;

        public String inet4_bind_address;

        public String inet6_bind_address;

        public String protect_path;

        public Integer routing_mark;

        public Boolean reuse_addr;

        public Long connect_timeout;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;

        // Generate note: option type:  public Boolean UDPFragmentDefault;

        public String domain_strategy;

        public Long fallback_delay;

        // End of public DialerOptions ;

        // Generate note: nested type ServerOptions
        public String server;

        public Integer server_port;

        // End of public ServerOptions ;

        public String uuid;

        public String flow;

        public String network;

        public OutboundTLSOptions tls;

        public MultiplexOptions multiplex;

        public V2RayTransportOptions transport;

        public String packet_encoding;

    }

    public static class VMessInboundOptions extends SingBoxOption {

        // Generate note: nested type ListenOptions
        public String listen;

        public Integer listen_port;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;

        // Generate note: option type:  public Boolean UDPFragmentDefault;

        public Long udp_timeout;

        public Boolean proxy_protocol;

        public Boolean proxy_protocol_accept_no_header;

        public String detour;

        // Generate note: nested type InboundOptions
        public Boolean sniff;

        public Boolean sniff_override_destination;

        public Long sniff_timeout;

        public String domain_strategy;

        // End of public InboundOptions ;

        // End of public ListenOptions ;

        public List<VMessUser> users;

        public InboundTLSOptions tls;

        public V2RayTransportOptions transport;

    }

    public static class VMessUser extends SingBoxOption {

        public String name;

        public String uuid;

        public Integer alterId;

    }

    public static class VMessOutboundOptions extends SingBoxOption {

        // Generate note: nested type DialerOptions
        public String detour;

        public String bind_interface;

        public String inet4_bind_address;

        public String inet6_bind_address;

        public String protect_path;

        public Integer routing_mark;

        public Boolean reuse_addr;

        public Long connect_timeout;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;

        // Generate note: option type:  public Boolean UDPFragmentDefault;

        public String domain_strategy;

        public Long fallback_delay;

        // End of public DialerOptions ;

        // Generate note: nested type ServerOptions
        public String server;

        public Integer server_port;

        // End of public ServerOptions ;

        public String uuid;

        public String security;

        public Integer alter_id;

        public Boolean global_padding;

        public Boolean authenticated_length;

        public String network;

        public OutboundTLSOptions tls;

        public String packet_encoding;

        public MultiplexOptions multiplex;

        public V2RayTransportOptions transport;

    }

    public static class WireGuardOutboundOptions extends SingBoxOption {

        // Generate note: nested type DialerOptions
        public String detour;

        public String bind_interface;

        public String inet4_bind_address;

        public String inet6_bind_address;

        public String protect_path;

        public Integer routing_mark;

        public Boolean reuse_addr;

        public Long connect_timeout;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;

        // Generate note: option type:  public Boolean UDPFragmentDefault;

        public String domain_strategy;

        public Long fallback_delay;

        // End of public DialerOptions ;

        public Boolean system_interface;

        public String interface_name;

        // Generate note: Listable
        public List<String> local_address;

        public String private_key;

        public List<WireGuardPeer> peers;

        // Generate note: nested type ServerOptions
        public String server;

        public Integer server_port;

        // End of public ServerOptions ;

        public String peer_public_key;

        public String pre_shared_key;

        // Generate note: Base64 String
        public String reserved;

        public Integer workers;

        public Integer mtu;

        public String network;

    }

    public static class WireGuardPeer extends SingBoxOption {

        // Generate note: nested type ServerOptions
        public String server;

        public Integer server_port;

        // End of public ServerOptions ;

        public String public_key;

        public String pre_shared_key;

        // Generate note: Listable
        public List<String> allowed_ips;

        // Generate note: Base64 String
        public String reserved;

    }

    public static class Inbound_TunOptions extends Inbound {

        public String interface_name;

        public Integer mtu;

        // Generate note: Listable
        public List<String> inet4_address;

        // Generate note: Listable
        public List<String> inet6_address;

        public Boolean auto_route;

        public Boolean strict_route;

        // Generate note: Listable
        public List<String> inet4_route_address;

        // Generate note: Listable
        public List<String> inet6_route_address;

        // Generate note: Listable
        public List<String> include_interface;

        // Generate note: Listable
        public List<String> exclude_interface;

        // Generate note: Listable
        public List<Integer> include_uid;

        // Generate note: Listable
        public List<String> include_uid_range;

        // Generate note: Listable
        public List<Integer> exclude_uid;

        // Generate note: Listable
        public List<String> exclude_uid_range;

        // Generate note: Listable
        public List<Integer> include_android_user;

        // Generate note: Listable
        public List<String> include_package;

        // Generate note: Listable
        public List<String> exclude_package;

        public Boolean endpoint_independent_nat;

        public Long udp_timeout;

        public String stack;

        public TunPlatformOptions platform;

        // Generate note: nested type InboundOptions
        public Boolean sniff;

        public Boolean sniff_override_destination;

        public Long sniff_timeout;

        public String domain_strategy;

        // End of public InboundOptions ;

    }

    public static class Inbound_RedirectOptions extends Inbound {

        // Generate note: nested type ListenOptions
        public String listen;

        public Integer listen_port;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;


        public Long udp_timeout;

        public Boolean proxy_protocol;

        public Boolean proxy_protocol_accept_no_header;

        public String detour;

        // Generate note: nested type InboundOptions
        public Boolean sniff;

        public Boolean sniff_override_destination;

        public Long sniff_timeout;

        public String domain_strategy;

        // End of public InboundOptions ;

        // End of public ListenOptions ;

    }

    public static class Inbound_TProxyOptions extends Inbound {

        // Generate note: nested type ListenOptions
        public String listen;

        public Integer listen_port;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;


        public Long udp_timeout;

        public Boolean proxy_protocol;

        public Boolean proxy_protocol_accept_no_header;

        public String detour;

        // Generate note: nested type InboundOptions
        public Boolean sniff;

        public Boolean sniff_override_destination;

        public Long sniff_timeout;

        public String domain_strategy;

        // End of public InboundOptions ;

        // End of public ListenOptions ;

        public String network;

    }

    public static class Inbound_DirectOptions extends Inbound {

        // Generate note: nested type ListenOptions
        public String listen;

        public Integer listen_port;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;


        public Long udp_timeout;

        public Boolean proxy_protocol;

        public Boolean proxy_protocol_accept_no_header;

        public String detour;

        // Generate note: nested type InboundOptions
        public Boolean sniff;

        public Boolean sniff_override_destination;

        public Long sniff_timeout;

        public String domain_strategy;

        // End of public InboundOptions ;

        // End of public ListenOptions ;

        public String network;

        public String override_address;

        public Integer override_port;

    }

    public static class Inbound_SocksOptions extends Inbound {

        // Generate note: nested type ListenOptions
        public String listen;

        public Integer listen_port;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;


        public Long udp_timeout;

        public Boolean proxy_protocol;

        public Boolean proxy_protocol_accept_no_header;

        public String detour;

        // Generate note: nested type InboundOptions
        public Boolean sniff;

        public Boolean sniff_override_destination;

        public Long sniff_timeout;

        public String domain_strategy;

        // End of public InboundOptions ;

        // End of public ListenOptions ;

        public List<User> users;

    }

    public static class Inbound_HTTPOptions extends Inbound {

        // Generate note: nested type ListenOptions
        public String listen;

        public Integer listen_port;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;


        public Long udp_timeout;

        public Boolean proxy_protocol;

        public Boolean proxy_protocol_accept_no_header;

        public String detour;

        // Generate note: nested type InboundOptions
        public Boolean sniff;

        public Boolean sniff_override_destination;

        public Long sniff_timeout;

        public String domain_strategy;

        // End of public InboundOptions ;

        // End of public ListenOptions ;

        public List<User> users;

        public Boolean set_system_proxy;

        public InboundTLSOptions tls;

    }

    public static class Inbound_MixedOptions extends Inbound {

        // Generate note: nested type ListenOptions
        public String listen;

        public Integer listen_port;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;


        public Long udp_timeout;

        public Boolean proxy_protocol;

        public Boolean proxy_protocol_accept_no_header;

        public String detour;

        // Generate note: nested type InboundOptions
        public Boolean sniff;

        public Boolean sniff_override_destination;

        public Long sniff_timeout;

        public String domain_strategy;

        // End of public InboundOptions ;

        // End of public ListenOptions ;

        public List<User> users;

        public Boolean set_system_proxy;

        public InboundTLSOptions tls;

    }

    public static class Inbound_ShadowsocksOptions extends Inbound {

        // Generate note: nested type ListenOptions
        public String listen;

        public Integer listen_port;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;


        public Long udp_timeout;

        public Boolean proxy_protocol;

        public Boolean proxy_protocol_accept_no_header;

        public String detour;

        // Generate note: nested type InboundOptions
        public Boolean sniff;

        public Boolean sniff_override_destination;

        public Long sniff_timeout;

        public String domain_strategy;

        // End of public InboundOptions ;

        // End of public ListenOptions ;

        public String network;

        public String method;

        public String password;

        public List<ShadowsocksUser> users;

        public List<ShadowsocksDestination> destinations;

    }

    public static class Inbound_VMessOptions extends Inbound {

        // Generate note: nested type ListenOptions
        public String listen;

        public Integer listen_port;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;


        public Long udp_timeout;

        public Boolean proxy_protocol;

        public Boolean proxy_protocol_accept_no_header;

        public String detour;

        // Generate note: nested type InboundOptions
        public Boolean sniff;

        public Boolean sniff_override_destination;

        public Long sniff_timeout;

        public String domain_strategy;

        // End of public InboundOptions ;

        // End of public ListenOptions ;

        public List<VMessUser> users;

        public InboundTLSOptions tls;

        public V2RayTransportOptions transport;

    }

    public static class Inbound_TrojanOptions extends Inbound {

        // Generate note: nested type ListenOptions
        public String listen;

        public Integer listen_port;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;


        public Long udp_timeout;

        public Boolean proxy_protocol;

        public Boolean proxy_protocol_accept_no_header;

        public String detour;

        // Generate note: nested type InboundOptions
        public Boolean sniff;

        public Boolean sniff_override_destination;

        public Long sniff_timeout;

        public String domain_strategy;

        // End of public InboundOptions ;

        // End of public ListenOptions ;

        public List<TrojanUser> users;

        public InboundTLSOptions tls;

        public ServerOptions fallback;

        public Map<String, ServerOptions> fallback_for_alpn;

        public V2RayTransportOptions transport;

    }

    public static class Inbound_NaiveOptions extends Inbound {

        // Generate note: nested type ListenOptions
        public String listen;

        public Integer listen_port;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;


        public Long udp_timeout;

        public Boolean proxy_protocol;

        public Boolean proxy_protocol_accept_no_header;

        public String detour;

        // Generate note: nested type InboundOptions
        public Boolean sniff;

        public Boolean sniff_override_destination;

        public Long sniff_timeout;

        public String domain_strategy;

        // End of public InboundOptions ;

        // End of public ListenOptions ;

        public List<User> users;

        public String network;

        public InboundTLSOptions tls;

    }

    public static class Inbound_HysteriaOptions extends Inbound {

        // Generate note: nested type ListenOptions
        public String listen;

        public Integer listen_port;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;


        public Long udp_timeout;

        public Boolean proxy_protocol;

        public Boolean proxy_protocol_accept_no_header;

        public String detour;

        // Generate note: nested type InboundOptions
        public Boolean sniff;

        public Boolean sniff_override_destination;

        public Long sniff_timeout;

        public String domain_strategy;

        // End of public InboundOptions ;

        // End of public ListenOptions ;

        public String up;

        public Integer up_mbps;

        public String down;

        public Integer down_mbps;

        public String obfs;

        public List<HysteriaUser> users;

        public Long recv_window_conn;

        public Long recv_window_client;

        public Integer max_conn_client;

        public Boolean disable_mtu_discovery;

        public InboundTLSOptions tls;

    }

    public static class Inbound_ShadowTLSOptions extends Inbound {

        // Generate note: nested type ListenOptions
        public String listen;

        public Integer listen_port;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;


        public Long udp_timeout;

        public Boolean proxy_protocol;

        public Boolean proxy_protocol_accept_no_header;

        public String detour;

        // Generate note: nested type InboundOptions
        public Boolean sniff;

        public Boolean sniff_override_destination;

        public Long sniff_timeout;

        public String domain_strategy;

        // End of public InboundOptions ;

        // End of public ListenOptions ;

        public Integer version;

        public String password;

        public List<ShadowTLSUser> users;

        public ShadowTLSHandshakeOptions handshake;

        public Map<String, ShadowTLSHandshakeOptions> handshake_for_server_name;

        public Boolean strict_mode;

    }

    public static class Inbound_VLESSOptions extends Inbound {

        // Generate note: nested type ListenOptions
        public String listen;

        public Integer listen_port;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;


        public Long udp_timeout;

        public Boolean proxy_protocol;

        public Boolean proxy_protocol_accept_no_header;

        public String detour;

        // Generate note: nested type InboundOptions
        public Boolean sniff;

        public Boolean sniff_override_destination;

        public Long sniff_timeout;

        public String domain_strategy;

        // End of public InboundOptions ;

        // End of public ListenOptions ;

        public List<VLESSUser> users;

        public InboundTLSOptions tls;

        public V2RayTransportOptions transport;

    }

    public static class Inbound_TUICOptions extends Inbound {

        // Generate note: nested type ListenOptions
        public String listen;

        public Integer listen_port;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;


        public Long udp_timeout;

        public Boolean proxy_protocol;

        public Boolean proxy_protocol_accept_no_header;

        public String detour;

        // Generate note: nested type InboundOptions
        public Boolean sniff;

        public Boolean sniff_override_destination;

        public Long sniff_timeout;

        public String domain_strategy;

        // End of public InboundOptions ;

        // End of public ListenOptions ;

        public List<TUICUser> users;

        public String congestion_control;

        public Long auth_timeout;

        public Boolean zero_rtt_handshake;

        public Long heartbeat;

        public InboundTLSOptions tls;

    }

    public static class Inbound_Hysteria2Options extends Inbound {

        // Generate note: nested type ListenOptions
        public String listen;

        public Integer listen_port;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;


        public Long udp_timeout;

        public Boolean proxy_protocol;

        public Boolean proxy_protocol_accept_no_header;

        public String detour;

        // Generate note: nested type InboundOptions
        public Boolean sniff;

        public Boolean sniff_override_destination;

        public Long sniff_timeout;

        public String domain_strategy;

        // End of public InboundOptions ;

        // End of public ListenOptions ;

        public Integer up_mbps;

        public Integer down_mbps;

        public Hysteria2Obfs obfs;

        public List<Hysteria2User> users;

        public Boolean ignore_client_bandwidth;

        public InboundTLSOptions tls;

        public String masquerade;

    }

    public static class Outbound_DirectOptions extends Outbound {

        // Generate note: nested type DialerOptions
        public String detour;

        public String bind_interface;

        public String inet4_bind_address;

        public String inet6_bind_address;

        public String protect_path;

        public Integer routing_mark;

        public Boolean reuse_addr;

        public Long connect_timeout;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;


        public String domain_strategy;

        public Long fallback_delay;

        // End of public DialerOptions ;

        public String override_address;

        public Integer override_port;

        public Integer proxy_protocol;

    }

    public static class Outbound_SocksOptions extends Outbound {

        // Generate note: nested type DialerOptions
        public String detour;

        public String bind_interface;

        public String inet4_bind_address;

        public String inet6_bind_address;

        public String protect_path;

        public Integer routing_mark;

        public Boolean reuse_addr;

        public Long connect_timeout;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;


        public String domain_strategy;

        public Long fallback_delay;

        // End of public DialerOptions ;

        // Generate note: nested type ServerOptions
        public String server;

        public Integer server_port;

        // End of public ServerOptions ;

        public String version;

        public String username;

        public String password;

        public String network;

        public UDPOverTCPOptions udp_over_tcp;

    }

    public static class Outbound_HTTPOptions extends Outbound {

        // Generate note: nested type DialerOptions
        public String detour;

        public String bind_interface;

        public String inet4_bind_address;

        public String inet6_bind_address;

        public String protect_path;

        public Integer routing_mark;

        public Boolean reuse_addr;

        public Long connect_timeout;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;


        public String domain_strategy;

        public Long fallback_delay;

        // End of public DialerOptions ;

        // Generate note: nested type ServerOptions
        public String server;

        public Integer server_port;

        // End of public ServerOptions ;

        public String username;

        public String password;

        public OutboundTLSOptions tls;

        public String path;

        public Map<String, String> headers;

    }

    public static class Outbound_ShadowsocksOptions extends Outbound {

        // Generate note: nested type DialerOptions
        public String detour;

        public String bind_interface;

        public String inet4_bind_address;

        public String inet6_bind_address;

        public String protect_path;

        public Integer routing_mark;

        public Boolean reuse_addr;

        public Long connect_timeout;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;


        public String domain_strategy;

        public Long fallback_delay;

        // End of public DialerOptions ;

        // Generate note: nested type ServerOptions
        public String server;

        public Integer server_port;

        // End of public ServerOptions ;

        public String method;

        public String password;

        public String plugin;

        public String plugin_opts;

        public String network;

        public UDPOverTCPOptions udp_over_tcp;

        public MultiplexOptions multiplex;

    }

    public static class Outbound_VMessOptions extends Outbound {

        // Generate note: nested type DialerOptions
        public String detour;

        public String bind_interface;

        public String inet4_bind_address;

        public String inet6_bind_address;

        public String protect_path;

        public Integer routing_mark;

        public Boolean reuse_addr;

        public Long connect_timeout;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;


        public String domain_strategy;

        public Long fallback_delay;

        // End of public DialerOptions ;

        // Generate note: nested type ServerOptions
        public String server;

        public Integer server_port;

        // End of public ServerOptions ;

        public String uuid;

        public String security;

        public Integer alter_id;

        public Boolean global_padding;

        public Boolean authenticated_length;

        public String network;

        public OutboundTLSOptions tls;

        public String packet_encoding;

        public MultiplexOptions multiplex;

        public V2RayTransportOptions transport;

    }

    public static class Outbound_TrojanOptions extends Outbound {

        // Generate note: nested type DialerOptions
        public String detour;

        public String bind_interface;

        public String inet4_bind_address;

        public String inet6_bind_address;

        public String protect_path;

        public Integer routing_mark;

        public Boolean reuse_addr;

        public Long connect_timeout;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;


        public String domain_strategy;

        public Long fallback_delay;

        // End of public DialerOptions ;

        // Generate note: nested type ServerOptions
        public String server;

        public Integer server_port;

        // End of public ServerOptions ;

        public String password;

        public String network;

        public OutboundTLSOptions tls;

        public MultiplexOptions multiplex;

        public V2RayTransportOptions transport;

    }

    public static class Outbound_WireGuardOptions extends Outbound {

        // Generate note: nested type DialerOptions
        public String detour;

        public String bind_interface;

        public String inet4_bind_address;

        public String inet6_bind_address;

        public String protect_path;

        public Integer routing_mark;

        public Boolean reuse_addr;

        public Long connect_timeout;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;


        public String domain_strategy;

        public Long fallback_delay;

        // End of public DialerOptions ;

        public Boolean system_interface;

        public String interface_name;

        // Generate note: Listable
        public List<String> local_address;

        public String private_key;

        public List<WireGuardPeer> peers;

        // Generate note: nested type ServerOptions
        public String server;

        public Integer server_port;

        // End of public ServerOptions ;

        public String peer_public_key;

        public String pre_shared_key;

        // Generate note: Base64 String
        public String reserved;

        public Integer workers;

        public Integer mtu;

        public String network;

    }

    public static class Outbound_HysteriaOptions extends Outbound {

        // Generate note: nested type DialerOptions
        public String detour;

        public String bind_interface;

        public String inet4_bind_address;

        public String inet6_bind_address;

        public String protect_path;

        public Integer routing_mark;

        public Boolean reuse_addr;

        public Long connect_timeout;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;


        public String domain_strategy;

        public Long fallback_delay;

        // End of public DialerOptions ;

        // Generate note: nested type ServerOptions
        public String server;

        public Integer server_port;

        // End of public ServerOptions ;

        public String up;

        public Integer up_mbps;

        public String down;

        public Integer down_mbps;

        public String obfs;

        // Generate note: Base64 String
        public String auth;

        public String auth_str;

        public Long recv_window_conn;

        public Long recv_window;

        public Boolean disable_mtu_discovery;

        public String network;

        public OutboundTLSOptions tls;

        public String hop_ports;

        public Integer hop_interval;

    }

    public static class Outbound_TorOptions extends Outbound {

        // Generate note: nested type DialerOptions
        public String detour;

        public String bind_interface;

        public String inet4_bind_address;

        public String inet6_bind_address;

        public String protect_path;

        public Integer routing_mark;

        public Boolean reuse_addr;

        public Long connect_timeout;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;


        public String domain_strategy;

        public Long fallback_delay;

        // End of public DialerOptions ;

        public String executable_path;

        public List<String> extra_args;

        public String data_directory;

        public Map<String, String> torrc;

    }

    public static class Outbound_SSHOptions extends Outbound {

        // Generate note: nested type DialerOptions
        public String detour;

        public String bind_interface;

        public String inet4_bind_address;

        public String inet6_bind_address;

        public String protect_path;

        public Integer routing_mark;

        public Boolean reuse_addr;

        public Long connect_timeout;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;


        public String domain_strategy;

        public Long fallback_delay;

        // End of public DialerOptions ;

        // Generate note: nested type ServerOptions
        public String server;

        public Integer server_port;

        // End of public ServerOptions ;

        public String user;

        public String password;

        public String private_key;

        public String private_key_path;

        public String private_key_passphrase;

        // Generate note: Listable
        public List<String> host_key;

        // Generate note: Listable
        public List<String> host_key_algorithms;

        public String client_version;

    }

    public static class Outbound_ShadowTLSOptions extends Outbound {

        // Generate note: nested type DialerOptions
        public String detour;

        public String bind_interface;

        public String inet4_bind_address;

        public String inet6_bind_address;

        public String protect_path;

        public Integer routing_mark;

        public Boolean reuse_addr;

        public Long connect_timeout;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;


        public String domain_strategy;

        public Long fallback_delay;

        // End of public DialerOptions ;

        // Generate note: nested type ServerOptions
        public String server;

        public Integer server_port;

        // End of public ServerOptions ;

        public Integer version;

        public String password;

        public OutboundTLSOptions tls;

    }

    public static class Outbound_ShadowsocksROptions extends Outbound {

        // Generate note: nested type DialerOptions
        public String detour;

        public String bind_interface;

        public String inet4_bind_address;

        public String inet6_bind_address;

        public String protect_path;

        public Integer routing_mark;

        public Boolean reuse_addr;

        public Long connect_timeout;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;


        public String domain_strategy;

        public Long fallback_delay;

        // End of public DialerOptions ;

        // Generate note: nested type ServerOptions
        public String server;

        public Integer server_port;

        // End of public ServerOptions ;

        public String method;

        public String password;

        public String obfs;

        public String obfs_param;

        public String protocol;

        public String protocol_param;

        public String network;

    }

    public static class Outbound_VLESSOptions extends Outbound {

        // Generate note: nested type DialerOptions
        public String detour;

        public String bind_interface;

        public String inet4_bind_address;

        public String inet6_bind_address;

        public String protect_path;

        public Integer routing_mark;

        public Boolean reuse_addr;

        public Long connect_timeout;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;


        public String domain_strategy;

        public Long fallback_delay;

        // End of public DialerOptions ;

        // Generate note: nested type ServerOptions
        public String server;

        public Integer server_port;

        // End of public ServerOptions ;

        public String uuid;

        public String flow;

        public String network;

        public OutboundTLSOptions tls;

        public MultiplexOptions multiplex;

        public V2RayTransportOptions transport;

        public String packet_encoding;

    }

    public static class Outbound_TUICOptions extends Outbound {

        // Generate note: nested type DialerOptions
        public String detour;

        public String bind_interface;

        public String inet4_bind_address;

        public String inet6_bind_address;

        public String protect_path;

        public Integer routing_mark;

        public Boolean reuse_addr;

        public Long connect_timeout;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;


        public String domain_strategy;

        public Long fallback_delay;

        // End of public DialerOptions ;

        // Generate note: nested type ServerOptions
        public String server;

        public Integer server_port;

        // End of public ServerOptions ;

        public String uuid;

        public String password;

        public String congestion_control;

        public String udp_relay_mode;

        public Boolean udp_over_stream;

        public Boolean zero_rtt_handshake;

        public Long heartbeat;

        public String network;

        public OutboundTLSOptions tls;

    }

    public static class Outbound_Hysteria2Options extends Outbound {

        // Generate note: nested type DialerOptions
        public String detour;

        public String bind_interface;

        public String inet4_bind_address;

        public String inet6_bind_address;

        public String protect_path;

        public Integer routing_mark;

        public Boolean reuse_addr;

        public Long connect_timeout;

        public Boolean tcp_fast_open;

        public Boolean tcp_multi_path;

        public Boolean udp_fragment;


        public String domain_strategy;

        public Long fallback_delay;

        // End of public DialerOptions ;

        // Generate note: nested type ServerOptions
        public String server;

        public Integer server_port;

        // End of public ServerOptions ;

        public Integer up_mbps;

        public Integer down_mbps;

        public Hysteria2Obfs obfs;

        public String password;

        public String network;

        public OutboundTLSOptions tls;

        public String hop_ports;

        public Integer hop_interval;

    }

    public static class Outbound_SelectorOptions extends Outbound {

        public List<String> outbounds;

        @SerializedName("default")
        public String default_;

    }

    public static class Outbound_URLTestOptions extends Outbound {

        public List<String> outbounds;

        public String url;

        public Long interval;

        public Integer tolerance;

    }

    public static class Rule_DefaultOptions extends Rule {

        // Generate note: Listable
        public List<String> inbound;

        public Integer ip_version;

        // Generate note: Listable
        public List<String> network;

        // Generate note: Listable
        public List<String> auth_user;

        // Generate note: Listable
        public List<String> protocol;

        // Generate note: Listable
        public List<String> domain;

        // Generate note: Listable
        public List<String> domain_suffix;

        // Generate note: Listable
        public List<String> domain_keyword;

        // Generate note: Listable
        public List<String> domain_regex;

        public List<String> rule_set;

        public Boolean source_ip_is_private;

        public Boolean rule_set_ipcidr_match_source;
        public Boolean ip_is_private;

        // Generate note: Listable
        public List<String> source_ip_cidr;

        // Generate note: Listable
        public List<String> ip_cidr;

        // Generate note: Listable
        public List<Integer> source_port;

        // Generate note: Listable
        public List<String> source_port_range;

        // Generate note: Listable
        public List<Integer> port;

        // Generate note: Listable
        public List<String> port_range;

        // Generate note: Listable
        public List<String> process_name;

        // Generate note: Listable
        public List<String> process_path;

        // Generate note: Listable
        public List<String> package_name;

        // Generate note: Listable
        public List<String> user;

        // Generate note: Listable
        public List<Integer> user_id;

        public String clash_mode;

        public Boolean invert;

        public String outbound;

    }

    public static class DNSRule_DefaultOptions extends DNSRule {

        // Generate note: Listable
        public List<String> inbound;

        public Integer ip_version;

        // Generate note: Listable
        public List<String> query_type;

        // Generate note: Listable
        public List<String> network;

        // Generate note: Listable
        public List<String> auth_user;

        // Generate note: Listable
        public List<String> protocol;

        // Generate note: Listable
        public List<String> domain;

        // Generate note: Listable
        public List<String> domain_suffix;

        // Generate note: Listable
        public List<String> domain_keyword;

        // Generate note: Listable
        public List<String> domain_regex;

        // Generate note: Listable
        public List<String> geosite;

        // Generate note: Listable
        public List<String> source_ip_cidr;

        // Generate note: Listable
        public List<Integer> source_port;

        // Generate note: Listable
        public List<String> source_port_range;

        // Generate note: Listable
        public List<Integer> port;

        // Generate note: Listable
        public List<String> port_range;

        // Generate note: Listable
        public List<String> process_name;

        // Generate note: Listable
        public List<String> process_path;

        // Generate note: Listable
        public List<String> package_name;

        // Generate note: Listable
        public List<String> user;

        // Generate note: Listable
        public List<Integer> user_id;

        // Generate note: Listable
        public List<String> outbound;

        public String clash_mode;

        public Boolean invert;

        public String server;

        public Boolean disable_cache;

        public Integer rewrite_ttl;

    }

    public static class V2RayTransportOptions_HTTPOptions extends V2RayTransportOptions {

        // Generate note: Listable
        public List<String> host;

        public String path;

        public String method;

        public Map<String, String> headers;

        public Long idle_timeout;

        public Long ping_timeout;

    }

    public static class V2RayTransportOptions_WebsocketOptions extends V2RayTransportOptions {

        public String path;

        public Map<String, String> headers;

        public Integer max_early_data;

        public String early_data_header_name;

    }


    public static class V2RayTransportOptions_GRPCOptions extends V2RayTransportOptions {

        public String service_name;

        public Long idle_timeout;

        public Long ping_timeout;

        public Boolean permit_without_stream;


    }

    public static class V2RayTransportOptions_HTTPUpgradeOptions extends V2RayTransportOptions {

        public String host;

        public String path;


    }

}
