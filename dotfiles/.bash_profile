# .bash_profile

# Get the aliases and functions
if [ -f ~/.bashrc ]; then
	. ~/.bashrc
fi

# User specific environment and startup programs

GOPATH=$HOME/go

export GOPATH

PATH=$PATH:$HOME/.local/bin:$HOME/bin:$GOPATH/bin

export PATH

GVIM_SERVER=GVIM_$$

export GVIM_SERVER

CSCOPE_EDITOR=$HOME/bin/vcl

export CSCOPE_EDITOR
