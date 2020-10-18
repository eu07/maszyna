/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "light.h"

struct opengl_light : public basic_light {

    GLuint id { (GLuint)-1 };

    void
        apply_intensity( float const Factor = 1.0f );
    void
        apply_angle();

    opengl_light &
        operator=( basic_light const &Right ) {
            basic_light::operator=( Right );
            return *this; }
};

//---------------------------------------------------------------------------
