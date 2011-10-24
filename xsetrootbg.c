/*
 * Copyright (c) 2011 Sviatoslav Chagaev <sviatoslav.chagaev@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * xsetrootbg - background image setter for X.
 *
 *
 * Loads the images specified on the command line and sets them as the
 * background for the default root window of X in one of the various modes.
 *
 * xsetrootbg can set the background for the whole virtual screen or for
 * individual Xinerama screens.
 *
 *
 * Requires the Imlib2 library.
 *
 *
 * Compilation:
 *	cc -o xsetrootbg xsetrootbg.c \
 *	    $(pkg-config --cflags --libs x11 xinerama imlib2)
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <err.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>
#include <Imlib2.h>

struct rect {
	int x;
	int y;
	int w;
	int h;
};

struct screen {
	int		 no; /* xinerama screen number */
	struct rect	 rc;
};

static int		 dbgprintf(int, const char *fmt, ...);
static void		*xcalloc(size_t, size_t);
static void		 usage(void);
static void		 init_imlib(Display *);
static struct screen	*get_screens(Display *, int *);
static int		 has_xinerama(Display *);
static Imlib_Image	 get_bg(Display *);
static void		 blend(Imlib_Image, Imlib_Image, struct rect *, int);
static void		 set_bg(Display *, Imlib_Image);

int		 dflag;
const char	*modes[] = {
	"none",
	"center",
	"stretch",
	"scale",
	"zoom",
	"tile",
	NULL,
};
enum {
	MODE_NONE,
	MODE_CENTER,
	MODE_STRETCH,
	MODE_SCALE,
	MODE_ZOOM,
	MODE_TILE,
	MODE_INVALID,
};

int
main(int argc, char **argv)
{
	Display		*dpy;
	struct screen	*screens, *scr;
	int		 n_screens;
	Imlib_Image	 canvas = NULL;
	extern char	*optarg;
	extern int	 optind;
	int		 ch;
	int		 mode = MODE_NONE;
	int		 scr_no;
	Imlib_Image	 image;
	int		 i;
	const char	*optstring = "hs:m:d";

	if (argc < 2)
		usage();

	while ((ch = getopt(argc, argv, optstring)) != -1)
		if (ch == 'd')
			++dflag;
	optind = 1;

	dpy = XOpenDisplay(NULL);
	if (dpy == NULL)
		errx(1, "XOpenDisplay");

	init_imlib(dpy);
	screens = get_screens(dpy, &n_screens);
	canvas = get_bg(dpy);

	for (i = 0; /* empty */; ++i) {
		scr_no = -2;
		image = NULL;

		while ((ch = getopt(argc, argv, optstring)) != -1) {
			switch (ch) {
			case 'h':
				usage();
				break;
			case 's':
				scr_no = atoi(optarg);
				break;
			case 'm':
				for (mode = 0; mode < MODE_INVALID; ++mode)
					if (!strcmp(modes[mode], optarg))
						break;
				if (mode == MODE_INVALID)
					usage();
				break;
			}
		}
		argc -= optind;
		argv += optind;

		if (argc == 0) {
			if (i == 0)
				usage();
			else
				break;
		}

		optind = 1;

		image = imlib_load_image(*argv);
		if (image == NULL) {
			warnx("%s: failed to load image", *argv);
			continue;
		}

		if (scr_no == -2) {
			/* screen number wasn't specified, blend onto
			 * big virtual screen
			 */
			blend(image, canvas, &screens->rc, mode);
		} else {
			/* screen number "-1" is a special value, means
			 * blend image to all screens
			 */
			for (scr = screens + 1; scr < screens + n_screens; ++scr)
				if (scr->no == scr_no || scr_no == -1)
					blend(image, canvas, &scr->rc, mode);
		}

		imlib_context_set_image(image);
		imlib_free_image();
	}

	set_bg(dpy, canvas);

	imlib_context_set_image(canvas);
	imlib_free_image();

	XCloseDisplay(dpy);

	return 0;
}

static int
dbgprintf(int level, const char *fmt, ...)
{
	va_list	 ap;
	int	 n;

	if (level <= dflag) {
		va_start(ap, fmt);
		n = vfprintf(stderr, fmt, ap);
		va_end(ap);
		return n;
	} else
		return 0;
}

