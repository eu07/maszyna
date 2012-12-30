//---------------------------------------------------------------------------

#ifndef ButtonH
#define ButtonH

#include "Model3d.h"
#include "QueryParserComp.hpp"

class TButton
{
private:
 //TButtonType eType;
 TSubModel *pModelOn,*pModelOff;
 bool bOn;
 int iFeedbackBit; //Ra: informacja zwrotna: 0=SHP, 1=CA, 2-na oporach
 void __fastcall Update();
public:
 __fastcall TButton();
 __fastcall ~TButton();
 void __fastcall Clear(int i=-1);
 inline void FeedbackBitSet(int i) {iFeedbackBit=1<<i;};
 inline void TurnOn() { bOn= true; Update(); };
 inline void TurnOff() { bOn= false; Update(); };
 inline void Switch() { bOn= !bOn; Update(); };
 inline bool Active() { return (pModelOn)||(pModelOff);};
 void __fastcall Init(AnsiString asName, TModel3d *pModel, bool bNewOn=false);
 void __fastcall Load(TQueryParserComp *Parser,TModel3d *pModel1,TModel3d *pModel2=NULL);
};

//---------------------------------------------------------------------------
#endif
