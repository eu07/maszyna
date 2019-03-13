#include "stdafx.h"
#include "pythonscreenviewer.h"
#include "application.h"
#include "gl/shader.h"
#include "gl/vao.h"
#include "Logs.h"

void texture_window_fb_resize(GLFWwindow *win, int w, int h)
{
	python_screen_viewer *texwindow = (python_screen_viewer*)glfwGetWindowUserPointer(win);
	texwindow->notify_window_size(win, w, h);
}

python_screen_viewer::python_screen_viewer(GLuint src, std::string surfacename)
{
	m_source = src;

	for (const auto &viewport : Global.python_viewports) {
		if (viewport.surface == surfacename) {
			window_config conf;
			conf.size = viewport.size;
			conf.offset = viewport.offset;
			conf.scale = viewport.scale;
			conf.window = Application.window(-1, true, conf.size.x, conf.size.y,
			                                 Application.find_monitor(viewport.monitor));

			glfwSetWindowUserPointer(conf.window, this);
			glfwSetFramebufferSizeCallback(conf.window, texture_window_fb_resize);

			m_windows.push_back(std::move(conf));
		}
	}

	gl::shader vert("texturewindow.vert");
	gl::shader frag("texturewindow.frag");
	m_shader = std::make_unique<gl::program>(std::vector<std::reference_wrapper<const gl::shader>>({vert, frag}));

	m_renderthread = std::make_unique<std::thread>(&python_screen_viewer::threadfunc, this);
}

python_screen_viewer::~python_screen_viewer()
{
	m_exit = true;
	m_renderthread->join();

	for (auto &window : m_windows)
		glfwDestroyWindow(window.window);
}

void python_screen_viewer::threadfunc()
{
	for (auto &window : m_windows) {
		glfwMakeContextCurrent(window.window);

		glfwSwapInterval(1);
		GLuint v;
		glGenVertexArrays(1, &v);
		glBindVertexArray(v);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_source);

		window.ubo = std::make_unique<gl::ubo>(sizeof(gl::scene_ubs), 0, GL_STREAM_DRAW);
	}

	while (!m_exit)
	{
		for (auto &window : m_windows) {
			glfwMakeContextCurrent(window.window);
			m_shader->unbind();
			gl::buffer::unbind();

			m_shader->bind();
			window.ubo->bind_uniform();

			m_ubs.projection = glm::mat4(glm::mat3(glm::translate(glm::scale(glm::mat3(), 1.0f / window.scale), window.offset)));
			window.ubo->update(m_ubs);

			glViewport(0, 0, window.size.x, window.size.y);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			glfwSwapBuffers(window.window);
		}
	}
}

void python_screen_viewer::notify_window_size(GLFWwindow *window, int w, int h)
{
	for (auto &conf : m_windows) {
		if (conf.window == window) {
			conf.size.x = w;
			conf.size.y = h;
			return;
		}
	}
}
