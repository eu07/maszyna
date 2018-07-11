#include "stdafx.h"

#include "postfx.h"

std::shared_ptr<gl::shader> gl::postfx::vertex;
std::shared_ptr<gl::vao> gl::postfx::vao;

gl::postfx::postfx(const std::string &s) : postfx(shader("postfx_" + s + ".frag"))
{
}

gl::postfx::postfx(const shader &s)
{
    if (!vertex)
        vertex = std::make_shared<gl::shader>("postfx.vert");
    if (!vao)
        vao = std::make_shared<gl::vao>();

    program.attach(*vertex);
    program.attach(s);
    program.link();
}

void gl::postfx::apply(opengl_texture &src, framebuffer *dst)
{
    if (dst)
    {
        dst->clear();
        dst->bind();
    }
    else
        framebuffer::unbind();

    program.bind();
    vao->bind();
    glActiveTexture(GL_TEXTURE0);
    src.bind();
    glDisable(GL_DEPTH_TEST);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
