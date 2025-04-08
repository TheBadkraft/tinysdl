// tinysdl.h
#ifndef TINY_SDL_H
#define TINY_SDL_H

#include <sigcore.h>

#ifdef _WIN32
#define TSDL_EXPORT __declspec(dllexport)
#else
#define TSDL_EXPORT __attribute__((visibility("default")))
#endif

#define TSDL_ERROR_SIZE 256 // Centralized size macro
extern char err_trace[TSDL_ERROR_SIZE];
static char logBuffer[128] = {0};

#define TSDL_INIT_VIDEO 0x0001 // Flag for video subsystem

#define CORE_VER "0.1.0" // Core version
#ifdef TSDL_BACKEND_X11
#define TSDL_BACKEND "X11"
#define BACKEND_ID "02" // X11 backend ID
#elif defined(TSDL_BACKEND_WIN32)
#define TSDL_BACKEND "Win32"
#define BACKEND_ID "03" // Win32 backend ID
#else
#define TSDL_BACKEND "GLFW" // Default
#define BACKEND_ID "01"     // GLFW backend ID
#endif

/** @brief Opaque handle to a TinySDL window */
typedef struct tinysdl_window_s *window;

/** @brief TinySDL window flags */
typedef enum
{
    TSDL_WINDOW_SHOWN = 1 << 0,
    TSDL_WINDOW_RESIZABLE = 1 << 2,
    TSDL_WINDOW_FULLSCREEN = 1 << 3,
    TSDL_WINDOW_CENTERED = 1 << 4,
    TSDL_WINDOW_MAXIMIZED = 1 << 5,
} TSDL_WindowFlags;
/** @brief TinySDL Boolean type */
typedef enum
{
    TSDL_FALSE = 0,
    TSDL_TRUE = 1,
} TSDL_Bool;
/** @brief TinySDL Error state */
typedef enum
{
    TSDL_ERR_NONE = 0,
    TSDL_ERR = 0x01,
    TSDL_ERR_INIT = 0x02,
    TSDL_ERR_WINDOW = 0x03,
    TSDL_ERR_GL = 0x04,
} TSDL_ErrorState;
/** @brief TinySDL window event types */
typedef enum
{
    TSDL_EVENT_NONE = 0X00,
    TSDL_EVENT_QUIT = 0X01,
    TSDL_EVENT_KEY_DOWN,
    TSDL_EVENT_KEY_UP,
    TSDL_EVENT_WINDOW_MOVED,
    TSDL_EVENT_WINDOW_RESIZED,
    TSDL_EVENT_WINDOW_MINIMIZED,
    TSDL_EVENT_WINDOW_MAXIMIZED,
    TSDL_EVENT_WINDOW_RESTORED,
    TSDL_EVENT_WINDOW_FOCUS_GAINED,
    TSDL_EVENT_WINDOW_FOCUS_LOST,
    TSDL_EVENT_WINDOW_EXPOSED,
    TSDL_EVENT_DROP,
    TSDL_EVENT_MOUSE_BUTTON_DOWN,
    TSDL_EVENT_MOUSE_BUTTON_UP,
    TSDL_EVENT_MOUSE_MOVED,
    TSDL_EVENT_MOUSE_WHEEL,
} TSDL_EventType;
/** @brief TinySDL key modifier flags */
typedef enum
{
    TSDL_MOD_NONE = 0,
    TSDL_MOD_LSHIFT = 1 << 0,
    TSDL_MOD_RSHIFT = 1 << 1,
    TSDL_MOD_LCTRL = 1 << 2,
    TSDL_MOD_RCTRL = 1 << 3,
    TSDL_MOD_LALT = 1 << 4,
    TSDL_MOD_RALT = 1 << 5,
    TSDL_MOD_LSUPER = 1 << 6,
    TSDL_MOD_RSUPER = 1 << 7,
} TSDL_ModKey;
/** @brief TinySDL event */
typedef struct
{
    TSDL_EventType type; // Event type
    union
    {
        struct
        {
            unsigned int keycode; //    TSDL_KEY_*
            int repeat;           //    0 = new press; 1 = repeat
            int mods;             //    TSDL_MOD_* (bitwise OR)
        } key;                    // Key event data
        struct
        {
            int w;             // New width
            int h;             // New height
            int is_fullscreen; // 1 = fullscreen, 0 = windowed
        } window_resized;      // Window resized event data
        struct
        {
            int x;      // New x position
            int y;      // New y position
        } window_moved; // Window moved event data
        struct
        {
            list paths; // List of dropped file paths
            int count;  // Number of dropped files
        } drop;
        struct
        {
            int button; // Button pressed (TSDL_BUTTON_LEFT, TSDL_BUTTON_RIGHT, etc.)
            int mods;   // Modifier keys (TSDL_MOD_* flags)
        } mouse_button; // Mouse button event data
        struct
        {
            int x;     // Mouse x position
            int y;     // Mouse y position
        } mouse_moved; // Mouse moved event data
        struct
        {
            double xoffset; // Horizontal scroll offset
            double yoffset; // Vertical scroll offset
        } mouse_wheel;      // Mouse wheel event data
    } data;                 // Event data
} TSDL_Event;
typedef TSDL_Event *Event;
/** @brief Keycode definitions. */
typedef enum
{
    TSDL_KEY_SPACE = 32,
    TSDL_KEY_APOSTROPHE = 39,
    TSDL_KEY_COMMA = 44,
    TSDL_KEY_MINUS = 45,
    TSDL_KEY_PERIOD = 46,
    TSDL_KEY_SLASH = 47,
    TSDL_KEY_0 = 48,
    TSDL_KEY_1 = 49,
    TSDL_KEY_2 = 50,
    TSDL_KEY_3 = 51,
    TSDL_KEY_4 = 52,
    TSDL_KEY_5 = 53,
    TSDL_KEY_6 = 54,
    TSDL_KEY_7 = 55,
    TSDL_KEY_8 = 56,
    TSDL_KEY_9 = 57,
    TSDL_KEY_SEMICOLON = 59,
    TSDL_KEY_EQUAL = 61,
    TSDL_KEY_A = 65,
    TSDL_KEY_B = 66,
    TSDL_KEY_C = 67,
    TSDL_KEY_D = 68,
    TSDL_KEY_E = 69,
    TSDL_KEY_F = 70,
    TSDL_KEY_G = 71,
    TSDL_KEY_H = 72,
    TSDL_KEY_I = 73,
    TSDL_KEY_J = 74,
    TSDL_KEY_K = 75,
    TSDL_KEY_L = 76,
    TSDL_KEY_M = 77,
    TSDL_KEY_N = 78,
    TSDL_KEY_O = 79,
    TSDL_KEY_P = 80,
    TSDL_KEY_Q = 81,
    TSDL_KEY_R = 82,
    TSDL_KEY_S = 83,
    TSDL_KEY_T = 84,
    TSDL_KEY_U = 85,
    TSDL_KEY_V = 86,
    TSDL_KEY_W = 87,
    TSDL_KEY_X = 88,
    TSDL_KEY_Y = 89,
    TSDL_KEY_Z = 90,
    TSDL_KEY_LEFT_BRACKET = 91,
    TSDL_KEY_BACKSLASH = 92,
    TSDL_KEY_RIGHT_BRACKET = 93,
    TSDL_KEY_GRAVE_ACCENT = 96,
    TSDL_KEY_ESCAPE = 256,
    TSDL_KEY_ENTER = 257,
    TSDL_KEY_TAB = 258,
    TSDL_KEY_BACKSPACE = 259,
    TSDL_KEY_INSERT = 260,
    TSDL_KEY_DELETE = 261,
    TSDL_KEY_RIGHT = 262,
    TSDL_KEY_LEFT = 263,
    TSDL_KEY_DOWN = 264,
    TSDL_KEY_UP = 265,
    TSDL_KEY_PAGE_UP = 266,
    TSDL_KEY_PAGE_DOWN = 267,
    TSDL_KEY_HOME = 268,
    TSDL_KEY_END = 269,
    TSDL_KEY_CAPS_LOCK = 280,
    TSDL_KEY_SCROLL_LOCK = 281,
    TSDL_KEY_NUM_LOCK = 282,
    TSDL_KEY_PRINT_SCREEN = 283,
    TSDL_KEY_PAUSE = 284,
    TSDL_KEY_F1 = 290,
    TSDL_KEY_F2 = 291,
    TSDL_KEY_F3 = 292,
    TSDL_KEY_F4 = 293,
    TSDL_KEY_F5 = 294,
    TSDL_KEY_F6 = 295,
    TSDL_KEY_F7 = 296,
    TSDL_KEY_F8 = 297,
    TSDL_KEY_F9 = 298,
    TSDL_KEY_F10 = 299,
    TSDL_KEY_F11 = 300,
    TSDL_KEY_F12 = 301,
    TSDL_KEY_KP_0 = 320,
    TSDL_KEY_KP_1 = 321,
    TSDL_KEY_KP_2 = 322,
    TSDL_KEY_KP_3 = 323,
    TSDL_KEY_KP_4 = 324,
    TSDL_KEY_KP_5 = 325,
    TSDL_KEY_KP_6 = 326,
    TSDL_KEY_KP_7 = 327,
    TSDL_KEY_KP_8 = 328,
    TSDL_KEY_KP_9 = 329,
    TSDL_KEY_KP_DECIMAL = 330,
    TSDL_KEY_KP_DIVIDE = 331,
    TSDL_KEY_KP_MULTIPLY = 332,
    TSDL_KEY_KP_SUBTRACT = 333,
    TSDL_KEY_KP_ADD = 334,
    TSDL_KEY_KP_ENTER = 335,
    TSDL_KEY_KP_EQUAL = 336,
    TSDL_KEY_LEFT_SHIFT = 340,
    TSDL_KEY_LEFT_CONTROL = 341,
    TSDL_KEY_LEFT_ALT = 342,
    TSDL_KEY_LEFT_SUPER = 343,
    TSDL_KEY_RIGHT_SHIFT = 344,
    TSDL_KEY_RIGHT_CONTROL = 345,
    TSDL_KEY_RIGHT_ALT = 346,
    TSDL_KEY_RIGHT_SUPER = 347,
    TSDL_KEY_MENU = 348
} TSDL_Keycode;