static void *
xcalloc(size_t nmemb, size_t size)
{
	void *p;

	p = calloc(nmemb, size);
	if (p == NULL)
		err(1, "calloc");
	return p;
}

static void
usage(void)
{
	fprintf(stderr, "usage: xsetrootbg -h\n");
	fprintf(stderr, "       xsetrootbg [-s <int>] [-m <none|center|stretch|scale|zoom|tile>] <imagefile>\n");
	exit(1);
}

static void
init_imlib(Display *dpy)
{
	Visual		*visual;
	Colormap	 colormap;

	visual = DefaultVisual(dpy, DefaultScreen(dpy));
	colormap = DefaultColormap(dpy, DefaultScreen(dpy));

	imlib_context_set_display(dpy);
	imlib_context_set_visual(visual);
	imlib_context_set_colormap(colormap);
	imlib_context_set_blend(1);
}

static struct screen *
get_screens(Display *dpy, int *n_screens)
{
	XineramaScreenInfo	*xineinfo, *xin;
	int			 n_xineinfo;
	Screen			*xscreen;
	struct screen		*screens, *scr;

	if (has_xinerama(dpy))
		xin = xineinfo = XineramaQueryScreens(dpy, &n_xineinfo);
	else
		n_xineinfo = 0;

	scr = screens = xcalloc(n_xineinfo + 1, sizeof(*screens));
	*n_screens = n_xineinfo + 1;
	/* fill out info about the big virtual screen, which may
	 * or may not contain Xinerama screens
	 */
	xscreen = DefaultScreenOfDisplay(dpy);
	scr->no = -1;
	scr->rc.x = 0;
	scr->rc.y = 0;
	scr->rc.w = WidthOfScreen(xscreen);
	scr->rc.h = HeightOfScreen(xscreen);

	/* fill out info about the Xinerama screens, if
	 * there are any...
	 */
	for (++scr; xin < xineinfo + n_xineinfo; ++scr, ++xin) {
		scr->no   = xin->screen_number;
		scr->rc.x = xin->x_org;
		scr->rc.y = xin->y_org;
		scr->rc.w = xin->width;
		scr->rc.h = xin->height;
	}
	XFree(xineinfo);

	return screens;
}

static int
has_xinerama(Display *dpy)
{
	int	 i, j;

	if (XineramaQueryExtension(dpy, &i, &j) == False)
		return 0;
	if (XineramaQueryVersion(dpy, &i, &j) == 0)
		return 0;
	if (XineramaIsActive(dpy) == False)
		return 0;
	return 1;
}

static Imlib_Image
get_bg(Display *dpy)
{
	Window		 root;
	Screen		*scr;
	Imlib_Image	 bg;

	root = DefaultRootWindow(dpy);
	scr = DefaultScreenOfDisplay(dpy);
	imlib_context_set_drawable(root);
	bg = imlib_create_image_from_drawable(None, 0, 0,
	    WidthOfScreen(scr), HeightOfScreen(scr), 0);

	imlib_context_set_image(bg);
	dbgprintf(1, "get_bg: bg.w=%d; bg.h=%d;\n", imlib_image_get_width(),
	    imlib_image_get_height());
	
	return bg;
}

