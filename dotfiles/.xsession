pgrep gnome-settings-daemon >/dev/null || /usr/libexec/gnome-settings-daemon &

{
    sleep 4
    pulseaudio --start &
    xsetroot -solid black
    xset dpms 0 0 600
    xrdb -merge ~/.Xresources.local
    setxkbmap -layout lv,ru -variant ,winkeys -option grp:caps_toggle -option grp_led:caps
    xset b off
    xset r rate 400 50
    xmodmap ~/.Xmodmap
} &

eval `ssh-agent`
ssh-add ~/.ssh/*_rsa

i3 || xterm

ssh-add -D
ssh-agent -k
