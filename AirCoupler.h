//---------------------------------------------------------------------------

#ifndef AirCouplerH
#define AirCouplerH

#include "Model3d.h"
#include "QueryParserComp.hpp"

class TAirCoupler
{
  private:
    //    TButtonType eType;
    TSubModel *pModelOn, *pModelOff, *pModelxOn;
    bool bOn;
    bool bxOn;
    void __fastcall Update();

  public:
    __fastcall TAirCoupler();
    __fastcall ~TAirCoupler();
    void __fastcall Clear();
    inline void TurnOn()
    {
        bOn = true;
        bxOn = false;
        Update();
    };
    inline void TurnOff()
    {
        bOn = false;
        bxOn = false;
        Update();
    };
    inline void TurnxOn()
    {
        bOn = false;
        bxOn = true;
        Update();
    };
    //  inline bool Active() { if ((pModelOn)||(pModelOff)) return true; return false;};
    int __fastcall GetStatus();
    void __fastcall Init(AnsiString asName, TModel3d *pModel);
    void __fastcall Load(TQueryParserComp *Parser, TModel3d *pModel);
    //  bool __fastcall Render();
};

//---------------------------------------------------------------------------
#endif