static void
blend(Imlib_Image src, Imlib_Image dst, struct rect *cliprc, int mode)
{
	struct rect	 srcrc;
	struct rect	 dstrc;
	double		 wratio, hratio, ratio;
	int		 i, j, ncols, nrows;

	dbgprintf(1, "blend: src=%p; dst=%p; rc={%d, %d, %d, %d}; mode=%d;\n",
	    src, dst, cliprc->x, cliprc->y, cliprc->w, cliprc->h, mode);

	imlib_context_set_image(src);
	srcrc.x = 0;
	srcrc.y = 0;
	srcrc.w = imlib_image_get_width();
	srcrc.h = imlib_image_get_height();

	switch (mode) {
	case MODE_NONE:
		dstrc.x = cliprc->x;
		dstrc.y = cliprc->y;
		dstrc.w = srcrc.w;
		dstrc.h = srcrc.h;
		dbgprintf(1, " srcrc={%d, %d, %d, %d}\n", srcrc.x, srcrc.y,
		    srcrc.w, srcrc.h);
		dbgprintf(1, " dstrc={%d, %d, %d, %d}\n", dstrc.x, dstrc.y,
		    dstrc.w, dstrc.h);
		break;
	case MODE_CENTER:
		dstrc.x = cliprc->x + (cliprc->w - srcrc.w) / 2;
		dstrc.y = cliprc->y + (cliprc->h - srcrc.h) / 2;
		dstrc.w = srcrc.w;
		dstrc.h = srcrc.h;
		dbgprintf(1, " srcrc={%d, %d, %d, %d}\n", srcrc.x, srcrc.y,
		    srcrc.w, srcrc.h);
		dbgprintf(1, " dstrc={%d, %d, %d, %d}\n", dstrc.x, dstrc.y,
		    dstrc.w, dstrc.h);
		break;
	case MODE_STRETCH:
		dstrc.x = cliprc->x;
		dstrc.y = cliprc->y;
		dstrc.w = cliprc->w;
		dstrc.h = cliprc->h;
		dbgprintf(1, " srcrc={%d, %d, %d, %d}\n", srcrc.x, srcrc.y,
		    srcrc.w, srcrc.h);
		dbgprintf(1, " dstrc={%d, %d, %d, %d}\n", dstrc.x, dstrc.y,
		    dstrc.w, dstrc.h);
		break;
	case MODE_SCALE:
	case MODE_ZOOM:
		wratio = (double) cliprc->w / (double) srcrc.w;
		hratio = (double) cliprc->h / (double) srcrc.h;
		if (mode == MODE_ZOOM)
			ratio = wratio > hratio ? wratio : hratio;
		else
			ratio = wratio < hratio ? wratio : hratio;
		dbgprintf(1, " wratio=%.3f; hratio=%.3f;\n", wratio, hratio);
		dbgprintf(1, " ratio=%.3f\n", ratio);
		dstrc.x = cliprc->x + (cliprc->w - srcrc.w * ratio) / 2;
		dstrc.y = cliprc->y + (cliprc->h - srcrc.h * ratio) / 2;
		dstrc.w = srcrc.w * ratio;
		dstrc.h = srcrc.h * ratio;
		dbgprintf(1, " srcrc={%d, %d, %d, %d}\n", srcrc.x, srcrc.y,
		    srcrc.w, srcrc.h);
		dbgprintf(1, " dstrc={%d, %d, %d, %d}\n", dstrc.x, dstrc.y,
		    dstrc.w, dstrc.h);
		break;
	case MODE_TILE:
		imlib_context_set_image(dst);
		imlib_context_set_cliprect(
		    cliprc->x, cliprc->y, cliprc->w, cliprc->h);
		nrows = cliprc->h / srcrc.h + 1;
		ncols = cliprc->w / srcrc.w + 1;
		dbgprintf(1, " nrows=%d; ncols=%d;\n", nrows, ncols);
		dstrc.w = srcrc.w;
		dstrc.h = srcrc.h;
		for (i = 0; i < nrows; ++i) {
			for (j = 0; j < ncols; ++j) {
				dstrc.x = j * srcrc.w;
				dstrc.y = i * srcrc.h;
				imlib_blend_image_onto_image(src, 1,
				    srcrc.x, srcrc.y, srcrc.w, srcrc.h,
				    dstrc.x, dstrc.y, dstrc.w, dstrc.h);
			}
		}

		return;

	default:
		warnx("%d: unknown image application mode", mode);
		return;
	}

	imlib_context_set_image(dst);
	imlib_context_set_cliprect(cliprc->x, cliprc->y, cliprc->w, cliprc->h);
	imlib_blend_image_onto_image(src, 1,
	    srcrc.x, srcrc.y, srcrc.w, srcrc.h,
	    dstrc.x, dstrc.y, dstrc.w, dstrc.h);
}

static void
set_bg(Display *dpy, Imlib_Image img)
{
	Pixmap	 pixmap, mask;
	Window	 root;

	root = DefaultRootWindow(dpy);

	imlib_context_set_image(img);
	imlib_context_set_drawable(root);
	imlib_render_pixmaps_for_whole_image(&pixmap, &mask);

	XSetWindowBackgroundPixmap(dpy, root, pixmap);
	XClearWindow(dpy, root);

	imlib_free_pixmap_and_mask(pixmap);
}
