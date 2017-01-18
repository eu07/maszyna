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
    Copyright (C) 2001-2004  Marcin Wozniak and others

*/

#include "stdafx.h"
#include "Event.h"
#include "Globals.h"
#include "Logs.h"
#include "Usefull.h"
#include "parser.h"
#include "Timer.h"
#include "MemCell.h"
#include "Ground.h"
#include "McZapkie\mctools.h"

TEvent::TEvent(string m)
{
    // asName=""; //czy nazwa eventu jest niezbêdna w tym przypadku? chyba nie
    evNext = evNext2 = NULL;
    bEnabled = false; // false dla eventów u¿ywanych do skanowania sygna³ów (nie dodawane do
    // kolejki)
    asNodeName = m; // nazwa obiektu powi¹zanego
    iQueued = 0; // nie zosta³ dodany do kolejki
    // bIsHistory=false;
    fDelay = 0;
    fStartTime = 0; // 0 nie ma sensu
    Type = m.empty() ? tp_Unknown :
                         tp_GetValues; // utworzenie niejawnego odczytu komórki pamiêci w torze
    for (int i = 0; i < 13; i++)
        Params[i].asPointer = NULL;
    evJoined = NULL; // nie ma kolejnego z t¹ sam¹ nazw¹, usuwane s¹ wg listy Next2
    Activator = NULL;
    iFlags = 0;
    // event niejawny jest tworzony przed faz¹ InitEvents, która podmienia nazwê komórki pamiêci na
    // wskaŸnik
    // Current->Params[8].asGroundNode=m; //to siê ustawi w InitEvents
    // Current->Params[9].asMemCell=m->MemCell;
    fRandomDelay = 0.0; // standardowo nie bêdzie dodatkowego losowego opóŸnienia
};

TEvent::~TEvent()
{
    switch (Type)
    { // sprz¹tanie
    case tp_Multiple:
        // SafeDeleteArray(Params[9].asText); //nie usuwaæ - nazwa obiektu powi¹zanego zamieniana na
        // wskaŸnik
        if (iFlags & conditional_memstring) // o ile jest ³añcuch do porównania w memcompare
            SafeDeleteArray(Params[10].asText);
        break;
    case tp_UpdateValues:
    case tp_AddValues:
        SafeDeleteArray(Params[0].asText);
        if (iFlags & conditional_memstring) // o ile jest ³añcuch do porównania w memcompare
            SafeDeleteArray(Params[10].asText);
        break;
    case tp_Animation: // nic
        // SafeDeleteArray(Params[9].asText); //nie usuwaæ - nazwa jest zamieniana na wskaŸnik do
        // submodelu
        if (Params[0].asInt == 4) // jeœli z pliku VMD
            delete[] Params[8].asPointer; // zwolniæ obszar
    case tp_GetValues: // nic
        break;
	case tp_PutValues: // params[0].astext stores the token
		SafeDeleteArray( Params[ 0 ].asText );
		break;
    }
    evJoined = NULL; // nie usuwaæ podczepionych tutaj
};

void TEvent::Init(){

};

