//  src/X11/tinysdl_x11.c
#include "tinysdl.h"
#include "tinysdl_core.h"
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <GL/glx.h>
#include <stdio.h>
#include <string.h>
//  internal
#include "../internal/tsdl_rendering.h"

#ifndef BACKEND_VER
#define BACKEND_VER "0.1.0"
#endif
#ifndef TSDL_VER
#define TSDL_VER CORE_VER "+" BACKEND_ID "_" BACKEND_VER
#endif

const string WM_DELETE_WINDOW = "WM_DELETE_WINDOW";

struct tinysdl_window_s
{
    Display *display;
    Window xwindow;
    GLXContext glx_context;
    int w, h;            // Track size
    int x, y;            // Track position
    int is_fullscreen;   // Fullscreen state
    int close_requested; // Quit flag
    struct
    {
        int w, h, x, y; // Restore state
    } resto;
};

static Display *global_display = NULL;
static Atom wm_delete_window = None;
static int is_initialized = TSDL_FALSE;
static window active_window = NULL; // Track current window
static int mod_state = TSDL_MOD_NONE;

// TSDL (X11) Functions =======================================================
static TSDL_Keycode map_keys(KeySym);

int tsdl_init_video(void)
{
    if (is_initialized)
    {
        return log_error(TSDL_ERR_INIT, "TinySDL is already initialized");
    }
    global_display = XOpenDisplay(NULL);
    if (!global_display)
    {
        return log_error(TSDL_ERR_GL, "Failed to open X11 display");
    }

    wm_delete_window = XInternAtom(global_display, WM_DELETE_WINDOW, TSDL_FALSE);
    wm_delete_window = XInternAtom(global_display, "WM_DELETE_WINDOW", TSDL_FALSE);
    if (wm_delete_window == None)
    {
        XCloseDisplay(global_display);
        global_display = NULL;
        return log_error(TSDL_ERR_INIT, "Failed to intern WM_DELETE_WINDOW");
    }
    LOG_STAT("WM_DELETE_WINDOW interned"); // Debug

    is_initialized = TSDL_TRUE;

    LOG_STAT("Initialized Backend (%s): flags=%d", TSDL_BACKEND, TSDL_INIT_VIDEO);

    return TSDL_ERR_NONE;
}
void tsdl_quit(void)
{
    if (!is_initialized)
    {
        log_error(TSDL_ERR_INIT, "TinySDL is not initialized");
        return;
    }
    if (active_window)
    {
        window_destroy(active_window); // Destroy window first
        active_window = NULL;
    }
    if (global_display)
    {
        XCloseDisplay(global_display);
        global_display = NULL;
    }

    is_initialized = TSDL_FALSE;

    LOG_STAT("Quit %s Backend", TSDL_BACKEND);
}
const string tsdl_getError(void)
{
    return err_trace;
}
int tsdl_pollEvent(Event event)
{
    if (!is_initialized || !event)
        return TSDL_FALSE;
    if (!global_display)
        return TSDL_FALSE; // Safety check

    if (XPending(global_display))
    {
        XEvent xev;
        XNextEvent(global_display, &xev);

        // Single-window assumption
        window win = active_window;
        if (!win || xev.xany.window != win->xwindow)
            return TSDL_FALSE;

        event->type = TSDL_EVENT_NONE; // Reset event type
        switch (xev.type)
        {
        case ClientMessage:
        {
            LOG_STAT("ClientMessage received: atom=%ld, expected=%ld",
                     (long)xev.xclient.data.l[0], (long)wm_delete_window);

            if ((Atom)xev.xclient.data.l[0] == wm_delete_window)
            {
                event->type = TSDL_EVENT_QUIT;
                win->close_requested = TSDL_TRUE;

                LOG_STAT("Quit event set: type=%d", event->type);
                // Log exact type
            }
        }

        break;
        case FocusIn:
        {
            event->type = TSDL_EVENT_WINDOW_FOCUS_GAINED;
        }

        break;
        case FocusOut:
        {
            event->type = TSDL_EVENT_WINDOW_FOCUS_LOST;
        }

        break;
        case ConfigureNotify:
        {
            Atom wm_state = XInternAtom(global_display, "_NET_WM_STATE", TSDL_FALSE);
            Atom max_horz = XInternAtom(global_display, "_NET_WM_STATE_MAXIMIZED_HORZ", TSDL_FALSE);
            Atom max_vert = XInternAtom(global_display, "_NET_WM_STATE_MAXIMIZED_VERT", TSDL_FALSE);
            Atom type;
            int format;
            unsigned long nitems, bytes_after;
            unsigned char *prop = NULL;

            if (XGetWindowProperty(global_display, win->xwindow, wm_state, 0, 1024, TSDL_FALSE,
                                   XA_ATOM, &type, &format, &nitems, &bytes_after, &prop) == Success &&
                prop)
            {
                Atom *states = (Atom *)prop;
                int maximized = TSDL_FALSE;
                for (unsigned long i = 0; i < nitems; i++)
                {
                    if (states[i] == max_horz || states[i] == max_vert)
                    {
                        maximized = TSDL_TRUE;
                        break;
                    }
                }
                if (maximized && !win->is_fullscreen)
                {
                    event->type = TSDL_EVENT_WINDOW_MAXIMIZED;
                }
                XFree(prop);
            }
            if (xev.xconfigure.width != win->w || xev.xconfigure.height != win->h)
            {
                event->type = TSDL_EVENT_WINDOW_RESIZED;
                event->data.window_resized.w = xev.xconfigure.width;
                event->data.window_resized.h = xev.xconfigure.height;
                event->data.window_resized.is_fullscreen = win->is_fullscreen;
            }
            else if (xev.xconfigure.x != win->x || xev.xconfigure.y != win->y)
            {
                event->type = TSDL_EVENT_WINDOW_MOVED;
                event->data.window_moved.x = xev.xconfigure.x;
                event->data.window_moved.y = xev.xconfigure.y;
            }
            win->w = xev.xconfigure.width;
            win->h = xev.xconfigure.height;
            win->x = xev.xconfigure.x;
            win->y = xev.xconfigure.y;
        }

        break;
        case MapNotify:
        {
            event->type = TSDL_EVENT_WINDOW_RESTORED;
        }

        break;
        case UnmapNotify:
        {
            event->type = TSDL_EVENT_WINDOW_MINIMIZED;
        }

        break;
        case Expose:
        {
            event->type = TSDL_EVENT_WINDOW_EXPOSED;
        }

        break;
        case KeyPress:
        {
            KeySym x11_key = XkbKeycodeToKeysym(global_display, xev.xkey.keycode, 0, xev.xkey.state & ShiftMask ? 1 : 0);
            TSDL_Keycode key = map_keys(x11_key);
            event->type = TSDL_EVENT_KEY_DOWN;
            event->data.key.keycode = key;
            event->data.key.repeat = (xev.xkey.state & 0x1000) ? 1 : 0;
            // Update modifier state
            switch (x11_key)
            {
            case XK_Shift_L:
                mod_state |= TSDL_MOD_LSHIFT;
                break;
            case XK_Shift_R:
                mod_state |= TSDL_MOD_RSHIFT;
                break;
            case XK_Control_L:
                mod_state |= TSDL_MOD_LCTRL;
                break;
            case XK_Control_R:
                mod_state |= TSDL_MOD_RCTRL;
                break;
            case XK_Alt_L:
                mod_state |= TSDL_MOD_LALT;
                break;
            case XK_Alt_R:
                mod_state |= TSDL_MOD_RALT;
                break;
            case XK_Super_L:
                mod_state |= TSDL_MOD_LSUPER;
                break;
            case XK_Super_R:
                mod_state |= TSDL_MOD_RSUPER;
                break;
            }
            event->data.key.mods = map_key_mods(xev.xkey.state);
            LOG_STAT("Key down: keycode=%d, mods=0x%x", event->data.key.keycode, event->data.key.mods);
        }

        break;
        case KeyRelease:
        {
            KeySym x11_key = XkbKeycodeToKeysym(global_display, xev.xkey.keycode, 0, xev.xkey.state & ShiftMask ? 1 : 0);
            TSDL_Keycode key = map_keys(x11_key);
            event->type = TSDL_EVENT_KEY_UP;
            event->data.key.keycode = key;
            event->data.key.repeat = 0;
            // Update modifier state
            switch (x11_key)
            {
            case XK_Shift_L:
                mod_state &= ~TSDL_MOD_LSHIFT;
                break;
            case XK_Shift_R:
                mod_state &= ~TSDL_MOD_RSHIFT;
                break;
            case XK_Control_L:
                mod_state &= ~TSDL_MOD_LCTRL;
                break;
            case XK_Control_R:
                mod_state &= ~TSDL_MOD_RCTRL;
                break;
            case XK_Alt_L:
                mod_state &= ~TSDL_MOD_LALT;
                break;
            case XK_Alt_R:
                mod_state &= ~TSDL_MOD_RALT;
                break;
            case XK_Super_L:
                mod_state &= ~TSDL_MOD_LSUPER;
                break;
            case XK_Super_R:
                mod_state &= ~TSDL_MOD_RSUPER;
                break;
            }
            event->data.key.mods = map_key_mods(xev.xkey.state);
            LOG_STAT("Key up: keycode=%d, mods=0x%x", event->data.key.keycode, event->data.key.mods);
        }

        break;
        case ButtonPress:
        {
            if (xev.xbutton.button == 4 || xev.xbutton.button == 5)
            {
                event->type = TSDL_EVENT_MOUSE_WHEEL;
                event->data.mouse_wheel.xoffset = 0;
                event->data.mouse_wheel.yoffset = (xev.xbutton.button == 4) ? 1 : -1;
            }
            else
            {
                event->type = TSDL_EVENT_MOUSE_BUTTON_DOWN;
                event->data.mouse_button.button = xev.xbutton.button - 1; // X11: 1=Left, TSDL: 0=Left
                event->data.mouse_button.mods = map_key_mods(xev.xbutton.state);
            }
        }

        break;
        case ButtonRelease:
        {
            if (xev.xbutton.button != 4 && xev.xbutton.button != 5)
            {
                event->type = TSDL_EVENT_MOUSE_BUTTON_UP;
                event->data.mouse_button.button = xev.xbutton.button - 1;
                event->data.mouse_button.mods = map_key_mods(xev.xbutton.state);
            }
        }

        break;
        case MotionNotify:
        {
            event->type = TSDL_EVENT_MOUSE_MOVED;
            event->data.mouse_moved.x = xev.xmotion.x;
            event->data.mouse_moved.y = xev.xmotion.y;
        }

        break;
        }

        if (event->type != TSDL_EVENT_NONE)
        {
            LOG_STAT("Event type set: %d", event->type);
            // Confirm event type
            return TSDL_TRUE;
        }
    }

    if (active_window && active_window->close_requested)
    {
        event->type = TSDL_EVENT_QUIT;
        active_window->close_requested = TSDL_FALSE;

        LOG_STAT("Quit event set: type=%d", event->type);
        // Log exact type

        return TSDL_TRUE;
    }

    return TSDL_FALSE;
}
const string tsdl_getVersion(void)
{
    return TSDL_VER;
}

