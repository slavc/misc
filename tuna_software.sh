#!/bin/bash
#
# Tune software after a fresh installation of GNU/Linux.
#

set -e # exit with error if any command in this script fails
# set -v # print commands being executed

fatal() {
	echo "${0##*/}: error: $@"
	exit 1
}

if [ `id -u` -eq 0 ]; then
	fatal this script must be run as the target user, not root
fi

if ! sudo true; then
	exit $?
fi

#
# Tune desktop environment
# 

for x in \
	'org.gnome.settings-daemon.plugins.power idle-dim false' \
	'org.gnome.desktop.session idle-delay 600' \
	'org.gnome.desktop.interface clock-show-weekday true' \
	'org.gnome.settings-daemon.plugins.color night-light-enabled true' \
	'org.gnome.desktop.wm.preferences focus-mode mouse' \
	'org.gnome.desktop.interface font-antialiasing rgba' \
	"org.gnome.desktop.peripherals.touchpad tap-to-click true" \
	"org.gnome.desktop.input-sources mru-sources \"[('xkb', 'us')]\"" \
	"org.gnome.desktop.input-sources sources \"[('xkb', 'us'), ('xkb', 'lv'), ('xkb', 'ru')]\"" \
	"org.gnome.desktop.wm.keybindings toggle-fullscreen \"['<Super>F11']\"" \
	"org.gtk.gtk4.Settings.FileChooser sort-directories-first true"
do
		eval gsettings set $x
done

#
# Tune software
#

# {
# 	code --install-extension golang.go ;
# 	code --install-extension ms-python.python ;
# 	code --install-extension redhat.vscode-yaml ;
# 	code --install-extension ms-vscode.cpptools ;
# 	code --install-extension 42crunch.vscode-openapi ;
# 	code --install-extension stkb.rewrap # rewrap text to 80 columns with Alt+q ;
# 	code --install-extension ms-vscode-remote.remote-ssh ;
# } || true

cat > ~/.tmux.conf << EOF
set -g mouse on
EOF

cat > ~/.vimrc << EOF
filetype plugin on
set bg=dark
set hlsearch
set incsearch
set ai
set ic
EOF

mkdir -p ~/.vim/ftdetect
mkdir -p ~/.vim/ftplugin

cat > ~/.vim/ftdetect/cpp.vim << EOF
autocmd BufRead,BufNewFile *.hpp setfiletype cpp
autocmd BufRead,BufNewFile *.cpp setfiletype cpp
autocmd BufRead,BufNewFile *.cc setfiletype cpp
EOF

cat > ~/.vim/ftplugin/cpp.vim << EOF
set autoindent
set expandtab
set shiftwidth=4
set softtabstop=4
set tabstop=4
EOF

sudo cp ~/.vimrc /root/
sudo chown root:root /root/.vimrc

terminus_font=$(fc-list Terminus | grep -i regular | head -1 | cut -f 2 -d ':' | sed 's/^ //g')
terminus_font="${terminus_font} 12"
terminus_font="${terminus_font// /\\ }"

cat > ~/.gvimrc << EOF
set ic
set belloff=all
set nowrap

colorscheme industry

set guifont=${terminus_font}
set guioptions=aegit

set cursorline
set colorcolumn=80
highlight CursorLine guibg=grey10
highlight ColorColumn guibg=grey10
EOF

git config --global user.email 'sviatoslav.chagaev@gmail.com'
git config --global user.name 'Sviatoslav Chagaev'
git config --global core.editor vim

if ! [ -f ~/.ssh/id_rsa ]; then
	ssh-keygen -q -f ~/.ssh/id_rsa -C 'sviatoslav.chagaev@gmail.com'
fi

cat > ~/.my_bashrc << EOF
export EDITOR=$(which vim)
export LESS=RSi

# Make it so Bash performs filename completion on `gv` (gvim client), `op`
# (open file in appropriate app).
complete -F _longopt gv
complete -F _longopt op

alias csc='cscope -Rq'

source ~/src/vulkan_sdk/default/setup-env.sh || true
EOF

if ! grep 'my_bashrc' ~/.bashrc >/dev/null 2>&1; then
	echo 'source ~/.my_bashrc' >> ~/.bashrc
fi
