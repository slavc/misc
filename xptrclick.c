/* Public domain. */

/*
 * NAME
 *  xptrclick - emulate a click of a pointer button.
 *
 * SYNOPSIS
 *  xptrclick <button_no>
 *
 * DESCRIPTION
 *  xptrclick connects to X server and inserts two events,
 *  ButtonPress and ButtonRelease for the specified button.
 *  Most programs will interpret it as a click.
 *
 *  button_no
 *   The number of button for which to emulate click. This
 *   should be a number between 1 and 5, inclusively.
 *   4 and 5 are usually mapped to the scroll-wheel.
 */

/* Compilation:
 *  cc -o xptrclick xptrclick.c $(pkg-config --cflags --libs x11) -lXext -lXtst
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>

#include <unistd.h>
#include <err.h>

Display *dpy;

int
main(int argc, char **argv)
{
	int event, error, major, minor;
	unsigned int buttons[] = {
		Button1,
		Button2,
		Button3,
		Button4,
		Button5,
	};
	const int nbuttons = sizeof(buttons)/sizeof(buttons[0]);
	int button_no;
	unsigned int button;

	if (argc < 2)
		errx(1, "usage: xptrclick <button_no>");
	button_no = atoi(argv[1]);
	if (button_no < 1 || button_no > nbuttons)
		errx(1, "button_no must be an integer between 1 and 5");
	button = buttons[button_no - 1];

	dpy = XOpenDisplay(NULL);
	if (dpy == NULL)
		errx(1, "XOpenDisplay");

	if (XTestQueryExtension(dpy, &event, &error, &major, &minor) == False)
		errx(1, "no XTest on this display");

	XTestFakeButtonEvent(dpy, button, True, CurrentTime);
	XTestFakeButtonEvent(dpy, button, False, CurrentTime);

	XCloseDisplay(dpy);

	return 0;
}
