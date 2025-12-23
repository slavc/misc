#!/bin/bash
#
# Tune a fresh installation of Ubuntu.
#

set -e # exit with error if any command in this script fails
# set -v # print commands being executed

fatal() {
	echo "${0##*/}: error: $@"
	exit 1
}

if [ `id -u` -eq 0 ]; then
	fatal this script must be run as the target user, not root
fi

if ! sudo true; then
	exit $?
fi

#
# Install packages
#

sudo apt -y update
sudo apt -y full-upgrade
sudo apt -y autoremove
sudo apt -y install \
	clang \
	cscope \
	fonts-terminus \
	gcc \
	gdb \
	git \
	gnome-shell-extensions \
	gnome-tweaks \
	golang \
	llvm \
	lm-sensors \
	mdadm \
	minicom \
	nmap \
	nmon \
	nvidia-vaapi-driver \
	python3-matplotlib \
	python3-numpy \
	s-tui \
	sshfs \
	subversion \
	tmux \
	transmission-gtk \
	unbound \
	universal-ctags \
	vdpauinfo \
	vim \
	vim-gtk3 \
	virt-manager \
	vlc \
	wireshark \
	wxmaxima \
	xchm \
	xclip \

if ! which code >/dev/null 2>&1; then
	sudo snap install --classic code
fi

#
# Use local Unbound as the DNS server.
#

sudo tee /etc/unbound/unbound.conf.d/99-local.conf >/dev/null << EOF
server:

prefetch: yes
harden-dnssec-stripped: yes
cache-max-ttl: 14400
cache-min-ttl: 11000
aggressive-nsec: yes
hide-identity: yes
hide-version: yes
use-caps-for-id: yes

num-threads: 4
msg-cache-slabs: 8
rrset-cache-slabs: 8
infra-cache-slabs: 8
key-cache-slabs: 8
rrset-cache-size: 256m
msg-cache-size: 128m
so-rcvbuf: 8m
EOF

sudo systemctl enable unbound.service
sudo systemctl restart unbound.service

sudo sed -i 's/^#DNS=.*/DNS=127.0.0.1 ::1/g' /etc/systemd/resolved.conf
sudo sed -i 's/^#DNSSEC=.*/DNSSEC=yes/g' /etc/systemd/resolved.conf

sudo systemctl restart systemd-resolved

#
# Fix screen tearing when playing video on X.org + Nvidia.
# 

if [ -d /etc/X11/xorg.conf.d/ ] && lsmod | grep '\<nvidia>' >/dev/null; then
	sudo tee /etc/X11/xorg.conf.d/99-no-screen-tearing.conf >/dev/null << EOF
Section "Monitor"
    # HorizSync source: edid, VertRefresh source: edid
    Identifier     "Monitor0"
    VendorName     "Unknown"
    ModelName      "Samsung LU28R55"
    HorizSync       30.0 - 135.0
    VertRefresh     40.0 - 60.0
    Option         "DPMS"
EndSection

Section "Device"
    Identifier     "Device0"
    Driver         "nvidia"
    VendorName     "NVIDIA Corporation"
    BoardName      "NVIDIA GeForce GTX 1080"
EndSection

Section "Screen"
    Identifier     "Screen0"
    Device         "Device0"
    Monitor        "Monitor0"
    DefaultDepth    24
    Option         "Stereo" "0"
    Option         "nvidiaXineramaInfoOrder" "HDMI-0"
    Option         "metamodes" "nvidia-auto-select +0+0 {ForceCompositionPipeline=On, ForceFullCompositionPipeline=On}"
    Option         "SLI" "Off"
    Option         "MultiGPU" "Off"
    Option         "BaseMosaic" "off"
    SubSection     "Display"
        Depth       24
    EndSubSection
EndSection
EOF
fi
