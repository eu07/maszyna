#include "renderer.h"
#include <GLFW/glfw3.h>

class python_screen_viewer
{
	struct window_config {
		GLFWwindow *window;
		glm::ivec2 size;

		glm::vec2 offset;
		glm::vec2 scale;

		std::unique_ptr<gl::ubo> ubo;
	};

	std::vector<window_config> m_windows;

	GLuint m_source;
	std::shared_ptr<std::thread> m_renderthread;
	std::unique_ptr<gl::program> m_shader;
	gl::scene_ubs m_ubs;

	std::atomic_bool m_exit = false;

	void threadfunc();

public:
	python_screen_viewer(GLuint src, std::string name);
	~python_screen_viewer();

	void notify_window_size(GLFWwindow *window, int w, int h);
};
