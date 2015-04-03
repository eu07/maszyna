//---------------------------------------------------------------------------

#ifndef ButtonH
#define ButtonH

#include "Model3d.h"
#include "QueryParserComp.hpp"

class TButton
{ // animacja dwustanowa, w³¹cza jeden z dwóch submodeli (jednego z nich mo¿e nie byæ)
  private:
    TSubModel *pModelOn, *pModelOff; // submodel dla stanu za³¹czonego i wy³¹czonego
    bool bOn;
    int iFeedbackBit; // Ra: bit informacji zwrotnej, do wyprowadzenia na pulpit
    void __fastcall Update();

  public:
    __fastcall TButton();
    __fastcall ~TButton();
    void __fastcall Clear(int i = -1);
    inline void FeedbackBitSet(int i) { iFeedbackBit = 1 << i; };
    inline void Turn(bool to)
    {
        bOn = to;
        Update();
    };
    inline void TurnOn()
    {
        bOn = true;
        Update();
    };
    inline void TurnOff()
    {
        bOn = false;
        Update();
    };
    inline void Switch()
    {
        bOn = !bOn;
        Update();
    };
    inline bool Active() { return (pModelOn) || (pModelOff); };
    void __fastcall Init(AnsiString asName, TModel3d *pModel, bool bNewOn = false);
    void __fastcall Load(TQueryParserComp *Parser, TModel3d *pModel1, TModel3d *pModel2 = NULL);
};

//---------------------------------------------------------------------------
#endif
