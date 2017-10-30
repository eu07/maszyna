/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak

*/

#include "stdafx.h"
#include "Gauge.h"
#include "parser.h"
#include "Model3d.h"
#include "Timer.h"
#include "logs.h"

void TGauge::Init(TSubModel *NewSubModel, TGaugeType eNewType, double fNewScale, double fNewOffset, double fNewFriction, double fNewValue)
{ // ustawienie parametrów animacji submodelu
    if (NewSubModel)
    { // warunek na wszelki wypadek, gdyby się submodel nie
        // podłączył
        fFriction = fNewFriction;
        fValue = fNewValue;
        fOffset = fNewOffset;
        fScale = fNewScale;
        SubModel = NewSubModel;
        eType = eNewType;
        if (eType == gt_Digital)
        {
            TSubModel *sm = SubModel->ChildGet();
            do
            { // pętla po submodelach potomnych i obracanie ich o kąt zależy od
                // cyfry w (fValue)
                if (sm->pName.size())
                { // musi mieć niepustą nazwę
                    if (sm->pName[0] >= '0')
                        if (sm->pName[0] <= '9')
                            sm->WillBeAnimated(); // wyłączenie optymalizacji
                }
                sm = sm->NextGet();
            } while (sm);
        }
        else // a banan może być z optymalizacją?
            NewSubModel->WillBeAnimated(); // wyłączenie ignowania jedynkowego transformu
    }
};

bool TGauge::Load(cParser &Parser, TModel3d *md1, TModel3d *md2, double mul) {

    std::string submodelname, gaugetypename;
    double scale, offset, friction;

    Parser.getTokens();
    if( Parser.peek() != "{" ) {
        // old fixed size config
        Parser >> submodelname;
        gaugetypename = Parser.getToken<std::string>( true );
        Parser.getTokens( 3, false );
        Parser
            >> scale
            >> offset
            >> friction;
    }
    else {
        // new, block type config
        // TODO: rework the base part into yaml-compatible flow style mapping
        cParser mappingparser( Parser.getToken<std::string>( false, "}" ) );
        submodelname = mappingparser.getToken<std::string>( false );
        gaugetypename = mappingparser.getToken<std::string>( true );
        mappingparser.getTokens( 3, false );
        mappingparser
            >> scale
            >> offset
            >> friction;
        // new, variable length section
        while( true == Load_mapping( mappingparser ) ) {
            ; // all work done by while()
        }
    }

	scale *= mul;
		TSubModel *submodel = md1->GetFromName( submodelname );
    if( scale == 0.0 ) {
        ErrorLog( "Scale of 0.0 defined for sub-model \"" + submodelname + "\" in 3d model \"" + md1->NameGet() + "\". Forcing scale of 1.0 to prevent division by 0" );
        scale = 1.0;
    }
    if (submodel) // jeśli nie znaleziony
        md2 = nullptr; // informacja, że znaleziony
    else if (md2) // a jest podany drugi model (np. zewnętrzny)
        submodel = md2->GetFromName(submodelname); // to może tam będzie, co za różnica gdzie
    if( submodel == nullptr ) {
        ErrorLog( "Failed to locate sub-model \"" + submodelname + "\" in 3d model \"" + md1->NameGet() + "\"" );
    }

    std::map<std::string, TGaugeType> gaugetypes {
        { "mov", gt_Move },
        { "wip", gt_Wiper },
        { "dgt", gt_Digital }
    };
    auto lookup = gaugetypes.find( gaugetypename );
    auto const type = (
        lookup != gaugetypes.end() ?
            lookup->second :
            gt_Rotate );
    Init(submodel, type, scale, offset, friction);

    return md2 != nullptr; // true, gdy podany model zewnętrzny, a w kabinie nie było
};

bool
TGauge::Load_mapping( cParser &Input ) {

    if( false == Input.getTokens( 2, true, ", \n\r\t" ) ) {
        return false;
    }

    std::string key, value;
    Input
        >> key
        >> value;

    if( key == "soundinc:" ) {
        m_soundfxincrease = (
            value != "none" ?
                TSoundsManager::GetFromName( value, true ) :
                nullptr );
    }
    else if( key == "sounddec:" ) {
        m_soundfxdecrease = (
            value != "none" ?
                TSoundsManager::GetFromName( value, true ) :
                nullptr );
    }
    else if( key.compare( 0, std::min<std::size_t>( key.size(), 5 ), "sound" ) == 0 ) {
        // sounds assigned to specific gauge values, defined by key soundFoo: where Foo = value
        auto const indexstart { key.find_first_of( "-1234567890" ) };
        auto const indexend { key.find_first_not_of( "-1234567890", indexstart ) };
        if( indexstart != std::string::npos ) {
            m_soundfxvalues.emplace(
                std::stoi( key.substr( indexstart, indexend - indexstart ) ),
                ( value != "none" ?
                    TSoundsManager::GetFromName( value, true ) :
                    nullptr ) );
        }
    }
    return true; // return value marks a key: value pair was extracted, nothing about whether it's recognized
}

void TGauge::PermIncValue(double fNewDesired)
{
    fDesiredValue = fDesiredValue + fNewDesired * fScale + fOffset;
    if (fDesiredValue - fOffset > 360 / fScale)
    {
        fDesiredValue = fDesiredValue - (360 / fScale);
        fValue = fValue - (360 / fScale);
    }
};

void TGauge::IncValue(double fNewDesired)
{ // używane tylko dla uniwersali
    fDesiredValue = fDesiredValue + fNewDesired * fScale + fOffset;
    if (fDesiredValue > fScale + fOffset)
        fDesiredValue = fScale + fOffset;
};

