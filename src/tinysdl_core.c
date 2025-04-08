// src/tinysdl_core.h
#include "tinysdl_core.h"
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <stdio.h>
#include <string.h>
//  internal
#include "internal/tsdl_rendering.h"

#ifndef BACKEND_VER
#define BACKEND_VER "0.1.0"
#endif
#ifndef TSDL_VER
#define TSDL_VER CORE_VER "+" BACKEND_ID "_" BACKEND_VER
#endif

struct tinysdl_window_s
{
   GLFWwindow *glfw_window; // GLFW window handle
   int w;                   // Window width
   int h;                   // Window height
   int is_fullscreen;       // Fullscreen flag
   int close_requested;     // Close requested flag
   struct
   {
      int w; // Restore to width
      int h; // Restore to height
      int x; // Restore to x position
      int y; // Restore to y position
   } resto;
};

static GLFWwindow *shared_context = NULL;
static window active_window = NULL;
static int is_initialized = 0; // Initialization flag
static queue ev_queue = NULL;  // Event queue
static int in_fs_toggle = 0;   // Flag to prevent recursive fullscreen toggle

// GLFW Callback Declarations =================================================
static void glfw_error_callback(int, const char *);
static void glfw_key_callback(GLFWwindow *, int, int, int, int);
static void glfw_window_size_callback(GLFWwindow *, int, int);
static void glfw_iconify_callback(GLFWwindow *, int);
static void glfw_maximize_callback(GLFWwindow *, int);
static void glfw_focus_callback(GLFWwindow *, int);
static void glfw_window_pos_callback(GLFWwindow *, int, int);
static void glfw_window_refresh_callback(GLFWwindow *);
static void glfw_drop_callback(GLFWwindow *, int, const char **);
static void glfw_mouse_button_callback(GLFWwindow *, int, int, int);
static void glfw_cursor_pos_callback(GLFWwindow *, double, double);
static void glfw_mouse_wheel_callback(GLFWwindow *, double, double);

// TSDL (OpenGL) Functions =============================================================
static TSDL_Keycode map_keys(int);

