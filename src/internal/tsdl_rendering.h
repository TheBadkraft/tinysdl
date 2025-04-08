//  internal/tsdl_rendering.h
//  =========================================================================

#include "tinysdl.h"

#ifndef TSDL_X11
void tsdl_swapBuffers(window);
void tsdl_clear(window);
void tsdl_clearColor(window, float, float, float, float);
void tsdl_setViewport(window, int, int, int, int);
#endif

#ifdef TSDL_X11
#include <X11/Xlib.h>
struct test_renderer_s
{
    Display *display;
    Window xwindow;
    GC gc;
};
typedef struct test_renderer_s *renderer;

renderer create_renderer(window);
void update_renderer(renderer);
void destroy_renderer(renderer);
#endif // TSDL_X11