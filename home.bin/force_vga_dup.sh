#!/bin/sh

xrandr --output VGA-0 --off
# values obtained using cvt(1)
xrandr --newmode my_mode 85.25 1368 1440 1576 1784 768 771 781 798 -hsync +vsync
xrandr --addmode VGA-0 my_mode
xrandr --output VGA-0 --same-as LVDS --mode my_mode
