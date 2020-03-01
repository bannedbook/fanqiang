# Brook之TLS+WebSocket+CDN翻墙教程

如果你的VPS IP 被墙了，或者你直接连接VPS的速度不理想，可以试试基于 [Brook之TLS+WebSocket翻墙](https://github.com/bannedbook/fanqiang/blob/master/v2ss/Brook%E4%B9%8BTLS+WebSocket%E7%BF%BB%E5%A2%99%E6%95%99%E7%A8%8B.md) 或者
[Brook之TLS+WebSocket+Web服务器翻墙](https://github.com/bannedbook/fanqiang/blob/master/v2ss/Brook%E4%B9%8BTLS%2BWebSocket%2BWeb%E7%BF%BB%E5%A2%99%E6%95%99%E7%A8%8B.md) 基础上，再套上Cloudflare CDN，可以复活被墙VPS，也许速度也会不一样哦。  

套CDN不要求装Web服务器，可以装Web服务器，也可以不装。

把你的域名添加到Cloudflare ，确保在Cloudflare DNS 设置处启用CDN，就是Proxy Status 那一列，那个金黄金黄的云彩图标要点亮。

另外，在 Cloudflare 的 SSL/TLS 设置中启用 Full 模式，并关掉 TLS 1.3 。

大概这样就可以了。等待一会儿，ping 域名试试看，如果ping 出的ip变成了 Cloudflare ip，那么我们已经成功给我们的域名套上了Cloudflare CDN。

客户端配置不变，跟前文一致。
