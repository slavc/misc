#!/bin/bash

set -e
# set -vx # uncomment to debug the script

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
# Upgrade, install packages
#

sudo dnf makecache

sudo dnf -y upgrade
sudo dnf -y install --allowerasing \
	bmon \
	clang \
	cscope \
	ctags \
	dpdk \
	dpdk-devel \
	dpdk-doc \
	dpdk-tools \
	gcc \
	gdb \
	golang \
	gotags \
	iperf3 \
	llvm \
	minicom \
	nmap \
	nmon \
	openssl \
	perf \
	s-tui \
	tcpdump \
	tmux \
	vim \
	vim-enhanced \
	vim-go \
	vim-golint \


sudo dnf -y remove PackageKit-command-not-found

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
autocmd BufRead,BufNewFile *.slang setfiletype cpp
autocmd BufRead,BufNewFile *.cu setfiletype cpp
EOF

cat > ~/.vim/ftplugin/cpp.vim << EOF
set autoindent
set expandtab
set shiftwidth=4
set softtabstop=4
set tabstop=4
EOF

sudo cp ~/.vimrc /root/
sudo chown root:root /root/.vimrc

git config --global user.email 'sviatoslav.chagaev@gmail.com'
git config --global user.name 'Sviatoslav Chagaev'
git config --global core.editor vim

cat > ~/.my_bashrc << EOF
export EDITOR=$(which vim)
export LESS=RSi
EOF

if ! grep 'my_bashrc' ~/.bashrc >/dev/null 2>&1; then
	echo 'source ~/.my_bashrc' >> ~/.bashrc
fi

#
# Apply special settings for some machines
#

case "$HOSTNAME" in 
xinfotech | fx)
	echo Applying special settings for host $HOSTNAME...
	sudo systemctl disable firewalld
	sudo systemctl stop firewalld
	echo '%wheel        ALL=(ALL)       NOPASSWD: ALL' | sudo tee /etc/sudoers.d/passwordless-sudo >/dev/null
	sudo grubby --update-kernel=ALL --args="mitigations=off" # Turn off CPU vulnerability mitigations to improve performance.
	;;
esac
