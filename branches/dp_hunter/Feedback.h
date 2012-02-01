//---------------------------------------------------------------------------

#ifndef FeedbackH
#define FeedbackH
//---------------------------------------------------------------------------

class Feedback
{//Ra: klasa statyczna generuj¹ca informacje zwrotne o stanie symulacji
private:
 static int iMode; //tryb pracy
 static int iConfig; //dodatkowa informacja o sprzêcie (np. numer LPT)
 static int iBits; //podstawowy zestaw lampek
 static void __fastcall BitsUpdate(int mask);
public:
 __fastcall Feedback();
 __fastcall ~Feedback();
 static void __fastcall ModeSet(int m,int h=0);
 static void __fastcall BitsSet(int mask);
 static void __fastcall BitsClear(int mask);
};

#endif

