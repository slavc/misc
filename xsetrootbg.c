#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <err.h>

#include <X11/Xlib.h>
#include <Imlib2.h>

void
usage(void)
{
	fprintf(stderr, "usage: xsetrootbg <image>\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	Display *dpy;
	Window root;
	Visual *visual;
	Colormap colormap;
	int depth;
	Imlib_Image image;
	Pixmap pixmap, mask;

	if (argc < 2)
		return 1;

	dpy = XOpenDisplay(NULL);
	if (dpy == NULL)
		errx(1, "XOpenDisplay");

	root = DefaultRootWindow(dpy);

	visual = DefaultVisual(dpy, DefaultScreen(dpy));
	depth = DefaultDepth(dpy, DefaultScreen(dpy));
	colormap = DefaultColormap(dpy, DefaultScreen(dpy));


	imlib_context_set_display(dpy);
	imlib_context_set_visual(visual);
	imlib_context_set_colormap(colormap);

	image = imlib_load_image(argv[1]);
	if (image == NULL)
		errx(1, "imlib_load_image");

	imlib_context_set_image(image);
	imlib_context_set_drawable(root);
	imlib_render_pixmaps_for_whole_image(&pixmap, &mask);

	XSetWindowBackgroundPixmap(dpy, root, pixmap);

	imlib_free_pixmap_and_mask(pixmap);
	imlib_free_image();

	XCloseDisplay(dpy);

	return 0;
}
