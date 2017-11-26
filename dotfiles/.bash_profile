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

EDITOR=vim

export EDITOR

VISUAL=vim

export VISUAL

PLAN9=/home/schagaev/src/plan9port

export PLAN9

PATH=$PATH:$PLAN9/bin

export COLUMNS
PS1_FG=green
PROMPT_COMMAND='if [ $? -eq 0 ]; then PS1_FG=green; else PS1_FG=red; fi'
PS1='`prompt -B -f "$PS1_FG" -b white "\w%"`\n'
