//---------------------------------------------------------------------------

#ifndef ConsoleH
#define ConsoleH
//---------------------------------------------------------------------------
class TConsoleDevice; //urz¹dzenie pod³¹czalne za pomoc¹ DLL
class TPoKeys55;
class TLPT;

class Console
{//Ra: klasa statyczna gromadz¹ca sygna³y steruj¹ce oraz informacje zwrotne
 //Ra: stan wejœcia zmieniany klawiatur¹ albo dedykowanym urz¹dzeniem
 //Ra: stan wyjœcia zmieniany przez symulacjê (mierniki, kontrolki)
private:
 static int iMode; //tryb pracy
 static int iConfig; //dodatkowa informacja o sprzêcie (np. numer LPT)
 static int iBits; //podstawowy zestaw lampek
 static TPoKeys55 *PoKeys55[2]; //mo¿e ich byæ kilka
 static TLPT *LPT;
 static void __fastcall BitsUpdate(int mask);
 //zmienne dla trybu "jednokabinowego", potrzebne do wspó³pracy z pulpitem (PoKeys)
 //u¿ywaj¹c klawiatury, ka¿dy pojazd powinien mieæ w³asny stan prze³¹czników
 //bazowym sterowaniem jest wirtualny strumieñ klawiatury
 //przy zmianie kabiny z PoKeys, do kabiny s¹ wysy³ane stany tych przycisków
 static int iSwitch[8]; //bistabilne w kabinie, za³¹czane z [Shift], wy³¹czane bez
 static int iButton[8]; //monostabilne w kabinie, za³¹czane podczas trzymania klawisza
public:
 __fastcall Console();
 __fastcall ~Console();
 static void __fastcall ModeSet(int m,int h=0);
 static void __fastcall BitsSet(int mask,int entry=0);
 static void __fastcall BitsClear(int mask,int entry=0);
 static int __fastcall On();
 static void __fastcall Off();
 static bool __fastcall Pressed(int x);
 static void __fastcall ValueSet(int x,double y);
 static void __fastcall Update();
 static float __fastcall AnalogGet(int x);
 static unsigned char __fastcall DigitalGet(int x);
 static void __fastcall OnKeyDown(int k);
 static void __fastcall OnKeyUp(int k);
};

#endif

