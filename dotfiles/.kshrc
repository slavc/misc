. /etc/ksh.kshrc

export COLUMNS
export PWD

if [ "$TERM" == "vt220" ]; then
	PS1='\w\$ '
else
	PS1='\[\e[1;30;47m\]`awk "BEGIN { n_cols = int(ENVIRON[\"COLUMNS\"]); txt = ENVIRON[\"PWD\"]; line = \"                                                                      \"; line_len = length(line); n = n_cols - length(txt); printf(\"%s\", txt); for (i = 0; i < n; i += line_len) { printf(\"%s\", substr(line, 1, n - i)); } printf(\"\\n\"); }"`\[\e[0m\]'
fi
VISUAL=/usr/bin/vi
EDITOR=$VISUAL
PAGER=/usr/bin/less
LESS=iRS
CVSROOT=anoncvs@anoncvs.estpak.ee:/OpenBSD
LSCOLORS=AxCxFxFxBxFxFxBHBHAHAH
export PS1 VISUAL EDITOR PAGER LESS CVSROOT LSCOLORS

set -o emacs

alias ls='/usr/local/bin/colorls -G'

ulimit -d $(expr 1024 \* 2048)

GVIM_SERVER=GVIM_$$
export GVIM_SERVER 
export CSCOPE_EDITOR=$HOME/bin/vcl

