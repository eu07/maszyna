#include "widgets/cameraview.h"
#include "stb/stb_image.h"
#include "Globals.h"
#include "Logs.h"

ui::cameraview_panel::cameraview_panel()
    : ui_panel(LOC_STR(cameraview_window), false)
{
	size_min = { -2, -2 };
}

void cameraview_window_callback(ImGuiSizeCallbackData *data) {
	data->DesiredSize.y = data->DesiredSize.x * 9.0f / 16.0f;
}

ui::cameraview_panel::~cameraview_panel() {
	if (!exit_thread) {
		exit_thread = true;
		{
			std::lock_guard<std::mutex> lock(mutex);
			stbi_image_free(image_ptr);
			image_ptr = nullptr;
		}
		cv.notify_all();
	}
	if (workthread.joinable())
		workthread.join();
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

	if (is_open)
		ImGui::SetNextWindowSizeConstraints(ImVec2(200, 200), ImVec2(2500, 2500), cameraview_window_callback);

	ui_panel::render();
}

void ui::cameraview_panel::render_contents()
{
	if (exit_thread) {
		if (workthread.joinable())
			workthread.join();
		exit_thread = false;
		workthread = std::thread(&cameraview_panel::workthread_func, this);
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
	if (!device)
		device = std::make_unique<VSDev>();

	while (!exit_thread) {
		uint8_t *buffer = nullptr;
		size_t len = device->GetFrameFromStream((char**)&buffer);

		if (buffer) {
			int w, h;
			uint8_t *image = stbi_load_from_memory(buffer, len, &w, &h, nullptr, 4);
			if (!image)
				ErrorLog(std::string(stbi_failure_reason()));

			device->FreeFrameBuffer((char*)buffer);

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
