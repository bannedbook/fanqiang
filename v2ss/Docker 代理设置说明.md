# Docker 代理设置说明

Docker pull 命令在 Linux 上不会使用您在 shell 会话中设置的 http_proxy、https_proxy 或 no_proxy 环境变量，因为实际的镜像拉取操作是由 Docker 守护进程（dockerd）执行的，而不是 Docker CLI 客户端。守护进程作为后台系统服务运行（通常由 systemd 管理），并且不会从用户会话或终端继承环境变量。因此，当前环境中的任何代理设置在守护进程进行网络操作（如从注册表获取镜像）时都会被忽略。

要在基于 systemd 的 Linux 发行版（如 Ubuntu、CentOS、Fedora）上为 Docker 守护进程配置代理（影响 docker pull、docker run 和其他守护进程处理的操），请按照以下步骤操作：

1. 为 Docker 服务创建一个 drop-in 配置文件：
   ```
   sudo mkdir -p /etc/systemd/system/docker.service.d
   sudo nano /etc/systemd/system/docker.service.d/http-proxy.conf
   ```

2. 在文件中添加代理环境变量（替换为您的实际代理 URL）：
   ```
   [Service]
   Environment="HTTP_PROXY=http://your-proxy-host:port/"
   Environment="HTTPS_PROXY=https://your-proxy-host:port/"
   Environment="NO_PROXY=localhost,127.0.0.1,::1,your-internal-domains"
   ```

3. 重新加载 systemd 并重启 Docker 服务：
   ```
   sudo systemctl daemon-reload
   sudo systemctl restart docker
   ```

4. 验证更改：
   ```
   sudo systemctl show --property=Environment docker
   ```

此设置确保守护进程在出站连接中使用代理。 如果您位于公司代理后面，具有 SSL 拦截或其他复杂情况，则可能需要额外的故障排除（例如，信任自定义 CA 证书）。

对于 docker build 命令，构建过程（由 BuildKit 或守护进程处理）可能需要将代理作为构建参数传递：
```
docker build --build-arg HTTP_PROXY=http://your-proxy-host:port/ --build-arg HTTPS_PROXY=https://your-proxy-host:port/ -t your-image .
```
如果您的系统不使用 systemd（例如，较旧的 init 系统），则改为在 /etc/default/docker 中配置代理，但这在现代 Linux 上不太常见。

**相关文章：**

推荐免费翻墙软件：<br>
<a href="https://github.com/bannedbook/fanqiang/wiki/%E5%AE%89%E5%8D%93%E7%BF%BB%E5%A2%99%E8%BD%AF%E4%BB%B6">安卓手机翻墙</a>、<a href="https://github.com/bannedbook/fanqiang/wiki/Chrome%E4%B8%80%E9%94%AE%E7%BF%BB%E5%A2%99%E5%8C%85" class="wiki-page-link">Chrome一键翻墙包</a>、<a href="https://github.com/bannedbook/fanqiang/tree/master/EdgeGo" target="_blank">EdgeGo-Edge一键翻墙包</a>、<a href="https://github.com/bannedbook/fanqiang/wiki/%E7%81%AB%E7%8B%90firefox%E4%B8%80%E9%94%AE%E7%BF%BB%E5%A2%99%E5%8C%85" class="wiki-page-link">火狐firefox一键翻墙包</a><br>

<b>或者也可以购买现成的翻墙服务(跟本库无关哦)：</b><br>
<a href="https://github.com/bannedbook/fanqiang/wiki/V2ray%E6%9C%BA%E5%9C%BA"><img src="https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/v2free.jpg" height="300" alt="V2free翻墙-不限流量、高速稳定、性价比超强"></a>
