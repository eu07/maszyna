//---------------------------------------------------------------------------

#ifndef skyH
#define skyH

#include "MdlMngr.h"

class TSky
{
private:
  TModel3d *mdCloud;
public:
  __fastcall Render();
  __fastcall TSky();
  __fastcall Init();
  __fastcall ~TSky();
};

//---------------------------------------------------------------------------
#endif
