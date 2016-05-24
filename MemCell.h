/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

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
    bool bCommand; // czy zawiera komendê dla zatrzymanego AI
    TEvent *OnSent; // event dodawany do kolejki po wys³aniu komendy zatrzymuj¹cej sk³ad
  public:
    AnsiString
        asTrackName; // McZapkie-100302 - zeby nazwe toru na ktory jest Putcommand wysylane pamietac
    TMemCell(vector3 *p);
    ~TMemCell();
    void Init();
    void UpdateValues(char *szNewText, double fNewValue1, double fNewValue2, int CheckMask);
    bool Load(cParser *parser);
    void PutCommand(TController *Mech, vector3 *Loc);
    bool Compare(char *szTestText, double fTestValue1, double fTestValue2, int CheckMask);
    bool Render();
    inline char * Text()
    {
        return szText;
    };
    inline double Value1()
    {
        return fValue1;
    };
    inline double Value2()
    {
        return fValue2;
    };
    inline vector3 Position()
    {
        return vPosition;
    };
    inline TCommandType Command()
    {
        return eCommand;
    };
    inline bool StopCommand()
    {
        return bCommand;
    };
    void StopCommandSent();
    TCommandType CommandCheck();
    bool IsVelocity();
    void AssignEvents(TEvent *e);
};

//---------------------------------------------------------------------------
#endif