int tsdl_init_video(void)
{
   if (is_initialized)
   {
      return log_error(TSDL_ERR_INIT, "TinySDL is already initialized");
   }
   if (!glfwInit())
   {
      return log_error(TSDL_ERR_GL, "Failed to initialize GLFW");
   }
   // Set the error callback early
   glfwSetErrorCallback(glfw_error_callback);
   glfwWindowHint(GLFW_VISIBLE, TSDL_FALSE);
   shared_context = glfwCreateWindow(1, 1, "shared", NULL, NULL);
   if (!shared_context)
   {
      glfwTerminate();
      return log_error(TSDL_ERR_GL, "Failed to create shared context");
   }
   glfwMakeContextCurrent(shared_context);

   // Set GLFW window hints
   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
   glfwWindowHint(GLFW_RED_BITS, 8);
   glfwWindowHint(GLFW_GREEN_BITS, 8);
   glfwWindowHint(GLFW_BLUE_BITS, 8);
   glfwWindowHint(GLFW_ALPHA_BITS, 8);
   glfwWindowHint(GLFW_DEPTH_BITS, 24);
   glfwWindowHint(GLFW_STENCIL_BITS, 8);

   ev_queue = Queue.new(DEFAULT_EVQUEUE_SIZE);
   if (!ev_queue)
   {
      glfwTerminate();
      return log_error(TSDL_ERR_INIT, "Failed to create event queue");
   }

   is_initialized = TSDL_TRUE;
   // [TODO] Task: replace "OpenGL" with define for backend
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

   // clear any remaining events
   Event event;
   while (!Queue.isEmpty(ev_queue))
   {
      event = Queue.dequeue(ev_queue);
      if (event->type == TSDL_EVENT_DROP)
      {
         clear_drop_paths(event);
      }
   }
   // free the event queue
   Queue.free(ev_queue);
   ev_queue = NULL;
   // free the active window & shared context
   if (active_window)
   {
      window_destroy(active_window);
      active_window = NULL;
   }
   if (shared_context)
   {
      glfwDestroyWindow(shared_context);
      shared_context = NULL;
   }
   // Terminate GLFW
   glfwTerminate();
   is_initialized = TSDL_FALSE;

   LOG_STAT("Quit %s Backend", TSDL_BACKEND);
}
const string tsdl_getError(void)
{
   return err_trace;
}
int tsdl_pollEvent(Event event)
{
   if (!is_initialized)
   {
      log_error(TSDL_ERR_INIT, "TinySDL is not initialized");
      return TSDL_FALSE;
   }

   glfwPollEvents();
   Event queued_ev = (Event)Queue.dequeue(ev_queue);
   if (queued_ev)
   {
      *event = *queued_ev;
      Mem.free(queued_ev);
      return TSDL_TRUE;
   }

   // Check for window close using current_window
   if (active_window && glfwWindowShouldClose(active_window->glfw_window) && !active_window->close_requested)
   {
      event->type = TSDL_EVENT_QUIT;
      active_window->close_requested = 1;

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
      log_error(TSDL_ERR_INIT, "TinySDL is not initialized");
      return NULL;
   }
   glfwWindowHint(GLFW_VISIBLE, (flags & TSDL_WINDOW_SHOWN) ? TSDL_TRUE : TSDL_FALSE);
   glfwWindowHint(GLFW_RESIZABLE, (flags & TSDL_WINDOW_RESIZABLE) ? TSDL_TRUE : TSDL_FALSE);
   glfwWindowHint(GLFW_MAXIMIZED, (flags & TSDL_WINDOW_MAXIMIZED) ? TSDL_TRUE : TSDL_FALSE);

   GLFWwindow *glfw_win = NULL;
   if (flags & TSDL_WINDOW_FULLSCREEN)
   {
      GLFWmonitor *monitor = glfwGetPrimaryMonitor();
      const GLFWvidmode *mode = glfwGetVideoMode(monitor);
      glfw_win = glfwCreateWindow(mode->width, mode->height, title, monitor, shared_context);
      w = mode->width;
      h = mode->height;
   }
   else
   {
      glfw_win = glfwCreateWindow(w, h, title, NULL, shared_context);
   }

   if (!glfw_win)
   {
      log_error(TSDL_ERR_GL, "Failed to create GLFW window");
      return NULL;
   }

   // Handle positioning
   if (!(flags & TSDL_WINDOW_FULLSCREEN) && !(flags & TSDL_WINDOW_MAXIMIZED))
   {
      if (flags & TSDL_WINDOW_CENTERED)
      {
         GLFWmonitor *monitor = glfwGetPrimaryMonitor();
         const GLFWvidmode *mode = glfwGetVideoMode(monitor);
         x = (mode->width - w) / 2;
         y = (mode->height - h) / 2;
         glfwSetWindowPos(glfw_win, x, y);
      }
      else
      {
         glfwSetWindowPos(glfw_win, x, y);
      }
   }

   window win = Mem.alloc(sizeof(struct tinysdl_window_s));
   if (!win)
   {
      glfwDestroyWindow(glfw_win);
      log_error(TSDL_ERR_WINDOW, "Failed to allocate memory for window");
      return NULL;
   }

   win->glfw_window = glfw_win;
   win->w = w;
   win->h = h;
   win->is_fullscreen = (flags & TSDL_WINDOW_FULLSCREEN) ? TSDL_TRUE : TSDL_FALSE;
   win->close_requested = 0;
   win->resto.w = w;
   win->resto.h = h;
   win->resto.x = x;
   win->resto.y = y;

   // set glfw callbacks
   glfwSetWindowUserPointer(glfw_win, win);
   glfwSetKeyCallback(glfw_win, glfw_key_callback);
   glfwSetWindowSizeCallback(glfw_win, glfw_window_size_callback);
   glfwSetWindowPosCallback(glfw_win, glfw_window_pos_callback);
   glfwSetWindowIconifyCallback(glfw_win, glfw_iconify_callback);
   glfwSetWindowMaximizeCallback(glfw_win, glfw_maximize_callback);
   glfwSetWindowFocusCallback(glfw_win, glfw_focus_callback);
   glfwSetWindowRefreshCallback(glfw_win, glfw_window_refresh_callback);
   glfwSetDropCallback(glfw_win, glfw_drop_callback);
   glfwSetMouseButtonCallback(glfw_win, glfw_mouse_button_callback);
   glfwSetCursorPosCallback(glfw_win, glfw_cursor_pos_callback);
   glfwSetScrollCallback(glfw_win, glfw_mouse_wheel_callback);

   glfwMakeContextCurrent(glfw_win);
   glfwSwapInterval(1); // Enable V-Sync

   active_window = win;

   LOG_STAT("Window=%s {(%d, %d)}|{(%d, %d)} flags=%d", title, x, y, w, h, flags);

   return active_window;
}
void window_close(window win)
{
   if (!win || !win->glfw_window)
   {
      log_error(TSDL_ERR_WINDOW, "Invalid window handle");
      LOG_STAT(tsdl_getError());

      return;
   }

   glfwSetWindowShouldClose(win->glfw_window, TSDL_TRUE);
   LOG_STAT("Window close requested");
}
void window_destroy(window win)
{
   if (!win || !win->glfw_window)
   {
      log_error(TSDL_ERR_WINDOW, "Invalid window handle");
      LOG_STAT(tsdl_getError());

      return;
   }

   glfwDestroyWindow(win->glfw_window);
   Mem.free(win);
   LOG_STAT("Window destroyed");
}
void window_toggleFullscreen(window win)
{
   if (!win || !win->glfw_window)
   {
      log_error(TSDL_ERR_WINDOW, "Attempt to toggle full screen on invalid (NULL) window handle");
      LOG_STAT(tsdl_getError());

      return;
   }

   LOG_STAT("Toggling fullscreen");
   in_fs_toggle = 1; // set toggling flag

   // Store current GL state
   glfwMakeContextCurrent(win->glfw_window);
   GLint viewport[4];
   glGetIntegerv(GL_VIEWPORT, viewport);
   GLfloat clear_color[4];
   glGetFloatv(GL_COLOR_CLEAR_VALUE, clear_color);

   if (win->is_fullscreen)
   {
      // Exit fullscreen: restore to original size and position
      glfwSetWindowMonitor(win->glfw_window, NULL, win->resto.x, win->resto.y, win->resto.w, win->resto.h, 0);
      glfwSetWindowSize(win->glfw_window, win->resto.w, win->resto.h); // Explicitly set size
      glfwSetWindowPos(win->glfw_window, win->resto.x, win->resto.y);  // Explicitly set position
      win->is_fullscreen = 0;

      // Ensure it’s not maximized
      glfwRestoreWindow(win->glfw_window);
   }
   else
   {
      // Enter fullscreen: save current state
      glfwGetWindowPos(win->glfw_window, &win->resto.x, &win->resto.y);
      glfwGetWindowSize(win->glfw_window, &win->resto.w, &win->resto.h);
      GLFWmonitor *monitor = glfwGetPrimaryMonitor();
      const GLFWvidmode *mode = glfwGetVideoMode(monitor);
      glfwSetWindowMonitor(win->glfw_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
      win->is_fullscreen = 1;
   }

   // Restore GL state
   glfwMakeContextCurrent(win->glfw_window);
   glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
   glClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
   glfwSwapInterval(1);

   in_fs_toggle = 0; // reset toggling flag

   LOG_STAT("Fullscreen mode=%d", win->is_fullscreen);
}
object window_getGLContext(window win)
{
   if (!win || !win->glfw_window)
   {
      log_error(TSDL_ERR_WINDOW, "Attempt to obtain GL context from invalid (NULL) window handle");
      LOG_STAT(tsdl_getError());

      return NULL;
   }

   glfwMakeContextCurrent(win->glfw_window);
   return win->glfw_window;
}

// GLFW Callbacks =============================================================
static void glfw_error_callback(int error, const char *description)
{
   LOG_STAT("[GLFW] [Error=%d] %s", error, description);
   log_error(TSDL_ERR_GL, logBuffer);
}
static void glfw_key_callback(GLFWwindow *glfw_window, int key, int scancode, int action, int mods)
{
   if (key == GLFW_KEY_UNKNOWN)
      return;

   Event ev = NULL;
   TSDL_Keycode mapped_key;

   switch (action)
   {
   case GLFW_PRESS:
   case GLFW_REPEAT:
      if (!(ev = create_event(TSDL_EVENT_KEY_DOWN)))
         return;
      ev->data.key.keycode = mapped_key = map_keys(key);
      ev->data.key.repeat = (action == GLFW_REPEAT) ? 1 : 0;
      ev->data.key.mods = map_key_mods(mods);

      break;
   case GLFW_RELEASE:
      if (!(ev = create_event(TSDL_EVENT_KEY_UP)))
         return;
      ev->data.key.keycode = mapped_key = map_keys(key);
      ev->data.key.repeat = 0;
      ev->data.key.mods = map_key_mods(mods);

      break;
   default:
      LOG_STAT("Unknown key action");
      return; // Ignore unknown actions
   }
   Queue.enqueue(ev_queue, ev);

   // Debug logging to verify key mapping
   LOG_STAT("Key event: GLFW=%d, TSDL=%d, action=%d", key, mapped_key, action);
}
static void glfw_window_size_callback(GLFWwindow *glfw_window, int w, int h)
{
   window win = (window)glfwGetWindowUserPointer(glfw_window);
   if (win)
   {
      if (!win->is_fullscreen)
      {
         win->w = w;
         win->h = h;
         if (!in_fs_toggle)
         {
            win->resto.w = w;
            win->resto.h = h;
         }
      }
   }
   Event ev = create_event(TSDL_EVENT_WINDOW_RESIZED);
   if (!ev)
      return;
   ev->data.window_resized.w = w;
   ev->data.window_resized.h = h;
   ev->data.window_resized.is_fullscreen = win ? win->is_fullscreen : 0;
   Queue.enqueue(ev_queue, ev);
}
static void glfw_iconify_callback(GLFWwindow *glfw_window, int iconified)
{
   Event ev = create_event(iconified ? TSDL_EVENT_WINDOW_MINIMIZED : TSDL_EVENT_WINDOW_RESTORED);
   if (!ev)
      return;
   LOG_STAT(iconified ? "Iconify: Minimized" : "Iconify: Restored"); // Debug log
   Queue.enqueue(ev_queue, ev);
}
static void glfw_maximize_callback(GLFWwindow *glfw_window, int maximized)
{
   Event ev = create_event(maximized ? TSDL_EVENT_WINDOW_MAXIMIZED : TSDL_EVENT_WINDOW_RESTORED);
   if (!ev)
      return;
   LOG_STAT(maximized ? "Maximize: Maximized" : "Maximize: Restored"); // Debug log
   Queue.enqueue(ev_queue, ev);
}
static void glfw_focus_callback(GLFWwindow *glfw_window, int focused)
{
   Event ev = create_event(focused ? TSDL_EVENT_WINDOW_FOCUS_GAINED : TSDL_EVENT_WINDOW_FOCUS_LOST);
   if (!ev)
      return;
   LOG_STAT(focused ? "Focus: Gained" : "Focus: Lost"); // Debug log
   Queue.enqueue(ev_queue, ev);
}
static void glfw_window_pos_callback(GLFWwindow *glfw_window, int x, int y)
{
   window win = (window)glfwGetWindowUserPointer(glfw_window);
   if (win && !win->is_fullscreen && !in_fs_toggle)
   {
      win->resto.x = x;
      win->resto.y = y;

      Event ev = create_event(TSDL_EVENT_WINDOW_MOVED);
      if (!ev)
         return;
      ev->data.window_moved.x = x;
      ev->data.window_moved.y = y;
      Queue.enqueue(ev_queue, ev);
   }
}
static void glfw_window_refresh_callback(GLFWwindow *glfw_window)
{
   Event ev = create_event(TSDL_EVENT_WINDOW_EXPOSED);
   if (!ev)
      return;
   Queue.enqueue(ev_queue, ev);
}
static void glfw_drop_callback(GLFWwindow *glfw_window, int count, const char **paths)
{
   Event ev = create_event(TSDL_EVENT_DROP);
   if (!ev)
      return;
   ev->data.drop.paths = copy_drop_paths(count, paths); // GLFW manages this memory, valid only during callback
   ev->data.drop.count = count;
   Queue.enqueue(ev_queue, ev);
}
static void glfw_mouse_button_callback(GLFWwindow *glfw_window, int button, int action, int mods)
{
   Event ev = create_event(action == GLFW_PRESS ? TSDL_EVENT_MOUSE_BUTTON_DOWN : TSDL_EVENT_MOUSE_BUTTON_UP);
   if (!ev)
      return;
   ev->data.mouse_button.button = button;
   ev->data.mouse_button.mods = map_key_mods(mods);
   Queue.enqueue(ev_queue, ev);
}
static void glfw_cursor_pos_callback(GLFWwindow *glfw_window, double x, double y)
{
   Event ev = create_event(TSDL_EVENT_MOUSE_MOVED);
   if (!ev)
      return;
   ev->data.mouse_moved.x = x;
   ev->data.mouse_moved.y = y;
   Queue.enqueue(ev_queue, ev);
}
static void glfw_mouse_wheel_callback(GLFWwindow *glfw_window, double xoffset, double yoffset)
{
   Event ev = create_event(TSDL_EVENT_MOUSE_WHEEL);
   if (!ev)
      return;
   ev->data.mouse_wheel.xoffset = xoffset;
   ev->data.mouse_wheel.yoffset = yoffset;
   Queue.enqueue(ev_queue, ev);
}

// Specialized Helper Functions ===============================================
int map_key_mods(int glfw_mods)
{
   int mods = TSDL_MOD_NONE;
   if (glfw_mods & GLFW_MOD_SHIFT)
   {
      // GLFW_MOD_SHIFT is true if either shift is down; check specific keys
      if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
         mods |= TSDL_MOD_LSHIFT;
      if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
         mods |= TSDL_MOD_RSHIFT;
   }
   if (glfw_mods & GLFW_MOD_CONTROL)
   {
      if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
         mods |= TSDL_MOD_LCTRL;
      if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)
         mods |= TSDL_MOD_RCTRL;
   }
   if (glfw_mods & GLFW_MOD_ALT)
   {
      if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_LEFT_ALT) == GLFW_PRESS)
         mods |= TSDL_MOD_LALT;
      if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_RIGHT_ALT) == GLFW_PRESS)
         mods |= TSDL_MOD_RALT;
   }
   if (glfw_mods & GLFW_MOD_SUPER)
   {
      if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_LEFT_SUPER) == GLFW_PRESS)
         mods |= TSDL_MOD_LSUPER;
      if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_RIGHT_SUPER) == GLFW_PRESS)
         mods |= TSDL_MOD_RSUPER;
   }
   return mods;
}
/*
 * Map GLFW key codes to TinySDL key codes
 * This is an explicit mapping of GLFW key codes to TinySDL key codes.
 */
