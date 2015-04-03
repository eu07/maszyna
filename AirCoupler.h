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
    void Update();

  public:
    TAirCoupler();
    ~TAirCoupler();
    void Clear();
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
    int GetStatus();
    void Init(AnsiString asName, TModel3d *pModel);
    void Load(TQueryParserComp *Parser, TModel3d *pModel);
    //  bool Render();
};

//---------------------------------------------------------------------------
#endif
