#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "Window.h"
#include "Taskbar.h"
#include "Background.h"
#include "Start_Menu.h"   // <-- Include this so prototypes are known

Display *dpy;

int main() {
    dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "Cannot open display\n");
        return 1;
    }

    Window root = DefaultRootWindow(dpy);

    // Set background
    set_background(dpy, root);

    // Initialize window manager
    init_window_manager(dpy, root);

    // Autostart an xterm
    if (fork() == 0) {
        execlp("xterm", "xterm", NULL);
        perror("execlp failed");
        _exit(1);
    }

    // Create taskbar and start menu
    create_taskbar(dpy);
    create_start_menu(dpy);

    // Main event loop
    XEvent ev;
    while (1) {
        XNextEvent(dpy, &ev);

        // Taskbar events
        if (ev.xany.window == taskbar_window) {
            handle_taskbar_event(dpy, &ev);
            continue;
        }

        // Start menu events
        if (ev.xany.window == create_start_menu(dpy)) {
            if (ev.type == Expose) draw_menu(dpy);
            else if (ev.type == ButtonPress) handle_menu_click(dpy, &ev);
            continue;
        }

        // Window manager events
        handle_event(dpy, &ev);
    }

    XCloseDisplay(dpy);
    return 0;  // <-- Ensure main ends properly
}
