/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "Button.h"
#include "parser.h"
#include "Model3d.h"
#include "Console.h"
#include "Logs.h"

void TButton::Clear(int i)
{
    pModelOn = nullptr;
    pModelOff = nullptr;
    m_state = false;
    if (i >= 0)
        FeedbackBitSet(i);
    Update(); // kasowanie bitu Feedback, o ile jakiś ustawiony
};

void TButton::Init(std::string const &asName, TModel3d *pModel, bool bNewOn)
{
    if( pModel == nullptr ) { return; }

    pModelOn = pModel->GetFromName( (asName + "_on").c_str() );
    pModelOff = pModel->GetFromName( (asName + "_off").c_str() );
    m_state = bNewOn;
    Update();
};

void TButton::Load(cParser &Parser, TModel3d *pModel1, TModel3d *pModel2) {

    std::string submodelname;

    Parser.getTokens();
    if( Parser.peek() != "{" ) {
        // old fixed size config
        Parser >> submodelname;
    }
    else {
        // new, block type config
        // TODO: rework the base part into yaml-compatible flow style mapping
        cParser mappingparser( Parser.getToken<std::string>( false, "}" ) );
        submodelname = mappingparser.getToken<std::string>( false );
        // new, variable length section
        while( true == Load_mapping( mappingparser ) ) {
            ; // all work done by while()
        }
    }

    if( pModel1 ) {
        // poszukiwanie submodeli w modelu
        Init( submodelname, pModel1, false );
        if( ( pModelOn != nullptr ) || ( pModelOff != nullptr ) ) {
            // we got our models, bail out
            return;
        }
    }
    if( pModel2 ) {
        // poszukiwanie submodeli w modelu
        Init( submodelname, pModel2, false );
        if( ( pModelOn != nullptr ) || ( pModelOff != nullptr ) ) {
            // we got our models, bail out
            return;
        }
    }
    // if we failed to locate even one state submodel, cry
    ErrorLog( "Failed to locate sub-model \"" + submodelname + "\" in 3d model \"" + pModel1->NameGet() + "\"" );
};

bool
TButton::Load_mapping( cParser &Input ) {

    if( false == Input.getTokens( 2, true, ", " ) ) {
        return false;
    }

    std::string key, value;
    Input
        >> key
        >> value;

    if( key == "soundinc:" ) {
        m_soundfxincrease = (
            value != "none" ?
                sound_man->create_sound(value) :
                nullptr );
    }
    else if( key == "sounddec:" ) {
        m_soundfxdecrease = (
            value != "none" ?
                sound_man->create_sound(value) :
                nullptr );
    }
    return true; // return value marks a key: value pair was extracted, nothing about whether it's recognized
}

void
TButton::Turn( bool const State ) {

    if( State != m_state ) {

        m_state = State;
        play();
        Update();
    }
}

void TButton::Update() {

    if( (  bData != nullptr )
     && ( *bData != m_state ) ) {

        m_state = ( *bData );
        play();
    }

    if( pModelOn  != nullptr ) { pModelOn->iVisible  =   m_state; }
    if( pModelOff != nullptr ) { pModelOff->iVisible = (!m_state); }

#ifdef _WIN32
    if (iFeedbackBit) {
        // jeżeli generuje informację zwrotną
        if (m_state) // zapalenie
            Console::BitsSet(iFeedbackBit);
        else
            Console::BitsClear(iFeedbackBit);
    }
#endif
};

void TButton::AssignBool(bool const *bValue) {

    bData = bValue;
}

void
TButton::play() {

    play(
        m_state == true ?
            m_soundfxincrease :
            m_soundfxdecrease );
}

void
TButton::play( sound* Sound ) {

    if( Sound == nullptr ) { return; }

	Sound->stop();
	Sound->play();
    return;
}
