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

python_screen_viewer::python_screen_viewer(std::shared_ptr<python_rt> rt, std::string surfacename)
{
	m_rt = rt;

	for (const auto &viewport : Global.python_viewports) {
		if (viewport.surface == surfacename) {
			auto conf = std::make_unique<window_state>();
			conf->size = viewport.size;
			conf->offset = viewport.offset;
			conf->scale = viewport.scale;

			GLFWmonitor *monitor = Application.find_monitor(viewport.monitor);
			if (!monitor && viewport.monitor != "window")
				continue;

			conf->window = Application.window(-1, true, conf->size.x, conf->size.y,
			                                 monitor, false, Global.python_sharectx);

			glfwSetWindowUserPointer(conf->window, this);
			glfwSetFramebufferSizeCallback(conf->window, texture_window_fb_resize);

			m_windows.push_back(std::move(conf));
		}
	}

	if (!m_windows.empty())
		m_renderthread = std::make_unique<std::thread>(&python_screen_viewer::threadfunc, this);
}

python_screen_viewer::~python_screen_viewer()
{
	m_exit = true;
	if (m_renderthread)
		m_renderthread->join();
}

void python_screen_viewer::threadfunc()
{
	for (auto &window : m_windows) {
		glfwMakeContextCurrent(window->window);

		glfwSwapInterval(Global.python_vsync ? 1 : 0);
		GLuint v;
		glGenVertexArrays(1, &v);
		glBindVertexArray(v);

		glActiveTexture(GL_TEXTURE0);

		if (Global.python_sharectx) {
			glBindTexture(GL_TEXTURE_2D, m_rt->shared_tex);
		}
		else {
			GLuint tex;
			glGenTextures(1, &tex);
			glBindTexture(GL_TEXTURE_2D, tex);
		}

		if (!Global.gfx_usegles && !Global.gfx_shadergamma)
			glEnable(GL_FRAMEBUFFER_SRGB);

		gl::program::unbind();
		gl::buffer::unbind();

		window->ubo = std::make_unique<gl::ubo>(sizeof(gl::scene_ubs), 0, GL_STREAM_DRAW);

		gl::shader vert("texturewindow.vert");
		gl::shader frag("texturewindow.frag");
		window->shader = std::make_unique<gl::program>(std::vector<std::reference_wrapper<const gl::shader>>({vert, frag}));
	}

	while (!m_exit)
	{
		auto start_time = std::chrono::high_resolution_clock::now();

		for (auto &window : m_windows) {

			unsigned char *image = nullptr;
			int format, components, width, height;

			if (!Global.python_sharectx) {
				std::lock_guard<std::mutex> guard(m_rt->mutex);

				if (window->timestamp == m_rt->timestamp)
					continue;

				window->timestamp = m_rt->timestamp;

				if (!m_rt->image)
					continue;

				format = m_rt->format;
				components = m_rt->components;
				width = m_rt->width;
				height = m_rt->height;

				size_t size = width * height * (components == GL_RGB ? 3 : 4);

				image = new unsigned char[size];
				memcpy(image, m_rt->image, size);
			}

			glfwMakeContextCurrent(window->window);
			gl::program::unbind();
			gl::buffer::unbind();

			window->shader->bind();
			window->ubo->bind_uniform();

			m_ubs.projection = glm::mat4(glm::mat3(glm::translate(glm::scale(glm::mat3(), 1.0f / window->scale), window->offset)));
			window->ubo->update(m_ubs);

			if (image) {
				glTexImage2D(
				    GL_TEXTURE_2D, 0,
				    format,
				    width, height, 0,
				    components, GL_UNSIGNED_BYTE, image);

				delete[] image;

				if (Global.python_mipmaps) {
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
					glGenerateMipmap(GL_TEXTURE_2D);
				}
				else {
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				}
			}

			glViewport(0, 0, window->size.x, window->size.y);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			glfwSwapBuffers(window->window);
		}

		auto frametime = std::chrono::high_resolution_clock::now() - start_time;

		if ((Global.python_minframetime - frametime).count() > 0.0f)
			std::this_thread::sleep_for(Global.python_minframetime - frametime);
	}
}

void python_screen_viewer::notify_window_size(GLFWwindow *window, int w, int h)
{
	for (auto &conf : m_windows) {
		if (conf->window == window) {
			conf->size.x = w;
			conf->size.y = h;
			return;
		}
	}
}

python_screen_viewer::window_state::~window_state()
{
	if (!window)
		return;

	if (!Global.python_sharectx) {
		GLFWwindow *current = glfwGetCurrentContext();

		glfwMakeContextCurrent(window);

		ubo = nullptr;
		shader = nullptr;

		gl::program::unbind();
		gl::buffer::unbind();

		glfwMakeContextCurrent(current);
	}

	glfwDestroyWindow(window);
}
