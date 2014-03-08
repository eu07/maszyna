//---------------------------------------------------------------------------

#ifndef TractionPowerH
#define TractionPowerH
#include "parser.h" //Tolaris-010603
//#include "QueryParserComp.hpp"

class TTractionPowerSource
{
private:
 double NominalVoltage;
 double VoltageFrequency;
 double InternalRes;
 double MaxOutputCurrent;
 double FastFuseTimeOut;
 int FastFuseRepetition;
 double SlowFuseTimeOut;
 bool Recuperation;

 double TotalCurrent;
 double TotalPreviousCurrent;
 double OutputVoltage;
 bool FastFuse;
 bool SlowFuse;
 double FuseTimer;
 int FuseCounter;
protected:

public: //zmienne publiczne
 TTractionPowerSource *psNode[2]; //zasilanie na ko�cach dla sekcji
 bool bSection; //czy jest sekcj�
public:
 //AnsiString asName;
 __fastcall TTractionPowerSource();
 __fastcall ~TTractionPowerSource();
 void __fastcall Init();
 bool __fastcall Load(cParser *parser);
 bool __fastcall Render();
 bool __fastcall Update(double dt);
 double __fastcall GetVoltage(double Current);
 void __fastcall VoltageSet(double v) {NominalVoltage=v;};
 void __fastcall PowerSet(TTractionPowerSource *ps);
};

//---------------------------------------------------------------------------
#endif
