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
 * xsetbg - background image setter for X.
 *
 *
 * Loads the images specified on the command line and sets them as the
 * background for the default root window of X in one of the various modes.
 *
 * xsetbg can set the background for the whole virtual screen or for
 * individual Xinerama screens.
 *
 * It doesn't clear the background before applying the image. Use 
 * xsetroot -solid <color> to reset the background to a certain color
 * before running xsetbg.
 *
 *
 * Requires the Imlib2 library.
 *
 *
 * Compilation:
 *   cc -o xsetbg xsetbg.c $(pkg-config --cflags --libs x11 xinerama imlib2)
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

#define DEFAULT_MODE		MODE_FILL
#define DEFAULT_SCREEN_LIST	"all"

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
static void		 foreach_screen(Display *, Imlib_Image, const char *,
			    struct screen *, int, const char *, int);
static void		 blend(Imlib_Image, Imlib_Image, struct rect *, int);
static void		 set_bg(Display *, Imlib_Image);

int		 dflag;
const char *modes[] = {
	"-copy",
	"-center",
	"-stretch",
	"-fit",
	"-fill",
	"-tile",
	NULL
};
enum {
	MODE_COPY,
	MODE_CENTER,
	MODE_STRETCH,
	MODE_FIT,
	MODE_FILL,
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
	int		 mode = DEFAULT_MODE;
	int		 scr_no;
	Imlib_Image	 image;
	int		 i, j;
	char		*scr_list = DEFAULT_SCREEN_LIST;
	char		*tok;


	if (argc < 2)
		usage();

	dpy = XOpenDisplay(NULL);
	if (dpy == NULL)
		errx(1, "XOpenDisplay");

	for (i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "-d"))
			++dflag;
		else if (!strcmp(argv[i], "-h"))
			usage();
		else if (!strcmp(argv[i], "-help"))
			usage();
		else if (!strcmp(argv[i], "--help"))
			usage();
		else
			break;
	}

	init_imlib(dpy);
	screens = get_screens(dpy, &n_screens);
	canvas = get_bg(dpy);

	for (/* empty */; i < argc; ++i) {

	next_arg:

		for (j = 0; j < MODE_INVALID; ++j) {
			if (!strcmp(argv[i], modes[j])) {
				mode = j;
				if (++i >= argc)
					usage();
				goto next_arg;
			}
		}
		if (!strcmp(argv[i], "-screen")) {
			if (++i >= argc)
				usage();
			scr_list = argv[i];
			if (++i >= argc)
				usage();
			goto next_arg;
		}
		
		/* ah, this must be the image filename */
		if (scr_list == NULL)
			scr_list = "all";
		foreach_screen(dpy, canvas, scr_list,
		    screens, n_screens, argv[i], mode);
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
	fprintf(stderr, "usage: xsetbg ([-screen <int,int,...|all|whole>] \\\n"
			"              [-copy|-center|-stretch|-scale|-fill|-tile] \\\n"
			"              <image>)*\n");
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

	/* fill out info about the Xinerama screens, if there are any */
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
foreach_screen(Display *dpy, Imlib_Image canvas, const char *scr_list,
    struct screen *screens, int n_screens, const char *filename, int mode)
{
	Imlib_Image	 img;
	char		 buf[128], *tok;
	struct screen	*scr;
	int		 i;

	img = imlib_load_image(filename);
	if (img == NULL) {
		warnx("%s: failed to load image", filename);
		return;
	}

	strncpy(buf, scr_list, sizeof(buf));
	buf[sizeof(buf)-1] = '\0';

	for (tok = strtok(buf, ","); tok != NULL; tok = strtok(NULL, ",")) {
		if (!strcmp(tok, "all")) {
			for (scr = screens + 1; scr < screens + n_screens; ++scr)
				blend(img, canvas, &scr->rc, mode);
		} else if (!strcmp(tok, "whole")) {
			blend(img, canvas, &screens->rc, mode);
		} else {
			i = atoi(tok);
			for (scr = screens + 1; scr < screens + n_screens; ++scr) {
				if (scr->no == i) {
					blend(img, canvas, &scr->rc, mode);
					break;
				}
			}
			if (scr >= screens + n_screens)
				warnx("%s: no such screen", tok);
		}
	}

	imlib_context_set_image(img);
	imlib_free_image();
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
	case MODE_COPY:
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
	case MODE_FIT:
	case MODE_FILL:
		wratio = (double) cliprc->w / (double) srcrc.w;
		hratio = (double) cliprc->h / (double) srcrc.h;
		if (mode == MODE_FILL)
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
		nrows = cliprc->h / srcrc.h + (cliprc->h % srcrc.h != 0 ? 1 : 0);
		ncols = cliprc->w / srcrc.w + (cliprc->w % srcrc.w != 0 ? 1 : 0);
		dbgprintf(1, " nrows=%d; ncols=%d;\n", nrows, ncols);
		dstrc.w = srcrc.w;
		dstrc.h = srcrc.h;
		for (i = 0; i < nrows; ++i) {
			dbgprintf(1, " row %i\n", i);
			for (j = 0; j < ncols; ++j) {
				dbgprintf(1, "  column %i\n", j);
				dstrc.x = cliprc->x + j * srcrc.w;
				dstrc.y = cliprc->y + i * srcrc.h;
				dbgprintf(1, "   dst={%i, %i}\n", dstrc.x,
				    dstrc.y);
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
