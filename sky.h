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

#include "Model3d.h"

class TSky {

    friend class opengl_renderer;

private:
    TModel3d *mdCloud { nullptr };

public:
    void Init();
};

//---------------------------------------------------------------------------
