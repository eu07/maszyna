//---------------------------------------------------------------------------
#ifndef PoKeys55H
#define PoKeys55H
//---------------------------------------------------------------------------
class TPoKeys55
{ // komunikacja z PoKeys bez okreœlania przeznaczenia pinów
    unsigned char cRequest; // numer ¿¹dania do sprawdzania odpowiedzi
    unsigned char OutputBuffer[65]; // Allocate a memory buffer equal to our endpoint size + 1
    unsigned char InputBuffer[65]; // Allocate a memory buffer equal to our endpoint size + 1
    int iPWM[8]; // 0-5:wyjœcia PWM,6:analogowe,7:czêstotliwoœc PWM
    int iPWMbits;
    int iLastCommand;
    int iFaza;
    int iRepeats; // liczba powtórzeñ
    bool bNoError; // zerowany po przepe³nieniu licznika powtórzeñ, ustawiany po udanej operacji
  public:
    float fAnalog[7]; // wejœcia analogowe, stan <0.0,1.0>
    int iInputs[8];
    TPoKeys55();
    ~TPoKeys55();
    bool Connect();
    bool Close();
    bool Write(unsigned char c, unsigned char b3, unsigned char b4 = 0,
                          unsigned char b5 = 0);
    bool Read();
    bool ReadLoop(int i);
    AnsiString Version();
    bool PWM(int x, float y);
    bool Update(bool pause);
};
//---------------------------------------------------------------------------
#endif
