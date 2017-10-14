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
#include "scenenode.h"
#include "dumb3d.h"
#include "names.h"

class TMemCell : public editor::basic_node {

  private:
    // content
    std::string szText;
    double fValue1 { 0.0 };
    double fValue2 { 0.0 };
    // other
    TCommandType eCommand { cm_Unknown };
    bool bCommand { false }; // czy zawiera komendę dla zatrzymanego AI
    TEvent *OnSent { nullptr }; // event dodawany do kolejki po wysłaniu komendy zatrzymującej skład
  public:
    std::string asTrackName; // McZapkie-100302 - zeby nazwe toru na ktory jest Putcommand wysylane pamietac

    TMemCell( scene::node_data const &Nodedata );
    // legacy constructor
    TMemCell( Math3D::vector3 *p );

    void
        UpdateValues( std::string const &szNewText, double const fNewValue1, double const fNewValue2, int const CheckMask );
    bool
        Load(cParser *parser);
    void
#ifdef EU07_USE_OLD_GROUNDCODE
        PutCommand( TController *Mech, Math3D::vector3 *Loc );
#else
        PutCommand( TController *Mech, glm::dvec3 const *Loc );
#endif
    bool
        Compare( std::string const &szTestText, double const fTestValue1, double const fTestValue2, int const CheckMask );
    std::string const &
        Text() const {
            return szText; }
    double
        Value1() const {
            return fValue1; };
    double
        Value2() const {
            return fValue2; };
    TCommandType
        Command() const {
            return eCommand; };
    bool
        StopCommand() const {
            return bCommand; };
    void StopCommandSent();
    TCommandType CommandCheck();
    bool IsVelocity();
    void AssignEvents(TEvent *e);
};



class memory_table : public basic_table<TMemCell> {

public:
    // legacy method, initializes traction after deserialization from scenario file
    void
        InitCells();
};

//---------------------------------------------------------------------------
