#include "Start_Menu.h"
#include "Taskbar.h"
#include <X11/Xlib.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define MENU_WIDTH 200
#define MENU_HEIGHT 120
#define ITEM_HEIGHT 30
#define PADDING 5

const char *apps[] = {"xterm", "eyes"};
int num_apps = sizeof(apps)/sizeof(apps[0]);

static Window menu_window = 0;

// Create menu (always mapped)
Window create_start_menu(Display *dpy) {
    if (menu_window) return menu_window;

    int screen = DefaultScreen(dpy);
    int screen_height = DisplayHeight(dpy, screen);

    menu_window = XCreateSimpleWindow(
        dpy, DefaultRootWindow(dpy),
        0, screen_height - MENU_HEIGHT - TASKBAR_HEIGHT,
        MENU_WIDTH, MENU_HEIGHT,
        1, 0xFFFFFF, 0x333333
    );

    XSelectInput(dpy, menu_window, ExposureMask | ButtonPressMask);
    XMapRaised(dpy, menu_window);

    return menu_window;
}

void draw_menu(Display *dpy) {
    if (!menu_window) return;

    GC gc = XCreateGC(dpy, menu_window, 0, NULL);
    XSetForeground(dpy, gc, 0xFFFFFF);

    for (int i = 0; i < num_apps; i++) {
        XDrawString(dpy, menu_window, gc,
                    PADDING, PADDING + (i+1)*ITEM_HEIGHT,
                    apps[i], strlen(apps[i]));
    }

    XFreeGC(dpy, gc);
}

// Handle click on menu items
void handle_menu_click(Display *dpy, XEvent *ev) {
    if (!menu_window || ev->type != ButtonPress) return;

    int idx = (ev->xbutton.y - PADDING) / ITEM_HEIGHT;
    if (idx >= 0 && idx < num_apps) {
        if (fork() == 0) {
            execlp(apps[idx], apps[idx], NULL);
            _exit(0);
        }
    }
}
