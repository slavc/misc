# .bash_profile

# Get the aliases and functions
if [ -f ~/.bashrc ]; then
	. ~/.bashrc
fi

# User specific environment and startup programs

PATH=/usr/java/latest/bin:$HOME/bin:$HOME/opt/bin:$PATH
CSCOPE_EDITOR=$HOME/bin/vcl
GOPATH=$HOME/src/go
USE_CCACHE=1

export PATH CSCOPE_EDITOR GOPATH USE_CCACHE

