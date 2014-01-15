//---------------------------------------------------------------------------

#ifndef ConsoleH
#define ConsoleH
//---------------------------------------------------------------------------
class TConsoleDevice; //urz�dzenie pod��czalne za pomoc� DLL
class TPoKeys55;
class TLPT;

class Console
{//Ra: klasa statyczna gromadz�ca sygna�y steruj�ce oraz informacje zwrotne
 //Ra: stan wej�cia zmieniany klawiatur� albo dedykowanym urz�dzeniem
 //Ra: stan wyj�cia zmieniany przez symulacj� (mierniki, kontrolki)
private:
 static int iMode; //tryb pracy
 static int iConfig; //dodatkowa informacja o sprz�cie (np. numer LPT)
 static int iBits; //podstawowy zestaw lampek
 static TPoKeys55 *PoKeys55[2]; //mo�e ich by� kilka
 static TLPT *LPT;
 static void __fastcall BitsUpdate(int mask);
 //zmienne dla trybu "jednokabinowego", potrzebne do wsp�pracy z pulpitem (PoKeys)
 //u�ywaj�c klawiatury, ka�dy pojazd powinien mie� w�asny stan prze��cznik�w
 //bazowym sterowaniem jest wirtualny strumie� klawiatury
 //przy zmianie kabiny z PoKeys, do kabiny s� wysy�ane stany tych przycisk�w
 static int iSwitch[8]; //bistabilne w kabinie, za��czane z [Shift], wy��czane bez
 static int iButton[8]; //monostabilne w kabinie, za��czane podczas trzymania klawisza
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

