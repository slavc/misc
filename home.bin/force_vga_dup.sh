#!/bin/sh

xrandr --output VGA-0 --off
xrandr --output HDMI-0 --off
# values obtained using cvt(1)
xrandr --newmode my_mode 118.25  1600 1696 1856 2112  900 903 908 934 -hsync +vsync
xrandr --addmode VGA-0 my_mode
xrandr --addmode HDMI-0 my_mode
xrandr --output VGA-0 --same-as LVDS --mode my_mode
xrandr --output HDMI-0 --same-as LVDS --mode my_mode

# 1600x900 59.95 Hz (CVT 1.44M9) hsync: 55.99 kHz; pclk: 118.25 MHz
#Modeline "1600x900_60.00"  
