#!/bin/sh

xrdb -merge ~/.Xresources
xrdb -merge ~/.Xresources_solarized_light
xrdb -merge ~/.Xresources.local
xset dpms 0 0 600
setxkbmap -layout lv,ru -variant ,winkeys -option grp:caps_toggle -option grp_led:caps
xset b off
xset r rate 400 50
xmodmap ~/.Xmodmap
xsetroot -solid black
synclient `cat ~/.synclient`

eval `ssh-agent`
ssh-add ~/.ssh/*_rsa

${HOME}/src/cwm/cwm || cwm || xterm

ssh-add -D
ssh-agent -k
