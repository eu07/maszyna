#include "stdafx.h"
#include "texturewindow.h"
#include "application.h"
#include "gl/shader.h"
#include "gl/vao.h"
#include "Logs.h"

void texture_window_fb_resize(GLFWwindow *win, int w, int h)
{
	texture_window *texwindow = (texture_window*)glfwGetWindowUserPointer(win);
	texwindow->notify_window_size(w, h);
}

texture_window::texture_window(texture_handle src, std::string surfacename)
{
	opengl_texture &tex = GfxRenderer.Texture(src);
	tex.create();
	m_source = tex.id;

	int window_w = m_win_w, window_h = m_win_h;

	auto iter = Global.python_monitormap.find(surfacename);
	if (iter != Global.python_monitormap.end())
		monitor = Application.find_monitor((*iter).second);

	if (monitor) {
		const GLFWvidmode *mode = glfwGetVideoMode(monitor);
		window_w = mode->width;
		window_h = mode->height;
	}

	m_window = Application.window(-1, true, window_w, window_h, monitor);

	glfwSetWindowUserPointer(m_window, this);
	glfwSetFramebufferSizeCallback(m_window, texture_window_fb_resize);

	m_renderthread = std::make_unique<std::thread>(&texture_window::threadfunc, this);
}

texture_window::~texture_window()
{
	m_exit = true;
	m_renderthread->join();
	glfwDestroyWindow(m_window);
}

void texture_window::threadfunc()
{
	glfwMakeContextCurrent(m_window);
	glfwSwapInterval(1);

	{
		GLuint v;
		glGenVertexArrays(1, &v);
		glBindVertexArray(v);
	}

	gl::shader vert("quad.vert");
	gl::shader frag("texturewindow.frag");
	gl::program shader(std::vector<std::reference_wrapper<const gl::shader>>({vert, frag}));
	gl::vao vao;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_source);
	shader.bind();
	vao.bind();

	while (!m_exit)
	{
		// if texture resized, update window size (maybe not necessary?)
		// don't care when on fullscreen
		if ((GLAD_GL_VERSION_3_3 || GLAD_GL_ES_VERSION_3_1) && !monitor)
		{
			int w, h;
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
			if (m_tex_w != w || m_tex_h != h)
			{
				m_tex_w = w;
				m_tex_h = h;
				m_win_w = w;
				m_win_h = h;
				glfwSetWindowSize(m_window, w, h); // eh, not thread-safe (but works?)
			}
		}

		glViewport(0, 0, m_win_w, m_win_h);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glfwSwapBuffers(m_window);
	}
}

void texture_window::notify_window_size(int w, int h)
{
	m_win_w = w;
	m_win_h = h;
}
