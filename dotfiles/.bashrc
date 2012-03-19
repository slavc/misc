# .bashrc

# Source global definitions
#if [ -f /etc/bashrc ]; then
#	. /etc/bashrc
#fi

if [ "$PS1" ]; then
  if [ -z "$PROMPT_COMMAND" ]; then
    case $TERM in
    xterm*)
      if [ -e /etc/sysconfig/bash-prompt-xterm ]; then
          PROMPT_COMMAND=/etc/sysconfig/bash-prompt-xterm
      else
          PROMPT_COMMAND='printf "\033]0;%s@%s:%s\007" "${USER}" "${HOSTNAME%%.*}" "${PWD/#$HOME/~}"'
      fi
      ;;
    screen)
      if [ -e /etc/sysconfig/bash-prompt-screen ]; then
          PROMPT_COMMAND=/etc/sysconfig/bash-prompt-screen
      else
          PROMPT_COMMAND='printf "\033]0;%s@%s:%s\033\\" "${USER}" "${HOSTNAME%%.*}" "${PWD/#$HOME/~}"'
      fi
      ;;
    *)
      [ -e /etc/sysconfig/bash-prompt-default ] && PROMPT_COMMAND=/etc/sysconfig/bash-prompt-default
      ;;
    esac
  fi
  # Turn on checkwinsize
  shopt -s checkwinsize
  [ "$PS1" = "\\s-\\v\\\$ " ] && PS1="[\u@\h \W]\\$ "
  # You might want to have e.g. tty in prompt (e.g. more virtual machines)
  # and console windows
  # If you want to do so, just add e.g.
  # if [ "$PS1" ]; then
  #   PS1="[\u@\h:\l \W]\\$ "
  # fi
  # to your custom modification shell script in /etc/profile.d/ directory
fi

for script in colorgrep colorls less x11-ssh-askpass; do
    [ -f /etc/profile.d/${script}.sh ] && . /etc/profile.d/${script}.sh
done

# User specific aliases and functions

alias vcl='gvim --servername ${GVIM_SERVER:=GVIM_$$} --remote-silent'
alias vcmd='gvim --servername ${GVIM_SERvER:=GVIM_$$} --remote-silent -c'

PATH=$HOME/.local/bin:$HOME/bin:$HOME/jdk/bin:$PATH
PS1='\u@\h:\j:\W\$ '
PAGER=/usr/bin/less
LESS=iS
EDITOR=/usr/bin/vim

export PATH PS1 PAGER LESS EDITOR
