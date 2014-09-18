#!/bin/sh
#
# You can tune a Fedora, but you can't tuna fish.
#

function error()
{
    echo "${0##*/}: $*" 1>&2
    exit 1
}

read -p 'Enter your username: ' username

yum -y upgrade || error 'failed to upgrade packages'
yum -y erase 'PackageKit-command-not-found' || error 'failed to uninstall PackageKit-command-not-found'
yum -y install \
    artwiz-aleczapka-fonts \
    audacity \
    autoconf \
    automake \
    byacc \
    ctags \
    cvs \
    devhelp \
    dkms \
    elementary-icon-theme \
    faenza-icon-theme \
    flex \
    freetype-devel \
    gcc \
    gcc-c++ \
    geeqie \
    gimp \
    gimp-help \
    gimp-help-browser \
    git \
    google-droid-\*-fonts \
    greybird-\*-theme \
    gstreamer-devel \
    gstreamer-plugins-base-devel \
    gtk-chtheme \
    gtk2-devel \
    gtk3-devel \
    gtk3-devel-docs \
    gnome-tweak-tool \
    inkscape \
    libX11-devel \
    libXft-devel \
    libXinerama-devel \
    libXrandr-devel \
    libXv-devel \
    libtool \
    make \
    man-pages \
    openssh-askpass \
    pavucontrol \
    pidgin \
    pidgin-sipe \
    pulseaudio-libs-devel \
    pygtk2-doc \
    subversion \
    sylpheed \
    tango-icon-theme \
    tango-icon-theme-extras \
    terminus-fonts \
    vim-X11 \
    vim-enhanced \
    xchat \
    xchm \
    xorg-x11-apps \
    xorg-x11-proto-devel \
    xorg-x11-utils \
    xorg-x11-xauth \
    xorg-x11-xinit-session \
    xterm \
    yasm \
    yasm-devel \
    @xfce-desktop \
|| error 'failed to install useful packages'


cat > /etc/profile.d/zzz_local.sh <<"EOF"
#!/bin/sh

alias vcl='gvim --servername ${GVIM_SERVER:=GVIM_$$} --remote-silent'
alias vcmd='gvim --servername ${GVIM_SERVER:=GVIM_$$} --remote-silent -c'

PATH=$HOME/opt/bin:$HOME/bin:$HOME/jdk/bin:$PATH
PS1='\u@\h:\j:\W\$ '
PAGER=/usr/bin/less
VISUAL=/usr/bin/vim
LESS=iS

export PATH PS1 PAGER VISUAL LESS
EOF


for f in gvimrc vimrc.local; do
	curl -s -o /etc/$f "https://raw.github.com/S010/misc/master/dotfiles/$f" || error 'failed to download VIM rc files'
done
echo -e '\n\n\n\nsource /etc/vimrc.local' >> /etc/vimrc

for f in .Xmodmap .Xresources .cwmrc ._dircolors .xsession; do
	curl -s -o $HOME/$f "https://raw.github.com/S010/misc/master/dotfiles/$f";
done;' || error 'failed to perform user setup'

su $username -c 'svn co https://core.fluendo.com/gstreamer/svn/trunk/ ~/src/fluendo
cd $HOME/src/fluendo/gst-fluendo-mp3;
./autogen.sh;
./configure --with-liboil;
make;
dstdir=$HOME/.gstreamer-0.10/plugins;
mkdir -p "$dstdir";
cp src/.libs/libgstflump3dec.so "$dstdir";

exit 0


