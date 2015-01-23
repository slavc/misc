#!/bin/bash

ps1fg=34
if [ `id -u` -eq 0 ]; then
    ps1fg=31
fi
if [ -x /usr/local/bin/prompt ]; then
    export COLUMNS
    PS1='$(/usr/local/bin/prompt -B -f'${ps1fg}' -b white \w\%\j)\n'
else
    PS1='\[\e[1;'${ps1fg}'m\]\w:\j \[\e[0m\]'
fi
unset ps1fg

PROMPT_COMMAND='printf "\033]0;'
if [ `id -u` -eq 0 ]; then
    PROMPT_COMMAND="${PROMPT_COMMAND}#"
else
    PROMPT_COMMAND="${PROMPT_COMMAND}\$"
fi
PROMPT_COMMAND="${PROMPT_COMMAND}"' ${PWD/#${HOME}/~}\007"'
export PROMPT_COMMAND

PAGER=/usr/bin/less
LESS=iSR
VISUAL=/usr/bin/vim
EDITOR=$VISUAL

export PAGER LESS VISUAL EDITOR

