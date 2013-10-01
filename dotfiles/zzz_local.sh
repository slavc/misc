#!/bin/bash

ps1fg=30
if [ `id -u` -eq 0 ]; then
    ps1fg=31
fi
export COLUMNS
export PWD
PS1='\[\e[1;'$ps1fg';47m\]`awk "BEGIN { n_cols = int(ENVIRON[\"COLUMNS\"]); txt = ENVIRON[\"PWD\"]; gsub(\"^\" ENVIRON[\"HOME\"], \"~\", txt); line = \"                                                                      \"; line_len = length(line); n = n_cols - length(txt); printf(\"%s\", txt); for (i = 0; i < n; i += line_len) { printf(\"%s\", substr(line, 1, n - i)); } }"`\[\e[0m\]\n'

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
LESS=iSR
VISUAL=/usr/bin/vim
EDITOR=$VISUAL

export PS1 PAGER LESS VISUAL EDITOR
