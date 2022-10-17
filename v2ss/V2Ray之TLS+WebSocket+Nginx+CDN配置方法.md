# V2Ray之TLS+WebSocket+Nginx+CDN配置方法

如果你的VPS IP 被墙了，或者你直接连接VPS的速度不理想，可以试试本文介绍的方法。

本文以Cloudflare CDN为例配置，据传，如果你不是使用 移动宽带 的用户，那么使用 Cloudflare 中转的速度相对来说是比较慢的，这个是因为线路的问题，无解。如果你使用移动网络的话，那么 Cloudflare 的中转节点可能会在香港，速度也许会不错 (不完全保证)。

体验了本文介绍的方法，如果速度不理想，可以考虑用国内的CDN替换Cloudflare，据说能体验飞一般的速度，也非常稳定，高峰期毫无压力，在重点 IP 段也无所畏惧。***目前和 V2Ray 兼容的 CDN 国外有 Cloudflare，国内阿里云，这两家的 CDN 是支持 WebSocket 的。剩下的几家不支持 WebSocket，也不会 keep TCP connection。因此 HTTP/2 回源也不支持（访问支持 HTTP/2 和回源支持 HTTP/2 是两回事）。 另外，使用国内 CDN 需要域名备案并服务商实名认证。使用有风险，入坑需谨慎。***

会用 Cloudflare，其它的 CDN 应该也许也不会有问题。但有一点，如果是为了复活被墙IP，则只能用Cloudflare等国外的CDN。

本文以[自建V2ray服务器简明教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAV2ray%E6%9C%8D%E5%8A%A1%E5%99%A8%E7%AE%80%E6%98%8E%E6%95%99%E7%A8%8B.md) 为基础，V2ray基本安装配置请参照此简明教程。

本文介绍的方法不太复杂，但对小白来说也不是很容易，如果你懒得折腾，那就用我们提供的免费翻墙软件吧：<br>
<a href="https://github.com/bannedbook/fanqiang/wiki/%E5%AE%89%E5%8D%93%E7%BF%BB%E5%A2%99%E8%BD%AF%E4%BB%B6">安卓手机翻墙</a>、<a href="https://github.com/bannedbook/fanqiang/wiki/Chrome%E4%B8%80%E9%94%AE%E7%BF%BB%E5%A2%99%E5%8C%85" class="wiki-page-link">Chrome一键翻墙包</a>、<a href="https://github.com/bannedbook/fanqiang/wiki/%E7%81%AB%E7%8B%90firefox%E4%B8%80%E9%94%AE%E7%BF%BB%E5%A2%99%E5%8C%85" class="wiki-page-link">火狐firefox一键翻墙包</a> <br>

<b>或者也可以购买现成的翻墙服务(跟本库无关哦，为支持我们，可考虑年付)：</b><br>
<a href="https://github.com/bannedbook/fanqiang/wiki/V2ray%E6%9C%BA%E5%9C%BA"><img src="https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/v2free.jpg" height="300" alt="V2free翻墙-不限流量、高速稳定、性价比超强"></a>

## 本文目录

* <a href="#注册一个域名">注册一个域名</a>
* <a href="#域名添加到cloudflare">域名添加到Cloudflare</a>
* <a href="#证书生成">证书生成</a>
* <a href="#vps-安装配置nginx">VPS 安装配置Nginx</a>
* <a href="#配置-v2ray">配置 V2Ray</a>
* <a href="#相关教程">相关教程</a>
* <a href="#v2ray伪装网站">V2ray伪装网站</a>
* <a href="#真实网站加装v2ray">真实网站+加装v2ray</a>

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

我们采用最简略的配置，VPS端无需自己生成证书，直接使用Cloudflare CDN 自动生成的证书。所以这一步你需要做的就是 do nothing. 这样配置不但超简易，而且配合Cloudflare SSL/TLS 的 Flexible 模式，比Full模式速度更快。

## VPS 安装配置Nginx
```
apt-get update
apt-get -y install nginx
```

然后替换/etc/nginx.conf为如下配置，此配置文件是通用配置，不管什么域名都可以使用，直接下载覆盖即可:

`wget https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/server-cfg/nginx.conf  -O -> /etc/nginx/nginx.conf`

