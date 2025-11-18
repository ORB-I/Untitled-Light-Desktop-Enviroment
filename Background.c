#include <X11/Xlib.h>
#include "Background.h"

void set_background(Display *dpy, Window root) {
    XSetWindowAttributes attrs;
    attrs.background_pixel = BlackPixel(dpy, DefaultScreen(dpy));
    XChangeWindowAttributes(dpy, root, CWBackPixel, &attrs);
    XClearWindow(dpy, root);
    XFlush(dpy);
}