void TGauge::DecValue(double fNewDesired)
{ // używane tylko dla uniwersali
    fDesiredValue = fDesiredValue - fNewDesired * fScale + fOffset;
    if (fDesiredValue < 0)
        fDesiredValue = 0;
};

// ustawienie wartości docelowej. plays provided fallback sound, if no sound was defined in the control itself
void
TGauge::UpdateValue( double fNewDesired, PSound Fallbacksound ) {

    auto const desiredtimes100 = static_cast<int>( std::round( 100.0 * fNewDesired ) );
    if( static_cast<int>( std::round( 100.0 * ( fDesiredValue - fOffset ) / fScale ) ) == desiredtimes100 ) {
        return;
    }
    fDesiredValue = fNewDesired * fScale + fOffset;
    // if there's any sound associated with new requested value, play it
    // check value-specific table first...
    if( desiredtimes100 % 100 == 0 ) {
        // filter out values other than full integers
        auto const lookup = m_soundfxvalues.find( desiredtimes100 / 100 );
        if( lookup != m_soundfxvalues.end() ) {
            play( lookup->second );
            return;
        }
    }
    // ...and if there isn't any, fall back on the basic set...
    auto const currentvalue = GetValue();
    if( ( currentvalue < fNewDesired ) && ( m_soundfxincrease != nullptr ) ) {
        play( m_soundfxincrease );
    }
    else if( ( currentvalue > fNewDesired ) && ( m_soundfxdecrease != nullptr ) ) {
        play( m_soundfxdecrease );
    }
    else if( Fallbacksound != nullptr ) {
        // ...and if that fails too, try the provided fallback sound from legacy system
        play( Fallbacksound );
    }
};

void TGauge::PutValue(double fNewDesired)
{ // McZapkie-281102: natychmiastowe wpisanie wartosci
    fDesiredValue = fNewDesired * fScale + fOffset;
    fValue = fDesiredValue;
};

double TGauge::GetValue() const {
    // we feed value in range 0-1 so we should be getting it reported in the same range
    return ( fValue - fOffset ) / fScale;
}

void TGauge::Update() {

    if( fValue != fDesiredValue ) {
        float dt = Timer::GetDeltaTime();
        if( ( fFriction > 0 ) && ( dt < 0.5 * fFriction ) ) {
            // McZapkie-281102: zabezpieczenie przed oscylacjami dla dlugich czasow
            fValue += dt * ( fDesiredValue - fValue ) / fFriction;
            if( std::abs( fDesiredValue - fValue ) <= 0.0001 ) {
                // close enough, we can stop updating the model
                fValue = fDesiredValue; // set it exactly as requested just in case it matters
            }
        }
        else {
            fValue = fDesiredValue;
        }
    }
    if( SubModel )
    { // warunek na wszelki wypadek, gdyby się submodel nie podłączył
        TSubModel *sm;
        switch (eType)
        {
        case gt_Rotate:
            SubModel->SetRotate(float3(0, 1, 0), fValue * 360.0);
            break;
        case gt_Move:
            SubModel->SetTranslate(float3(0, 0, fValue));
            break;
        case gt_Wiper:
            SubModel->SetRotate(float3(0, 1, 0), fValue * 360.0);
            sm = SubModel->ChildGet();
            if (sm)
            {
                sm->SetRotate(float3(0, 1, 0), fValue * 360.0);
                sm = sm->ChildGet();
                if (sm)
                    sm->SetRotate(float3(0, 1, 0), fValue * 360.0);
            }
            break;
        case gt_Digital: // Ra 2014-07: licznik cyfrowy
            sm = SubModel->ChildGet();
/*			std::string n = FormatFloat( "0000000000", floor( fValue ) ); // na razie tak trochę bez sensu
*/			std::string n( "000000000" + std::to_string( static_cast<int>( std::floor( fValue ) ) ) );
			if( n.length() > 10 ) { n.erase( 0, n.length() - 10 ); } // also dumb but should work for now
            do
            { // pętla po submodelach potomnych i obracanie ich o kąt zależy od cyfry w (fValue)
                if( sm->pName.size() ) {
                    // musi mieć niepustą nazwę
                    if( ( sm->pName[ 0 ] >= '0' )
                     && ( sm->pName[ 0 ] <= '9' ) ) {

                        sm->SetRotate(
                            float3( 0, 1, 0 ),
                            -36.0 * ( n[ '0' + 9 - sm->pName[ 0 ] ] - '0' ) );
                    }
                }
                sm = sm->NextGet();
            } while (sm);
            break;
        }
    }
};

void TGauge::Render(){};

void TGauge::AssignFloat(float *fValue)
{
    cDataType = 'f';
    fData = fValue;
};
void TGauge::AssignDouble(double *dValue)
{
    cDataType = 'd';
    dData = dValue;
};
void TGauge::AssignInt(int *iValue)
{
    cDataType = 'i';
    iData = iValue;
};
void TGauge::UpdateValue()
{ // ustawienie wartości docelowej z parametru
    switch (cDataType)
    { // to nie jest zbyt optymalne, można by zrobić osobne funkcje
    case 'f':
        fDesiredValue = (*fData) * fScale + fOffset;
        break;
    case 'd':
        fDesiredValue = (*dData) * fScale + fOffset;
        break;
    case 'i':
        fDesiredValue = (*iData) * fScale + fOffset;
        break;
    }
};

void
TGauge::play( PSound Sound ) {

    if( Sound == nullptr ) { return; }

    Sound->SetCurrentPosition( 0 );
    Sound->SetVolume( DSBVOLUME_MAX );
    Sound->Play( 0, 0, 0 );
    return;
}

//---------------------------------------------------------------------------
