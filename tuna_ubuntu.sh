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
	minicom \
	nmap \
	nmon \
	nvidia-vaapi-driver \
	python3-matplotlib \
	python3-numpy \
	s-tui \
	sshfs \
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

#
# Visual Studio Code
#

if ! which code >/dev/null 2>&1; then
	sudo snap install --classic code
fi

{
	code --install-extension golang.go ;
	code --install-extension ms-python.python ;
	code --install-extension redhat.vscode-yaml ;
	code --install-extension ms-vscode.cpptools ;
	code --install-extension 42crunch.vscode-openapi ;
	code --install-extension stkb.rewrap # rewrap text to 80 columns with Alt+q ;
	code --install-extension ms-vscode-remote.remote-ssh ;
} || true

#
# Tune desktop environment
# 

for x in \
	'org.gnome.settings-daemon.plugins.power idle-dim false' \
	'org.gnome.desktop.session idle-delay 600' \
	'org.gnome.desktop.interface clock-show-weekday true' \
	'org.gnome.settings-daemon.plugins.color night-light-enabled true' \
	'org.gnome.desktop.wm.preferences focus-mode mouse' \
	'org.gnome.desktop.interface font-antialiasing rgba' \
	"org.gnome.desktop.peripherals.touchpad tap-to-click true" 
do
		eval gsettings set $x
done

#
# Tune software
#

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

sudo cp ~/.vimrc /root/
sudo chown root:root /root/.vimrc

cat > ~/.gvimrc << EOF
set ic
set belloff=all
set nowrap

colorscheme industry

set guifont=Terminus\ (TTF)\ 12
set guioptions=aegit

set cursorline
set colorcolumn=80
highlight CursorLine guibg=grey10
highlight ColorColumn guibg=grey10
EOF

git config --global user.email 'sviatoslav.chagaev@gmail.com'
git config --global user.name 'Sviatoslav Chagaev'
git config --global core.editor vim

if ! [ -f ~/.ssh/id_rsa ]; then
	ssh-keygen -q -f ~/.ssh/id_rsa -C 'sviatoslav.chagaev@gmail.com'
fi

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

conn_id=$(nmcli -f uuid,device connection | grep enx | awk '{ print $1 }')
sudo nmcli connection modify $conn_id ipv4.ignore-auto-dns yes
sudo nmcli connection modify $conn_id ipv4.dns "127.0.0.1"
sudo nmcli connection modify $conn_id ipv6.ignore-auto-dns yes
sudo nmcli connection modify $conn_id ipv6.dns "::1"
