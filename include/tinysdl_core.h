// tinysdl_core.h
#ifndef TINY_SDL_CORE_H
#define TINY_SDL_CORE_H

#include "tinysdl.h"

#define DEFAULT_EVQUEUE_SIZE 16

int tsdl_init_video(void);          // Initialize TinySDL (only has video subsystem)
void tsdl_quit(void);               // Quit TinySDL
const string tsdl_getError(void);   // Get the last error message
int tsdl_pollEvent(TSDL_Event *);   // Poll for events
const string tsdl_getVersion(void); // Get the core version + backend version info

// Window functions
window window_create(string, int, int, int, int, int); // Create a window (title, x, y, w, h, flags)
void window_close(window);                             // Close a window
void window_destroy(window);                           // Destroy a window
void window_toggleFullscreen(window);                  // Toggle fullscreen mode
object window_getGLContext(window);                    // Get OpenGL context

#endif // TINY_SDL_CORE_H