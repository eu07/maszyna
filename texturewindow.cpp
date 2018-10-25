#include "stdafx.h"
#include "texturewindow.h"
#include "application.h"
#include "gl/shader.h"
#include "gl/vao.h"

void texture_window_fb_resize(GLFWwindow *win, int w, int h)
{
	texture_window *texwindow = (texture_window*)glfwGetWindowUserPointer(win);
	texwindow->notify_window_size(w, h);
}

texture_window::texture_window(texture_handle src)
{
	opengl_texture &tex = GfxRenderer.Texture(src);
	tex.create();
	m_source = tex.id;

	glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);

	GLFWwindow *root = Application.window();
	m_window = glfwCreateWindow(m_win_w, m_win_h, ("EU07: texture " + std::to_string(m_source)).c_str(), nullptr, root);

	glfwSetWindowUserPointer(m_window, this);
	glfwSetFramebufferSizeCallback(m_window, texture_window_fb_resize);

	glfwFocusWindow(root);

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
