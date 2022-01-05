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

#include "parser.h"
#include "Logs.h"

//---------------------------------------------------------------------------

TTractionPowerSource::TTractionPowerSource( scene::node_data const &Nodedata ) : basic_node( Nodedata ) {}
// legacy constructor

void TTractionPowerSource::Init(double const u, double const i)
{ // ustawianie zasilacza przy braku w scenerii
    NominalVoltage = u;
    VoltageFrequency = 0;
    MaxOutputCurrent = i;
};

bool TTractionPowerSource::Load(cParser *parser) {

    parser->getTokens( 10, false );
    *parser
        >> m_area.center.x
        >> m_area.center.y
        >> m_area.center.z
        >> NominalVoltage
        >> VoltageFrequency
        >> InternalRes
        >> MaxOutputCurrent
        >> FastFuseTimeOut
        >> FastFuseRepetition
        >> SlowFuseTimeOut;

    std::string token { parser->getToken<std::string>() };
    if( token == "recuperation" ) {
        Recuperation = true;
    }
    else if( token == "section" ) {
        // odłącznik sekcyjny
        // nie jest źródłem zasilania, a jedynie informuje o prądzie odłączenia sekcji z obwodu
        bSection = true;
    }
    // skip rest of the section
    while( ( false == token.empty() )
        && ( token != "end" ) ) {

        token = parser->getToken<std::string>();
    }

    if( InternalRes < 0.1 ) {
        // coś mała ta rezystancja była...
        // tak około 0.2, wg
        // http://www.ikolej.pl/fileadmin/user_upload/Seminaria_IK/13_05_07_Prezentacja_Kruczek.pdf
		InternalRes = 0.2;
	}
    return true;
};

bool TTractionPowerSource::Update(double dt)
{ // powinno być wykonane raz na krok fizyki
  // iloczyn napięcia i admitancji daje prąd
    if( FastFuse || SlowFuse ) {
        TotalCurrent = 0.0;
    }
	if (TotalCurrent > MaxOutputCurrent) {

        FastFuse = true;
        FuseCounter += 1;
        if (FuseCounter > FastFuseRepetition) {

            SlowFuse = true;
            ErrorLog( "Power overload: \"" + m_name + "\" disabled for " + std::to_string( SlowFuseTimeOut ) + "s" );
        }
        else {
            ErrorLog( "Power overload: \"" + m_name + "\" disabled for " + std::to_string( FastFuseTimeOut ) + "s" );
        }

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
		TotalAdmitance += 1.0 / res; // połączenie równoległe rezystancji jest równoważne sumie admitancji
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

// serialize() subclass details, sends content of the subclass to provided stream
void
TTractionPowerSource::serialize_( std::ostream &Output ) const {

    // TODO: implement
}

// deserialize() subclass details, restores content of the subclass from provided stream
void
TTractionPowerSource::deserialize_( std::istream &Input ) {

    // TODO: implement
}

// export() subclass details, sends basic content of the class in legacy (text) format to provided stream
void
TTractionPowerSource::export_as_text_( std::ostream &Output ) const {
    // header
    Output << "tractionpowersource ";
    // placement
    Output
        << location().x << ' '
        << location().y << ' '
        << location().z << ' ';
    // basic attributes
    Output
        << NominalVoltage << ' '
        << VoltageFrequency << ' '
        << InternalRes << ' '
        << MaxOutputCurrent << ' '
        << FastFuseTimeOut << ' '
        << FastFuseRepetition << ' '
        << SlowFuseTimeOut << ' ';
    // optional attributes
    if( true == Recuperation ) {
        Output << "recuperation ";
    }
    if( true == bSection ) {
        Output << "section ";
    }
    // footer
    Output
        << "end"
        << "\n";
}



// legacy method, calculates changes in simulation state over specified time
void
powergridsource_table::update( double const Deltatime ) {

    for( auto *powersource : m_items ) {
        powersource->Update( Deltatime );
    }
}

//---------------------------------------------------------------------------
