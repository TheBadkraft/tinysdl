//	test_interface.c
#include "tinysdl.h"
#include <sigtest.h>
#include <stdio.h>
#include <unistd.h>

// Assert.isTrue(condition, "fail message");
// Assert.isFalse(condition, "fail message");
// Assert.areEqual(obj1, obj2, INT, "fail message");
// Assert.areEqual(obj1, obj2, PTR, "fail message");
// Assert.areEqual(obj1, obj2, STRING, "fail message");

void setup_test(void)
{
	printf("\n");
	flogf(stdout, "Interface Tests: setup_test\n");
	fflush(stdout);
}

//	test tinysdl interface
void test_window_create(void)
{
	printf("\n");
	fflush(stdout);

	// 	test init
	Assert.isTrue(TinySDL.init_video(TSDL_INIT_VIDEO) == 0, "TinySDL init failed");

	//	test createWindow
	window win = TinySDL.createWindow("TinySDL Window", 0, 0, 800, 600, TSDL_WINDOW_SHOWN);
	Assert.isTrue(win != NULL, "TinySDL createWindow failed");

	//	test window destruction
	TinySDL.destroyWindow(win);

	//	test quit
	TinySDL.quit();
}
//	test destroy null window
void test_window_destroy_null(void)
{
	printf("\n");
	fflush(stdout);

	//	test destroy null window
	TinySDL.destroyWindow(NULL);
}
//	test event polling
void test_event_polling(void)
{
	printf("\n");
	fflush(stdout);

	Assert.isTrue(TinySDL.init_video(TSDL_INIT_VIDEO) == 0, "TinySDL init failed");
	window win = TinySDL.createWindow("TinySDL Window", 0, 0, 800, 600, TSDL_WINDOW_SHOWN);
	Assert.isTrue(win != NULL, "TinySDL createWindow failed");

	TSDL_Event event;
	sleep(1);
	Assert.isTrue(TinySDL.pollEvent(&event) >= 0, "TinySDL pollEvent failed");

	TinySDL.destroyWindow(win);
	TinySDL.quit();
}

// Register test cases
__attribute__((constructor)) void init_sigtest_tests(void)
{
	// register_test("setup_test", setup_test);
	register_test("test_window_create", test_window_create);
	register_test("test_window_destroy_null", test_window_destroy_null);
	register_test("test_event_polling", test_event_polling);
}
