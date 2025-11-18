#pragma once
#include <X11/Xlib.h>

// Forward declare client for Taskbar
typedef struct Client {
    Window frame;
    Window client;
    char *title;
    struct Client *next;
} Client;

// Window manager functions
void init_window_manager(Display *dpy, Window root);
void handle_event(Display *dpy, XEvent *ev);

// Client helpers
Client* find_client_by_frame(Window frame);
Client* find_client_by_client(Window client);
void remove_client(Display *dpy, Client *c);

// Expose clients_head for taskbar iteration
extern Client *clients_head;
