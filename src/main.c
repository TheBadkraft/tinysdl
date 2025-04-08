// src/main.c
#include "tinysdl.h"
#include <sigcore.h>
#include <stdio.h>
#include <unistd.h>
//  internal
#include "internal/tsdl_rendering.h"

int main(void)
{
	LOG_STAT("Launching Test Application");
	int running = 0;

	// Initialize TinySDL with video subsystem
	if (TinySDL.init_video() != 0)
	{
		LOG_STAT("[TinySDL] *** Failed: %s\n", TinySDL.getError());
		return -1;
	}

	LOG_STAT("backend=%s version=%s", TSDL_BACKEND, TinySDL.getVersion());

	// Create a window
	window win = TinySDL.window->create("TinySDL Window", 100, 100, 800, 600, TSDL_WINDOW_SHOWN | TSDL_WINDOW_CENTERED | TSDL_WINDOW_RESIZABLE | TSDL_WINDOW_FULLSCREEN);
	if (!win)
	{
		LOG_STAT("[TinySDL] *** Failed: %s\n", TinySDL.getError());
		TinySDL.quit();
		return -1;
	}

	//	we got this far; let's start the event loop
	running = 1;
	TSDL_Event event;
	LOG_STAT("Starting event loop");
	while (running)
	{
		tsdl_clear(win);
		tsdl_clearColor(win, 0.2f, 0.3f, 0.3f, 1.0f);

		while (TinySDL.pollEvent(&event))
		{
			switch (event.type)
			{
			case TSDL_EVENT_QUIT:
				LOG_STAT("Quit event received");
				running = 0;

				break;
			case TSDL_EVENT_KEY_DOWN:
				switch (event.data.key.keycode)
				{
				case TSDL_KEY_Q:
					TinySDL.window->close(win);

					break;
				case TSDL_KEY_F11:
					TinySDL.window->toggleFullscreen(win);

					break;
				}

				break;
			case TSDL_EVENT_WINDOW_MOVED:
				LOG_STAT("Moved:");
				printf("   %d x %d\n", event.data.window_moved.x, event.data.window_moved.y);

				break;
			case TSDL_EVENT_WINDOW_RESIZED:
				LOG_STAT("Resized:");
				printf("   %d x %d\n", event.data.window_resized.w, event.data.window_resized.h);
				tsdl_setViewport(win, 0, 0, event.data.window_resized.w, event.data.window_resized.h);

				break;
			case TSDL_EVENT_WINDOW_MINIMIZED:
				LOG_STAT("Minimized");

				break;
			case TSDL_EVENT_WINDOW_MAXIMIZED:
				LOG_STAT("Maximized");

				break;
			case TSDL_EVENT_WINDOW_RESTORED:
				LOG_STAT("Restored");

				break;
			case TSDL_EVENT_WINDOW_FOCUS_GAINED:
				LOG_STAT("Focus gained");

				break;
			case TSDL_EVENT_WINDOW_FOCUS_LOST:
				LOG_STAT("Focus lost");

				break;
			case TSDL_EVENT_WINDOW_EXPOSED:
				LOG_STAT("Window exposed");

				break;
			case TSDL_EVENT_MOUSE_BUTTON_DOWN:
				LOG_STAT("Mouse down:");
				printf("   button=%d, mods=%d\n", event.data.mouse_button.button, event.data.mouse_button.mods);

				break;
			case TSDL_EVENT_MOUSE_BUTTON_UP:
				LOG_STAT("Mouse up:");
				printf("   button=%d, mods=%d\n", event.data.mouse_button.button, event.data.mouse_button.mods);

				break;
			case TSDL_EVENT_MOUSE_MOVED:
				LOG_STAT("Mouse moved:");
				printf("   x=%.1d, y=%.1d\n", event.data.mouse_moved.x, event.data.mouse_moved.y);

				break;
			case TSDL_EVENT_MOUSE_WHEEL:
				LOG_STAT("Mouse wheel:");
				printf("   xoffset=%.1f, yoffset=%.1f\n", event.data.mouse_wheel.xoffset, event.data.mouse_wheel.yoffset);

				break;
			case TSDL_EVENT_DROP:
				/*
				 *		Drop event
				 *		- paths: list of dropped paths -- user shouold copy the strings if persistence is
				 *		  necessary. The backend will free the list and its contents when the next event
				 *		  is processed.
				 */
				LOG_STAT("Files dropped:");
				iterator it = Array.getIterator(event.data.drop.paths, LIST);
				while (Iterator.hasNext(it))
				{
					string path = Iterator.next(it);
					printf("   %s\n", path);
				}
				Iterator.free(it);

				break;
			default:
				break;
			}
		}
		//	rendering
		tsdl_swapBuffers(win);
	}

	TinySDL.quit();

	return 0;
}