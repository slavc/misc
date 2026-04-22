#!/bin/bash
#
# Tune a fresh installation of Ubuntu Server.
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
	gcc \
	gdb \
	git \
	golang \
	llvm \
	lm-sensors \
	minicom \
	nmap \
	nmon \
	s-tui \
	sshfs \
	tmux \
	universal-ctags \
	vim \


#
# Apply special settings for some machines.
#

case $HOSTNAME in
prodesk | core)
	echo Applying special settings for host $HOSTNAME...
	echo 'GRUB_CMDLINE_LINUX_DEFAULT="${GRUB_CMDLINE_LINUX_DEFAULT} mitigations=off"' | sudo tee /etc/default/grub.d/10-disable-mitigations.cfg
	sudo update-grub
	sudo sed -i 's/^%sudo.*$/%sudo   ALL=(ALL:ALL) NOPASSWD: ALL/' /etc/sudoers
	;;
esac

