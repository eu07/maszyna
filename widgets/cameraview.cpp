#include "widgets/cameraview.h"
#include "extras/VS_Dev.h"
#include "stb/stb_image.h"
#include "Globals.h"
#include "Logs.h"

ui::cameraview_panel::cameraview_panel()
    : ui_panel(LOC_STR(cameraview_window), false)
{
	size_min = {200, 200};
	size_max = {2000, 2000};
}

void ui::cameraview_panel::render()
{
	if (!is_open && !exit_thread) {
		exit_thread = true;
		{
			std::lock_guard<std::mutex> lock(mutex);
			stbi_image_free(image_ptr);
			image_ptr = nullptr;
		}
		cv.notify_all();
	}
	ui_panel::render();
}

void ui::cameraview_panel::render_contents()
{
	if (exit_thread) {
		if (workthread.joinable())
			workthread.join();
		workthread = std::thread(&cameraview_panel::workthread_func, this);
		exit_thread = false;
	}

	if (!texture) {
		texture.emplace();
		texture->make_stub();
	}

	texture->bind(0);

	{
		std::lock_guard<std::mutex> lock(mutex);
		if (image_ptr) {

			glActiveTexture(GL_TEXTURE0);

			glTexImage2D(
			    GL_TEXTURE_2D, 0,
			    GL_SRGB8_ALPHA8,
			    image_w, image_h, 0,
			    GL_RGBA, GL_UNSIGNED_BYTE, image_ptr);

			stbi_image_free(image_ptr);

			if (Global.python_mipmaps) {
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glGenerateMipmap(GL_TEXTURE_2D);
			}
			else {
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			}

			image_ptr = nullptr;
		}
	}
	cv.notify_all();

	ImVec2 surface_size = ImGui::GetContentRegionAvail();
	ImGui::Image(reinterpret_cast<void*>(texture->id), surface_size);
}

void ui::cameraview_panel::workthread_func()
{
	VSDev vsdev;

	while (!exit_thread) {
		uint8_t *buffer = nullptr;
		size_t len = vsdev.GetFrameFromStream((char**)&buffer);

		if (buffer) {
			int w, h;
			uint8_t *image = stbi_load_from_memory(buffer, len, &w, &h, nullptr, 4);
			if (!image)
				ErrorLog(std::string(stbi_failure_reason()));

			vsdev.FreeFrameBuffer((char*)buffer);

			std::unique_lock<std::mutex> lock(mutex);
			if (image_ptr)
				cv.wait(lock, [this]{return !image_ptr;});

			image_w = w;
			image_h = h;
			image_ptr = image;
		}
		else
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
}
