/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "Classes.h"
#include "dumb3d.h"

class TMemCell
{
  private:
    Math3D::vector3 vPosition;
    std::string szText;
    double fValue1;
    double fValue2;
    TCommandType eCommand;
    bool bCommand; // czy zawiera komendę dla zatrzymanego AI
    TEvent *OnSent; // event dodawany do kolejki po wysłaniu komendy zatrzymującej skład
  public:
    std::string asTrackName; // McZapkie-100302 - zeby nazwe toru na ktory jest Putcommand wysylane pamietac
    TMemCell( Math3D::vector3 *p );
    void Init();
    void UpdateValues( std::string const &szNewText, double const fNewValue1, double const fNewValue2, int const CheckMask );
    bool Load(cParser *parser);
    void PutCommand( TController *Mech, Math3D::vector3 *Loc );
    bool Compare( std::string const &szTestText, double const fTestValue1, double const fTestValue2, int const CheckMask );
    bool Render();
    inline std::string const &Text() { return szText; }
    inline double Value1()
    {
        return fValue1;
    };
    inline double Value2()
    {
        return fValue2;
    };
    inline Math3D::vector3 Position()
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
