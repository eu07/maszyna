//---------------------------------------------------------------------------

#ifndef AirCouplerH
#define AirCouplerH

#include "Model3d.h"
#include "QueryParserComp.hpp"

//typedef enum { gt_Unknown, gt_Rotate, gt_Move } TButtonType;

class TAirCoupler
{
private:

//    TButtonType eType;
    TSubModel *pModelOn,*pModelOff,*pModelxOn;
    bool bOn;
    bool bxOn;    

  bool __fastcall Update();
public:
  __fastcall TAirCoupler();
  __fastcall ~TAirCoupler();
  __fastcall Clear();  
  inline void TurnOn() { bOn= true; bxOn= false; Update(); };
  inline void TurnOff() { bOn= false; bxOn= false; Update(); };
  inline void TurnxOn() { bOn= false; bxOn= true; Update(); };
//  inline bool Active() { if ((pModelOn)||(pModelOff)) return true; return false;};
  int __fastcall GetStatus();
  bool __fastcall Init(AnsiString asName, TModel3d *pModel);  
  bool __fastcall Load(TQueryParserComp *Parser, TModel3d *pModel);
//  bool __fastcall Render();
};

//---------------------------------------------------------------------------
#endif
