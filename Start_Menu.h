#ifndef START_MENU_H
#define START_MENU_H

#include <X11/Xlib.h>

// Create menu window
Window create_start_menu(Display *dpy);

// Draw menu contents
void draw_menu(Display *dpy);

// Handle click inside menu
void handle_menu_click(Display *dpy, XEvent *ev);


#endif
