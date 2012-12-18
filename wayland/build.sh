#!/bin/sh

function error() {
    echo "$0: $@" 1>&2
    exit 1
}

if ! [ -d $WLD/share/X11/xkb/rules ]; then
    mkdir -p $WLD/share/X11/xkb/rules
    ln -s /usr/share/X11/xkb/rules/evdev $WLD/share/X11/xkb/rules/
    ln -s /usr/bin/xkbcomp $WLD/bin/
fi

for p in macros wayland drm mesa libxkbcommon pixman cairo weston libxtrans xserver xf86-video-ati; do
    cd $p || error $p hasn\'t been cloned
    case $p in
    mesa)
        ./autogen.sh --prefix=$WLD --enable-gles2 --disable-gallium-egl --with-egl-platforms=x11,wayland,drm --enable-gbm --enable-shared-glapi --with-gallium-drivers=r300,r600,swrast,nouveau
        ;;
    libxkbcommon)
        ./autogen.sh --prefix=$WLD --with-xkb-config-root=/usr/share/X11/xkb
        ;;
    cairo)
        ./autogen.sh --prefix=$WLD --enable-gl --enable-xcb
        ;;
    *)
        ./autogen.sh --prefix=$WLD || error $p autogen failed
        ;;
    esac
    make -j 4 || error $p make failed
    if [ "$p" != "weston" ]; then
        make install || error $p make install failed
    fi
    cd ..
done
