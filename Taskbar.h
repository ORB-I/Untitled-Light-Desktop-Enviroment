#ifndef TASKBAR_H
#define TASKBAR_H

#include <X11/Xlib.h>
#include "Window.h"

#define TASKBAR_HEIGHT 28
#define BUTTON_MAX_WIDTH 120

extern Window taskbar_window;

Window create_taskbar(Display *dpy);
void add_taskbar_button(Display *dpy, Client *c);
void remove_taskbar_button(Display *dpy, Client *c);
void draw_taskbar(Display *dpy);
void redraw_taskbar_button(Display *dpy, Client *c);
void handle_taskbar_event(Display *dpy, XEvent *ev);

#endif
