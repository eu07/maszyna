#include "stdafx.h"
#include "uilayer.h"
#include "translation.h"

namespace ui
{
class cameraview_panel : public ui_panel
{
	std::atomic_bool exit_thread = true;
	std::thread workthread;

	void workthread_func();

  public:
	cameraview_panel();

	void render() override;
	void render_contents() override;
};
} // namespace ui
