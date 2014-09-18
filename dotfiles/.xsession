#!/bin/sh

#pgrep gnome-settings-daemon > /dev/null || /usr/libexec/gnome-settings-daemon &
xset +fp /usr/share/fonts/artwiz-aleczapka
xrdb -merge ~/.Xresources
xrdb -merge ~/.Xresources_solarized_dark

{
    sleep 6
    pulseaudio --start &
    xset dpms 0 0 600
    xrdb -merge ~/.Xresources.local
    setxkbmap -layout lv,ru -variant ,winkeys -option grp:caps_toggle -option grp_led:caps
    xset b off
    xset r rate 400 50
    xmodmap ~/.Xmodmap
    sleep 2
    xsetroot -solid black
    xsetbg -screen all -fill ~/Misc/wallpapers/city-hazy-blurred-unsharp-night-rain-1920x1080.jpg
    pgrep mpd >/dev/null 2>&1 || mpd >/dev/null 2>&1 &
} &

eval `ssh-agent`
ssh-add ~/.ssh/*_rsa

${HOME}/src/cwm/cwm || xterm

ssh-add -D
ssh-agent -k
