//  src/tinysdl.c

/*
    Inspired by SDL 3.2.8 (https://github.com/libsdl-org/SDL), zlib licensed.
    =========================================================================

    This is a minimal implementation of SDL-like functionality for X11.
    It is not intended to be a complete replacement for SDL, but rather a
    demonstration of how to create a simple windowing system using X11.
 */

#include "tinysdl.h"
#include <string.h>

#ifdef TSDL_MOCK
#include "tinysdl_mock.h"
#else
#include "tinysdl_core.h"
#endif

char err_trace[TSDL_ERROR_SIZE] = "";

// Shared Helper Functions ====================================================
TSDL_ErrorState log_error(TSDL_ErrorState err_state, const string msg)
{
    static string format = "[TinySDL] Error(%d): %s\n";
    size_t current_len = strlen(err_trace);
    size_t remaining = TSDL_ERROR_SIZE - current_len - 1; // -1 for null terminator
    if (remaining > 0)
    {
        snprintf(err_trace + current_len, remaining, format, err_state, msg);
    }

    return err_state;
}
void log_stat(const string msg)
{
    static string format = "[TinySDL%s] %s\n";
    const char *backend = "";
#ifdef TSDL_MOCK
    backend = " :: Mock";
#elif defined(TSDL_BACKEND_X11)
    backend = " :: X11";
#else
    backend = " :: OpenGL";
#endif

    fprintf(stdout, format, backend, msg);
    fflush(stdout);
}
list copy_drop_paths(int count, const char **paths)
{
    if (count <= 0 || !paths)
    {
        log_error(TSDL_ERR, "Invalid paths or count");
        return NULL;
    }

    list path_list = List.new(count);
    if (!path_list)
    {
        log_error(TSDL_ERR, "Failed to create list for paths");
        return NULL;
    }
    string p = NULL;
    for (int i = 0; i < count; i++)
    {
        p = strdup(paths[i]);
        if (!p)
        {
            log_error(TSDL_ERR, "Failed to copy path");
            // I'm not cleaning up here because there could be other paths already added
            // or valid paths that follow. List will be cleaned up before the next event
            // is processed or when the program exits.
        }

        LOG_STAT("Adding Path[%d]=%s", i, p);
        // Debug log

        List.add(path_list, p);
    }

    return path_list;
}
void clear_drop_paths(Event ev)
{
    if (ev->type == TSDL_EVENT_DROP && ev->data.drop.paths)
    {
        LOG_STAT("Clearing drop paths");
        for (int i = 0; i < ev->data.drop.count; i++)
        {
            String.free(List.getAt(ev->data.drop.paths, i));
        }
        List.clear(ev->data.drop.paths);
        List.free(ev->data.drop.paths);
        ev->data.drop.count = 0;
        ev->data.drop.paths = NULL;
    }
}
Event create_event(TSDL_EventType type)
{
    Event event = (Event)Mem.alloc(sizeof(TSDL_Event));
    if (!event)
    {
        LOG_STAT("Failed to allocate memory for event [type=%d", type);
        log_error(TSDL_ERR, logBuffer);
        LOG_STAT("%s", tsdl_getError());

        return NULL;
    }
    memset(event, 0, sizeof(TSDL_Event));
    event->type = type;

    return event;
}

static IWindow window_impl;
static ITinySDL tinysdl_impl;

static void assign_implementation(void)
{
#ifdef TSDL_MOCK
    window_impl.create = mock_create;
    window_impl.close = mock_close;
    window_impl.destroy = mock_destroy;
    window_impl.toggleFullscreen = mock_toggleFullscreen;
    window_impl.getGLContext = mock_getGLContext;

    tinysdl_impl.window = &window_impl;
    tinysdl_impl.init = mock_init;
    tinysdl_impl.quit = mock_quit;
    tinysdl_impl.getError = mock_getError;
    tinysdl_impl.pollEvent = mock_pollEvent;
    tinysdl_impl.getVersion = mock_getVersion;
#else
    window_impl.create = window_create;
    window_impl.close = window_close;
    window_impl.destroy = window_destroy;
    window_impl.toggleFullscreen = window_toggleFullscreen;
    window_impl.getGLContext = window_getGLContext;

    tinysdl_impl.window = &window_impl;
    tinysdl_impl.init_video = tsdl_init_video;
    tinysdl_impl.quit = tsdl_quit;
    tinysdl_impl.getError = tsdl_getError;
    tinysdl_impl.pollEvent = tsdl_pollEvent;
    tinysdl_impl.getVersion = tsdl_getVersion;
#endif
}

ITinySDL TinySDL =
    {
        .window = &window_impl,
        .init_video = NULL,
        .quit = NULL,
        .pollEvent = NULL,
        .getError = NULL,
        .getVersion = NULL,
};

ITinySDL *get_tinysdl(void)
{
    return &TinySDL;
}

/** @brief Load up interface when constructed */
__attribute__((constructor)) static void tinysdl_init_constructor(void)
{
    // Initialize the TinySDL interface
    assign_implementation();
    *(ITinySDL *)&TinySDL = tinysdl_impl;
}