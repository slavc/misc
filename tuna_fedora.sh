#!/bin/bash

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

sudo dnf -y install \
	https://mirrors.rpmfusion.org/free/fedora/rpmfusion-free-release-$(rpm -E %fedora).noarch.rpm \
	https://mirrors.rpmfusion.org/nonfree/fedora/rpmfusion-nonfree-release-$(rpm -E %fedora).noarch.rpm

sudo dnf makecache

sudo dnf -y upgrade
sudo dnf -y install --allowerasing \
	clang \
	cscope \
	ctags \
	dpdk \
	dpdk-devel \
	dpdk-doc \
	dpdk-tools \
	ffmpeg \
	fuse-sshfs \
	fuse-sshfs \
	gcc \
	gdb \
	geeqie \
	gnome-extensions-app \
	gnome-shell-extension-freon \
	golang \
	google-droid-fonts-all \
	gotags \
	iperf3 \
	libva \
	libva-utils \
	llvm \
	minicom \
	nmap \
	nmon \
	openssl \
	python3-matplotlib \
	python3-numpy \
	python3-pip \
	s-tui \
	sdcc \
	sdcc-libc-sources \
	stb-devel \
	terminus-fonts \
	tmux \
	transmission \
	unbound \
	vim \
	vim-X11 \
	vim-enhanced \
	vim-go \
	vim-golint \
	virt-manager \
	vlc \
	wireshark \
	wxMaxima \


flatpak install -y jupyterlab

sudo usermod -a -G wireshark `id -n -u`

pip3 install stcgal

if lsmod | grep i915 >/dev/null 2>&1; then
	sudo dnf -y install \
		libva-intel-driver \
		libva-intel-hybrid-driver \
		gstreamer1-vaapi \
		intel-media-driver \
		igt-gpu-tools
fi

# 
# Install Visual Studio Code
# 

if ! which code >/dev/null 2>&1; then
	sudo rpm --import https://packages.microsoft.com/keys/microsoft.asc
	sudo sh -c 'echo -e "[code]\nname=Visual Studio Code\nbaseurl=https://packages.microsoft.com/yumrepos/vscode\nenabled=1\ngpgcheck=1\ngpgkey=https://packages.microsoft.com/keys/microsoft.asc" > /etc/yum.repos.d/vscode.repo'
	sudo dnf -y check-update
	sudo dnf -y install code
fi

#
# Use local Unbound as the DNS server.
#

sudo tee /etc/unbound/conf.d/99-local.conf >/dev/null << EOF
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
sudo systemctl start unbound.service

sudo rm -f /etc/NetworkManager/conf.d/90-dns-none.conf
sudo tee -a /etc/NetworkManager/conf.d/90-dns-none.conf >/dev/null <<EOF
[main]
dns=none
EOF
sudo systemctl reload NetworkManager

sudo rm -f /etc/resolv.conf
sudo tee -a /etc/resolv.conf >/dev/null << EOF
nameserver 127.0.0.1
options edns0 trust-ad
search home
EOF
