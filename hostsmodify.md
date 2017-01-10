# 修改hosts文件的方法大全

为什么要修改hosts文件呢，我们这里修改hosts文件的目的主要是为了<a href="https://github.com/bannedbook/fanqiang/wiki/hosts%E7%BF%BB%E5%A2%99" target="_blank">hosts翻墙</a>。

<h2>修改hosts文件-方法一</h2>
1.找到Hosts文件，将Hosts文件复制到桌面。（Windows7系统Hosts文件路径为：C:\WINDOWS\system32\drivers\etc\hosts）

2.用记事本打开，修改内容，然后保存。

3.将修改好的Hosts文件再复制回Hosts文件目录，提示是否覆盖时选择覆盖即可。

通过这个方式可以解决提示无权修改Hosts文件，以后需要修改Hosts时就不再需要再复制到桌面了，因为文件属性已经修改，以后只需要在Hosts目录下修改即可


<h2>修改hosts文件-方法二</h2>

用文本编辑器打开hosts文件，将以下内容复制进去，保存即可（hosts 文件没有后缀）<br/>

<br/>Windows 用户可以以管理员身份直接运行 notepad "%SystemRoot%\system32\drivers\etc\hosts" 进行 编辑

<br/>Linux 和Mac用户用文本编辑器（需要root权限）修改 /etc/hosts 即可开始编辑

<br/>关闭某个IPv6的转发请在那一行的最前面添加#号，启用请去除最前面#号，每行中间的#号是为了区分地址 和注释，不用理睬