// Window Functions ===========================================================
window window_create(string title, int x, int y, int w, int h, int flags)
{
    if (!is_initialized)
    {
        log_error(TSDL_ERR_INIT, "Window creation failed; No display available");
        return NULL;
    }

    window win = Mem.alloc(sizeof(struct tinysdl_window_s));
    if (!win)
    {
        /*
            If Mem.alloc fails, global_display remains open, which is fine. It’s a shared
            resource tied to init, not individual windows. Closing it here would break
            multi-window scenarios (if we add them) and force a re-init, which isn’t ideal.
         */
        log_error(TSDL_ERR_WINDOW, "Window memory allocation failed");
        return NULL;
    }

    win->display = global_display;
    win->xwindow = XCreateSimpleWindow(
        win->display, DefaultRootWindow(win->display),
        x, y, w, h, 0,
        BlackPixel(win->display, DefaultScreen(win->display)),
        WhitePixel(win->display, DefaultScreen(win->display)));
    if (!win->xwindow)
    {
        Mem.free(win);
        log_error(TSDL_ERR_WINDOW, "Failed to create X11 window");
        return NULL;
    }

    //  set initial window properties
    win->w = w;
    win->h = h;
    win->x = x;
    win->y = y;
    win->is_fullscreen = (flags & TSDL_WINDOW_FULLSCREEN) ? TSDL_TRUE : TSDL_FALSE;
    win->resto.w = w;
    win->resto.h = h;
    win->resto.x = x;
    win->resto.y = y;
    win->close_requested = TSDL_FALSE;

    //  set the window title
    XStoreName(win->display, win->xwindow, title);
    //  enable the window close button
    if (!XSetWMProtocols(win->display, win->xwindow, &wm_delete_window, 1))
    {
        XDestroyWindow(win->display, win->xwindow);
        Mem.free(win);
        log_error(TSDL_ERR_WINDOW, "Failed to set WM protocols");
        return NULL;
    }

    XSelectInput(win->display, win->xwindow, StructureNotifyMask | ExposureMask | KeyPressMask | KeyReleaseMask | FocusChangeMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask);

    int attribs[] = {GLX_RGBA, GLX_DOUBLEBUFFER, GLX_DEPTH_SIZE, 24, None};
    XVisualInfo *vi = glXChooseVisual(win->display, DefaultScreen(win->display), attribs);
    if (!vi)
    {
        XDestroyWindow(win->display, win->xwindow);
        Mem.free(win);
        log_error(TSDL_ERR_GL, "Failed to choose GLX visual");
        return NULL;
    }
    win->glx_context = glXCreateContext(win->display, vi, NULL, GL_TRUE);
    if (!win->glx_context)
    {
        XDestroyWindow(win->display, win->xwindow);
        Mem.free(win);
        log_error(TSDL_ERR_GL, "Failed to create GLX context");
        return NULL;
    }
    glXMakeCurrent(win->display, win->xwindow, win->glx_context);
    active_window = win;

    //  set window flags
    if (flags & TSDL_WINDOW_SHOWN)
    {
        XMapWindow(win->display, win->xwindow);
    }
    if (flags & TSDL_WINDOW_FULLSCREEN)
    {
        Atom wm_state = XInternAtom(win->display, "_NET_WM_STATE", TSDL_FALSE);
        Atom fullscreen = XInternAtom(win->display, "_NET_WM_STATE_FULLSCREEN", TSDL_FALSE);
        XEvent xev = {0};
        xev.type = ClientMessage;
        xev.xclient.window = win->xwindow;
        xev.xclient.message_type = wm_state;
        xev.xclient.format = 32;
        xev.xclient.data.l[0] = 1;
        xev.xclient.data.l[1] = fullscreen;
        XSendEvent(win->display, DefaultRootWindow(win->display), TSDL_FALSE, SubstructureRedirectMask | SubstructureNotifyMask, &xev);
    }
    else if (flags & TSDL_WINDOW_MAXIMIZED)
    {
        Atom wm_state = XInternAtom(win->display, "_NET_WM_STATE", TSDL_FALSE);
        Atom max_horz = XInternAtom(win->display, "_NET_WM_STATE_MAXIMIZED_HORZ", TSDL_FALSE);
        Atom max_vert = XInternAtom(win->display, "_NET_WM_STATE_MAXIMIZED_VERT", TSDL_FALSE);
        XEvent xev = {0};
        xev.type = ClientMessage;
        xev.xclient.window = win->xwindow;
        xev.xclient.message_type = wm_state;
        xev.xclient.format = 32;
        xev.xclient.data.l[0] = 1; // _NET_WM_STATE_ADD
        xev.xclient.data.l[1] = max_horz;
        xev.xclient.data.l[2] = max_vert;
        XSendEvent(win->display, DefaultRootWindow(win->display), TSDL_FALSE, SubstructureRedirectMask | SubstructureNotifyMask, &xev);
    }
    else if (flags & TSDL_WINDOW_CENTERED)
    {
        int screen = DefaultScreen(win->display);
        int screen_width = DisplayWidth(win->display, screen);
        int screen_height = DisplayHeight(win->display, screen);
        x = (screen_width - w) / 2;
        y = (screen_height - h) / 2;
        XMoveWindow(win->display, win->xwindow, x, y);
        win->x = x;
        win->y = y;
        win->resto.x = x;
        win->resto.y = y;
    }
    else
    {
        XMoveWindow(win->display, win->xwindow, x, y);
    }

    if (flags & TSDL_WINDOW_SHOWN)
    {
        XMapWindow(win->display, win->xwindow);
    }
    XFlush(win->display);
    active_window = win;

    LOG_STAT("Window=%s {(%d, %d)}|{(%d, %d)} flags=%d", title, x, y, w, h, flags);

    return win;
}
void window_close(window win)
{
    if (!win || !win->xwindow)
    {
        log_error(TSDL_ERR_WINDOW, "Invalid window handle");
        return;
    }
    win->close_requested = TSDL_TRUE; // Mark for closure

    LOG_STAT("Window close requested");
}
void window_destroy(window win)
{
    if (!win)
    {
        log_error(TSDL_ERR_WINDOW, "Attempt to destroy null window");
        return;
    }
    if (win->xwindow && win->display)
    {
        glXMakeCurrent(win->display, None, NULL);
        if (win->glx_context)
        {
            glXDestroyContext(win->display, win->glx_context);
            win->glx_context = NULL;
        }
        XDestroyWindow(win->display, win->xwindow);
        XFlush(win->display);
    }

    if (win == active_window)
        active_window = NULL;
    Mem.free(win);

    //  [TODO] TASK: logging
    LOG_STAT("Window destroyed\n");
}
void window_toggleFullscreen(window win)
{
    Atom wm_state = XInternAtom(win->display, "_NET_WM_STATE", False);
    Atom fullscreen = XInternAtom(win->display, "_NET_WM_STATE_FULLSCREEN", False);

    XEvent event;
    memset(&event, 0, sizeof(event));

    event.type = ClientMessage;
    event.xclient.window = win->xwindow;
    event.xclient.message_type = wm_state;
    event.xclient.format = 32;
    event.xclient.data.l[0] = 2; // _NET_WM_STATE_TOGGLE
    event.xclient.data.l[1] = fullscreen;
    event.xclient.data.l[2] = 0; // No second property

    XSendEvent(win->display, DefaultRootWindow(win->display), False,
               SubstructureRedirectMask | SubstructureNotifyMask,
               &event);

    XFlush(win->display);

    LOG_STAT("Toggled fullscreen");
}
object window_getGLContext(window win)
{
    if (!win || !win->xwindow || win->glx_context)
    {
        log_error(TSDL_ERR_WINDOW, "Attempt to obtain GL context from invalid (NULL) window handle");
        return NULL;
    }
    glXMakeCurrent(win->display, win->xwindow, win->glx_context);
    return (object)win->glx_context;
}

