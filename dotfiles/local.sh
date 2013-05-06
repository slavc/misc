#!/bin/sh

alias vcl='gvim --servername ${GVIM_SERVER:=GVIM_$$} --remote-silent'
alias vcl2='gvim --servername ${GVIM2_SERVER:=GVIM2_$$} --remote-silent'
alias vcmd='gvim --servername ${GVIM_SERvER:=GVIM_$$} --remote-silent -c'

PATH=$HOME/opt/bin:$HOME/jdk/bin:$PATH
if [ `id -u` -eq 0 ]; then
    PS1='\[\e[1;31m\]┌─<\w>\[\e[0m\]\n\[\e[1;31m\]└▶\[\e[0m\] '
else
    PS1='\[\e[1;32m\]┌─<\w>\[\e[0m\]\n\[\e[1;32m\]└▶\[\e[0m\] '
fi
case "$TERM" in
*rxvt*|*xterm*)
    PROMPT_COMMAND='echo -ne "\e]0;'
    if [ `id -u` -eq 0 ]; then
        PROMPT_COMMAND="${PROMPT_COMMAND}#"
    else
        PROMPT_COMMAND="${PROMPT_COMMAND}\$"
    fi
    PROMPT_COMMAND="${PROMPT_COMMAND} ${PWD}\007\""
    export PROMPT_COMMAND
    ;;
esac
PAGER=/usr/bin/less
VISUAL=/usr/bin/vim
LESS=iS

export PATH PS1 PAGER VISUAL LESS
