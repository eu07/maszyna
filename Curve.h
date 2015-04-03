//---------------------------------------------------------------------------

#ifndef CurveH
#define CurveH

#include "QueryParserComp.hpp"
#include "Usefull.h"

class TCurve
{
  public:
    TCurve();
    ~TCurve();
    bool Init(int n, int c);
    float GetValue(int c, float p);
    bool SetValue(int c, float p, float v);
    bool Load(TQueryParserComp *Parser);
    bool LoadFromFile(AnsiString asName);
    bool SaveToFile(AnsiString asName);

    int iNumValues;
    int iNumCols;

  private:
    float **Values;
};
//---------------------------------------------------------------------------
#endif
