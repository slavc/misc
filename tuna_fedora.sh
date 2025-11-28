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

# Install packages

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
	gnome-shell-extension-freon \
	golang \
	google-droid-fonts-all \
	gotags \
	iperf3 \
	libva \
	libva-utils \
	llvm \
	nmap \
	nmon \
	minicom \
	openssl \
	python3-matplotlib \
	python3-numpy \
	python3-pip \
	s-tui \
	sdcc \
	sdcc-libc-sources \
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

# Visual Studio Code

if ! which code >/dev/null 2>&1; then
	sudo rpm --import https://packages.microsoft.com/keys/microsoft.asc
	sudo sh -c 'echo -e "[code]\nname=Visual Studio Code\nbaseurl=https://packages.microsoft.com/yumrepos/vscode\nenabled=1\ngpgcheck=1\ngpgkey=https://packages.microsoft.com/keys/microsoft.asc" > /etc/yum.repos.d/vscode.repo'
	sudo dnf -y check-update
	sudo dnf -y install code
fi

code --install-extension golang.go
code --install-extension ms-python.python
code --install-extension redhat.vscode-yaml
code --install-extension ms-vscode.cpptools
code --install-extension 42crunch.vscode-openapi
code --install-extension stkb.rewrap # rewrap text to 80 columns with Alt+q
code --install-extension ms-vscode-remote.remote-ssh

# Tune desktop

for x in \
	'org.gnome.settings-daemon.plugins.power idle-dim false' \
	'org.gnome.desktop.session idle-delay 600' \
	'org.gnome.desktop.interface clock-show-weekday true' \
	'org.gnome.settings-daemon.plugins.color night-light-enabled true' \
	'org.gnome.desktop.wm.preferences focus-mode mouse'
do
		eval gsettings set $x
done

# Tune various software

cat > ~/.tmux.conf << EOF
set -g mouse on
EOF

cat > ~/.vimrc << EOF
set bg=dark
set hlsearch
set incsearch
set ai
set ic
EOF

git config --global user.email 'sviatoslav.chagaev@gmail.com'
git config --global user.name 'Sviatoslav Chagaev'
git config --global core.editor vim

if ! [ -f ~/.ssh/id_rsa ]; then
	ssh-keygen -q -f ~/.ssh/id_rsa -C 'sviatoslav.chagaev@gmail.com'
fi

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
