xsetroot -solid black
setxkbmap -layout lv,ru -variant ,winkeys -option grp:caps_toggle -option grp_led:caps
xmodmap ~/.Xmodmap
xset b off

eval `ssh-agent`
ssh-add ~/.ssh/*_rsa

~/src/cwm/cwm || xterm

ssh-add -D
ssh-agent -k
