#include <X11/Xlib.h>
#include <string.h>
#include <stdlib.h>
#include "Taskbar.h"
#include "Window.h"

typedef struct TaskbarButton {
    Client *client;
    int x, y, w, h;
    struct TaskbarButton *next;
} TaskbarButton;

static TaskbarButton *buttons_head = NULL;
Window taskbar_window = 0;
int taskbar_height = TASKBAR_HEIGHT;

static Client *active_client = NULL;

// Truncate text to fit button width
static void fit_text(Display *dpy, XFontStruct *font, const char *src, char *dst, int max_w) {
    if (!src) { dst[0] = '\0'; return; }

    strncpy(dst, src, 255);
    dst[255] = '\0';

    int w = XTextWidth(font, dst, strlen(dst));
    if (w <= max_w) return; // Text fits, no truncation needed

    // Only truncate if it's too wide
    while (strlen(dst) > 0 && XTextWidth(font, dst, strlen(dst)) > max_w - XTextWidth(font, "...", 3)) {
        dst[strlen(dst)-1] = '\0';
    }

    // Add ellipsis if we truncated
    int len = strlen(dst);
    if (len > 3) {
        dst[len-3] = '.';
        dst[len-2] = '.';
        dst[len-1] = '.';
    }
}

// Relayout buttons along the taskbar
static void relayout_buttons(Display *dpy) {
    int count = 0;
    TaskbarButton *b = buttons_head;
    while (b) { count++; b = b->next; }
    if (count == 0) return;

    int screen = DefaultScreen(dpy);
    int sw = DisplayWidth(dpy, screen);
    int usable = sw - (count + 1) * 4; // spacing
    int final_w = usable / count;
    if (final_w > BUTTON_MAX_WIDTH) final_w = BUTTON_MAX_WIDTH;

    int x = 2;
    b = buttons_head;
    while (b) {
        b->x = x;
        b->y = 2;
        b->w = final_w;
        b->h = taskbar_height - 4;
        x += final_w + 4;
        b = b->next;
    }
}

// Draw the taskbar
void draw_taskbar(Display *dpy) {
    if (!taskbar_window) return;
    int screen = DefaultScreen(dpy);
    GC gc = XCreateGC(dpy, taskbar_window, 0, NULL);

    // Background
    XSetForeground(dpy, gc, 0x444444);
    XFillRectangle(dpy, taskbar_window, gc, 0, 0, DisplayWidth(dpy, screen), taskbar_height);

    XFontStruct *font = XLoadQueryFont(dpy, "fixed");
    if (!font) font = XQueryFont(dpy, XGContextFromGC(gc));

    // Draw buttons
    TaskbarButton *b = buttons_head;
    while (b) {
        XSetForeground(dpy, gc, (b->client==active_client)?0x555555:0x222222);
        XFillRectangle(dpy, taskbar_window, gc, b->x, b->y, b->w, b->h);

        char buf[256];
        fit_text(dpy, font, b->client->title ? b->client->title : "(untitled)", buf, b->w-6);

        XSetForeground(dpy, gc, WhitePixel(dpy, screen));
        XDrawString(dpy, taskbar_window, gc, b->x+3, b->y+b->h/2+font->ascent/2-2, buf, strlen(buf));

        b = b->next;
    }

    if (font) XFreeFont(dpy, font);
    XFreeGC(dpy, gc);
}

// Create taskbar window
Window create_taskbar(Display *dpy) {
    int screen = DefaultScreen(dpy);
    int sw = DisplayWidth(dpy, screen);
    int sh = DisplayHeight(dpy, screen);

    if (taskbar_window) return taskbar_window;
    taskbar_window = XCreateSimpleWindow(dpy, RootWindow(dpy, screen),
                                         0, sh-taskbar_height, sw, taskbar_height,
                                         0, BlackPixel(dpy, screen), 0x444444);
    XSelectInput(dpy, taskbar_window, ExposureMask | ButtonPressMask);
    XMapRaised(dpy, taskbar_window);
    return taskbar_window;
}

void add_taskbar_button(Display *dpy, Client *c) {
    TaskbarButton *btn = calloc(1, sizeof(TaskbarButton));
    btn->client = c;
    btn->next = NULL;

    if (!buttons_head) buttons_head = btn;
    else {
        TaskbarButton *it = buttons_head;
        while (it->next) it = it->next;
        it->next = btn;
    }

    relayout_buttons(dpy);
    draw_taskbar(dpy);
}

void remove_taskbar_button(Display *dpy, Client *c) {
    TaskbarButton **pp = &buttons_head;
    while (*pp) {
        if ((*pp)->client == c) {
            TaskbarButton *dead = *pp;
            *pp = (*pp)->next;
            free(dead);
            break;
        }
        pp = &((*pp)->next);
    }
    relayout_buttons(dpy);
    draw_taskbar(dpy);
}

void redraw_taskbar_button(Display *dpy, Client *c) {
    relayout_buttons(dpy);
    draw_taskbar(dpy);
}

// Click handler
void handle_taskbar_event(Display *dpy, XEvent *ev) {
    if (ev->type != ButtonPress) return;
    int mx = ev->xbutton.x;

    TaskbarButton *b = buttons_head;
    while (b) {
        if (mx >= b->x && mx <= b->x+b->w) {
            XRaiseWindow(dpy, b->client->frame);
            active_client = b->client;
            draw_taskbar(dpy);
            return;
        }
        b = b->next;
    }
}
