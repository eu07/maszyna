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
using namespace std;

class TMemCell
{
  private:
    vector3 vPosition;
#ifdef EU07_USE_OLD_TMEMCELL_TEXT_ARRAY
    char *szText;
#else
    std::string szText;
#endif
    double fValue1;
    double fValue2;
    TCommandType eCommand;
    bool bCommand; // czy zawiera komendę dla zatrzymanego AI
    TEvent *OnSent; // event dodawany do kolejki po wysłaniu komendy zatrzymującej skład
  public:
    string
        asTrackName; // McZapkie-100302 - zeby nazwe toru na ktory jest Putcommand wysylane pamietac
    TMemCell(vector3 *p);
#ifdef EU07_USE_OLD_TMEMCELL_TEXT_ARRAY
    ~TMemCell();
#endif
    void Init();
#ifdef EU07_USE_OLD_TMEMCELL_TEXT_ARRAY
    void UpdateValues( char const *szNewText, double const fNewValue1, double const fNewValue2, int const CheckMask );
#else
    void UpdateValues( std::string const &szNewText, double const fNewValue1, double const fNewValue2, int const CheckMask );
#endif
    bool Load(cParser *parser);
    void PutCommand(TController *Mech, vector3 *Loc);
    bool Compare( char const *szTestText, double const fTestValue1, double const fTestValue2, int const CheckMask );
#ifndef EU07_USE_OLD_TMEMCELL_TEXT_ARRAY
    bool Compare( std::string const &szTestText, double const fTestValue1, double const fTestValue2, int const CheckMask );
#endif
    bool Render();
#ifdef EU07_USE_OLD_TMEMCELL_TEXT_ARRAY
    inline char * Text()
    {
        return szText;
    };
#else
    inline std::string const &Text() { return szText; }
#endif
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
