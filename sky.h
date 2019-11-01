/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/
//---------------------------------------------------------------------------

#pragma once

#include "Classes.h"

class TSky {

    friend opengl_renderer;
    friend opengl33_renderer;

public:
    TSky() = default;

    void Init();

private:
    TModel3d *mdCloud { nullptr };
};

//---------------------------------------------------------------------------