void TEvent::Conditions(cParser *parser, string s)
{ // przetwarzanie warunków, wspólne dla Multiple i UpdateValues
    if (s == "condition")
    { // jesli nie "endevent"
        std::string token, str;
        if (!asNodeName.empty())
        { // podczepienie ³añcucha, jeœli nie jest pusty
			// NOTE: source of a memory leak -- the array never gets deleted. fix the destructor
            Params[9].asText = new char[asNodeName.length() + 1]; // usuwane i zamieniane na
            // wskaŸnik
            strcpy(Params[9].asText, asNodeName.c_str());
        }
        parser->getTokens();
        *parser >> token;
		str = token;
        if (str == "trackoccupied")
            iFlags |= conditional_trackoccupied;
        else if (str == "trackfree")
            iFlags |= conditional_trackfree;
        else if (str == "propability")
        {
            iFlags |= conditional_propability;
            parser->getTokens();
            *parser >> Params[10].asdouble;
        }
        else if (str == "memcompare")
        {
            iFlags |= conditional_memcompare;
            parser->getTokens(1, false); // case sensitive
            *parser >> token;
			str = token;
            if (str != "*") //"*" - nie brac command pod uwage
            { // zapamiêtanie ³añcucha do porównania
                Params[10].asText = new char[255];
                strcpy(Params[10].asText, str.c_str());
                iFlags |= conditional_memstring;
            }
            parser->getTokens();
            *parser >> token;
			str = token;
            if (str != "*") //"*" - nie brac val1 pod uwage
            {
                Params[11].asdouble = atof(str.c_str());
                iFlags |= conditional_memval1;
            }
            parser->getTokens();
            *parser >> token;
            str = token;
            if (str != "*") //"*" - nie brac val2 pod uwage
            {
                Params[12].asdouble = atof(str.c_str());
                iFlags |= conditional_memval2;
            }
        }
        parser->getTokens();
        *parser >> token;
        s = token; // ewentualnie dalej losowe opóŸnienie
    }
    if (s == "randomdelay")
    { // losowe opóŸnienie
        std::string token;
        parser->getTokens();
        *parser >> fRandomDelay; // Ra 2014-03-11
        parser->getTokens();
        *parser >> token; // endevent
    }
};

