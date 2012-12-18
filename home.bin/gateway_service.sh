#!/bin/sh

wifi_if=wlan0
ext_if=em1
int_if=p3p1

function setup_nat() {
    ext_if=$1
    int_if=$2
    iptables -t nat -A POSTROUTING -o $ext_if -j MASQUERADE
    iptables -A FORWARD -i $ext_if -o $int_if -m state --state RELATED,ESTABLISHED -j ACCEPT
    iptables -A FORWARD -i $int_if -o $ext_if -j ACCEPT
}

function stop() {
    systemctl stop dnsmasq.service
    systemctl stop hostapd.service

    iptables -F
    sysctl net.ipv4.conf.all.forwarding=0
    sysctl net.ipv6.conf.all.forwarding=0

    ifconfig $int_if down
    ifconfig $wifi_if down
}

function start() {
    stop

    ifconfig $int_if inet 192.168.0.1 netmask 255.255.255.0
    ifconfig $wifi_if inet 192.168.1.1 netmask 255.255.255.0

    sysctl net.ipv4.conf.all.forwarding=1
    sysctl net.ipv6.conf.all.forwarding=1
    iptables -F
    setup_nat $ext_if $int_if
    setup_nat $ext_if $wifi_if
    iptables -A INPUT -i $ext_if -p tcp --dport domain -j REJECT

    systemctl start dnsmasq.service
    systemctl start hostapd.service
}

function restart() {
    stop
    start
}

function handle_int() {
    stop
    exit 0
}

case $1 in
start|stop|restart)
    $1
    exit $?
    ;;
*)
    start
    if getopts D opt; then
        # Stay in foreground, stop the service when script is terminated
        trap handle_int INT
        trap handle_int QUIT
        trap handle_int TERM
        trap handle_int HUP
        while true; do
            sleep 60
        done
    fi
    ;;
esac


