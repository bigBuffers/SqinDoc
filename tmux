如何在tmux中scrollback page
1.linux普通模式翻屏（翻页）：shift+PgUp或者shift+PgDn
2.tmux模型下翻屏（翻页）：C-b pageup/pagedown
3 按q退出

修改tmux scrollback 缓存大小命令
1 open ～/.tmux.conf
2 set-option -g history-limit 3000

Tmux 重要概念
第一，Tmux中，千万不要去背和记长度超过1个字母的命令！所有都按照自己的顺手程度，在.tmux.conf配置文件中绑定快捷键，甚至窗口改变大小的命令也不用记，只需改为用鼠标调整即可。
第二，在Tmux逻辑中，分清楚Server > Session > Window > Pane这个大小和层级顺序是极其重要的，直接关系到工作效率：
    Server：是整个tmux的后台服务。有时候更改配置不生效，就要使用tmux kill-server来重启tmux。
    Session：是tmux的所有会话。我之前就错把这个session当成窗口用，造成了很多不便里。一把只要保存一个session就足够了。
    Window：相当于一个工作区，包含很多分屏，可以针对每种任务分一个Window。如下载一个Window，编程一个window。
    Pane：是在Window里面的小分屏。最常用也最好用。

Tmux常用命令参考
#启动新session：
$ tmux [new -s 会话名 -n 窗口名]

#恢复session：
$ tmux at [-t 会话名]

#列出所有sessions：
$ tmux ls

#关闭session：
$ tmux kill-session -t 会话名

#关闭整个tmux服务器：
$ tmux kill-server

Tmux 常用内部命令

    所谓内部命令，就是进入Tmux后，并按下前缀键后的命令，一般前缀键为Ctrl+b. 虽然ctrl和b离得很远但是不建议改前缀键，因为别的键也不见得方便好记不冲突。还是记忆默认的比较可靠。

    刷新配置文件：<前缀键>r
    下载和更新Plugins：<前缀键>I
    Window 窗体：
        关闭当前Window: <前缀键>&
        创建新Window: <前缀键>c
        列出所有Windows: <前缀键>w
        后一个Window: <前缀键>n
        前一个Window: <前缀键>p
        重命名当前Window: <前缀键>,
        修改当前Window位置（序号）：.
    Pane 小面板：
        关闭当前Pane: <前缀键>x
        上下分割Pane: <前缀键>%
        左右分割Pane: <前缀键>"
        最大化/最小化 Pane: <前缀键>z
        显示每个Pane的编号，可以按下数字键选中Pane: <前缀键>q
        与上一个窗格交换位置: <前缀键>{
        与下一个窗格交换位置: <前缀键>}
    Session 会话:
        启动新会话: <前缀键>:new<回车>
        列出所有会话: <前缀键>s
        重命名当前会话: <前缀键>$