void TEvent::Load(cParser *parser, vector3 *org)
{
    int i;
    int ti;
    double tf;
    std::string token;
    //string str;
    char *ptr;

    bEnabled = true; // zmieniane na false dla eventów u¿ywanych do skanowania sygna³ów

    parser->getTokens();
    *parser >> token;
    asName = ToLower(token); // u¿ycie parametrów mo¿e dawaæ wielkie

    parser->getTokens();
    *parser >> token;
    // str = AnsiString(token.c_str());

    if (token == "exit")
        Type = tp_Exit;
    else if (token == "updatevalues")
        Type = tp_UpdateValues;
    else if (token == "getvalues")
        Type = tp_GetValues;
    else if (token == "putvalues")
        Type = tp_PutValues;
    else if (token == "disable")
        Type = tp_Disable;
    else if (token == "sound")
        Type = tp_Sound;
    else if (token == "velocity")
        Type = tp_Velocity;
    else if (token == "animation")
        Type = tp_Animation;
    else if (token == "lights")
        Type = tp_Lights;
    else if (token == "visible")
        Type = tp_Visible; // zmiana wyœwietlania obiektu
    else if (token == "switch")
        Type = tp_Switch;
    else if (token == "dynvel")
        Type = tp_DynVel;
    else if (token == "trackvel")
        Type = tp_TrackVel;
    else if (token == "multiple")
        Type = tp_Multiple;
    else if (token == "addvalues")
        Type = tp_AddValues;
    else if (token == "copyvalues")
        Type = tp_CopyValues;
    else if (token == "whois")
        Type = tp_WhoIs;
    else if (token == "logvalues")
        Type = tp_LogValues;
    else if (token == "voltage")
        Type = tp_Voltage; // zmiana napiêcia w zasilaczu (TractionPowerSource)
    else if (token == "message")
        Type = tp_Message; // wyœwietlenie komunikatu
    else if (token == "friction")
        Type = tp_Friction; // zmiana tarcia na scenerii
    else
        Type = tp_Unknown;

    parser->getTokens();
    *parser >> fDelay;

    parser->getTokens();
    *parser >> token;
    // str = AnsiString(token.c_str());

    if (token != "none")
        asNodeName = token; // nazwa obiektu powi¹zanego

    if (asName.substr(0, 5) == "none_")
        Type = tp_Ignored; // Ra: takie s¹ ignorowane

    switch (Type)
    {
    case tp_AddValues:
        iFlags = update_memadd; // dodawanko
    case tp_UpdateValues:
        // if (Type==tp_UpdateValues) iFlags=0; //co modyfikowaæ
        parser->getTokens(1, false); // case sensitive
        *parser >> token;
        // str = AnsiString(token.c_str());
        Params[0].asText = new char[token.length() + 1];
        strcpy(Params[0].asText, token.c_str());
        if (token != "*") // czy ma zostaæ bez zmian?
            iFlags |= update_memstring;
        parser->getTokens();
        *parser >> token;
        // str = AnsiString(token.c_str());
        if (token != "*") // czy ma zostaæ bez zmian?
        {
            Params[1].asdouble = atof(token.c_str());
            iFlags |= update_memval1;
        }
        else
            Params[1].asdouble = 0;
        parser->getTokens();
        *parser >> token;
        // str = AnsiString(token.c_str());
        if (token != "*") // czy ma zostaæ bez zmian?
        {
            Params[2].asdouble = atof(token.c_str());
            iFlags |= update_memval2;
        }
        else
            Params[2].asdouble = 0;
        parser->getTokens();
        *parser >> token;
        Conditions(parser, token); // sprawdzanie warunków
        break;
    case tp_CopyValues:
        Params[9].asText = NULL;
        iFlags = update_memstring | update_memval1 | update_memval2; // normalanie trzy
        i = 0;
        parser->getTokens();
        *parser >> token; // nazwa drugiej komórki (Ÿród³owej)
        while (token.compare("endevent") != 0)
        {
            switch (++i)
            { // znaczenie kolejnych parametrów
            case 1: // nazwa drugiej komórki (Ÿród³owej)
                Params[9].asText = new char[token.length() + 1]; // usuwane i zamieniane na wskaŸnik
                strcpy(Params[9].asText, token.c_str());
                break;
            case 2: // maska wartoœci
                iFlags = stol_def(token,
                             (update_memstring | update_memval1 | update_memval2));
                break;
            }
            parser->getTokens();
            *parser >> token;
        }
        break;
    case tp_WhoIs:
        iFlags = update_memstring | update_memval1 | update_memval2; // normalanie trzy
        i = 0;
        parser->getTokens();
        *parser >> token; // nazwa drugiej komórki (Ÿród³owej)
        while (token.compare("endevent") != 0)
        {
            switch (++i)
            { // znaczenie kolejnych parametrów
            case 1: // maska wartoœci
                iFlags = stol_def(token,
                             (update_memstring | update_memval1 | update_memval2));
                break;
            }
            parser->getTokens();
            *parser >> token;
        }
        break;
    case tp_GetValues:
    case tp_LogValues:
        parser->getTokens(); //"endevent"
        *parser >> token;
        break;
    case tp_PutValues:
        parser->getTokens(3);
        *parser >> Params[3].asdouble >> Params[4].asdouble >> Params[5].asdouble; // po³o¿enie
        // X,Y,Z
        if (org)
        { // przesuniêcie
            // tmp->pCenter.RotateY(aRotate.y/180.0*M_PI); //Ra 2014-11: uwzglêdnienie rotacji
            Params[3].asdouble += org->x; // wspó³rzêdne w scenerii
            Params[4].asdouble += org->y;
            Params[5].asdouble += org->z;
        }
        // Params[12].asInt=0;
        parser->getTokens(1, false); // komendy 'case sensitive'
        *parser >> token;
        // str = AnsiString(token.c_str());
        if (token.substr(0, 19) == "PassengerStopPoint:")
        {
            if (token.find("#"))
				token = token.substr(0, token.find("#") - 1); // obciêcie unikatowoœci
            bEnabled = false; // nie do kolejki (dla SetVelocity te¿, ale jak jest do toru
            // dowi¹zany)
            Params[6].asCommand = cm_PassengerStopPoint;
        }
        else if (token == "SetVelocity")
        {
            bEnabled = false;
            Params[6].asCommand = cm_SetVelocity;
        }
        else if (token == "RoadVelocity")
        {
            bEnabled = false;
            Params[6].asCommand = cm_RoadVelocity;
        }
        else if (token == "SectionVelocity")
        {
            bEnabled = false;
            Params[6].asCommand = cm_SectionVelocity;
        }
        else if (token == "ShuntVelocity")
        {
            bEnabled = false;
            Params[6].asCommand = cm_ShuntVelocity;
        }
        //else if (str == "SetProximityVelocity")
        //{
        //    bEnabled = false;
        //    Params[6].asCommand = cm_SetProximityVelocity;
        //}
        else if (token == "OutsideStation")
        {
            bEnabled = false; // ma byæ skanowny, aby AI nie przekracza³o W5
            Params[6].asCommand = cm_OutsideStation;
        }
        else
            Params[6].asCommand = cm_Unknown;
        Params[0].asText = new char[token.length() + 1];
        strcpy(Params[0].asText, token.c_str());
        parser->getTokens();
        *parser >> token;
        // str = AnsiString(token.c_str());
        if (token == "none")
            Params[1].asdouble = 0.0;
        else
            try
            {
                Params[1].asdouble = atof(token.c_str());
            }
            catch (...)
            {
                Params[1].asdouble = 0.0;
                WriteLog("Error: number expected in PutValues event, found: " + token);
            }
        parser->getTokens();
        *parser >> token;
        // str = AnsiString(token.c_str());
        if (token == "none")
            Params[2].asdouble = 0.0;
        else
            try
            {
                Params[2].asdouble = atof(token.c_str());
            }
            catch (...)
            {
                Params[2].asdouble = 0.0;
                WriteLog("Error: number expected in PutValues event, found: " + token);
            }
        parser->getTokens();
        *parser >> token;
        break;
    case tp_Lights:
        i = 0;
        do
        {
            parser->getTokens();
            *parser >> token;
            if (token.compare("endevent") != 0)
            {
                // str = AnsiString(token.c_str());
                if (i < 8)
                    Params[i].asdouble = atof(token.c_str()); // teraz mo¿e mieæ u³amek
                i++;
            }
        } while (token.compare("endevent") != 0);
        break;
    case tp_Visible: // zmiana wyœwietlania obiektu
        parser->getTokens();
        *parser >> token;
        // str = AnsiString(token.c_str());
        Params[0].asInt = atoi(token.c_str());
        parser->getTokens();
        *parser >> token;
        break;
    case tp_Velocity:
        parser->getTokens();
        *parser >> token;
        // str = AnsiString(token.c_str());
        Params[0].asdouble = atof(token.c_str()) * 0.28;
        parser->getTokens();
        *parser >> token;
        break;
    case tp_Sound:
        // Params[0].asRealSound->Init(asNodeName.c_str(),Parser->GetNextSymbol().ToDouble(),Parser->GetNextSymbol().ToDouble(),Parser->GetNextSymbol().ToDouble(),Parser->GetNextSymbol().ToDouble());
        // McZapkie-070502: dzwiek przestrzenny (ale do poprawy)
        // Params[1].asdouble=Parser->GetNextSymbol().ToDouble();
        // Params[2].asdouble=Parser->GetNextSymbol().ToDouble();
        // Params[3].asdouble=Parser->GetNextSymbol().ToDouble(); //polozenie X,Y,Z - do poprawy!
        parser->getTokens();
        *parser >> Params[0].asInt; // 0: wylaczyc, 1: wlaczyc; -1: wlaczyc zapetlone
        parser->getTokens();
        *parser >> token;
        break;
    case tp_Exit:
        while ((ptr = strchr(strdup(asNodeName.c_str()), '_')) != NULL)
            *ptr = ' ';
        parser->getTokens();
        *parser >> token;
        break;
    case tp_Disable:
        parser->getTokens();
        *parser >> token;
        break;
    case tp_Animation:
        parser->getTokens();
        *parser >> token;
        Params[0].asInt = 0; // na razie nieznany typ
        if (token.compare("rotate") == 0)
        { // obrót wzglêdem osi
            parser->getTokens();
            *parser >> token;
            Params[9].asText = new char[token.size() + 1]; // nazwa submodelu
            std::strcpy(Params[9].asText, token.c_str());
            Params[0].asInt = 1;
            parser->getTokens(4);
            *parser
				>> Params[1].asdouble
				>> Params[2].asdouble
				>> Params[3].asdouble
				>> Params[4].asdouble;
        }
        else if (token.compare("translate") == 0)
        { // przesuw o wektor
            parser->getTokens();
            *parser >> token;
            Params[9].asText = new char[token.size() + 1]; // nazwa submodelu
            std::strcpy(Params[9].asText, token.c_str());
            Params[0].asInt = 2;
            parser->getTokens(4);
            *parser
				>> Params[1].asdouble
				>> Params[2].asdouble
				>> Params[3].asdouble
				>> Params[4].asdouble;
        }
        else if (token.compare("digital") == 0)
        { // licznik cyfrowy
            parser->getTokens();
            *parser >> token;
            Params[9].asText = new char[token.size() + 1]; // nazwa submodelu
            std::strcpy(Params[9].asText, token.c_str());
            Params[0].asInt = 8;
            parser->getTokens(4); // jaki ma byæ sens tych parametrów?
            *parser
				>> Params[1].asdouble
				>> Params[2].asdouble
				>> Params[3].asdouble
				>> Params[4].asdouble;
        }
        else if (token.substr(token.length() - 4, 4) == ".vmd") // na razie tu, mo¿e bêdzie inaczej
        { // animacja z pliku VMD
//			TFileStream *fs = new TFileStream( "models\\" + AnsiString( token.c_str() ), fmOpenRead );
			{
				std::ifstream file( "models\\" + token, std::ios::binary | std::ios::ate ); file.unsetf( std::ios::skipws );
				auto size = file.tellg();   // ios::ate already positioned us at the end of the file
				file.seekg( 0, std::ios::beg ); // rewind the caret afterwards
				Params[ 7 ].asInt = size;
				Params[8].asPointer = new char[size];
				file.read( static_cast<char*>(Params[ 8 ].asPointer), size ); // wczytanie pliku
			}
			parser->getTokens();
            *parser >> token;
            Params[9].asText = new char[token.size() + 1]; // nazwa submodelu
            std::strcpy(Params[9].asText, token.c_str());
            Params[0].asInt = 4; // rodzaj animacji
            parser->getTokens(4);
            *parser
				>> Params[1].asdouble
				>> Params[2].asdouble
				>> Params[3].asdouble
				>> Params[4].asdouble;
        }
        parser->getTokens();
        *parser >> token;
        break;
    case tp_Switch:
        parser->getTokens();
        *parser >> Params[0].asInt;
        parser->getTokens();
        *parser >> token;
        // str = AnsiString(token.c_str());
        if (token != "endevent")
        {
            Params[1].asdouble = atof(token.c_str()); // prêdkoœæ liniowa ruchu iglic
            parser->getTokens();
            *parser >> token;
            // str = AnsiString(token.c_str());
        }
        else
            Params[1].asdouble = -1.0; // u¿yæ domyœlnej
        if (token != "endevent")
        {
            Params[2].asdouble =
                atof(token.c_str()); // dodatkowy ruch drugiej iglicy (zamkniêcie nastawnicze)
            parser->getTokens();
            *parser >> token;
        }
        else
            Params[2].asdouble = -1.0; // u¿yæ domyœlnej
        break;
    case tp_DynVel:
        parser->getTokens();
        *parser >> Params[0].asdouble; // McZapkie-090302 *0.28;
        parser->getTokens();
        *parser >> token;
        break;
    case tp_TrackVel:
        parser->getTokens();
        *parser >> Params[0].asdouble; // McZapkie-090302 *0.28;
        parser->getTokens();
        *parser >> token;
        break;
    case tp_Multiple:
        i = 0;
        ti = 0; // flaga dla else
        parser->getTokens();
        *parser >> token;
        // str = AnsiString(token.c_str());
        while (token != "endevent" && token != "condition" &&
			token != "randomdelay")
        {
            if ((token.substr(0, 5) != "none_") ? (i < 8) : false)
            { // eventy rozpoczynaj¹ce siê od "none_" s¹ ignorowane
                if (token != "else")
                {
                    Params[i].asText = new char[255];
                    strcpy(Params[i].asText, token.c_str());
                    if (ti)
                        iFlags |= conditional_else << i; // oflagowanie dla eventów "else"
                    i++;
                }
                else
                    ti = !ti; // zmiana flagi dla s³owa "else"
            }
            else if (i >= 8)
                ErrorLog("Bad event: \"" + token + "\" ignored in multiple \"" + asName + "\"!");
            else
                WriteLog("Event \"" + token + "\" ignored in multiple \"" + asName + "\"!");
            parser->getTokens();
            *parser >> token;
            // str = AnsiString(token.c_str());
        }
        Conditions(parser, token); // sprawdzanie warunków
        break;
    case tp_Voltage: // zmiana napiêcia w zasilaczu (TractionPowerSource)
    case tp_Friction: // zmiana przyczepnosci na scenerii
        parser->getTokens();
        *parser >> Params[0].asdouble; // Ra 2014-01-27
        parser->getTokens();
        *parser >> token;
        break;
    case tp_Message: // wyœwietlenie komunikatu
        do
        {
            parser->getTokens();
            *parser >> token;
            // str = AnsiString(token.c_str());
        } while (token != "endevent");
        break;
    case tp_Ignored: // ignorowany
    case tp_Unknown: // nieznany
        do
        {
            parser->getTokens();
            *parser >> token;
            // str = AnsiString(token.c_str());
        } while (token != "endevent");
        WriteLog("Bad event: \"" + asName +
                 (Type == tp_Unknown ? "\" has unknown type." : "\" is ignored."));
        break;
    }
};

