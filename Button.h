//---------------------------------------------------------------------------

#ifndef ButtonH
#define ButtonH

#include "Model3d.h"

//typedef enum { gt_Unknown, gt_Rotate, gt_Move } TButtonType;

class TButton
{
private:

//    TButtonType eType;
    TSubModel *pModelOn,*pModelOff;
    bool bOn;

  bool __fastcall Update();
public:
  __fastcall TButton();
  __fastcall ~TButton();
  __fastcall Clear();  
  inline void TurnOn() { bOn= true; Update(); };
  inline void TurnOff() { bOn= false; Update(); };
  inline void Switch() { bOn= !bOn; Update(); };
  inline bool Active() { if ((pModelOn)||(pModelOff)) return true; return false;};
  bool __fastcall Init(AnsiString asName, TModel3d *pModel, bool bNewOn=false);
  bool __fastcall Load(TQueryParserComp *Parser, TModel3d *pModel);
//  bool __fastcall Render();
};

//---------------------------------------------------------------------------
#endif