// Shared Helper Declarations =================================================
TSDL_ErrorState log_error(TSDL_ErrorState, const string);
void log_stat(const string);

list copy_drop_paths(int, const char **);
void clear_drop_paths(Event);
int map_key_mods(int);
Event create_event(TSDL_EventType);

#ifdef TSDL_DEBUG
// Variadic macro to handle both cases
#define LOG_STAT(fmt, ...)                      \
    do                                          \
    {                                           \
        sprintf(logBuffer, fmt, ##__VA_ARGS__); \
        log_stat(logBuffer);                    \
    } while (0)
#else
#define LOG_STAT(fmt, ...) \
    do                     \
    {                      \
    } while (0)
#endif

//  Interfaces ================================================================
/** @brief Interface for the Window */
typedef struct IWindow
{
    /** @brief Create a window with title, position, size, and flags */
    window (*create)(string, int, int, int, int, int);
    /** @brief Close the window */
    void (*close)(window);
    /** @brief Destroy a window and free its resources */
    void (*destroy)(window);
    /** @brief Toggle window fullscreen mode */
    void (*toggleFullscreen)(window);
    /** @brief Get GL context (for OpenGL users)*/
    object (*getGLContext)(window);
} IWindow;
/** @brief Interface for TinySDL core functionality */
typedef struct ITinySDL
{
    /** #brief Window interface */
    const IWindow *window;
    /** @brief Initialize TinySDL with video subsystem */
    int (*init_video)(void);
    /** @brief Shut down TinySDL and free resources */
    void (*quit)(void);
    /** @brief Run polling loop */
    int (*pollEvent)(TSDL_Event *event);
    /** @brief Get the last error message */
    const string (*getError)(void);
    /** @brief Get the core version + backend version info */
    const string (*getVersion)(void);
} ITinySDL;
/** @brief Global instance of the TinySDL interface */
extern ITinySDL TinySDL;

// Add this export function declaration
#ifdef __cplusplus
extern "C"
{
#endif

    /** @brief Get the TinySDL interface instance */
    TSDL_EXPORT ITinySDL *get_tinysdl(void);

#ifdef __cplusplus
}
#endif

#endif // TINY_SDL_H