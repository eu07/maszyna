//---------------------------------------------------------------------------
#ifndef PoKeys55H
#define PoKeys55H
//---------------------------------------------------------------------------
class TPoKeys55
{//komunikacja z PoKeys bez okreœlania przeznaczenia pinów
 unsigned char cRequest; //numer ¿¹dania do sprawdzania odpowiedzi
 unsigned char OutputBuffer[65]; //Allocate a memory buffer equal to our endpoint size + 1
 unsigned char InputBuffer[65]; //Allocate a memory buffer equal to our endpoint size + 1
 int iPWM[8]; //0-5:wyjœcia PWM,6:analogowe,7:czêstotliwoœc PWM
 int iPWMbits;
 int iLastCommand;
 int iFaza;
 int iRepeats; //liczba powtórzeñ
public:
 float fAnalog[7]; //wejœcia analogowe, stan <0.0,1.0>
 int iInputs[8];
 __fastcall TPoKeys55();
 __fastcall ~TPoKeys55();
 bool __fastcall Connect();
 bool __fastcall Close();
 bool __fastcall Write(unsigned char c,unsigned char b3,unsigned char b4=0,unsigned char b5=0);
 bool __fastcall Read();
 bool __fastcall ReadLoop(int i);
 AnsiString __fastcall Version();
 bool __fastcall PWM(int x,float y);
 bool __fastcall Update();
};
//---------------------------------------------------------------------------
#endif
