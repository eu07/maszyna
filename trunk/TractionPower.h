//---------------------------------------------------------------------------

#ifndef TractionPowerH
#define TractionPowerH
#include "parser.h" //Tolaris-010603

class TGroundNode;

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
 double TotalAdmitance;
 double TotalPreviousAdmitance;
 double OutputVoltage;
 bool FastFuse;
 bool SlowFuse;
 double FuseTimer;
 int FuseCounter;
protected:

public: //zmienne publiczne
 TTractionPowerSource *psNode[2]; //zasilanie na koñcach dla sekcji
 bool bSection; //czy jest sekcj¹
 TGroundNode *gMyNode; //Ra 2015-03: znowu prowizorka, aby mieæ nazwê do logowania
public:
 //AnsiString asName;
 __fastcall TTractionPowerSource();
 __fastcall ~TTractionPowerSource();
 void __fastcall Init(double u,double i);
 bool __fastcall Load(cParser *parser);
 bool __fastcall Render();
 bool __fastcall Update(double dt);
 double __fastcall CurrentGet(double res);
 void __fastcall VoltageSet(double v) {NominalVoltage=v;};
 void __fastcall PowerSet(TTractionPowerSource *ps);
};

//---------------------------------------------------------------------------
#endif
