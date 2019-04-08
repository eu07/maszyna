#include "widgets/cameraview.h"

ui::cameraview_panel::cameraview_panel()
    : ui_panel(LOC_STR(cameraview_window), false)
{

}

void ui::cameraview_panel::render()
{
	if (!is_open && !exit_thread)
		exit_thread = true;
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
}

void ui::cameraview_panel::workthread_func()
{
	while (!exit_thread);
	    std::this_thread::sleep_for(std::chrono::seconds(1));
}
