/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

class opengl_skydome {

public:
// constructors
    opengl_skydome() = default;
// destructor
    ~opengl_skydome();
// methods
    // updates data stores on the opengl end. NOTE: unbinds buffers
    void update();
    // draws the skydome
    void render();

private:
// members
    GLuint m_indexbuffer{ (GLuint)-1 }; // id of the buffer holding index data on the opengl end
    std::size_t m_indexcount { 0 };
    GLuint m_vertexbuffer{ (GLuint)-1 }; // id of the buffer holding vertex data on the opengl end
    GLuint m_coloursbuffer{ (GLuint)-1 }; // id of the buffer holding colour data on the opengl end
};
