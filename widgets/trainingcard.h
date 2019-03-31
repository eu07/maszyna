#include "uilayer.h"

class trainingcard_panel : public ui_panel
{
	std::string place;
	std::string trainee_name;
	std::string instructor_name;
	std::string remarks;

	std::optional<std::time_t> start_time_wall;

	std::thread save_thread;
	std::atomic<int> state;

	void save_thread_func();
	void clear();

	virtual int StartRecording( void ) {
		return 1;
	}
	virtual int EndRecording( std::string training_identifier ) {
		std::this_thread::sleep_for(std::chrono::duration<float>(1.0f));
		return 1;
	}

  public:
	trainingcard_panel();

	void render_contents() override;
};
