#!/bin/sh

# Configures a fresh Fedora installation the way I like it =)
# Run as the user.

function error()
{
    echo "${0##*/}: $*" 1>&2
    exit 1
}

if [ `id -u` -ne 0 ]; then
    echo 'Please enter the root password in the following prompt.'
    su root "$0" # perform actions which require root privileges

    cd
    mkdir -p src tmp .ssh || error 'failed to create the required directories'

    if ! fgrep 'Host github.com' ~/.ssh/config > /dev/null; then
        echo 'Host github.com' >> ~/.ssh/config
        echo "IdentityFile $HOME/.ssh/github_rsa" >> ~/.ssh/config
    fi
    chmod 700 .ssh
    chmod 600 .ssh/*

    cd src

    if ! [ -d ~/src/cwm ] && ! [ -d ~/src/misc ]; then
        git clone git@github.com:S010/cwm.git
        git clone git@github.com:S010/misc.git
    fi

    cd ~/src/cwm
    make

    find ~/src/misc/dotfiles -name '.*' -type f -exec cp \{\} ~ \;

    curl -s -o ~/tmp/darklime.tgz http://art.gnome.org/download/themes/gtk2/1364/GTK2-ClearlooksDarkLime.tar.gz && mkdir -p ~/.themes && tar -C ~/.themes -zxf ~/tmp/darklime.tgz
    curl -s -o ~/tmp/darkilouche.tbz2 http://art.gnome.org/download/themes/gtk2/1285/GTK2-Darkilouche.tar.bz2 && mkdir -p ~/.themes && tar -C ~/.themes -jxf ~/tmp/darkilouche.tbz2


    svn co https://core.fluendo.com/gstreamer/svn/trunk/ ~/src/fluendo
    cd ~/src/fluendo/gst-fluendo-mp3
    ./autogen.sh
    ./configure --with-liboil
    make
    dstdir=$HOME/.gstreamer-0.10/plugins
    mkdir -p "$dstdir"
    cp src/.libs/libgstflump3dec.so "$dstdir"


else # perform actions which require root privileges
    yum -y upgrade || error 'failed to upgrade packages'
    yum -y erase 'PackageKit-command-not-found' || error 'failed to uninstall PackageKit-command-not-found'
    yum -y install \
        artwiz-aleczapka-fonts \
        audacity \
        byacc \
        flex \
        conky \
        ctags \
        cvs \
        freetype-devel \
	automake \
	autoconf \
        gcc \
        gcc-c++ \
        gimp \
        gimp-help \
        gimp-help-browser \
        git \
        inkscape \
        libX11-devel \
        libXft-devel \
        libXinerama-devel \
        libXrandr-devel \
        libXv-devel \
        gstreamer-devel \
        gstreamer-plugins-base-devel \
        pulseaudio-libs-devel \
        gtk2-devel \
        gtk3-devel \
        make \
        man-pages \
        mupdf \
        mutt \
        openssh-askpass \
        pidgin \
        pidgin-sipe \
        subversion \
        sylpheed \
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
        gtk3-devel-docs \
        pygtk2-doc \
        devhelp \
        pavucontrol \
        geeqie \
        gtk-chtheme \
        tango-icon-theme \
        tango-icon-theme-extras \
        yasm \
        yasm-devel \
        liboil \
        liboil-devel \
        libtool \
    || error 'failed to install useful packages'
fi

exit 0


