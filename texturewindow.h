#include "renderer.h"
#include <GLFW/glfw3.h>

class texture_window
{
	GLFWwindow *m_window;
	GLuint m_source;
	std::shared_ptr<std::thread> m_renderthread;
	GLFWmonitor *monitor = nullptr;

	bool m_exit = false;

	int m_win_w = 500, m_win_h = 500;
	int m_tex_w = 0, m_tex_h = 0;

	void threadfunc();

public:
	texture_window(GLuint src, std::string name);
	~texture_window();

	void notify_window_size(int w, int h);
};
