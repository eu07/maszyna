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
#include "DynObj.h"
#include "Timer.h"
#include "Logs.h"
#include "renderer.h"

TGauge::TGauge( sound_source const &Soundtemplate ) :
    m_soundtemplate( Soundtemplate )
{
    m_soundfxincrease = m_soundtemplate;
    m_soundfxdecrease = m_soundtemplate;
}

void TGauge::Init(TSubModel *Submodel, TGaugeAnimation Type, float Scale, float Offset, float Friction, float Value, float const Endvalue, float const Endscale, bool const Interpolatescale )
{ // ustawienie parametrów animacji submodelu
    SubModel = Submodel;
    m_value = Value;
    m_animation = Type;
    m_scale = Scale;
    m_offset = Offset;
    m_friction = Friction;
    m_interpolatescale = Interpolatescale;
    m_endvalue = Endvalue;
    m_endscale = Endscale;

    if( Submodel == nullptr ) {
        // warunek na wszelki wypadek, gdyby się submodel nie podłączył
        return;
    }

    if( m_animation == TGaugeAnimation::gt_Digital ) {

        TSubModel *sm = SubModel->ChildGet();
        do {
            // pętla po submodelach potomnych i obracanie ich o kąt zależy od cyfry w (fValue)
            if( sm->pName.size() ) { // musi mieć niepustą nazwę
                if( sm->pName[ 0 ] >= '0' )
                    if( sm->pName[ 0 ] <= '9' )
                        sm->WillBeAnimated(); // wyłączenie optymalizacji
            }
            sm = sm->NextGet();
        } while( sm );
    }
    else // a banan może być z optymalizacją?
        Submodel->WillBeAnimated(); // wyłączenie ignowania jedynkowego transformu
    // pass submodel location to defined sounds
    auto const nulloffset { glm::vec3{} };
    auto const offset{ model_offset() };
    if( m_soundfxincrease.offset() == nulloffset ) {
        m_soundfxincrease.offset( offset );
    }
    if( m_soundfxdecrease.offset() == nulloffset ) {
        m_soundfxdecrease.offset( offset );
    }
    for( auto &soundfxrecord : m_soundfxvalues ) {
        if( soundfxrecord.second.offset() == nulloffset ) {
            soundfxrecord.second.offset( offset );
        }
    }
};

void TGauge::Load( cParser &Parser, TDynamicObject const *Owner, double const mul ) {

    std::string submodelname, gaugetypename;
    float scale, endscale, endvalue, offset, friction;
    endscale = -1;
    endvalue = -1;
    bool interpolatescale { false };

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
        if( ( gaugetypename == "rotvar" )
         || ( gaugetypename == "movvar" ) ) {
            interpolatescale = true;
            Parser.getTokens( 2, false );
            Parser
                >> endvalue
                >> endscale;
        }
    }
    else {
        // new, block type config
        // TODO: rework the base part into yaml-compatible flow style mapping
        submodelname = Parser.getToken<std::string>( false );
        gaugetypename = Parser.getToken<std::string>( true );
        Parser.getTokens( 3, false );
        Parser
            >> scale
            >> offset
            >> friction;
        if( ( gaugetypename == "rotvar" )
         || ( gaugetypename == "movvar" ) ) {
            interpolatescale = true;
            Parser.getTokens( 2, false );
            Parser
                >> endvalue
                >> endscale;
        }
        // new, variable length section
        while( true == Load_mapping( Parser ) ) {
            ; // all work done by while()
        }
    }

    // bind defined sounds with the button owner
    m_soundfxincrease.owner( Owner );
    m_soundfxdecrease.owner( Owner );
    for( auto &soundfxrecord : m_soundfxvalues ) {
        soundfxrecord.second.owner( Owner );
    }

	scale *= mul;
    if( interpolatescale ) {
        endscale *= mul;
    }
    TSubModel *submodel { nullptr };
    std::array<TModel3d *, 2> sources { Owner->mdKabina, Owner->mdLowPolyInt };
    for( auto const *source : sources ) {
        if( ( source != nullptr )
         && ( submodel = source->GetFromName( submodelname ) ) != nullptr ) {
            // got what we wanted, bail out
            break;
        }
    }
    if( submodel == nullptr ) {
        ErrorLog( "Bad model: failed to locate sub-model \"" + submodelname + "\" in 3d model(s) of \"" + Owner->name() + "\"", logtype::model );
    }

    std::map<std::string, TGaugeAnimation> gaugetypes {
        { "rot", TGaugeAnimation::gt_Rotate },
        { "rotvar", TGaugeAnimation::gt_Rotate },
        { "mov", TGaugeAnimation::gt_Move },
        { "movvar", TGaugeAnimation::gt_Move },
        { "wip", TGaugeAnimation::gt_Wiper },
        { "dgt", TGaugeAnimation::gt_Digital }
    };
    auto lookup = gaugetypes.find( gaugetypename );
    auto const type = (
        lookup != gaugetypes.end() ?
            lookup->second :
            TGaugeAnimation::gt_Unknown );

    Init( submodel, type, scale, offset, friction, 0, endvalue, endscale, interpolatescale );

