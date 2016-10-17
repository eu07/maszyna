/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

/*
    MaSzyna EU07 locomotive simulator component
    Copyright (C) 2004  Maciej Czapkiewicz and others

*/

//#include "system.hpp"
//#include "classes.hpp"
#pragma hdrstop

#include "Mover.h"
#include "mctools.h"
#include "Timer.h"
#include "Globals.h"
#include "TractionPower.h"

#include "Usefull.h"
#include "Ground.h"

//---------------------------------------------------------------------------

TTractionPowerSource::TTractionPowerSource(TGroundNode *node)
{
    NominalVoltage = 0;
    VoltageFrequency = 0;
    InternalRes = 0.2;
    MaxOutputCurrent = 0;
    FastFuseTimeOut = 1;
    FastFuseRepetition = 3;
    SlowFuseTimeOut = 60;
    Recuperation = false;

    TotalAdmitance = 1e-10; // 10Mom - jakaœ tam up³ywnoœæ
    TotalPreviousAdmitance = 1e-10; // zero jest szkodliwe
    OutputVoltage = 0;
    FastFuse = false;
    SlowFuse = false;
    FuseTimer = 0;
    FuseCounter = 0;
    psNode[0] = NULL; // sekcje zostan¹ pod³¹czone do zasilaczy
    psNode[1] = NULL;
    bSection = false; // sekcja nie jest Ÿród³em zasilania, tylko grupuje przês³a
	gMyNode = node;
};

TTractionPowerSource::~TTractionPowerSource(){};

void TTractionPowerSource::Init(double u, double i)
{ // ustawianie zasilacza przy braku w scenerii
    NominalVoltage = u;
    VoltageFrequency = 0;
    MaxOutputCurrent = i;
};

bool TTractionPowerSource::Load(cParser *parser)
{
	std::string token;
    // AnsiString str;
    // str= Parser->GetNextSymbol()LowerCase();
    // asName= str;
    parser->getTokens(5);
    *parser >> NominalVoltage >> VoltageFrequency >> InternalRes >> MaxOutputCurrent >>
        FastFuseTimeOut;
    parser->getTokens();
    *parser >> FastFuseRepetition;
    parser->getTokens();
    *parser >> SlowFuseTimeOut;
    parser->getTokens();
    *parser >> token;
    if (token.compare("recuperation") == 0)
        Recuperation = true;
    else if (token.compare("section") == 0) // od³¹cznik sekcyjny
        bSection = true; // nie jest Ÿród³em zasilania, a jedynie informuje o pr¹dzie od³¹czenia
    // sekcji z obwodu
    parser->getTokens();
    *parser >> token;
    if (token.compare("end") != 0)
        Error("tractionpowersource end statement missing");
    // if (!bSection) //od³¹cznik sekcji zasadniczo nie ma impedancji (0.01 jest OK)
    if (InternalRes < 0.1) // coœ ma³a ta rezystancja by³a...
        InternalRes = 0.2; // tak oko³o 0.2, wg
    // http://www.ikolej.pl/fileadmin/user_upload/Seminaria_IK/13_05_07_Prezentacja_Kruczek.pdf
    return true;
};

bool TTractionPowerSource::Render()
{
    return true;
};

bool TTractionPowerSource::Update(double dt)
{ // powinno byæ wykonane raz na krok fizyki
  //    if (NominalVoltage * TotalPreviousAdmitance >
  //        MaxOutputCurrent * 0.00000005) // iloczyn napiêcia i admitancji daje pr¹d
  //        ErrorLog("Power overload: \"" + gMyNode->asName + "\" with current " + AnsiString(NominalVoltage * TotalPreviousAdmitance) + "A");
	if (NominalVoltage * TotalPreviousAdmitance >
        MaxOutputCurrent) // iloczyn napiêcia i admitancji daje pr¹d
    {
        FastFuse = true;
        FuseCounter += 1;
        if (FuseCounter > FastFuseRepetition)
        {
            SlowFuse = true;
            ErrorLog("Power overload: \"" + gMyNode->asName + "\" disabled for " +
                     AnsiString(SlowFuseTimeOut) + "s");
        }
        else
            ErrorLog("Power overload: \"" + gMyNode->asName + "\" disabled for " +
                     AnsiString(FastFuseTimeOut) + "s");
        FuseTimer = 0;
    }
    if (FastFuse || SlowFuse)
    { // jeœli któryœ z bezpieczników zadzia³a³
        TotalAdmitance = 0;
        FuseTimer += dt;
        if (!SlowFuse)
        { // gdy szybki, odczekaæ krótko i za³¹czyæ
            if (FuseTimer > FastFuseTimeOut)
                FastFuse = false;
        }
        else if (FuseTimer > SlowFuseTimeOut)
        {
            SlowFuse = false;
            FuseCounter = 0; // dajemy znów szansê
        }
    }
    TotalPreviousAdmitance = TotalAdmitance; // u¿ywamy admitancji z poprzedniego kroku
    if (TotalPreviousAdmitance == 0.0)
        TotalPreviousAdmitance = 1e-10; // przynajmniej minimalna up³ywnoœæ
    TotalAdmitance = 1e-10; // a w aktualnym kroku sumujemy admitancjê
    return true;
};

double TTractionPowerSource::CurrentGet(double res)
{ // pobranie wartoœci pr¹du przypadaj¹cego na rezystancjê (res)
    // niech pamiêta poprzedni¹ admitancjê i wg niej przydziela pr¹d
    if (SlowFuse || FastFuse)
    { // czekanie na zanik obci¹¿enia sekcji
        if (res < 100.0) // liczenie czasu dopiero, gdy obci¹¿enie zniknie
            FuseTimer = 0;
        return 0;
    }
	if ((res > 0) || ((res < 0) && (Recuperation || true)))
		TotalAdmitance +=
            1.0 / res; // po³¹czenie równoleg³e rezystancji jest równowa¿ne sumie admitancji
	float NomVolt = (TotalPreviousAdmitance < 0 ? NominalVoltage * 1.083 : NominalVoltage);
	TotalCurrent = (TotalPreviousAdmitance != 0.0) ?
		NomVolt / (InternalRes + 1.0 / TotalPreviousAdmitance) :
		0.0; // napiêcie dzielone przez sumê rezystancji wewnêtrznej i obci¹¿enia
	OutputVoltage = NomVolt - InternalRes * TotalCurrent; // napiêcie na obci¹¿eniu
	return TotalCurrent / (res * TotalPreviousAdmitance); // pr¹d proporcjonalny do udzia³u (1/res)
    // w ca³kowitej admitancji
};

void TTractionPowerSource::PowerSet(TTractionPowerSource *ps)
{ // wskazanie zasilacza w obiekcie sekcji
    if (!psNode[0])
        psNode[0] = ps;
    else if (!psNode[1])
        psNode[1] = ps;
    // else ErrorLog("nie mo¿e byæ wiêcej punktów zasilania ni¿ dwa");
};

//---------------------------------------------------------------------------

#pragma package(smart_init)
