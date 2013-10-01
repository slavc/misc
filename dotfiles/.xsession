pgrep gnome-settings-daemon >/dev/null || /usr/libexec/gnome-settings-daemon &
xset +fp /usr/share/fonts/artwiz-aleczapka
xrdb -load ~/.Xresources_solarized_light

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
} &

eval `ssh-agent`
ssh-add ~/.ssh/*_rsa

i3 || xterm

ssh-add -D
ssh-agent -k
