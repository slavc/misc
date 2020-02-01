#!/bin/sh

set -e

if ip link show dev wlan1 | grep 00:01:02 >/dev/null; then
	int_if=wlan1
	ext_if=wlan2
else
	int_if=wlan2
	ext_if=wlan1
fi

# nmcli d set wlan1 managed no
sed -i 's/[=]wlan.*/='${int_if}'/g' /etc/hostapd/hostapd.conf
systemctl start hostapd
ip addr flush dev $int_if
ip addr add 192.168.0.1/24 dev $int_if
ip link set up dev $int_if
pkill -9 dnsmasq || true
dnsmasq
sysctl net.ipv4.conf.all.forwarding=1 > /dev/null 2>&1
iptables -t nat -F
iptables -t nat -A POSTROUTING -o $ext_if -j MASQUERADE
