#!/bin/bash

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
    PROMPT_COMMAND="${PROMPT_COMMAND}"' ${PWD/#${HOME}/~}\007"'
    export PROMPT_COMMAND
    ;;
esac
PAGER=/usr/bin/less
LESS=iS
VISUAL=/usr/bin/vim
EDITOR=$VISUAL

export PS1 PAGER LESS VISUAL EDITOR

vcl() {
    local cmd='gvim --servername ${GVIM_SERVER:=GVIM_$$} --remote-silent'
    while [ $# -gt 0 ]; do
        local filename=${1%:*}
        local lineno=${1##*:}
        if [ "$lineno" != "$filename" ]; then
            local lineopt="+${lineno}"
        fi
        eval $cmd "$lineopt" "$filename"
        shift
    done
}
