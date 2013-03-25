//---------------------------------------------------------------------------
#ifndef PoKeys55H
#define PoKeys55H
//---------------------------------------------------------------------------
class TPoKeys55
{
 unsigned char cRequest; //numer ¿¹dania do sprawdzania odpowiedzi
 unsigned char OutputBuffer[65]; //Allocate a memory buffer equal to our endpoint size + 1
 unsigned char InputBuffer[65]; //Allocate a memory buffer equal to our endpoint size + 1
 int iPWM[7];
 int iPWMbits;
 int iLastCommand;
 int iFaza;
public:
 float fAnalog[7]; //wejœcia analogowe, stan <0.0,1.0>
 __fastcall TPoKeys55();
 __fastcall ~TPoKeys55();
 bool __fastcall Connect();
 bool __fastcall Write(unsigned char c,unsigned char b3,unsigned char b4=0);
 bool __fastcall Read();
 bool __fastcall ReadLoop(int i);
 AnsiString __fastcall Version();
 bool __fastcall PWM(int x,float y);
 bool __fastcall Update();
};
//---------------------------------------------------------------------------
#endif
