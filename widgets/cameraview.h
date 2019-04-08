#include "stdafx.h"
#include "uilayer.h"
#include "translation.h"

namespace ui
{
class cameraview_panel : public ui_panel
{
	std::atomic_bool exit_thread = true;
	std::thread workthread;

	uint8_t* image_ptr = nullptr;
	int image_w, image_h;

	std::mutex mutex;
	std::condition_variable cv;

	std::optional<opengl_texture> texture;

	void workthread_func();

  public:
	cameraview_panel();

	void render() override;
	void render_contents() override;
};
} // namespace ui
