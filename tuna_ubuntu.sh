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
	xchm \

if ! which code >/dev/null 2>&1; then
	sudo snap install --classic code
fi

#
# Tune software
#

cat > ~/.tmux.conf << EOF
set -g mouse on
EOF

cat > ~/.vimrc << EOF
filetype plugin on
set bg=dark
set hlsearch
set incsearch
set ai
set ic
EOF

mkdir -p ~/.vim/ftdetect
mkdir -p ~/.vim/ftplugin

cat > ~/.vim/ftdetect/cpp.vim << EOF
autocmd BufRead,BufNewFile *.hpp setfiletype cpp
autocmd BufRead,BufNewFile *.cpp setfiletype cpp
autocmd BufRead,BufNewFile *.cc setfiletype cpp
EOF

cat > ~/.vim/ftplugin/cpp.vim << EOF
setlocal autoindent
setlocal expandtab
setlocal shiftwidth=4
setlocal softtabstop=4
setlocal tabstop=4
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
