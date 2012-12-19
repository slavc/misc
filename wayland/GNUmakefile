WLD=$(HOME)/opt
LD_LIBRARY_PATH=$(WLD)/lib
PKG_CONFIG_PATH=$(WLD)/lib/pkgconfig/:$(WLD)/share/pkgconfig/
ACLOCAL_DIR=$(WLD)/share/aclocal
ACLOCAL=aclocal -I $$ACLOCAL_DIR

MAKE_FLAGS+=-j 8
CFLAGS+=-g
CXXFLAGS+=-g

COMPONENTS=macros/ wayland/ drm/ mesa/ libxkbcommon/ pixman/ cairo/ weston/ libxtrans/ xwayland/ xf86-video-intel/ xf86-video-ati/ xf86-video-wlshm/ xf86-video-nouveau/

all: $(WLD)/ $(COMPONENTS)

$(WLD)/:
	mkdir -p $(WLD)/

macros/:
	git clone git://anongit.freedesktop.org/xorg/util/macros $@
	$@/autogen.sh --prefix=$(WLD)

wayland/:
	git clone git://anongit.freedesktop.org/wayland/wayland $@
	$@/autogen.sh --prefix=$(WLD)

drm/:
	git clone git://anongit.freedesktop.org/git/mesa/drm $@
	$@/autogen.sh --prefix=$(WLD)

mesa/:
	git clone git://anongit.freedesktop.org/mesa/mesa $@
	$@/autogen.sh --prefix=$(WLD) --enable-gles2 --disable-gallium-egl --with-egl-platforms=x11,wayland,drm --enable-gbm --enable-shared-glapi --with-gallium-drivers=r300,r600,swrast,nouveau

libxkbcommon/:
	git clone git://people.freedesktop.org/xorg/lib/libxkbcommon.git $@
	$@/autogen.sh --prefix=$(WLD) --with-xkb-config-root=/usr/share/X11/xkb

pixman/:
	git clone git://anongit.freedesktop.org/pixman $@
	$@/autogen.sh --prefix=$(WLD)

cairo/:
	git clone git://anongit.freedesktop.org/cairo $@
	$@/autogen.sh --prefix=$(WLD) --enable-gl --enable-xcb

weston/:
	git clone git://anongit.freedesktop.org/wayland/weston $@
	$@/autogen.sh --prefix=$(WLD)

libxtrans/:
	git clone git://anongit.freedesktop.org/xorg/lib/libxtrans $@
	$@/autogen.sh --prefix=$(WLD)

xwayland/:
	git clone git://anongit.freedesktop.org/xorg/xserver -b xwayland-1.12 $@
	$@/autogen.sh --prefix=$(WLD)

xf86-video-intel/:
	git clone git://anongit.freedesktop.org/xorg/driver/xf86-video-intel -b xwayland $@
	$@/autogen.sh --prefix=$(WLD)

xf86-video-ati/:
	git clone https://github.com/RAOF/xf86-video-ati -b xwayland $@
	$@/autogen.sh --prefix=$(WLD)

xf86-video-wlshm/:
	git clone git://people.freedesktop.org/~iksaif/xf86-video-wlshm $@
	$@/autogen.sh --prefix=$(WLD)

xf86-video-nouveau/:
	git clone https://github.com/RAOF/xf86-video-nouveau -b xwayland $@
	$@/autogen.sh --prefix=$(WLD)