void TEvent::AddToQuery(TEvent *e)
{ // dodanie eventu do kolejki
    if (evNext ? (e->fStartTime >= evNext->fStartTime) : false)
        evNext->AddToQuery(e); // sortowanie wg czasu
    else
    { // dodanie z przodu
        e->evNext = evNext;
        evNext = e;
    }
}

//---------------------------------------------------------------------------

string TEvent::CommandGet()
{ // odczytanie komendy z eventu
    switch (Type)
    { // to siê wykonuje równie¿ sk³adu jad¹cego bez obs³ugi
    case tp_GetValues:
        return string(Params[9].asMemCell->Text());
    case tp_PutValues:
        return string(Params[0].asText);
    }
    return ""; // inne eventy siê nie licz¹
};

TCommandType TEvent::Command()
{ // odczytanie komendy z eventu
    switch (Type)
    { // to siê wykonuje równie¿ dla sk³adu jad¹cego bez obs³ugi
    case tp_GetValues:
        return Params[9].asMemCell->Command();
    case tp_PutValues:
        return Params[6].asCommand; // komenda zakodowana binarnie
    }
    return cm_Unknown; // inne eventy siê nie licz¹
};

double TEvent::ValueGet(int n)
{ // odczytanie komendy z eventu
    n &= 1; // tylko 1 albo 2 jest prawid³owy
    switch (Type)
    { // to siê wykonuje równie¿ sk³adu jad¹cego bez obs³ugi
    case tp_GetValues:
        return n ? Params[9].asMemCell->Value1() : Params[9].asMemCell->Value2();
    case tp_PutValues:
        return Params[2 - n].asdouble;
    }
    return 0.0; // inne eventy siê nie licz¹
};

vector3 TEvent::PositionGet()
{ // pobranie wspó³rzêdnych eventu
    switch (Type)
    { //
    case tp_GetValues:
        return Params[9].asMemCell->Position(); // wspó³rzêdne pod³¹czonej komórki pamiêci
    case tp_PutValues:
        return vector3(Params[3].asdouble, Params[4].asdouble, Params[5].asdouble);
    }
    return vector3(0, 0, 0); // inne eventy siê nie licz¹
};

bool TEvent::StopCommand()
{ //
    if (Type == tp_GetValues)
        return Params[9].asMemCell->StopCommand(); // info o komendzie z komórki
    return false;
};

void TEvent::StopCommandSent()
{
    if (Type == tp_GetValues)
        Params[9].asMemCell->StopCommandSent(); // komenda z komórki zosta³a wys³ana
};

void TEvent::Append(TEvent *e)
{ // doczepienie kolejnych z t¹ sam¹ nazw¹
    if (evJoined)
        evJoined->Append(e); // rekurencja! - góra kilkanaœcie eventów bêdzie potrzebne
    else
    {
        evJoined = e;
        e->bEnabled = true; // ten doczepiony mo¿e byæ tylko kolejkowany
    }
};
