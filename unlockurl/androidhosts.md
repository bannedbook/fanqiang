# 安卓修改hosts文件的方法

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

直接用手机浏览器下载老D提供的hosts文件（百度网盘里的，不带.txt的）用 RE管理器 （前提需要手机已Root）复制或者移动至 /system/etc/hosts 粘貼或者覆盖即可。

 

至于iOS设备如何修改hosts，可能就需要越狱了，有些麻烦，这里暂且不提。本人目前使用的苹果设备用到的google服务不多，如果有朋友有什么好的方法，也可以留言告诉我。

转载请注明来自 老D，本文标题：Android修改hosts文件的方法介绍, 转载请保留本声明！
