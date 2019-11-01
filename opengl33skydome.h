/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "gl/vao.h"
#include "gl/shader.h"
#include "gl/buffer.h"

class opengl33_skydome {

public:
// constructors
    opengl33_skydome() = default;
// destructor
    ~opengl33_skydome() {;}
// methods
    // updates data stores on the opengl end. NOTE: unbinds buffers
    void update();
    // draws the skydome
    void render();

private:
// members
	std::optional<gl::buffer> m_vertexbuffer;
	std::optional<gl::buffer> m_indexbuffer;
	std::optional<gl::buffer> m_coloursbuffer;
	std::optional<gl::program> m_shader;
	std::optional<gl::vao> m_vao;
};
