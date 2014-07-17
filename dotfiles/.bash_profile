# .bash_profile

# Get the aliases and functions
if [ -f ~/.bashrc ]; then
	. ~/.bashrc
fi

# User specific environment and startup programs

PATH=$HOME/bin:$HOME/opt/bin:$PATH
CSCOPE_EDITOR=$HOME/bin/vcl

export PATH CSCOPE_EDITOR

