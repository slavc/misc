######################################################################
#!/bin/sh
# $OpenBSD: xinitrc.cpp,v 1.10 2013/05/19 07:22:53 ajacoutot Exp $

userresources=$HOME/.Xresources
usermodmap=$HOME/.Xmodmap
sysresources=/etc/X11/xinit/.Xresources
sysmodmap=/etc/X11/xinit/.Xmodmap

# merge in defaults and keymaps

if [ -f $sysresources ]; then
    xrdb -merge $sysresources
fi

if [ -f $sysmodmap ]; then
    xmodmap $sysmodmap
fi

if [ -f "$userresources" ]; then
    xrdb -merge "$userresources"
fi

if [ -f "$usermodmap" ]; then
    xmodmap "$usermodmap"
fi

# if we have private ssh key(s), start ssh-agent and add the key(s)
id1=$HOME/.ssh/identity
id2=$HOME/.ssh/id_dsa
id3=$HOME/.ssh/id_rsa
id4=$HOME/.ssh/id_ecdsa
if [ -z "$SSH_AGENT_PID" ];
then
	if [ -x /usr/bin/ssh-agent ] && [ -f $id1 -o -f $id2 -o -f $id3 -o -f $id4 ];
	then
		eval `ssh-agent -s`
		ssh-add < /dev/null
	fi
fi

# if dbus is installed, start its daemon
if [ -x /usr/local/bin/dbus-launch -a -z "${DBUS_SESSION_BUS_ADDRESS}" ]; then
	eval `dbus-launch --sh-syntax --exit-with-session`
fi

# start some nice programs

xsetroot -solid black
xrdb -merge ~/.Xresources_solarized_dark
~/bin/xsetbg -center /home/slava/misc/wallpapers/Cute-Puffer-Fish-Desktop-Wallpaper.jpg &
xset +fp /usr/local/lib/X11/fonts/terminus
xset dpms 0 0 600
setxkbmap -layout lv,ru -variant ,winkeys -option grp:caps_toggle -option grp_led:caps
xset b off
xset r rate 400 50
$HOME/src/cwm/cwm || xterm


if [ "$SSH_AGENT_PID" ]; then
	ssh-add -D < /dev/null
	eval `ssh-agent -s -k`
fi