static TSDL_Keycode map_keys(int glfw_key)
{
   switch (glfw_key)
   {
   case GLFW_KEY_SPACE:
      return TSDL_KEY_SPACE;
   case GLFW_KEY_APOSTROPHE:
      return TSDL_KEY_APOSTROPHE;
   case GLFW_KEY_COMMA:
      return TSDL_KEY_COMMA;
   case GLFW_KEY_MINUS:
      return TSDL_KEY_MINUS;
   case GLFW_KEY_PERIOD:
      return TSDL_KEY_PERIOD;
   case GLFW_KEY_SLASH:
      return TSDL_KEY_SLASH;
   case GLFW_KEY_0:
      return TSDL_KEY_0;
   case GLFW_KEY_1:
      return TSDL_KEY_1;
   case GLFW_KEY_2:
      return TSDL_KEY_2;
   case GLFW_KEY_3:
      return TSDL_KEY_3;
   case GLFW_KEY_4:
      return TSDL_KEY_4;
   case GLFW_KEY_5:
      return TSDL_KEY_5;
   case GLFW_KEY_6:
      return TSDL_KEY_6;
   case GLFW_KEY_7:
      return TSDL_KEY_7;
   case GLFW_KEY_8:
      return TSDL_KEY_8;
   case GLFW_KEY_9:
      return TSDL_KEY_9;
   case GLFW_KEY_SEMICOLON:
      return TSDL_KEY_SEMICOLON;
   case GLFW_KEY_EQUAL:
      return TSDL_KEY_EQUAL;
   case GLFW_KEY_A:
      return TSDL_KEY_A;
   case GLFW_KEY_B:
      return TSDL_KEY_B;
   case GLFW_KEY_C:
      return TSDL_KEY_C;
   case GLFW_KEY_D:
      return TSDL_KEY_D;
   case GLFW_KEY_E:
      return TSDL_KEY_E;
   case GLFW_KEY_F:
      return TSDL_KEY_F;
   case GLFW_KEY_G:
      return TSDL_KEY_G;
   case GLFW_KEY_H:
      return TSDL_KEY_H;
   case GLFW_KEY_I:
      return TSDL_KEY_I;
   case GLFW_KEY_J:
      return TSDL_KEY_J;
   case GLFW_KEY_K:
      return TSDL_KEY_K;
   case GLFW_KEY_L:
      return TSDL_KEY_L;
   case GLFW_KEY_M:
      return TSDL_KEY_M;
   case GLFW_KEY_N:
      return TSDL_KEY_N;
   case GLFW_KEY_O:
      return TSDL_KEY_O;
   case GLFW_KEY_P:
      return TSDL_KEY_P;
   case GLFW_KEY_Q:
      return TSDL_KEY_Q;
   case GLFW_KEY_R:
      return TSDL_KEY_R;
   case GLFW_KEY_S:
      return TSDL_KEY_S;
   case GLFW_KEY_T:
      return TSDL_KEY_T;
   case GLFW_KEY_U:
      return TSDL_KEY_U;
   case GLFW_KEY_V:
      return TSDL_KEY_V;
   case GLFW_KEY_W:
      return TSDL_KEY_W;
   case GLFW_KEY_X:
      return TSDL_KEY_X;
   case GLFW_KEY_Y:
      return TSDL_KEY_Y;
   case GLFW_KEY_Z:
      return TSDL_KEY_Z;
   case GLFW_KEY_LEFT_BRACKET:
      return TSDL_KEY_LEFT_BRACKET;
   case GLFW_KEY_BACKSLASH:
      return TSDL_KEY_BACKSLASH;
   case GLFW_KEY_RIGHT_BRACKET:
      return TSDL_KEY_RIGHT_BRACKET;
   case GLFW_KEY_GRAVE_ACCENT:
      return TSDL_KEY_GRAVE_ACCENT;
   case GLFW_KEY_ESCAPE:
      return TSDL_KEY_ESCAPE;
   case GLFW_KEY_ENTER:
      return TSDL_KEY_ENTER;
   case GLFW_KEY_TAB:
      return TSDL_KEY_TAB;
   case GLFW_KEY_BACKSPACE:
      return TSDL_KEY_BACKSPACE;
   case GLFW_KEY_INSERT:
      return TSDL_KEY_INSERT;
   case GLFW_KEY_DELETE:
      return TSDL_KEY_DELETE;
   case GLFW_KEY_RIGHT:
      return TSDL_KEY_RIGHT;
   case GLFW_KEY_LEFT:
      return TSDL_KEY_LEFT;
   case GLFW_KEY_DOWN:
      return TSDL_KEY_DOWN;
   case GLFW_KEY_UP:
      return TSDL_KEY_UP;
   case GLFW_KEY_PAGE_UP:
      return TSDL_KEY_PAGE_UP;
   case GLFW_KEY_PAGE_DOWN:
      return TSDL_KEY_PAGE_DOWN;
   case GLFW_KEY_HOME:
      return TSDL_KEY_HOME;
   case GLFW_KEY_END:
      return TSDL_KEY_END;
   case GLFW_KEY_CAPS_LOCK:
      return TSDL_KEY_CAPS_LOCK;
   case GLFW_KEY_SCROLL_LOCK:
      return TSDL_KEY_SCROLL_LOCK;
   case GLFW_KEY_NUM_LOCK:
      return TSDL_KEY_NUM_LOCK;
   case GLFW_KEY_PRINT_SCREEN:
      return TSDL_KEY_PRINT_SCREEN;
   case GLFW_KEY_PAUSE:
      return TSDL_KEY_PAUSE;
   case GLFW_KEY_F1:
      return TSDL_KEY_F1;
   case GLFW_KEY_F2:
      return TSDL_KEY_F2;
   case GLFW_KEY_F3:
      return TSDL_KEY_F3;
   case GLFW_KEY_F4:
      return TSDL_KEY_F4;
   case GLFW_KEY_F5:
      return TSDL_KEY_F5;
   case GLFW_KEY_F6:
      return TSDL_KEY_F6;
   case GLFW_KEY_F7:
      return TSDL_KEY_F7;
   case GLFW_KEY_F8:
      return TSDL_KEY_F8;
   case GLFW_KEY_F9:
      return TSDL_KEY_F9;
   case GLFW_KEY_F10:
      return TSDL_KEY_F10;
   case GLFW_KEY_F11:
      return TSDL_KEY_F11;
   case GLFW_KEY_F12:
      return TSDL_KEY_F12;
   case GLFW_KEY_KP_0:
      return TSDL_KEY_KP_0;
   case GLFW_KEY_KP_1:
      return TSDL_KEY_KP_1;
   case GLFW_KEY_KP_2:
      return TSDL_KEY_KP_2;
   case GLFW_KEY_KP_3:
      return TSDL_KEY_KP_3;
   case GLFW_KEY_KP_4:
      return TSDL_KEY_KP_4;
   case GLFW_KEY_KP_5:
      return TSDL_KEY_KP_5;
   case GLFW_KEY_KP_6:
      return TSDL_KEY_KP_6;
   case GLFW_KEY_KP_7:
      return TSDL_KEY_KP_7;
   case GLFW_KEY_KP_8:
      return TSDL_KEY_KP_8;
   case GLFW_KEY_KP_9:
      return TSDL_KEY_KP_9;
   case GLFW_KEY_KP_DECIMAL:
      return TSDL_KEY_KP_DECIMAL;
   case GLFW_KEY_KP_DIVIDE:
      return TSDL_KEY_KP_DIVIDE;
   case GLFW_KEY_KP_MULTIPLY:
      return TSDL_KEY_KP_MULTIPLY;
   case GLFW_KEY_KP_SUBTRACT:
      return TSDL_KEY_KP_SUBTRACT;
   case GLFW_KEY_KP_ADD:
      return TSDL_KEY_KP_ADD;
   case GLFW_KEY_KP_ENTER:
      return TSDL_KEY_KP_ENTER;
   case GLFW_KEY_KP_EQUAL:
      return TSDL_KEY_KP_EQUAL;
   case GLFW_KEY_LEFT_SHIFT:
      return TSDL_KEY_LEFT_SHIFT;
   case GLFW_KEY_LEFT_CONTROL:
      return TSDL_KEY_LEFT_CONTROL;
   case GLFW_KEY_LEFT_ALT:
      return TSDL_KEY_LEFT_ALT;
   case GLFW_KEY_LEFT_SUPER:
      return TSDL_KEY_LEFT_SUPER;
   case GLFW_KEY_RIGHT_SHIFT:
      return TSDL_KEY_RIGHT_SHIFT;
   case GLFW_KEY_RIGHT_CONTROL:
      return TSDL_KEY_RIGHT_CONTROL;
   case GLFW_KEY_RIGHT_ALT:
      return TSDL_KEY_RIGHT_ALT;
   case GLFW_KEY_RIGHT_SUPER:
      return TSDL_KEY_RIGHT_SUPER;
   case GLFW_KEY_MENU:
      return TSDL_KEY_MENU;
   default:
      return glfw_key; // Fallback to raw GLFW keycode if unmapped
   }
}

// Internal Rendering Functions ===============================================
void tsdl_swapBuffers(window win)
{
   if (!win || !win->glfw_window)
   {
      log_error(TSDL_ERR_GL, "Attempt to swap logBuffers on null window");
      return;
   }
   glfwSwapBuffers(win->glfw_window);
}
void tsdl_clear(window win)
{
   if (!win || !win->glfw_window)
   {
      log_error(TSDL_ERR_WINDOW, "Attempt to clear window on null window");
      return;
   }
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
void tsdl_clearColor(window win, float r, float g, float b, float a)
{
   if (!win || !win->glfw_window)
   {
      log_error(TSDL_ERR_WINDOW, "Attempt to set clear color on null window");
      return;
   }
   glClearColor(r, g, b, a);
}
void tsdl_setViewport(window win, int x, int y, int w, int h)
{
   if (!win || !win->glfw_window)
   {
      log_error(TSDL_ERR_WINDOW, "Attempt to set viewport on null window");
      return;
   }
   glViewport(x, y, w, h);
}