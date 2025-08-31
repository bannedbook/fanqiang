### OpenWRT 软路由，如何下载安装SSR plus 插件

fw876/helloworld 仓库是一个 OpenWRT 的自定义软件包集合，其中包含 SSR Plus（ShadowsocksR Plus+）等插件（如 luci-app-ssr-plus）。这个仓库主要用于将插件集成到 OpenWRT 的构建系统中，而不是直接在运行中的路由器上安装 IPK 文件。如果你想在已运行的 OpenWRT 软路由上安装，需要先确保你的系统支持自定义 feeds，或者通过编译自定义固件来集成插件。

**前提条件：**
- 你的 OpenWRT 系统已安装，并有 SSH 或终端访问权限。
- 安装 clang（如果需要编译某些组件）：使用 `opkg update && opkg install clang`（如果 opkg 支持）。
- 如果你的 OpenWRT 版本是 21.02 或更低，需要手动升级 Golang 工具链到 1.21 或更高版本，以编译 Xray-core（SSR Plus 可能依赖它）。你可以从 OpenWRT 官方文档或源代码中获取升级方法。
- 这些方法假设你在 OpenWRT 的源代码目录下操作（如在构建环境中）。如果你是新手，建议先备份系统。

仓库提供了三种集成方法，我会逐一解释。选择一种适合你的方式（推荐方法 3，作为 feed 添加，最简单）。安装后，需要运行 `make menuconfig` 选择插件（如 Network > luci-app-ssr-plus），然后编译固件（`make -j$(nproc)`）并刷入路由器，或者如果有预编译 IPK，可以直接 opkg 安装（但仓库未提供发布版，需要自己构建）。

#### 方法 1：直接克隆仓库
1. 移除旧的 helloworld 目录（如果存在）：
   ```
   rm -rf package/helloworld
   ```
2. 克隆仓库：
   ```
   git clone --depth=1 https://github.com/fw876/helloworld.git package/helloworld
   ```
3. 更新上游提交（以后需要更新时运行）：
   ```
   git -C package/helloworld pull
   ```
4. 移除仓库（如果不再需要）：
   ```
   rm -rf package/helloworld
   ```

#### 方法 2：添加为 Git 子模块
1. 移除旧的 helloworld 目录（如果存在）：
   ```
   rm -rf package/helloworld
   ```
2. 添加子模块：
   ```
   git submodule add -f --name helloworld https://github.com/fw876/helloworld.git package/helloworld
   ```
3. 更新上游提交（以后需要更新时运行）：
   ```
   git submodule update --remote package/helloworld
   ```
4. 移除子模块（如果不再需要）：
   ```
   git submodule deinit -f package/helloworld
   git rm -f package/helloworld
   git reset HEAD .gitmodules
   rm -rf .git/modules{/,/package/}helloworld
   ```

#### 方法 3：添加为 OpenWRT Feed（推荐，适合集成到 feeds.conf）
1. 编辑 feeds.conf.default 文件，移除旧的 helloworld 行（如果存在）：
   ```
   sed -i "/helloworld/d" "feeds.conf.default"
   ```
2. 添加新 feed：
   ```
   echo "src-git helloworld https://github.com/fw876/helloworld.git" >> "feeds.conf.default"
   ```
3. 更新 feed 并安装：
   ```
   ./scripts/feeds update helloworld
   ./scripts/feeds install -a -f -p helloworld
   ```
4. 移除 feed（如果不再需要）：
   ```
   sed -i "/helloworld/d" "feeds.conf.default"
   ./scripts/feeds clean
   ./scripts/feeds update -a
   ./scripts/feeds install -a
   ```

**安装后步骤：**
- 运行 `make menuconfig`，在菜单中选择 SSR Plus 相关插件（通常在 LuCI > Applications 或 Network 类别下，搜索 ssr-plus）。
- 编译固件：`make -j$(nproc)`（使用多核加速）。
- 下载生成的 bin 文件，刷入路由器（通过 sysupgrade）。
- 在路由器 Web 界面（LuCI）中配置 SSR Plus：添加服务器、设置代理规则等。

**注意事项：**
- 这个仓库没有发布预编译的包（Releases 为空），所以需要自己构建。如果你是初学者，建议参考 OpenWRT 官方文档（openwrt.org）学习如何构建自定义固件。
- 如果遇到编译错误（如依赖缺失），检查 Golang 版本或安装缺失的包（如 `opkg install golang`）。
- 如果需要更多细节，可以查看仓库的 Makefile 或子目录中的具体插件代码。

**相关文章：**

推荐免费翻墙软件：<br>
<a href="https://github.com/bannedbook/fanqiang/wiki/%E5%AE%89%E5%8D%93%E7%BF%BB%E5%A2%99%E8%BD%AF%E4%BB%B6">安卓手机翻墙</a>、<a href="https://github.com/bannedbook/fanqiang/wiki/Chrome%E4%B8%80%E9%94%AE%E7%BF%BB%E5%A2%99%E5%8C%85" class="wiki-page-link">Chrome一键翻墙包</a>、<a href="https://github.com/bannedbook/fanqiang/tree/master/EdgeGo" target="_blank">EdgeGo-Edge一键翻墙包</a>、<a href="https://github.com/bannedbook/fanqiang/wiki/%E7%81%AB%E7%8B%90firefox%E4%B8%80%E9%94%AE%E7%BF%BB%E5%A2%99%E5%8C%85" class="wiki-page-link">火狐firefox一键翻墙包</a><br>

<b>或者也可以购买现成的翻墙服务(跟本库无关哦)：</b><br>
<a href="https://github.com/bannedbook/fanqiang/wiki/V2ray%E6%9C%BA%E5%9C%BA"><img src="https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/v2free.jpg" height="300" alt="V2free翻墙-不限流量、高速稳定、性价比超强"></a>
