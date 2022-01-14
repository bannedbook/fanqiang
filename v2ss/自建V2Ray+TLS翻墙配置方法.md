# 自建V2Ray+TLS翻墙配置方法

## bannedbook修订历史
- 2020-02-26 bannedbook:为保护主域名免被封，增加子域名配置方式。
- 2020-02-26 bannedbook:增加个人使用体验，增加域名注册推荐，启用Google BBR加速推荐。

技术上，目前v2ray比[Shadowsocks翻墙](https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAShadowsocks%E6%9C%8D%E5%8A%A1%E5%99%A8%E7%AE%80%E6%98%8E%E6%95%99%E7%A8%8B.md)效果更好，据网络传言，2019年中期开始Shadowsocks能够被墙自动探测阻断，因此，目前[自建V2ray翻墙](https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAV2ray%E6%9C%8D%E5%8A%A1%E5%99%A8%E7%AE%80%E6%98%8E%E6%95%99%E7%A8%8B.md) 就成了更加成熟和稳定的翻墙方案。

以个人实际感受而言，近期使用Shadowsocks翻墙即使是使用了新版的AEAD加密模式，也是一两周时间会被封端口，不得不定期更换Shadowsocks服务器监听端口。而同时在使用的v2ray（仅仅默认配置，未加TLS配置）则一直坚挺。而V2Ray+TLS翻墙模式，则比[V2Ray默认配置](https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAV2ray%E6%9C%8D%E5%8A%A1%E5%99%A8%E7%AE%80%E6%98%8E%E6%95%99%E7%A8%8B.md)更加安全稳定，翻墙效果更好，不易被检测，因此推荐愿意动手折腾的网友不妨折腾一下。

线路上，最少用cn2，如果预算允许，最好上cn2 gia，推荐[搬瓦工VPS,可选cn2或cn2 gia线路](https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E7%BF%BB%E5%A2%99VPS%E6%8E%A8%E8%8D%90%EF%BC%9A%E6%90%AC%E7%93%A6%E5%B7%A5VPS%E8%B4%AD%E4%B9%B0%E6%95%99%E7%A8%8B.md)。在2017年时，普通的cn2线路就可流畅看1080P youtube，但到2019、2020年，cn2线路则经常卡顿。
在操作系统上，可加入最新的bbr技术,详见： [最简单的Google BBR 一键加速VPS教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E6%9C%80%E7%AE%80%E5%8D%95%E7%9A%84Google%20BBR%20%E4%B8%80%E9%94%AE%E5%8A%A0%E9%80%9FVPS%E6%95%99%E7%A8%8B.md)。BBR技术，可以更好地抢占带宽，从而达到同样带宽，速度更快的目的。

本文以[自建V2ray服务器简明教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAV2ray%E6%9C%8D%E5%8A%A1%E5%99%A8%E7%AE%80%E6%98%8E%E6%95%99%E7%A8%8B.md) 为基础，介绍更加安全可靠的V2Ray+TLS翻墙配置方法。

