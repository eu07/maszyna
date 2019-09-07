#include "uilayer.h"

class trainingcard_panel : public ui_panel
{
	std::string place;
	std::string trainee_name;
	std::string trainee_birthdate;
	std::string trainee_company;
	std::string instructor_name;
	std::string track_segment;
	std::string remarks;

	std::optional<std::time_t> start_time_wall;
	float distance = 0.0f;

	std::thread save_thread;
	std::atomic<int> state;

	void save_thread_func();
	void clear();

	virtual int StartRecording( void ) {
		return 1;
	}
	virtual int EndRecording( std::string training_identifier ) {
		return 1;
	}

  public:
	trainingcard_panel();
	~trainingcard_panel();

	void render_contents() override;
	const std::string *is_recording();

	std::string recording_timestamp;
};