下面是nginx.conf的内容，为方便大家，制作了这个nginx通用配置文件并上传到github，大家可以直接使用上面的命令下载覆盖即可。

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
		
    location /bannedbook { # 与 V2Ray 配置中的 path 保持一致
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

### V2Ray服务器配置

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
          "alterId": 0
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

## 相关教程

* <a href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAV2ray%E6%9C%8D%E5%8A%A1%E5%99%A8%E7%AE%80%E6%98%8E%E6%95%99%E7%A8%8B.md">自建V2ray服务器简明教程</a>
* <a href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAV2Ray%2BTLS%E7%BF%BB%E5%A2%99%E9%85%8D%E7%BD%AE%E6%96%B9%E6%B3%95.md">自建V2Ray+TLS翻墙配置方法</a>
* <a href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/V2Ray%E4%B9%8BTLS%2BWebSocket%E7%BF%BB%E5%A2%99%E6%96%B9%E6%B3%95.md">V2Ray之TLS+WebSocket翻墙方法</a>
* [Brook之TLS+WebSocket+CDN翻墙教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/Brook%E4%B9%8BTLS%2BWebSocket%2BCDN%E7%BF%BB%E5%A2%99%E6%95%99%E7%A8%8B.md)
* [Brook之TLS+WebSocket+Web服务器翻墙教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/Brook%E4%B9%8BTLS%2BWebSocket%2BWeb%E7%BF%BB%E5%A2%99%E6%95%99%E7%A8%8B.md)
* [Brook之TLS+WebSocket翻墙教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/Brook%E4%B9%8BTLS%2BWebSocket%E7%BF%BB%E5%A2%99%E6%95%99%E7%A8%8B.md)
* <a title="自建Shadowsocks服务器简明教程" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAShadowsocks%E6%9C%8D%E5%8A%A1%E5%99%A8%E7%AE%80%E6%98%8E%E6%95%99%E7%A8%8B.md">自建Shadowsocks服务器简明教程</a>
* <a title="SSH连接VPS教程" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/SSH%E8%BF%9E%E6%8E%A5VPS%E6%95%99%E7%A8%8B.md">SSH连接VPS教程</a>
* <a title="V2ray官方一键安装脚本" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/V2ray%E5%AE%98%E6%96%B9%E4%B8%80%E9%94%AE%E5%AE%89%E8%A3%85%E8%84%9A%E6%9C%AC.md">V2ray官方一键安装脚本</a>
* <a title="Windows版V2ray客户端安装配置指南" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/Windows%E7%89%88V2ray%E5%AE%A2%E6%88%B7%E7%AB%AF%E5%AE%89%E8%A3%85%E9%85%8D%E7%BD%AE%E6%8C%87%E5%8D%97.md">Windows版V2ray客户端安装配置指南.md</a>
* <a title="使用FileZilla和VPS传输文件教程" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E4%BD%BF%E7%94%A8FileZilla%E5%92%8CVPS%E4%BC%A0%E8%BE%93%E6%96%87%E4%BB%B6%E6%95%99%E7%A8%8B.md">使用FileZilla和VPS传输文件教程</a>
* <a title="最简单的Google BBR 一键加速VPS教程" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E6%9C%80%E7%AE%80%E5%8D%95%E7%9A%84Google%20BBR%20%E4%B8%80%E9%94%AE%E5%8A%A0%E9%80%9FVPS%E6%95%99%E7%A8%8B.md">最简单的Google BBR 一键加速VPS教程</a>
* <a title="翻墙VPS推荐：搬瓦工VPS购买教程" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E7%BF%BB%E5%A2%99VPS%E6%8E%A8%E8%8D%90%EF%BC%9A%E6%90%AC%E7%93%A6%E5%B7%A5VPS%E8%B4%AD%E4%B9%B0%E6%95%99%E7%A8%8B.md">翻墙VPS推荐：搬瓦工VPS购买教程</a>
* <a  href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%B4%AD%E4%B9%B0Vultr%20VPS%E5%9B%BE%E6%96%87%E6%95%99%E7%A8%8B.md">购买Vultr VPS图文教程【新用户赠送100美元】</a>

### V2Ray客户端配置

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
                "alterId": 0
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

## V2ray伪装网站

这一步不是必须，只是为了隐藏和伪装的更好。说是伪装，其实我们安装了nginx web服务器，也就是已经安装了一个真正的网站，试试用浏览器打开你的域名，会看到nginx的默认首页。为了隐藏和伪装的更逼真，你可以考虑弄一些英文网页放到vps 的/var/www/html目录下，当然，必须包括一个index.html , 这样会使你的网站看起来更象一个真实的网站。

## 真实网站+加装v2ray

更往前想一步，如果你已经有一个真正的网站，而网站域名没有被墙，那么完全可以利用本文介绍的方法，在原有的真实网站基础上，开辟一个path，加装V2ray用来翻墙。这就是完全以真实网站为掩护来翻墙了。