V2ray 从 v1.19 起引入了 TLS，TLS 中文译名是传输层安全，如果你没听说过，请 Google 了解一下。以下给出些我认为介绍较好的文章链接：

 [SSL/TLS协议运行机制的概述](http://www.ruanyifeng.com/blog/2014/02/ssl_tls.html)

 [传输层安全协议](https://zh.wikipedia.org/wiki/%E5%82%B3%E8%BC%B8%E5%B1%A4%E5%AE%89%E5%85%A8%E5%8D%94%E8%AD%B0)


## 注册一个域名

如果已经注册有域名了可以跳过。
TLS 需要一个域名，域名有免费的和有付费的，如果你不舍得为一个域名每年花点钱，用个免费域名也可以，但总体来说付费的会优于免费的，我看到有网友因为用了免费域名，结果出现域名解析故障而翻墙失败。所以，还是推荐买个便宜点的域名。关于域名注册商，推荐[namesilo](https://www.namesilo.com/register.php?rid=43ac240wm) ,这家域名商有不少便宜的域名选择，比如.xyz域名，一年才0.99美元，很便宜，而且可选择免费域名隐私保护。为了方便，在本文中我就忽略如何注册购买域名了。关于如何获取域名，具体可搜索相关文章教程。

**注册好域名之后务必记得添加一个 A 记录指向你的 VPS!**

另外，为了避免主域名被封锁，推荐先使用子域名，所以预备一些子域名也 A 记录指向你的 VPS! 比如：
v01.mydomain.me v02.mydomain.me v03.mydomain.me v04.mydomain.me v05.mydomain.me v06.mydomain.me v07.mydomain.me v08.mydomain.me v09.mydomain.me

**以下假设注册的域名为 mydomain.me，请将之替换成自己的域名。**

## 证书生成

TLS 是证书认证机制，所以使用 TLS 需要证书，证书也有免费付费的，同样的这里使用免费证书，证书认证机构为 [Let's Encrypt](https://letsencrypt.org/)。
证书的生成有许多方法，这里使用的是比较简单的方法：使用 [acme.sh](https://github.com/Neilpang/acme.sh) 脚本生成，本部分说明部分内容参考于[acme.sh README](https://github.com/Neilpang/acme.sh/blob/master/README.md)。

证书有两种，一种是 ECC 证书（内置公钥是 ECDSA 公钥），一种是 RSA 证书（内置 RSA 公钥）。简单来说，同等长度 ECC 比 RSA 更安全,也就是说在具有同样安全性的情况下，ECC 的密钥长度比 RSA 短得多（加密解密会更快）。但问题是 ECC 的兼容性会差一些，Android 4.x 以下和 Windows XP 不支持。只要您的设备不是非常老的老古董，建议使用 ECC 证书。

以下将给出这两类证书的生成方法，请大家根据自身的情况自行选择其中一种证书类型。

证书生成只需在服务器上操作。

### 安装 acme.sh

执行以下命令，acme.sh 会安装到 ~/.acme.sh 目录下。
```
$ curl  https://get.acme.sh | sh
% Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
                               Dload  Upload   Total   Spent    Left  Speed
100   671  100   671    0     0    680      0 --:--:-- --:--:-- --:--:--   679
% Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
                               Dload  Upload   Total   Spent    Left  Speed
100  112k  100  112k    0     0   690k      0 --:--:-- --:--:-- --:--:--  693k
[Fri 30 Dec 01:03:32 GMT 2016] Installing from online archive.
[Fri 30 Dec 01:03:32 GMT 2016] Downloading https://github.com/Neilpang/acme.sh/archive/master.tar.gz
[Fri 30 Dec 01:03:33 GMT 2016] Extracting master.tar.gz
[Fri 30 Dec 01:03:33 GMT 2016] Installing to /home/user/.acme.sh
[Fri 30 Dec 01:03:33 GMT 2016] Installed to /home/user/.acme.sh/acme.sh
[Fri 30 Dec 01:03:33 GMT 2016] Installing alias to '/home/user/.profile'
[Fri 30 Dec 01:03:33 GMT 2016] OK, Close and reopen your terminal to start using acme.sh
[Fri 30 Dec 01:03:33 GMT 2016] Installing cron job
no crontab for user
no crontab for user
[Fri 30 Dec 01:03:33 GMT 2016] Good, bash is found, so change the shebang to use bash as preferred.
[Fri 30 Dec 01:03:33 GMT 2016] OK
[Fri 30 Dec 01:03:33 GMT 2016] Install success!

```
安装成功后执行 `source ~/.bashrc` 以确保脚本所设置的命令别名生效。

如果安装报错，那么可能是因为系统缺少 acme.sh 所需要的依赖项，acme.sh 的依赖项主要是 netcat(nc)，我们通过以下命令来安装这些依赖项，然后重新安装一遍 acme.sh:

```
$ sudo apt-get -y install netcat
```


### 使用 acme.sh 生成证书

#### 如果报错 Please install socat tools first.

如果生成证书时报错 `......Please install socat tools first.` , 请先执行以下命令：

`apt-get install socat`  or  `yum install socat`

#### 主域名证书生成
此步骤仅仅生产主域名的证书，为了防止主域名被封锁，我们可以同时生成一些子域名证书，使用的时候先使用子域名，如果子域名被封了，可以再换一个子域名来用，不用重新申请新的域名。而如果直接使用主域名，主域名一旦被封，就只能申请新域名了。如果想同时生成一些子域名，请跳过此步骤，直接看后文：同时生成多个子域名证书。

执行以下命令生成主域名证书：
以下的命令会临时监听 80 端口，请确保执行该命令前 80 端口没有使用
```
$ sudo ~/.acme.sh/acme.sh --issue -d mydomain.me --standalone -k ec-256
[Fri Dec 30 08:59:12 HKT 2016] Standalone mode.
[Fri Dec 30 08:59:12 HKT 2016] Single domain='mydomain.me'
[Fri Dec 30 08:59:12 HKT 2016] Getting domain auth token for each domain
[Fri Dec 30 08:59:12 HKT 2016] Getting webroot for domain='mydomain.me'
[Fri Dec 30 08:59:12 HKT 2016] _w='no'
[Fri Dec 30 08:59:12 HKT 2016] Getting new-authz for domain='mydomain.me'
[Fri Dec 30 08:59:14 HKT 2016] The new-authz request is ok.
[Fri Dec 30 08:59:14 HKT 2016] mydomain.me is already verified, skip.
[Fri Dec 30 08:59:14 HKT 2016] mydomain.me is already verified, skip http-01.
[Fri Dec 30 08:59:14 HKT 2016] mydomain.me is already verified, skip http-01.
[Fri Dec 30 08:59:14 HKT 2016] Verify finished, start to sign.
[Fri Dec 30 08:59:16 HKT 2016] Cert success.
-----BEGIN CERTIFICATE-----
MIIEMTCCAxmgAwIBAgISA1+gJF5zwUDjNX/6Xzz5fo3lMA0GCSqGSIb3DQEBCwUA
MEoxCzAJBgNVBAYTAlVTMRYwFAYDVQQKEw1MZXQncyBFbmNyeXB0MSMwIQYDVQQD
ExpMZXQncyBFbmNyeXB0IEF1dGhvcml0eSBYMzAeFw0xNjEyMjkyMzU5MDBaFw0x
NzAzMjkyMzU5MDBaMBcxFTATBgNVBAMTDHdlYWtzYW5kLmNvbTBZMBMGByqGSM49
****************************************************************
4p40tm0aMB837XQ9jeAXvXulhVH/7/wWZ8/vkUUvuHSCYHagENiq/3DYj4a85Iw9
+6u1r7atYHJ2VwqSamiyTGDQuhc5wdXIQxY/YQQqkAmn5tLsTZnnOavc4plANT40
zweiG8vcIvMVnnkM0TSz8G1yzv1nOkruN3ozQkLMu6YS7lk/ENBN7DBtYVSmJeU2
VAXE+zgRaP7JFOqK6DrOwhyE2LSgae83Wq/XgXxjfIo1Zmn2UmlE0sbdNKBasnf9
gPUI45eltrjcv8FCSTOUcT7PWCa3
-----END CERTIFICATE-----
[Fri Dec 30 08:59:16 HKT 2016] Your cert is in  /root/.acme.sh/mydomain.me_ecc/mydomain.me.cer
[Fri Dec 30 08:59:16 HKT 2016] Your cert key is in  /root/.acme.sh/mydomain.me_ecc/mydomain.me.key
[Fri Dec 30 08:59:16 HKT 2016] The intermediate CA cert is in  /root/.acme.sh/mydomain.me_ecc/ca.cer
[Fri Dec 30 08:59:16 HKT 2016] And the full chain certs is there:  /root/.acme.sh/mydomain.me_ecc/fullchain.cer
```
`-k` 表示密钥长度，后面的值可以是 `ec-256` 、`ec-384`、`2048`、`3072`、`4096`、`8192`，带有 `ec` 表示生成的是 ECC 证书，没有则是 RSA 证书。在安全性上 256 位的 ECC 证书等同于 3072 位的 RSA 证书。

#### 同时生成多个子域名证书

执行以下命令生成多个子域名证书：
以下的命令会临时监听 80 端口，请确保执行该命令前 80 端口没有使用，并确保所有子域名的A记录到VPS的ip地址。
```
$ sudo ~/.acme.sh/acme.sh --issue --standalone -k ec-256 -d mydomain.me -d www.mydomain.me -d v01.mydomain.me -d v02.mydomain.me -d v03.mydomain.me -d v04.mydomain.me -d v05.mydomain.me -d v06.mydomain.me -d v07.mydomain.me -d v08.mydomain.me -d v09.mydomain.me
[Wed 26 Feb 2020 07:16:01 AM MSK] Standalone mode.
[Wed 26 Feb 2020 07:16:01 AM MSK] Standalone mode.
[Wed 26 Feb 2020 07:16:01 AM MSK] Standalone mode.
[Wed 26 Feb 2020 07:16:01 AM MSK] Standalone mode.
[Wed 26 Feb 2020 07:16:01 AM MSK] Standalone mode.
[Wed 26 Feb 2020 07:16:01 AM MSK] Standalone mode.
[Wed 26 Feb 2020 07:16:01 AM MSK] Standalone mode.
[Wed 26 Feb 2020 07:16:02 AM MSK] Standalone mode.
[Wed 26 Feb 2020 07:16:02 AM MSK] Standalone mode.
[Wed 26 Feb 2020 07:16:02 AM MSK] Standalone mode.
[Wed 26 Feb 2020 07:16:02 AM MSK] Multi domain='DNS:mydomain.me,DNS:v01.mydomain.me,DNS:v02.mydomain.me,DNS:v03.mydomain.me,DNS:v04.mydomain.me,DNS:v05.mydomain.me,DNS:v06.mydomain.me,DNS:v07.mydomain.me,DNS:v08.mydomain.me,DNS:v09.mydomain.me'
[Wed 26 Feb 2020 07:16:02 AM MSK] Getting domain auth token for each domain
[Wed 26 Feb 2020 07:16:14 AM MSK] Getting webroot for domain='mydomain.me'
[Wed 26 Feb 2020 07:16:14 AM MSK] Getting webroot for domain='v01.mydomain.me'
[Wed 26 Feb 2020 07:16:14 AM MSK] Getting webroot for domain='v02.mydomain.me'
[Wed 26 Feb 2020 07:16:14 AM MSK] Getting webroot for domain='v03.mydomain.me'
[Wed 26 Feb 2020 07:16:14 AM MSK] Getting webroot for domain='v04.mydomain.me'
[Wed 26 Feb 2020 07:16:14 AM MSK] Getting webroot for domain='v05.mydomain.me'
[Wed 26 Feb 2020 07:16:14 AM MSK] Getting webroot for domain='v06.mydomain.me'
[Wed 26 Feb 2020 07:16:15 AM MSK] Getting webroot for domain='v07.mydomain.me'
[Wed 26 Feb 2020 07:16:15 AM MSK] Getting webroot for domain='v08.mydomain.me'
[Wed 26 Feb 2020 07:16:15 AM MSK] Getting webroot for domain='v09.mydomain.me'
[Wed 26 Feb 2020 07:16:15 AM MSK] Verifying: mydomain.me
[Wed 26 Feb 2020 07:16:15 AM MSK] Standalone mode server
[Wed 26 Feb 2020 07:16:20 AM MSK] Success
[Wed 26 Feb 2020 07:16:20 AM MSK] Verifying: v01.mydomain.me
[Wed 26 Feb 2020 07:16:20 AM MSK] Standalone mode server
[Wed 26 Feb 2020 07:16:25 AM MSK] Success
[Wed 26 Feb 2020 07:16:25 AM MSK] Verifying: v02.mydomain.me
[Wed 26 Feb 2020 07:16:25 AM MSK] Standalone mode server
[Wed 26 Feb 2020 07:16:30 AM MSK] Success
[Wed 26 Feb 2020 07:16:30 AM MSK] Verifying: v03.mydomain.me
[Wed 26 Feb 2020 07:16:30 AM MSK] Standalone mode server
[Wed 26 Feb 2020 07:16:34 AM MSK] Success
[Wed 26 Feb 2020 07:16:34 AM MSK] Verifying: v04.mydomain.me
[Wed 26 Feb 2020 07:16:34 AM MSK] Standalone mode server
[Wed 26 Feb 2020 07:16:39 AM MSK] Success
[Wed 26 Feb 2020 07:16:39 AM MSK] Verifying: v05.mydomain.me
[Wed 26 Feb 2020 07:16:39 AM MSK] Standalone mode server
[Wed 26 Feb 2020 07:16:44 AM MSK] Success
[Wed 26 Feb 2020 07:16:44 AM MSK] Verifying: v06.mydomain.me
[Wed 26 Feb 2020 07:16:44 AM MSK] Standalone mode server
[Wed 26 Feb 2020 07:16:49 AM MSK] Success
[Wed 26 Feb 2020 07:16:49 AM MSK] Verifying: v07.mydomain.me
[Wed 26 Feb 2020 07:16:49 AM MSK] Standalone mode server
[Wed 26 Feb 2020 07:16:54 AM MSK] Success
[Wed 26 Feb 2020 07:16:54 AM MSK] Verifying: v08.mydomain.me
[Wed 26 Feb 2020 07:16:54 AM MSK] Standalone mode server
[Wed 26 Feb 2020 07:16:59 AM MSK] Success
[Wed 26 Feb 2020 07:16:59 AM MSK] Verifying: v09.mydomain.me
[Wed 26 Feb 2020 07:16:59 AM MSK] Standalone mode server
[Wed 26 Feb 2020 07:17:04 AM MSK] Success
[Wed 26 Feb 2020 07:17:04 AM MSK] Verify finished, start to sign.
[Wed 26 Feb 2020 07:17:04 AM MSK] Lets finalize the order, Le_OrderFinalize: https://acme-v02.api.letsencrypt.org/acme/finalize/79079285/2448775497
[Wed 26 Feb 2020 07:17:05 AM MSK] Download cert, Le_LinkCert: https://acme-v02.api.letsencrypt.org/acme/cert/0350b8df26fbc60c527ecb8c0101584651cd
[Wed 26 Feb 2020 07:17:06 AM MSK] Cert success.
-----BEGIN CERTIFICATE-----
MIIFFTCCA/2gAwIBAgISA1C43yb7xgxSfsuMAQFYRlHNMA0GCSqGSIb3DQEBCwUA
****************************************************************
cecDBr/zgr92GmyUSId5tCWP+hlnGM7qpgunXSxU5x7yEZOwhkNwivmuHjWFAUnc
GlXu957MecVO
-----END CERTIFICATE-----
[Wed 26 Feb 2020 07:17:06 AM MSK] Your cert is in  /root/.acme.sh/mydomain.me_ecc/mydomain.me.cer 
[Wed 26 Feb 2020 07:17:06 AM MSK] Your cert key is in  /root/.acme.sh/mydomain.me_ecc/mydomain.me.key 
[Wed 26 Feb 2020 07:17:06 AM MSK] The intermediate CA cert is in  /root/.acme.sh/mydomain.me_ecc/ca.cer 
[Wed 26 Feb 2020 07:17:06 AM MSK] And the full chain certs is there:  /root/.acme.sh/mydomain.me_ecc/fullchain.cer 
```

#### 证书更新

由于 Let's Encrypt 的证书有效期只有 3 个月，因此需要 90 天至少要更新一次证书，acme.sh 脚本会每 60 天自动更新证书。也可以手动更新。

手动更新 ECC 证书，执行：
```
$ sudo ~/.acme.sh/acme.sh --renew -d mydomain.com --force --ecc
```
或者同时更新子域名，执行：
```
$ sudo ~/.acme.sh/acme.sh --renew -d mydomain.me -d www.mydomain.me -d v01.mydomain.me -d v02.mydomain.me -d v03.mydomain.me -d v04.mydomain.me -d v05.mydomain.me -d v06.mydomain.me -d v07.mydomain.me -d v08.mydomain.me -d v09.mydomain.me --force --ecc
```

如果是 RSA 证书则执行：
```
$ sudo ~/.acme.sh/acme.sh --renew -d mydomain.com --force
```
或者同时更新子域名，执行：
```
$ sudo ~/.acme.sh/acme.sh --renew -d mydomain.me -d www.mydomain.me -d v01.mydomain.me -d v02.mydomain.me -d v03.mydomain.me -d v04.mydomain.me -d v05.mydomain.me -d v06.mydomain.me -d v07.mydomain.me -d v08.mydomain.me -d v09.mydomain.me --force
```

**由于本例中将证书生成到 `/etc/v2ray/` 文件夹，更新证书之后还得把新证书生成到 /etc/v2ray。**

### 安装证书和密钥

#### ECC 证书

将证书和密钥安装到 /etc/v2ray 中：
```
$ sudo ~/.acme.sh/acme.sh --installcert -d mydomain.me --fullchainpath /etc/v2ray/v2ray.crt --keypath /etc/v2ray/v2ray.key --ecc
```
或者同时将子域名证书和密钥安装到 /etc/v2ray 中：
```
$ sudo ~/.acme.sh/acme.sh --installcert -d mydomain.me -d www.mydomain.me -d v01.mydomain.me -d v02.mydomain.me -d v03.mydomain.me -d v04.mydomain.me -d v05.mydomain.me -d v06.mydomain.me -d v07.mydomain.me -d v08.mydomain.me -d v09.mydomain.me --fullchainpath /etc/v2ray/v2ray.crt --keypath /etc/v2ray/v2ray.key --ecc
```

#### RSA 证书

```
$ sudo ~/.acme.sh/acme.sh --installcert -d mydomain.me --fullchainpath /etc/v2ray/v2ray.crt --keypath /etc/v2ray/v2ray.key
```
或者同时将子域名证书和密钥安装到 /etc/v2ray 中：
```
$ sudo ~/.acme.sh/acme.sh --installcert -d mydomain.me -d www.mydomain.me -d v01.mydomain.me -d v02.mydomain.me -d v03.mydomain.me -d v04.mydomain.me -d v05.mydomain.me -d v06.mydomain.me -d v07.mydomain.me -d v08.mydomain.me -d v09.mydomain.me --fullchainpath /etc/v2ray/v2ray.crt --keypath /etc/v2ray/v2ray.key
```

**注意：无论什么情况，密钥(即上面的v2ray.key)都不能泄漏，如果你不幸泄漏了密钥，可以使用 acme.sh 将原证书吊销，再生成新的证书，吊销方法请自行参考 acme.sh 的手册**

## 配置 V2Ray

### 服务器

```javascript
{
  "inbounds": [
    {
      "port": 443, // 建议使用 443 端口
      "protocol": "vmess",    
      "settings": {
        "clients": [
          {
            "id": "23ad6b10-8d1a-40f7-8ad0-e3e35cd38297",  
            "alterId": 0
          }
        ]
      },
      "streamSettings": {
        "network": "tcp",
        "security": "tls", // security 要设置为 tls 才会启用 TLS
        "tlsSettings": {
          "certificates": [
            {
              "certificateFile": "/etc/v2ray/v2ray.crt", // 证书文件
              "keyFile": "/etc/v2ray/v2ray.key" // 密钥文件
            }
          ]
        }
      }
    }
  ],
  "outbounds": [
    {
      "protocol": "freedom",
      "settings": {}
    }
  ]
}
```

### 客户端

```javascript
{
  "inbounds": [
    {
      "port": 1080,
      "protocol": "socks",
      "sniffing": {
        "enabled": true,
        "destOverride": ["http", "tls"]
      },
      "settings": {
        "auth": "noauth"
      }
    }
  ],
  "outbounds": [
    {
      "protocol": "vmess",
      "settings": {
        "vnext": [
          {
            "address": "mydomain.me", // tls 需要域名，所以这里应该填自己的域名。如果前面配置了子域名，可以使用其中一个子域名，子域名被封可换另一个子域名
            "port": 443,
            "users": [
              {
                "id": "23ad6b10-8d1a-40f7-8ad0-e3e35cd38297",
                "alterId": 0
              }
            ]
          }
        ]
      },
      "streamSettings": {
        "network": "tcp",
        "security": "tls" // 客户端的 security 也要设置为 tls
      }
    }
  ]
}
```

## 验证

一般来说，按照以上步骤操作完成，V2Ray 客户端能够正常联网说明 TLS 已经成功启用。但要是有个可靠的方法来验证是否正常开启 TLS 无疑更令人放心。
验证的方法有很多，我仅介绍一种小白化一点的，便是 [Qualys SSL Labs's SSL Server Test](https://www.ssllabs.com/ssltest/index.html)。

**注意：使用 Qualys SSL Labs's SSL Server Test 要求使用 443 端口，意味着你服务器配置的 inbound.port 应当是 443**

打开 [Qualys SSL Labs's SSL Server Test](https://www.ssllabs.com/ssltest/index.html)，在
Hostname 中输入你的域名，点提交，过一会结果就出来了。
![](/resource/images/tls_test1.png)

![](/resource/images/tls_test2.png)
这是对于你的 TLS/SSL 的一个总体评分，我这里评分为 A，看来还不错。有这样的界面算是成功了。

![](/resource/images/tls_test3.png)
这是关于证书的信息。从图中可以看出，我的这个证书有效期是从 2016 年 12 月 27 号到 2017 年的 3 月 27 号，密钥是 256 位的 ECC，证书签发机构是 Let's Encrypt，重要的是最后一行，`Trusted` 为 `Yes`，表明我这个证书可信。

## 温馨提醒

**V2Ray 的 TLS 不是伪装或混淆，这是完整、真正的 TLS。因此才需要域名和证书。下一篇我们将要介绍的 [V2ray WS(WebSocket)](https://github.com/bannedbook/fanqiang/blob/master/v2ss/V2Ray%E4%B9%8BTLS%2BWebSocket%E7%BF%BB%E5%A2%99%E6%96%B9%E6%B3%95.md) 也不是伪装。因此，V2ray搭建好后还可以套CDN【详见：[V2Ray之TLS+WebSocket+Nginx+CDN配置方法](https://github.com/bannedbook/fanqiang/blob/master/v2ss/V2Ray%E4%B9%8BTLS+WebSocket+Nginx+CDN%E9%85%8D%E7%BD%AE%E6%96%B9%E6%B3%95.md)】，套CDN不是一个必须的步骤，但套CDN可以有效保护IP，甚至被墙的ip也能复活，如果套国内CDN，据说速度可以飞起来。**  


-----
本文fork修订自 ToutyRater 的教程，感谢ToutyRater。
