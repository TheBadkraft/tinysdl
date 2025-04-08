// tinysdl_mock.c
#include "tinysdl_mock.h"
#include <stdio.h>
#include <string.h>

struct tinysdl_window_s
{
   int dummy;
};

int mock_init(int flags)
{
   // we can check valid flag configurations to mock error consitions

   LOG_STAT("Initialized: flags=%d", flags);

   return 0;
}
void mock_quit(void)
{
   LOG_STAT("Quit");
}
const string mock_getError(void)
{
   return err_trace;
}
int mock_pollEvent(TSDL_Event *event)
{
   return 0;
}

window mock_create(string title, int x, int y, int w, int h, int flags)
{
   window win = Mem.alloc(sizeof(struct tinysdl_window_s));
   if (!win)
   {
      log_error(TSDL_ERR_WINDOW, "Memory allocation failed");
      return NULL;
   }
   win->dummy = 1;
   LOG_STAT("Window=%s {(%d, %d)}|{(%d, %d)} flags=%d",
            title, x, y, w, h, flags);

   return win;
}
void mock_close(window win)
{
   if (!win)
   {
      log_error(TSDL_ERR_WINDOW, "Attempt to close null window");
      return;
   }
   LOG_STAT("Window closed");
}
void mock_destroy(window win)
{
   if (!win)
   {
      log_error(TSDL_ERR_WINDOW, "Attempt to destroy null window");
      LOG_STAT("Attempt to destroy null window");
      return;
   }
   Mem.free(win);
   LOG_STAT("Window destroyed");
}
void mock_toggleFullscreen(window win)
{
   if (!win)
   {
      log_error(TSDL_ERR_WINDOW, "Attempt to toggle fullscreen on null window");
      return;
   }
   LOG_STAT("Fullscreen toggled");
}
void *mock_getGLContext(window win)
{
   if (!win)
      return NULL;
   return (void *)0xCAFEFEED;
}