// Specialized Helper Functions ===============================================
int map_key_mods(int x11_mods)
{
    int mods = mod_state; // Start with tracked state
    if (!(x11_mods & ShiftMask))
    {
        mods &= ~(TSDL_MOD_LSHIFT | TSDL_MOD_RSHIFT); // Clear if mask off
    }
    if (!(x11_mods & ControlMask))
    {
        mods &= ~(TSDL_MOD_LCTRL | TSDL_MOD_RCTRL);
    }
    if (!(x11_mods & Mod1Mask))
    {
        mods &= ~(TSDL_MOD_LALT | TSDL_MOD_RALT);
    }
    if (!(x11_mods & Mod4Mask))
    {
        mods &= ~(TSDL_MOD_LSUPER | TSDL_MOD_RSUPER);
    }
    return mods;
}
TSDL_Keycode map_keys(KeySym x11_key)
{
    switch (x11_key)
    {
    case XK_space:
        return TSDL_KEY_SPACE;
    case XK_apostrophe:
        return TSDL_KEY_APOSTROPHE;
    case XK_comma:
        return TSDL_KEY_COMMA;
    case XK_minus:
        return TSDL_KEY_MINUS;
    case XK_period:
        return TSDL_KEY_PERIOD;
    case XK_slash:
        return TSDL_KEY_SLASH;
    case XK_0:
        return TSDL_KEY_0;
    case XK_1:
        return TSDL_KEY_1;
    case XK_2:
        return TSDL_KEY_2;
    case XK_3:
        return TSDL_KEY_3;
    case XK_4:
        return TSDL_KEY_4;
    case XK_5:
        return TSDL_KEY_5;
    case XK_6:
        return TSDL_KEY_6;
    case XK_7:
        return TSDL_KEY_7;
    case XK_8:
        return TSDL_KEY_8;
    case XK_9:
        return TSDL_KEY_9;
    case XK_semicolon:
        return TSDL_KEY_SEMICOLON;
    case XK_equal:
        return TSDL_KEY_EQUAL;
    case XK_a:
    case XK_A:
        return TSDL_KEY_A;
    case XK_b:
    case XK_B:
        return TSDL_KEY_B;
    case XK_c:
    case XK_C:
        return TSDL_KEY_C;
    case XK_d:
    case XK_D:
        return TSDL_KEY_D;
    case XK_e:
    case XK_E:
        return TSDL_KEY_E;
    case XK_f:
    case XK_F:
        return TSDL_KEY_F;
    case XK_g:
    case XK_G:
        return TSDL_KEY_G;
    case XK_h:
    case XK_H:
        return TSDL_KEY_H;
    case XK_i:
    case XK_I:
        return TSDL_KEY_I;
    case XK_j:
    case XK_J:
        return TSDL_KEY_J;
    case XK_k:
    case XK_K:
        return TSDL_KEY_K;
    case XK_l:
    case XK_L:
        return TSDL_KEY_L;
    case XK_m:
    case XK_M:
        return TSDL_KEY_M;
    case XK_n:
    case XK_N:
        return TSDL_KEY_N;
    case XK_o:
    case XK_O:
        return TSDL_KEY_O;
    case XK_p:
    case XK_P:
        return TSDL_KEY_P;
    case XK_q:
    case XK_Q:
        return TSDL_KEY_Q;
    case XK_r:
    case XK_R:
        return TSDL_KEY_R;
    case XK_s:
    case XK_S:
        return TSDL_KEY_S;
    case XK_t:
    case XK_T:
        return TSDL_KEY_T;
    case XK_u:
    case XK_U:
        return TSDL_KEY_U;
    case XK_v:
    case XK_V:
        return TSDL_KEY_V;
    case XK_w:
    case XK_W:
        return TSDL_KEY_W;
    case XK_x:
    case XK_X:
        return TSDL_KEY_X;
    case XK_y:
    case XK_Y:
        return TSDL_KEY_Y;
    case XK_z:
    case XK_Z:
        return TSDL_KEY_Z;
    case XK_bracketleft:
        return TSDL_KEY_LEFT_BRACKET;
    case XK_backslash:
        return TSDL_KEY_BACKSLASH;
    case XK_bracketright:
        return TSDL_KEY_RIGHT_BRACKET;
    case XK_grave:
        return TSDL_KEY_GRAVE_ACCENT;
    case XK_Escape:
        return TSDL_KEY_ESCAPE;
    case XK_Return:
        return TSDL_KEY_ENTER;
    case XK_Tab:
        return TSDL_KEY_TAB;
    case XK_BackSpace:
        return TSDL_KEY_BACKSPACE;
    case XK_Insert:
        return TSDL_KEY_INSERT;
    case XK_Delete:
        return TSDL_KEY_DELETE;
    case XK_Right:
        return TSDL_KEY_RIGHT;
    case XK_Left:
        return TSDL_KEY_LEFT;
    case XK_Down:
        return TSDL_KEY_DOWN;
    case XK_Up:
        return TSDL_KEY_UP;
    case XK_Page_Up:
        return TSDL_KEY_PAGE_UP;
    case XK_Page_Down:
        return TSDL_KEY_PAGE_DOWN;
    case XK_Home:
        return TSDL_KEY_HOME;
    case XK_End:
        return TSDL_KEY_END;
    case XK_Caps_Lock:
        return TSDL_KEY_CAPS_LOCK;
    case XK_Scroll_Lock:
        return TSDL_KEY_SCROLL_LOCK;
    case XK_Num_Lock:
        return TSDL_KEY_NUM_LOCK;
    case XK_Print:
        return TSDL_KEY_PRINT_SCREEN;
    case XK_Pause:
        return TSDL_KEY_PAUSE;
    case XK_F1:
        return TSDL_KEY_F1;
    case XK_F2:
        return TSDL_KEY_F2;
    case XK_F3:
        return TSDL_KEY_F3;
    case XK_F4:
        return TSDL_KEY_F4;
    case XK_F5:
        return TSDL_KEY_F5;
    case XK_F6:
        return TSDL_KEY_F6;
    case XK_F7:
        return TSDL_KEY_F7;
    case XK_F8:
        return TSDL_KEY_F8;
    case XK_F9:
        return TSDL_KEY_F9;
    case XK_F10:
        return TSDL_KEY_F10;
    case XK_F11:
        return TSDL_KEY_F11;
    case XK_F12:
        return TSDL_KEY_F12;
    case XK_KP_0:
        return TSDL_KEY_KP_0;
    case XK_KP_1:
        return TSDL_KEY_KP_1;
    case XK_KP_2:
        return TSDL_KEY_KP_2;
    case XK_KP_3:
        return TSDL_KEY_KP_3;
    case XK_KP_4:
        return TSDL_KEY_KP_4;
    case XK_KP_5:
        return TSDL_KEY_KP_5;
    case XK_KP_6:
        return TSDL_KEY_KP_6;
    case XK_KP_7:
        return TSDL_KEY_KP_7;
    case XK_KP_8:
        return TSDL_KEY_KP_8;
    case XK_KP_9:
        return TSDL_KEY_KP_9;
    case XK_KP_Decimal:
        return TSDL_KEY_KP_DECIMAL;
    case XK_KP_Divide:
        return TSDL_KEY_KP_DIVIDE;
    case XK_KP_Multiply:
        return TSDL_KEY_KP_MULTIPLY;
    case XK_KP_Subtract:
        return TSDL_KEY_KP_SUBTRACT;
    case XK_KP_Add:
        return TSDL_KEY_KP_ADD;
    case XK_KP_Enter:
        return TSDL_KEY_KP_ENTER;
    case XK_KP_Equal:
        return TSDL_KEY_KP_EQUAL;
    case XK_Shift_L:
        return TSDL_KEY_LEFT_SHIFT;
    case XK_Control_L:
        return TSDL_KEY_LEFT_CONTROL;
    case XK_Alt_L:
        return TSDL_KEY_LEFT_ALT;
    case XK_Super_L:
        return TSDL_KEY_LEFT_SUPER;
    case XK_Shift_R:
        return TSDL_KEY_RIGHT_SHIFT;
    case XK_Control_R:
        return TSDL_KEY_RIGHT_CONTROL;
    case XK_Alt_R:
        return TSDL_KEY_RIGHT_ALT;
    case XK_Super_R:
        return TSDL_KEY_RIGHT_SUPER;
    case XK_Menu:
        return TSDL_KEY_MENU;
    default:
        return x11_key; // Fallback to raw keysym if unmapped
    }
}

