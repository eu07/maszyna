//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "Feedback.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)
//Ra: Obiekt generuj�cy informacje zwrotne o stanie symulacji

__fastcall TFeedback::TFeedback()
{
};
__fastcall TFeedback::~TFeedback()
{
};
void __fastcall TFeedback::BitsSet(int mask)
{//ustawienie bit�w o podanej masce
 if ((iBits&mask)!=mask)
 {iBits|=mask;
  BitsUpdate();
 }
};
void __fastcall TFeedback::BitsClear(int mask)
{//zerowanie bit�w o podanej masce
 if (iBits&mask)
 {iBits&=~mask;
  BitsUpdate();
 }
};
void __fastcall TFeedback::BitsUpdate()
{//aktualizacja stanu interfejsu
};

