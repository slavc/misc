package main

import (
	"image"
	"image/png"
	"log"
	"net/http"
	"unsafe"
)

/*
#cgo CFLAGS: -std=c99 -pedantic -O2
#cgo LDFLAGS: -lX11
#include <unistd.h>
#include <err.h>
#include <stdint.h>
#include <string.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

static Display *dpy;
static Window win;
static XWindowAttributes winattr;

void
openDisplay(void)
{
	dpy = XOpenDisplay(NULL);
	if (dpy == NULL)
		errx(1, "failed to open X display");

	win = DefaultRootWindow(dpy);

	XGetWindowAttributes(dpy, win, &winattr);
}

XImage *
takeScreenshot(void)
{
	XImage *img;

	img = XGetImage(dpy, win, 0, 0, winattr.width, winattr.height,
	    AllPlanes, ZPixmap);
	return img;
}


// XDestroyImage is a macro, hence the wrapper.
void
freeScreenshot(XImage *img)
{
	XDestroyImage(img);
}

void
copyPixels(void *dstptr, void *srcptr, size_t n)
{
	uint32_t *dst = dstptr;
	uint32_t *src = srcptr;
	register uint32_t pix;

	while (n--) {
		pix = *src++;
		pix = pix & 0xff00ff00 | (((pix & 0xff) << 16) | ((pix & 0xff0000) >> 16));
		*dst++ = pix;
	}
}
*/
import "C"

const HomepageHtml = `<!DOCTYPE html>
<html>
    <head>
        <title>WebScreenCast</title>
        <style>
            html {
                margin: 0;
                height: 100%;
            }

            body {
                margin: 0;
                height: 100%;
                background-color: black;
                background-image: url('screenshot.png');
                background-size: contain;
                background-repeat: no-repeat;
                background-position: center center;
                background-origin: content-box;
            }
        </style>
    </head>
    <body>
    </body>
    <script>
        var INTERVAL = 500; // ms

        var counter = 0;
        var img = new Image();

        img.onload = function() {
            document.body.style.backgroundImage = "url('screenshot.png?x=" + counter + "')";
            counter++;
            window.setTimeout(update, INTERVAL);
        };

        function update() {
            img.src = "screenshot.png?x=" + counter;
        }

        window.setTimeout(update, INTERVAL);
    </script>
</html>`

func main() {
	C.openDisplay()

	http.HandleFunc("/", serveHomepage)
	http.HandleFunc("/screenshot.png", serveScreenshot)
	http.ListenAndServe(":8080", nil)
}

func serveHomepage(w http.ResponseWriter, r *http.Request) {
	w.Write([]byte(HomepageHtml))
}

func serveScreenshot(w http.ResponseWriter, r *http.Request) {
	// FIXME make sure the XImage has compatible format
	ximg := C.takeScreenshot()
	img := image.NewRGBA(image.Rectangle{image.Point{0, 0}, image.Point{int(ximg.width), int(ximg.height)}})
	C.copyPixels(unsafe.Pointer(&img.Pix[0]), unsafe.Pointer(ximg.data), C.size_t(ximg.width * ximg.height))
	C.freeScreenshot(ximg)

	err := png.Encode(w, img)
	if err != nil {
		log.Printf("Failed to encode image: %v", err)
	}
}
