//	test_renderer.c
#include <sigtest.h>
#include <stdio.h>
#include "tinysdl.h"
// internal
#include "../src/internal/tsdl_rendering.h"

// Assert.isTrue(condition, "fail message");
// Assert.isFalse(condition, "fail message");
// Assert.areEqual(obj1, obj2, INT, "fail message");
// Assert.areEqual(obj1, obj2, PTR, "fail message");
// Assert.areEqual(obj1, obj2, STRING, "fail message");

void setup_test(void)
{
    printf("\n");
    flogf(stdout, "Renderer Tests: setup_test\n");
    fflush(stdout);
}

//  create a rendeerer
void test_create_renderer(void)
{
    printf("\n");
    fflush(stdout);

    Assert.isTrue(TinySDL.init_video(TSDL_INIT_VIDEO) == 0, "TinySDL init failed");
    window win = TinySDL.createWindow("TinySDL Window", 0, 0, 800, 600, TSDL_WINDOW_SHOWN);

    renderer r = create_renderer(win);
    Assert.isTrue(r != NULL, "Renderer creation failed");

    //  clean up renderer
    destroy_renderer(r);

    //  clean up TinySDL
    TinySDL.destroyWindow(win);
    TinySDL.quit();
}

// Register test cases
__attribute__((constructor)) void init_sigtest_tests(void)
{
    register_test("setup_test", setup_test);
    register_test("test_create_renderer", test_create_renderer);
}