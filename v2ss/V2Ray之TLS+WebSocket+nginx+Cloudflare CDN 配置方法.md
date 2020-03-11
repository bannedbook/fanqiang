# V2Ray之TLS+WebSocket+nginx+Cloudflare CDN 配置方法

如果你的VPS IP 被墙了，或者你直接连接VPS的速度不理想，可以试试本文介绍的方法。

本文以[自建V2ray服务器简明教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAV2ray%E6%9C%8D%E5%8A%A1%E5%99%A8%E7%AE%80%E6%98%8E%E6%95%99%E7%A8%8B.md) 为基础，介绍更加安全可靠的V2Ray+TLS翻墙配置方法。

## 注册一个域名

如果已经注册有域名了可以跳过。
TLS 需要一个域名，域名有免费的和有付费的，如果你不舍得为一个域名每年花点钱，用个免费域名也可以，但总体来说付费的会优于免费的，我看到有网友因为用了免费域名，结果出现域名解析故障而翻墙失败。所以，还是推荐买个便宜点的域名。关于域名注册商，推荐[namesilo](https://www.namesilo.com/register.php?rid=43ac240wm) ,这家域名商有不少便宜的域名选择，比如.xyz域名，一年才0.99美元，很便宜，而且可选择免费域名隐私保护。为了方便，在本文中我就忽略如何注册购买域名了。关于如何获取域名，具体可搜索相关文章教程。

**注册好域名之后务必记得添加一个 A 记录指向你的 VPS!**

另外，为了避免主域名被封锁，推荐先使用子域名，但本文以主域名为例! 

**以下假设注册的域名为 mydomain.me，请将之替换成自己的域名。**

## 域名添加到Cloudflare 

确保在Cloudflare DNS 设置处启用CDN，就是Proxy Status 那一列，那个金黄金黄的云彩图标要点亮，A记录指向你的VPS IP地址。

另外，在 Cloudflare 的 SSL/TLS 设置中启用 Flexible 模式，并关掉 TLS 1.3 。

大概这样就可以了。等待一会儿，ping 域名试试看，如果ping 出的ip变成了 Cloudflare ip，那么我们已经成功给我们的域名套上了Cloudflare CDN。

## 证书生成

我们采用最简略的配置，VPS端无需自己生成证书，直接使用Cloudflare CDN 自动生成的证书。所以这一步你需要做的事nothing.

## VPS 安装配置Nginx
```
apt=get update
apt-get -y install nginx
```

然后替换/etc/nginx.conf为如下配置
```javascript
user www-data;
worker_processes auto;
pid /run/nginx.pid;
include /etc/nginx/modules-enabled/*.conf;
worker_rlimit_nofile  655350;
events {
	use epoll;
	worker_connections 65536;
}

http {
	sendfile on;
	tcp_nopush on;
	tcp_nodelay on;
	keepalive_timeout 65;
	types_hash_max_size 2048;
	include /etc/nginx/mime.types;
	default_type application/octet-stream;
	ssl_protocols TLSv1 TLSv1.1 TLSv1.2; # Dropping SSLv3, ref: POODLE
	ssl_prefer_server_ciphers on;
	access_log /var/log/nginx-access.log;
	error_log /var/log/nginx-error.log;

	gzip on;
	server {
		listen 80 default_server;
		listen [::]:80 default_server;
		root /var/www/html;
	
		index index.html index.htm index.nginx-debian.html;
	
		server_name _;
	
		location / {
			try_files $uri $uri/ =404;
		}
		
    location /v2free { # 与 V2Ray 配置中的 path 保持一致
	    proxy_redirect off;
	    proxy_pass http://127.0.0.1:10000; #假设WebSocket监听在环回地址的10000端口上
	    proxy_http_version 1.1;
	    proxy_set_header Upgrade $http_upgrade;
	    proxy_set_header Connection "upgrade";
	    proxy_set_header Host $http_host;
	
	    # Show realip in v2ray access.log
	    proxy_set_header X-Real-IP $remote_addr;
	    proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;	
    }	
	}
}

```

然后，测试nginx配置： nginx -t
重新载入配置： nginx -s reload

## 配置 V2Ray

### 服务器

```javascript
{
  "log": {
    "loglevel": "warning",
    "access": "/dev/null",
    "error": "/dev/null"
  },
  "inbounds": [{
    "listen":"127.0.0.1",
    "port": 10000,
    "protocol": "vmess",
    "settings": {
      "clients": [
        {
          "id": "de20d937-ca8f-af14-ea07-20b45447d371",
          "level": 1,
          "alterId": 64
        }
      ]
    },
	"streamSettings": {
        "network": "ws",
        "wsSettings": {
        "path": "/bannedbook"
        }
    }
  }],
  "outbounds": [
    {
      "protocol": "freedom",
      "settings": {},
      "tag": "allowed"
    },
    {
      "protocol": "blackhole",
      "settings": {},
      "tag": "blocked"
    }
  ],
  "routing": {
    "rules": [
      {
        "type": "field",
        "ip": [
          "geoip:private"
        ],
        "outboundTag": "blocked"
      }
    ]
  }
}
```

修改配置后记得测试配置：/usr/bin/v2ray/v2ray -test -config /etc/v2ray/config.json
重启v2ray: service v2ray   restart

### 客户端

```javascript
{
  "inbounds": [
    {
      "port": 1080,
      "listen": "127.0.0.1",
      "protocol": "socks",
      "sniffing": {
        "enabled": true,
        "destOverride": ["http", "tls"]
      },
      "settings": {
        "auth": "noauth",
        "udp": false
      }
    }
  ],
  "outbounds": [
    {
      "protocol": "vmess",
      "settings": {
        "vnext": [
          {
            "address": "mydomain.me",
            "port": 443,
            "users": [
              {
                "id": "de20d937-ca8f-af14-ea07-20b45447d371",
                "alterId": 64
              }
            ]
          }
        ]
      },
      "streamSettings": {
        "network": "ws",
        "security": "tls",
        "wsSettings": {
          "path": "/bannedbook"
        }
      }
    }
  ]
}
```
