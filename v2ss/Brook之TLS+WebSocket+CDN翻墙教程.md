# Brook之TLS+WebSocket+CDN翻墙教程

如果你的VPS IP 被墙了，或者你直接连接VPS的速度不理想，可以试试基于 [Brook之TLS+WebSocket翻墙](https://github.com/bannedbook/fanqiang/blob/master/v2ss/Brook%E4%B9%8BTLS+WebSocket%E7%BF%BB%E5%A2%99%E6%95%99%E7%A8%8B.md) 或者
[Brook之TLS+WebSocket+Web服务器翻墙](https://github.com/bannedbook/fanqiang/blob/master/v2ss/Brook%E4%B9%8BTLS%2BWebSocket%2BWeb%E7%BF%BB%E5%A2%99%E6%95%99%E7%A8%8B.md) 基础上，再套上Cloudflare CDN，可以复活被墙VPS，也许速度也会不一样哦。  

套CDN不要求装Web服务器，可以装Web服务器，也可以不装。

把你的域名添加到Cloudflare ，确保在Cloudflare DNS 设置处启用CDN，就是Proxy Status 那一列，那个金黄金黄的云彩图标要点亮。

另外，在 Cloudflare 的 SSL/TLS 设置中启用 Full 模式，并关掉 TLS 1.3 。

大概这样就可以了。等待一会儿，ping 域名试试看，如果ping 出的ip变成了 Cloudflare ip，那么我们已经成功给我们的域名套上了Cloudflare CDN。

客户端启动命令行不变，跟前文一致。
如果是原先被墙的vps，现在应该已经复活了！

## 相关翻墙教程


* [Brook之TLS+WebSocket+CDN翻墙教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/Brook%E4%B9%8BTLS%2BWebSocket%2BCDN%E7%BF%BB%E5%A2%99%E6%95%99%E7%A8%8B.md)

* [Brook之TLS+WebSocket+Web服务器翻墙教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/Brook%E4%B9%8BTLS%2BWebSocket%2BWeb%E7%BF%BB%E5%A2%99%E6%95%99%E7%A8%8B.md)

* [Brook之TLS+WebSocket翻墙教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/Brook%E4%B9%8BTLS%2BWebSocket%E7%BF%BB%E5%A2%99%E6%95%99%E7%A8%8B.md)

* <a href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAV2ray%E6%9C%8D%E5%8A%A1%E5%99%A8%E7%AE%80%E6%98%8E%E6%95%99%E7%A8%8B.md">自建V2ray服务器简明教程</a>

* <a href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAV2Ray%2BTLS%E7%BF%BB%E5%A2%99%E9%85%8D%E7%BD%AE%E6%96%B9%E6%B3%95.md">自建V2Ray+TLS翻墙配置方法</a>

* <a href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/V2Ray%E4%B9%8BTLS%2BWebSocket%E7%BF%BB%E5%A2%99%E6%96%B9%E6%B3%95.md">V2Ray之TLS+WebSocket翻墙方法</a>

* <a title="自建Shadowsocks服务器简明教程" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAShadowsocks%E6%9C%8D%E5%8A%A1%E5%99%A8%E7%AE%80%E6%98%8E%E6%95%99%E7%A8%8B.md">自建Shadowsocks服务器简明教程</a>

* <a title="SSH连接VPS教程" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/SSH%E8%BF%9E%E6%8E%A5VPS%E6%95%99%E7%A8%8B.md">SSH连接VPS教程</a>

* <a title="V2ray官方一键安装脚本" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/V2ray%E5%AE%98%E6%96%B9%E4%B8%80%E9%94%AE%E5%AE%89%E8%A3%85%E8%84%9A%E6%9C%AC.md">V2ray官方一键安装脚本</a>

* <a title="Windows版V2ray客户端安装配置指南" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/Windows%E7%89%88V2ray%E5%AE%A2%E6%88%B7%E7%AB%AF%E5%AE%89%E8%A3%85%E9%85%8D%E7%BD%AE%E6%8C%87%E5%8D%97.md">Windows版V2ray客户端安装配置指南.md</a>

* <a title="使用FileZilla和VPS传输文件教程" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E4%BD%BF%E7%94%A8FileZilla%E5%92%8CVPS%E4%BC%A0%E8%BE%93%E6%96%87%E4%BB%B6%E6%95%99%E7%A8%8B.md">使用FileZilla和VPS传输文件教程</a>

* <a title="最简单的Google BBR 一键加速VPS教程" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E6%9C%80%E7%AE%80%E5%8D%95%E7%9A%84Google%20BBR%20%E4%B8%80%E9%94%AE%E5%8A%A0%E9%80%9FVPS%E6%95%99%E7%A8%8B.md">最简单的Google BBR 一键加速VPS教程</a>

* <a title="翻墙VPS推荐：搬瓦工VPS购买教程" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E7%BF%BB%E5%A2%99VPS%E6%8E%A8%E8%8D%90%EF%BC%9A%E6%90%AC%E7%93%A6%E5%B7%A5VPS%E8%B4%AD%E4%B9%B0%E6%95%99%E7%A8%8B.md">翻墙VPS推荐：搬瓦工VPS购买教程</a>

* <a  href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%B4%AD%E4%B9%B0Vultr%20VPS%E5%9B%BE%E6%96%87%E6%95%99%E7%A8%8B.md">购买Vultr VPS图文教程【新用户赠送100美元】</a>
