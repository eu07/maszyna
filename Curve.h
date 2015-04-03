//---------------------------------------------------------------------------

#ifndef CurveH
#define CurveH

#include "QueryParserComp.hpp"
#include "Usefull.h"

class TCurve
{
  public:
    __fastcall TCurve();
    __fastcall ~TCurve();
    bool __fastcall Init(int n, int c);
    float __fastcall GetValue(int c, float p);
    bool __fastcall SetValue(int c, float p, float v);
    bool __fastcall Load(TQueryParserComp *Parser);
    bool __fastcall LoadFromFile(AnsiString asName);
    bool __fastcall SaveToFile(AnsiString asName);

    int iNumValues;
    int iNumCols;

  private:
    float **Values;
};
//---------------------------------------------------------------------------
#endif
