# 如何使用 v2rayN 配置链式代理

## 温馨提示

如需通过已有翻墙节点再连接 SOCKS/HTTP 代理，可按以下教程配置链式代理；在将 SOCKS/HTTP 代理集成到 v2rayN 之前，请先向代理服务商获取地址、端口、用户名、密码等参数。

## 一、前置准备

1. 请确保您的 [v2rayN](https://github.com/fqfqgo/v2rayN/releases) 为 7.15.7 或更高版本
2. 已获取 SOCKS 或 HTTP 代理参数（包括地址、端口、用户名、密码）
3. 本机网络连接稳定

## 二、详细配置教程

### 1. 创建订阅分组

打开 [v2rayN](https://github.com/bannedbook/fanqiang/blob/master/windows/V2RayN.md) 客户端进入主界面，单击【订阅分组】选项，新建以下 3 个分组：翻墙节点、链式代理、住宅节点（用于存放 SOCKS/HTTP）。

![订阅分组：翻墙节点、链式代理、住宅节点](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/v2rayn-proxy-chain01.png)

### 2. 配置翻墙节点

进入翻墙节点分组，根据情况，使用订阅链接或节点 URL 导入你的节点，或者手工添加节点都可以。

**推荐：**  
[![V2free翻墙-不限流量、高速稳定、性价比超强](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/v2free.jpg)](https://github.com/bannedbook/fanqiang/wiki/V2ray%E6%9C%BA%E5%9C%BA)

### 3. 配置 SOCKS/HTTP 住宅节点

基于已创建的住宅节点分组，点击【配置项】添加代理配置。协议支持 HTTP / SOCKS5，可按自身需求选择。

此处为 SOCKS5 协议实操演示。

![配置项中添加 SOCKS 或 HTTP](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/v2rayn-proxy-chain02.png)

在弹窗中分别对应填入代理服务商提供的参数：地址、端口、用户名、密码，点击【确定】保存配置。

![填写 SOCKS 地址、端口、用户名、密码](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/v2rayn-proxy-chain03.png)

![SOCKS/HTTP 住宅节点已添加完成](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/v2rayn-proxy-chain04.png)

### 4. 配置链式代理

点击【链式代理】选项，进入链式代理配置页面。在【配置项】窗口中选择添加链式代理配置。

![配置项中添加链式代理](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/v2rayn-proxy-chain05.png)

![链式代理配置窗口](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/v2rayn-proxy-chain06.png)

右键单击空白区域，在弹出窗口中选择“添加子项”，并依次将配置好的翻墙节点、SOCKS/HTTP 住宅节点添加至链式列表，最后点击确定。

![添加子项](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/v2rayn-proxy-chain07.png)

![子项列表确认保存](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/v2rayn-proxy-chain08.png)

## 三、节点使用测试

完成上述配置后，测试节点连接状态，确保代理可正常使用：

1. 在【链式代理】页面，选中配置好的链式节点，并将其设为活动
2. 等待测试完成，若该节点显示延迟时间（如图“201ms”）则表示住宅节点连接正常、可用
3. 在该页面找到【Tun 模式】按钮，点击启用

![将链式节点设为活动](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/v2rayn-proxy-chain09.png)

![启用 Tun 并查看延迟](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/v2rayn-proxy-chain10.png)

## 四、注意事项

1. 定期更新 v2rayN 客户端至最新版本，提升连接稳定性和安全性；
2. 若遇到配置问题，请先核对协议类型（SOCKS5 / HTTP）与参数是否填写正确。

所有操作完成后，即可通过 v2rayN 使用链式代理访问 SOCKS/HTTP 代理服务。
