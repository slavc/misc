# .bashrc

# Source global definitions
if [ -f /etc/bashrc ]; then
	. /etc/bashrc
fi

# User specific aliases and functions

GVIM_SERVER=GVIM_$$
export GVIM_SERVER 

alias cu='cu -p com'

