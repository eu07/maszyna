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