// Internal Rendering Functions ===============================================
void tsdl_swapBuffers(window win)
{
    if (!win || !win->xwindow || !win->glx_context)
    {
        log_error(TSDL_ERR_WINDOW, "Invalid window for swap");
        return;
    }
    glXSwapBuffers(win->display, win->xwindow);
}
void tsdl_clear(window win)
{
    if (!win || !win->xwindow || !win->glx_context)
    {
        log_error(TSDL_ERR_WINDOW, "Invalid window for clear");
        return;
    }
    glXMakeCurrent(win->display, win->xwindow, win->glx_context);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
void tsdl_clearColor(window win, float r, float g, float b, float a)
{
    if (!win || !win->xwindow || !win->glx_context)
    {
        log_error(TSDL_ERR_WINDOW, "Invalid window for clear color");
        return;
    }
    glXMakeCurrent(win->display, win->xwindow, win->glx_context);
    glClearColor(r, g, b, a);
}
void tsdl_setViewport(window win, int x, int y, int w, int h)
{
    if (!win || !win->xwindow || !win->glx_context)
    {
        log_error(TSDL_ERR_WINDOW, "Invalid window for viewport");
        return;
    }
    glXMakeCurrent(win->display, win->xwindow, win->glx_context);
    glViewport(x, y, w, h);
}