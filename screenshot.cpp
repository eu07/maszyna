#include "stdafx.h"
#include "screenshot.h"
#include "Globals.h"
#include "Logs.h"
#include <png.h>

void screenshot_manager::screenshot_save_thread( char *img, int w, int h )
{
	png_image png;
	memset(&png, 0, sizeof(png_image));
	png.version = PNG_IMAGE_VERSION;
    png.width = w;
    png.height = h;

    int stride;
    if (Global.gfx_usegles)
    {
        png.format = PNG_FORMAT_RGBA;
        stride = -w * 4;

        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
                img[(y * w + x) * 4 + 3] = 0xFF;
    }
    else
    {
        png.format = PNG_FORMAT_RGB;
        stride = -w * 3;
    }

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

    if (png_image_write_to_file(&png, filename.c_str(), 0, img, stride, nullptr) == 1)
		WriteLog("saved " + filename);
	else
		WriteLog("failed to save " + filename);

	delete[] img;
}

void screenshot_manager::make_screenshot()
{
    char *img = new char[Global.fb_size.x * Global.fb_size.y * 4];
    glReadPixels(0, 0, Global.fb_size.x, Global.fb_size.y, Global.gfx_usegles ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)img);
	//m7t: use pbo

    std::thread t(screenshot_save_thread, img, Global.fb_size.x, Global.fb_size.y);
	t.detach();
}

