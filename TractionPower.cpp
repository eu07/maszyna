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

#include "stdafx.h"
#include "TractionPower.h"
#include "Logs.h"
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

    TotalAdmitance = 1e-10; // 10Mom - jakaś tam upływność
    TotalPreviousAdmitance = 1e-10; // zero jest szkodliwe
    OutputVoltage = 0;
    FastFuse = false;
    SlowFuse = false;
    FuseTimer = 0;
    FuseCounter = 0;
    psNode[0] = NULL; // sekcje zostaną podłączone do zasilaczy
    psNode[1] = NULL;
    bSection = false; // sekcja nie jest źródłem zasilania, tylko grupuje przęsła
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
    else if (token.compare("section") == 0) // odłącznik sekcyjny
        bSection = true; // nie jest źródłem zasilania, a jedynie informuje o prądzie odłączenia
    // sekcji z obwodu
    parser->getTokens();
    *parser >> token;
    if (token.compare("end") != 0)
        Error("tractionpowersource end statement missing");
    // if (!bSection) //odłącznik sekcji zasadniczo nie ma impedancji (0.01 jest OK)
    if (InternalRes < 0.1) // coś mała ta rezystancja była...
        InternalRes = 0.2; // tak około 0.2, wg
    // http://www.ikolej.pl/fileadmin/user_upload/Seminaria_IK/13_05_07_Prezentacja_Kruczek.pdf
    return true;
};

bool TTractionPowerSource::Render()
{
    return true;
};

bool TTractionPowerSource::Update(double dt)
{ // powinno być wykonane raz na krok fizyki
  //    if (NominalVoltage * TotalPreviousAdmitance >
  //        MaxOutputCurrent * 0.00000005) // iloczyn napięcia i admitancji daje prąd
  //        ErrorLog("Power overload: \"" + gMyNode->asName + "\" with current " + AnsiString(NominalVoltage * TotalPreviousAdmitance) + "A");
	if (NominalVoltage * TotalPreviousAdmitance >
        MaxOutputCurrent) // iloczyn napięcia i admitancji daje prąd
    {
        FastFuse = true;
        FuseCounter += 1;
        if (FuseCounter > FastFuseRepetition)
        {
            SlowFuse = true;
            ErrorLog("Power overload: \"" + gMyNode->asName + "\" disabled for " +
                     std::to_string(SlowFuseTimeOut) + "s");
        }
        else
            ErrorLog("Power overload: \"" + gMyNode->asName + "\" disabled for " +
                     std::to_string(FastFuseTimeOut) + "s");
        FuseTimer = 0;
    }
    if (FastFuse || SlowFuse)
    { // jeśli któryś z bezpieczników zadziałał
        TotalAdmitance = 0;
        FuseTimer += dt;
        if (!SlowFuse)
        { // gdy szybki, odczekać krótko i załączyć
            if (FuseTimer > FastFuseTimeOut)
                FastFuse = false;
        }
        else if (FuseTimer > SlowFuseTimeOut)
        {
            SlowFuse = false;
            FuseCounter = 0; // dajemy znów szansę
        }
    }
    TotalPreviousAdmitance = TotalAdmitance; // używamy admitancji z poprzedniego kroku
    if (TotalPreviousAdmitance == 0.0)
        TotalPreviousAdmitance = 1e-10; // przynajmniej minimalna upływność
    TotalAdmitance = 1e-10; // a w aktualnym kroku sumujemy admitancję
    return true;
};

double TTractionPowerSource::CurrentGet(double res)
{ // pobranie wartości prądu przypadającego na rezystancję (res)
    // niech pamięta poprzednią admitancję i wg niej przydziela prąd
    if (SlowFuse || FastFuse)
    { // czekanie na zanik obciążenia sekcji
        if (res < 100.0) // liczenie czasu dopiero, gdy obciążenie zniknie
            FuseTimer = 0;
        return 0;
    }
	if ((res > 0) || ((res < 0) && (Recuperation || true)))
		TotalAdmitance +=
            1.0 / res; // połączenie równoległe rezystancji jest równoważne sumie admitancji
	float NomVolt = (TotalPreviousAdmitance < 0 ? NominalVoltage * 1.083 : NominalVoltage);
	TotalCurrent = (TotalPreviousAdmitance != 0.0) ?
		NomVolt / (InternalRes + 1.0 / TotalPreviousAdmitance) :
		0.0; // napięcie dzielone przez sumę rezystancji wewnętrznej i obciążenia
	OutputVoltage = NomVolt - InternalRes * TotalCurrent; // napięcie na obciążeniu
	return TotalCurrent / (res * TotalPreviousAdmitance); // prąd proporcjonalny do udziału (1/res)
    // w całkowitej admitancji
};

void TTractionPowerSource::PowerSet(TTractionPowerSource *ps)
{ // wskazanie zasilacza w obiekcie sekcji
    if (!psNode[0])
        psNode[0] = ps;
    else if (!psNode[1])
        psNode[1] = ps;
    // else ErrorLog("nie może być więcej punktów zasilania niż dwa");
};

//---------------------------------------------------------------------------
