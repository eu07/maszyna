//---------------------------------------------------------------------------
#ifndef PoKeys55H
#define PoKeys55H
//---------------------------------------------------------------------------
class TPoKeys55
{//komunikacja z PoKeys bez okre�lania przeznaczenia pin�w
 unsigned char cRequest; //numer ��dania do sprawdzania odpowiedzi
 unsigned char OutputBuffer[65]; //Allocate a memory buffer equal to our endpoint size + 1
 unsigned char InputBuffer[65]; //Allocate a memory buffer equal to our endpoint size + 1
 int iPWM[8]; //0-5:wyj�cia PWM,6:analogowe,7:cz�stotliwo�c PWM
 int iPWMbits;
 int iLastCommand;
 int iFaza;
 int iRepeats; //liczba powt�rze�
public:
 float fAnalog[7]; //wej�cia analogowe, stan <0.0,1.0>
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
