//---------------------------------------------------------------------------

#ifndef MemCellH
#define MemCellH

#include "Classes.h"
#include "dumb3d.h"
using namespace Math3D;

class TMemCell
{
private:
 vector3 vPosition;
 char *szText;
 double fValue1;
 double fValue2;
 TCommandType eCommand;
 bool bCommand; //czy zawiera komendê dla zatrzymanego AI
 TEvent *OnSent; //event dodawany do kolejki po wys³aniu komendy zatrzymuj¹cej sk³ad
public:
 AnsiString asTrackName; //McZapkie-100302 - zeby nazwe toru na ktory jest Putcommand wysylane pamietac
 __fastcall TMemCell(vector3 *p);
 __fastcall ~TMemCell();
 void __fastcall Init();
 void __fastcall UpdateValues(char *szNewText, double fNewValue1, double fNewValue2, int CheckMask);
 bool __fastcall Load(cParser *parser);
 void __fastcall PutCommand(TController *Mech, vector3 *Loc);
 bool __fastcall Compare(char *szTestText, double fTestValue1, double fTestValue2, int CheckMask);
 bool __fastcall Render();
 inline char* __fastcall Text() {return szText;};
 inline double __fastcall Value1() {return fValue1;};
 inline double __fastcall Value2() {return fValue2;};
 inline vector3 __fastcall Position() {return vPosition;};
 inline TCommandType __fastcall Command() {return eCommand;};
 inline bool __fastcall StopCommand() {return bCommand;};
 void __fastcall StopCommandSent();
 TCommandType __fastcall CommandCheck();
 bool __fastcall IsVelocity();
 void __fastcall AssignEvents(TEvent *e);
};

//---------------------------------------------------------------------------
#endif
