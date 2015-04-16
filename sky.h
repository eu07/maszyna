//---------------------------------------------------------------------------

#ifndef skyH
#define skyH

#include "MdlMngr.h"

class TSky
{
  private:
    TModel3d *mdCloud;

  public:
    TSky();
    ~TSky();
    void Init();
    void Render();
};

//---------------------------------------------------------------------------
#endif
/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/