//    return md2 != nullptr; // true, gdy podany model zewnętrzny, a w kabinie nie było
};

bool
TGauge::Load_mapping( cParser &Input ) {

    // token can be a key or block end
    auto const key { Input.getToken<std::string>( true, "\n\r\t  ,;" ) };
    if( ( true == key.empty() ) || ( key == "}" ) ) { return false; }
    // if not block end then the key is followed by assigned value or sub-block
    if( key == "type:" ) {
        auto const gaugetype { Input.getToken<std::string>( true, "\n\r\t  ,;" ) };
        m_type = (
            gaugetype == "impulse" ? TGaugeType::push :
            gaugetype == "return" ? TGaugeType::push :
            TGaugeType::toggle ); // default
    }
    else if( key == "soundinc:" ) {
        m_soundfxincrease.deserialize( Input, sound_type::single );
    }
    else if( key == "sounddec:" ) {
        m_soundfxdecrease.deserialize( Input, sound_type::single );
    }
    else if( key.compare( 0, std::min<std::size_t>( key.size(), 5 ), "sound" ) == 0 ) {
        // sounds assigned to specific gauge values, defined by key soundFoo: where Foo = value
        auto const indexstart { key.find_first_of( "-1234567890" ) };
        auto const indexend { key.find_first_not_of( "-1234567890", indexstart ) };
        if( indexstart != std::string::npos ) {
            m_soundfxvalues.emplace(
                std::stoi( key.substr( indexstart, indexend - indexstart ) ),
                sound_source( m_soundtemplate ).deserialize( Input, sound_type::single ) );
        }
    }
    return true; // return value marks a key: value pair was extracted, nothing about whether it's recognized
}

void
TGauge::UpdateValue( float fNewDesired ) {

    return UpdateValue( fNewDesired, nullptr );
}

void
TGauge::UpdateValue( float fNewDesired, sound_source &Fallbacksound ) {

    return UpdateValue( fNewDesired, &Fallbacksound );
}

// ustawienie wartości docelowej. plays provided fallback sound, if no sound was defined in the control itself
void
TGauge::UpdateValue( float fNewDesired, sound_source *Fallbacksound ) {

    auto const desiredtimes100 = static_cast<int>( std::round( 100.0 * fNewDesired ) );
    if( desiredtimes100 == static_cast<int>( std::round( 100.0 * m_targetvalue ) ) ) {
        return;
    }
    m_targetvalue = fNewDesired;
    // if there's any sound associated with new requested value, play it
    // check value-specific table first...
    auto const fullinteger { desiredtimes100 % 100 == 0 };
    if( fullinteger ) {
        // filter out values other than full integers
        auto const lookup = m_soundfxvalues.find( desiredtimes100 / 100 );
        if( lookup != m_soundfxvalues.end() ) {
            lookup->second.play();
            return;
        }
    }
    else {
        // toggle the control to continous range/exclusive sound mode from now on
        m_soundtype = sound_flags::exclusive;
    }
    // ...and if there isn't any, fall back on the basic set...
    auto const currentvalue = GetValue();
    // HACK: crude way to discern controls with continuous and quantized value range
    if( ( currentvalue < fNewDesired )
     && ( false == m_soundfxincrease.empty() ) ) {
        // shift up
        m_soundfxincrease.play( m_soundtype );
    }
    else if( ( currentvalue > fNewDesired )
          && ( false == m_soundfxdecrease.empty() ) ) {
        // shift down
        m_soundfxdecrease.play( m_soundtype );
    }
    else if( Fallbacksound != nullptr ) {
        // ...and if that fails too, try the provided fallback sound from legacy system
        Fallbacksound->play( m_soundtype );
    }
};

