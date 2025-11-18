#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Window.h"
#include "Taskbar.h"

#define TITLE_BAR_HEIGHT 20
#define BORDER_WIDTH 2
#define CLOSE_SIZE 16
#define CLOSE_PADDING 4

Client *clients_head = NULL;
static int dragging = 0;
static Window drag_window;
static int drag_offset_x, drag_offset_y;

// Find client by frame
Client* find_client_by_frame(Window frame) {
    Client *c = clients_head;
    while (c) {
        if (c->frame == frame) return c;
        c = c->next;
    }
    return NULL;
}

// Find client by client window
Client* find_client_by_client(Window client) {
    Client *c = clients_head;
    while (c) {
        if (c->client == client) return c;
        c = c->next;
    }
    return NULL;
}

// Remove client from list and free memory
void remove_client(Display *dpy, Client *c) {
    // Remove from list
    if (clients_head == c) {
        clients_head = c->next;
    } else {
        Client *prev = clients_head;
        while (prev && prev->next != c) prev = prev->next;
        if (prev) prev->next = c->next;
    }

    // Destroy frame
    XDestroyWindow(dpy, c->frame);
    free(c->title);
    free(c);
}

// Draw the title bar text and close button
void draw_title(Display *dpy, Window frame, const char *title) {
    GC gc = XCreateGC(dpy, frame, 0, NULL);
    XSetForeground(dpy, gc, 0xFFFFFF); // white text

    // Clear title bar area
    XClearArea(dpy, frame, 0, 0, 0, TITLE_BAR_HEIGHT, False);
    XDrawString(dpy, frame, gc, 5, 15, title, strlen(title));

    // Draw close button
    XWindowAttributes attr;
    XGetWindowAttributes(dpy, frame, &attr);
    int close_x = attr.width - CLOSE_SIZE - CLOSE_PADDING;
    int close_y = (TITLE_BAR_HEIGHT - CLOSE_SIZE) / 2;

    XSetForeground(dpy, gc, 0xFF0000); // red button
    XFillRectangle(dpy, frame, gc, close_x, close_y, CLOSE_SIZE, CLOSE_SIZE);

    // Draw "X" inside close button
    XSetForeground(dpy, gc, 0xFFFFFF);
    XDrawLine(dpy, frame, gc, close_x, close_y, close_x + CLOSE_SIZE, close_y + CLOSE_SIZE);
    XDrawLine(dpy, frame, gc, close_x, close_y + CLOSE_SIZE, close_x + CLOSE_SIZE, close_y);

    XFreeGC(dpy, gc);
}

void init_window_manager(Display *dpy, Window root) {
    XSelectInput(dpy, root, SubstructureRedirectMask | SubstructureNotifyMask);
}

