//---------------------------------------------------------------------------

#ifndef skyH
#define skyH

#include "MdlMngr.h"

class TSky
{
private:
  TModel3d *mdCloud1;
  TModel3d *mdCloud2;
public:
  __fastcall TSky();
  __fastcall ~TSky();
  void __fastcall Init();
  void __fastcall RenderA();
  void __fastcall RenderB();
};

//---------------------------------------------------------------------------
#endif