void TGauge::PutValue(float fNewDesired)
{ // McZapkie-281102: natychmiastowe wpisanie wartosci
    m_targetvalue = fNewDesired;
    m_value = m_targetvalue;
};

float TGauge::GetValue() const {
    // we feed value in range 0-1 so we should be getting it reported in the same range
    return m_value;
}

float TGauge::GetDesiredValue() const {
    // we feed value in range 0-1 so we should be getting it reported in the same range
    return m_targetvalue;
}

void TGauge::Update() {

    if( m_value != m_targetvalue ) {
        float dt = Timer::GetDeltaTime();
        if( ( m_friction > 0 ) && ( dt < 0.5 * m_friction ) ) {
            // McZapkie-281102: zabezpieczenie przed oscylacjami dla dlugich czasow
            m_value += dt * ( m_targetvalue - m_value ) / m_friction;
            if( std::abs( m_targetvalue - m_value ) <= 0.0001 ) {
                // close enough, we can stop updating the model
                m_value = m_targetvalue; // set it exactly as requested just in case it matters
            }
        }
        else {
            m_value = m_targetvalue;
        }
    }
    if( SubModel )
    { // warunek na wszelki wypadek, gdyby się submodel nie podłączył
        switch (m_animation) {
            case TGaugeAnimation::gt_Rotate: {
                SubModel->SetRotate( float3( 0, 1, 0 ), GetScaledValue() * 360.0 );
                break;
            }
            case TGaugeAnimation::gt_Move: {
                SubModel->SetTranslate( float3( 0, 0, GetScaledValue() ) );
                break;
            }
            case TGaugeAnimation::gt_Wiper: {
                auto const scaledvalue { GetScaledValue() };
                SubModel->SetRotate( float3( 0, 1, 0 ), scaledvalue * 360.0 );
                auto *sm = SubModel->ChildGet();
                if( sm ) {
                    sm->SetRotate( float3( 0, 1, 0 ), scaledvalue * 360.0 );
                    sm = sm->ChildGet();
                    if( sm )
                        sm->SetRotate( float3( 0, 1, 0 ), scaledvalue * 360.0 );
                }
                break;
            }
            case TGaugeAnimation::gt_Digital: {
                // Ra 2014-07: licznik cyfrowy
                auto *sm = SubModel->ChildGet();
/*  			std::string n = FormatFloat( "0000000000", floor( fValue ) ); // na razie tak trochę bez sensu
*/	    		std::string n( "000000000" + std::to_string( static_cast<int>( std::floor( GetScaledValue() ) ) ) );
                if( n.length() > 10 ) { n.erase( 0, n.length() - 10 ); } // also dumb but should work for now
                do { // pętla po submodelach potomnych i obracanie ich o kąt zależy od cyfry w (fValue)
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
                } while( sm );
                break;
            }
            default: {
                break;
            }
        }
    }
};

void TGauge::AssignFloat(float *fValue)
{
    m_datatype = 'f';
    fData = fValue;
};

void TGauge::AssignDouble(double *dValue)
{
    m_datatype = 'd';
    dData = dValue;
};

void TGauge::AssignInt(int *iValue)
{
    m_datatype = 'i';
    iData = iValue;
};

void TGauge::UpdateValue()
{ // ustawienie wartości docelowej z parametru
    switch (m_datatype)
    { // to nie jest zbyt optymalne, można by zrobić osobne funkcje
    case 'f':
        UpdateValue( *fData );
        break;
    case 'd':
        UpdateValue( *dData );
        break;
    case 'i':
        UpdateValue( *iData );
        break;
    default:
        break;
    }
};

float TGauge::GetScaledValue() const {

    return (
        ( false == m_interpolatescale ) ?
            m_value * m_scale + m_offset :
            m_value
            * interpolate(
                m_scale, m_endscale,
                clamp(
                    m_value / m_endvalue,
                    0.f, 1.f ) )
            + m_offset );
}

// returns offset of submodel associated with the button from the model centre
glm::vec3
TGauge::model_offset() const {

    return (
        SubModel != nullptr ?
            SubModel->offset( 1.f ) :
            glm::vec3() );
}

TGaugeType
TGauge::type() const {
    return m_type;
}
//---------------------------------------------------------------------------
