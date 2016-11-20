#!/bin/sh

xrdb -merge ~/.Xresources
xrdb -merge ~/.Xresources_solarized_dark
xrdb -merge ~/.Xresources.local
xset dpms 0 0 600
setxkbmap -layout lv,ru -variant ,winkeys -option grp:caps_toggle -option grp_led:caps
xset b off
xset r rate 400 50
xmodmap ~/.Xmodmap
synclient `cat ~/.synclient`
xinput set-prop 13 291 1 # air: enable tapping
xinput set-prop 13 279 1 # air: enable natural scrolling
xsetroot -solid black
#display -window root ~/misc/wallpapers/sombrero.png
xset +fp /usr/local/share/fonts/artwiz-aleczapka
conky -x 0 -y 0 -w `xdpyinfo | awk '/root window id/{ print $4 }'`

eval `ssh-agent`
ssh-add ~/.ssh/*_rsa

${HOME}/src/cwm/cwm || cwm || xterm

ssh-add -D
ssh-agent -k

