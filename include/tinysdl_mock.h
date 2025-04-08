// tinysdl_mock.h
#ifndef TINY_SDL_MOCK_H
#define TINY_SDL_MOCK_H

#include "tinysdl.h"

// tinysdl mocks
int mock_init(int);               // Mock initialization function (flags)
void mock_quit(void);             // Mock quit function
const string mock_getError(void); // Mock error retrieval function
int mock_pollEvent(TSDL_Event *); // Mock event polling function (*event)

// window mocks
window mock_create(string, int, int, int, int, int); // Mock window creation function (title, x, y, w, h, flags)
void mock_close(window);                             // Mock window close function (window)
void mock_destroy(window);                           // Mock window destroy function (window)
void mock_toggleFullscreen(window);                  // Mock toggle fullscreen function (window)
object mock_getGLContext(window);                    // Mock get GL context function (window)

#endif // TINY_SDL_MOCK_H