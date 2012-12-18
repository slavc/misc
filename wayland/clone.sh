#!/bin/sh

function error() {
    exit_code=$1
    shift
    echo "$@" 1>&2
    exit $exit_code
}

for url in \
    git://anongit.freedesktop.org/xorg/util/macros \
    git://anongit.freedesktop.org/wayland/wayland \
    git://anongit.freedesktop.org/git/mesa/drm \
    git://anongit.freedesktop.org/mesa/mesa \
    git://people.freedesktop.org/xorg/lib/libxkbcommon.git \
    git://anongit.freedesktop.org/pixman \
    git://anongit.freedesktop.org/cairo \
    git://anongit.freedesktop.org/wayland/weston \
    git://anongit.freedesktop.org/xorg/lib/libxtrans \
    'git://anongit.freedesktop.org/xorg/xserver -b xwayland-1.12' \
    'git://anongit.freedesktop.org/xorg/driver/xf86-video-intel -b xwayland' \
    'https://github.com/RAOF/xf86-video-ati -b xwayland' \
    'git://people.freedesktop.org/~iksaif/xf86-video-wlshm' \
    'https://github.com/RAOF/xf86-video-nouveau -b xwayland' \
;
do
    git clone $url || error 1 failed to clone $url
done
