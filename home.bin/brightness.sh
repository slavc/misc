#!/bin/sh

max_path=/sys/devices/pci0000:00/0000:00:01.0/backlight/acpi_video0/max_brightness
val_path=/sys/devices/pci0000:00/0000:00:01.0/backlight/acpi_video0/brightness

function dec() {
    val=`cat $val_path`
    if [ $val -gt 0 ]; then
        echo $(($val-1)) > $val_path
    fi
}

function inc() {
    val=`cat $val_path`
    max=`cat $max_path`
    if [ $val -lt $max ]; then
        echo $(($val+1)) > $val_path
    fi
}

function print() {
    val=`cat $val_path`
    echo $val
}

case "$1" in
-)
    dec
    ;;
+)
    inc
    ;;
*)
    print
    ;;
esac
