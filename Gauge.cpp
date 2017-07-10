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

TGauge::TGauge()
{
    eType = gt_Unknown;
    fFriction = 0.0;
    fDesiredValue = 0.0;
    fValue = 0.0;
    fOffset = 0.0;
    fScale = 1.0;
    fStepSize = 5;
    // iChannel=-1; //kanał analogowej komunikacji zwrotnej
    SubModel = NULL;
};

TGauge::~TGauge(){};

void TGauge::Clear()
{
    SubModel = NULL;
    eType = gt_Unknown;
    fValue = 0;
    fDesiredValue = 0;
};

void TGauge::Init(TSubModel *NewSubModel, TGaugeType eNewType, double fNewScale, double fNewOffset,
                  double fNewFriction, double fNewValue)
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

bool TGauge::Load(cParser &Parser, TModel3d *md1, TModel3d *md2, double mul)
{
	std::string str1 = Parser.getToken<std::string>(false);
	std::string str2 = Parser.getToken<std::string>();
	Parser.getTokens( 3, false );
	double val3, val4, val5;
	Parser
		>> val3
		>> val4
		>> val5;
	val3 *= mul;
		TSubModel *sm = md1->GetFromName( str1.c_str() );
    if( val3 == 0.0 ) {
        ErrorLog( "Scale of 0.0 defined for sub-model \"" + str1 + "\" in 3d model \"" + md1->NameGet() + "\". Forcing scale of 1.0 to prevent division by 0" );
        val3 = 1.0;
    }
    if (sm) // jeśli nie znaleziony
        md2 = NULL; // informacja, że znaleziony
    else if (md2) // a jest podany drugi model (np. zewnętrzny)
        sm = md2->GetFromName(str1.c_str()); // to może tam będzie, co za różnica gdzie
    if( sm == nullptr ) {
        ErrorLog( "Failed to locate sub-model \"" + str1 + "\" in 3d model \"" + md1->NameGet() + "\"" );
    }

    if (str2 == "mov")
        Init(sm, gt_Move, val3, val4, val5);
    else if (str2 == "wip")
        Init(sm, gt_Wiper, val3, val4, val5);
    else if (str2 == "dgt")
        Init(sm, gt_Digital, val3, val4, val5);
    else
        Init(sm, gt_Rotate, val3, val4, val5);
    return md2 != nullptr; // true, gdy podany model zewnętrzny, a w kabinie nie było
};

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

void TGauge::UpdateValue(double fNewDesired)
{ // ustawienie wartości docelowej
    fDesiredValue = fNewDesired * fScale + fOffset;
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

//---------------------------------------------------------------------------