void handle_event(Display *dpy, XEvent *ev) {
    switch (ev->type) {

        case MapRequest: {
            Window client = ev->xmaprequest.window;

            XWindowAttributes attr;
            XGetWindowAttributes(dpy, client, &attr);

            int screen_width = DisplayWidth(dpy, DefaultScreen(dpy));
            int screen_height = DisplayHeight(dpy, DefaultScreen(dpy));

            int x = (screen_width - attr.width) / 2;
            int y = (screen_height - attr.height) / 2;

            // Create frame window
            Window frame = XCreateSimpleWindow(
                dpy, DefaultRootWindow(dpy),
                x, y,
                attr.width,
                attr.height + TITLE_BAR_HEIGHT,
                BORDER_WIDTH,
                0xff0000, // red border
                0x444444  // dark gray title bar
            );

            // Listen for property changes on client (title updates)
            XSelectInput(dpy, client, PropertyChangeMask);

            // Reparent client
            XAddToSaveSet(dpy, client);
            XReparentWindow(dpy, client, frame, 0, TITLE_BAR_HEIGHT);

            // Listen for frame events
            XSelectInput(dpy, frame,
                         SubstructureRedirectMask |
                         SubstructureNotifyMask |
                         ButtonPressMask |
                         ButtonReleaseMask |
                         ButtonMotionMask |
                         ExposureMask);

            // Map everything
            XMapWindow(dpy, frame);
            XMapWindow(dpy, client);
            XRaiseWindow(dpy, frame);

            // Set WM_DELETE_WINDOW so we can handle closes
            Atom wm_delete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
            XSetWMProtocols(dpy, client, &wm_delete, 1);

            // Store client
            Client *c = malloc(sizeof(Client));
            c->frame = frame;
            c->client = client;
            c->next = clients_head;
            clients_head = c;

            // Fetch title
            char *name = NULL;
            if (XFetchName(dpy, client, &name) > 0 && name != NULL) {
                c->title = strdup(name);
                XFree(name);
            } else {
                c->title = strdup("Unnamed");
            }

            draw_title(dpy, frame, c->title);
            add_taskbar_button(dpy, c);

            break;
        }

        case ConfigureRequest: {
            XConfigureRequestEvent *e = &ev->xconfigurerequest;
            XWindowChanges wc;
            wc.x = e->x;
            wc.y = e->y;
            wc.width = e->width;
            wc.height = e->height;
            wc.border_width = e->border_width;
            wc.sibling = e->above;
            wc.stack_mode = e->detail;
            XConfigureWindow(dpy, e->window, e->value_mask, &wc);
            break;
        }

        case Expose: {
            Client *c = find_client_by_frame(ev->xexpose.window);
            if (c && c->title) {
                draw_title(dpy, c->frame, c->title);
            }
            break;
        }

        case PropertyNotify: {
            if (ev->xproperty.atom == XA_WM_NAME) {
                Client *c = find_client_by_client(ev->xproperty.window);
                if (c) {
                    free(c->title);
                    char *name = NULL;
                    if (XFetchName(dpy, c->client, &name) > 0 && name != NULL) {
                        c->title = strdup(name);
                        XFree(name);
                    } else {
                        c->title = strdup("Unnamed");
                    }

                    draw_title(dpy, c->frame, c->title);

                    redraw_taskbar_button(dpy, c);

                }
            }
            break;
        }

        case ClientMessage:
        case DestroyNotify: {
            Window w = (ev->type == DestroyNotify)
           ? ev->xdestroywindow.window
           : ev->xclient.window;

            Client *c = find_client_by_client(w);
            if (!c) c = find_client_by_frame(w);

            if (c) {
                remove_taskbar_button(dpy, c);
                remove_client(dpy, c);
            }
            break;
        }

        case ButtonPress: {
            Client *c = find_client_by_frame(ev->xbutton.window);
            if (!c) break;

            // Get frame width to position close button
            XWindowAttributes attr;
            XGetWindowAttributes(dpy, c->frame, &attr);
            int close_x = attr.width - CLOSE_SIZE - CLOSE_PADDING;
            int close_y = (TITLE_BAR_HEIGHT - CLOSE_SIZE) / 2;

            // Check if click is on close button
            if (ev->xbutton.y >= close_y && ev->xbutton.y <= close_y + CLOSE_SIZE &&
                ev->xbutton.x >= close_x && ev->xbutton.x <= close_x + CLOSE_SIZE) {
                remove_taskbar_button(dpy, c);
                remove_client(dpy, c);
                break;
            }

            // Otherwise, check if click is on title bar for dragging
            if (ev->xbutton.y < TITLE_BAR_HEIGHT) {
                dragging = 1;
                drag_window = c->frame;
                drag_offset_x = ev->xbutton.x;
                drag_offset_y = ev->xbutton.y;
                XGrabPointer(dpy, DefaultRootWindow(dpy), True,
                             ButtonMotionMask | ButtonReleaseMask,
                             GrabModeAsync, GrabModeAsync,
                             None, None, CurrentTime);
                XRaiseWindow(dpy, drag_window);
            }
            break;
        }

        case MotionNotify: {
            if (dragging) {
                int new_x = ev->xmotion.x_root - drag_offset_x;
                int new_y = ev->xmotion.y_root - drag_offset_y;
                XMoveWindow(dpy, drag_window, new_x, new_y);
            }
            break;
        }

        case ButtonRelease: {
            if (dragging) {
                dragging = 0;
                XUngrabPointer(dpy, CurrentTime);
            }
            break;
        }

        default:
            break;
    }
}
