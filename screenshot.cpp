#include "stdafx.h"
#include "screenshot.h"
#include "Globals.h"
#include "Logs.h"
#include <png.h>

void screenshot_manager::screenshot_save_thread( char *img )
{
	png_image png;
	memset(&png, 0, sizeof(png_image));
	png.version = PNG_IMAGE_VERSION;
	png.width = Global.iWindowWidth;
	png.height = Global.iWindowHeight;
	png.format = PNG_FORMAT_RGB;

	char datetime[64];
	time_t timer;
	struct tm* tm_info;
	time(&timer);
	tm_info = localtime(&timer);
	strftime(datetime, 64, "%Y-%m-%d_%H-%M-%S", tm_info);

	uint64_t perf;
#ifdef _WIN32
	QueryPerformanceCounter((LARGE_INTEGER*)&perf);
#elif __unix__
	timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	perf = ts.tv_nsec;
#endif

	std::string filename = Global.screenshot_dir + "/" + std::string(datetime) +
	                       "_" + std::to_string(perf) + ".png";

	if (png_image_write_to_file(&png, filename.c_str(), 0, img, -Global.iWindowWidth * 3, nullptr) == 1)
		WriteLog("saved " + filename);
	else
		WriteLog("failed to save " + filename);

	delete[] img;
}

void screenshot_manager::make_screenshot()
{
	char *img = new char[Global.iWindowWidth * Global.iWindowHeight * 3];
	glReadPixels(0, 0, Global.iWindowWidth, Global.iWindowHeight, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)img);
	//m7t: use pbo

	std::thread t(screenshot_save_thread, img);
	t.detach();
}

