# 安卓修改hosts文件的方法

为什么要修改hosts文件呢，我们这里修改hosts文件的目的主要是为了<a href="https://github.com/bannedbook/fanqiang/wiki/hosts%E7%BF%BB%E5%A2%99">hosts翻墙</a>。

本文介绍三种Android手机修改hosts文件的方法，但修改hosts文件一定要谨慎：Android手机hosts文件的换行符必须是n而不是windows的rn，使用Notepad++打开hosts文件，依次点击菜单中的“视图–显示符号–显示所有字符”，如果行末是LF就没问题，CR LF结束则需要替换所有的CR LF为LF。

Android手机hosts文件路径：/system/etc/hosts

<h2>修改hosts方法一：需重启</h2>

修改host文件首先需要Android手机获取Root权限

使用Root Explorer管理器或ES文件浏览器装载/system可写状态，找到/system/etc/hosts的文件，使用文本编辑器打开编辑后保存

保存后重启手机即可生效

<h2>修改hosts方法二：不需重启</h2>

将hosts文件拷贝到电脑，电脑端修改后复制回手机，这种方法不需要重启

<h2>修改hosts方法三：</h2>

各种android市场中寻找修改hosts的app，例如：

hosts 助手

smartHosts

<h2>修改hosts方法四：不需要重启</h2>

直接用手机浏览器下载我们提供的<strong><a href="https://raw.githubusercontent.com/kgfw/fg/master/hosts/hosts.zip">hosts文件</a></strong>
用 RE管理器 （前提需要手机已Root）复制或者移动至 /system/etc/hosts 粘貼或者覆盖即可。

参考：<a href="https://github.com/bannedbook/fanqiang/blob/master/unlockurl/hostsmodify.md">Windows及其它平台修改hosts文件的方法</a>
