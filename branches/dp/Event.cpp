//---------------------------------------------------------------------------

/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak and others

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include    "system.hpp"
#include    "classes.hpp"
#pragma hdrstop

#include "Event.h"
#include "parser.h"
#include "Timer.h"
#include "Usefull.h"
#include "MemCell.h"
//#include "Ground.h"
#pragma package(smart_init)

__fastcall TEvent::TEvent()
{
 Next=Next2=NULL;
 bEnabled=false;
 asNodeName="";
 bLaunched=false;
 bIsHistory=false;
 fDelay=0;
 fStartTime=0;
 Type=tp_Unknown;
 for (int i=0;i<10;i++)
  Params[i].asPointer=NULL;
}

__fastcall TEvent::~TEvent()
{
 switch (Type)
 {case tp_UpdateValues:
  case tp_AddValues:
   SafeDeleteArray(Params[0].asText);
 }
}

void __fastcall TEvent::Init()
{

}

void __fastcall TEvent::Load(cParser* parser,vector3 *org)
{
    int i;
    int ti;
    double tf;
    std::string token;
    AnsiString str;
    char *ptr;

    bEnabled=true;

    parser->getTokens();
    *parser >> token;
    asName=AnsiString(token.c_str()).LowerCase(); //u¿ycie parametrów mo¿e dawaæ wielkie

    parser->getTokens();
    *parser >> token;
    str=AnsiString(token.c_str());

    if (str==AnsiString("exit"))
        Type=tp_Exit;
    else
    if (str==AnsiString("updatevalues"))
        Type=tp_UpdateValues;
    else
    if (str==AnsiString("getvalues"))
        Type=tp_GetValues;
    else
    if (str==AnsiString("putvalues"))
        Type=tp_PutValues;
    else
    if (str==AnsiString("disable"))
        Type=tp_Disable;
    else
    if (str==AnsiString("sound"))
        Type=tp_Sound;
    else
    if (str==AnsiString("velocity"))
        Type=tp_Velocity;
    else
    if (str==AnsiString("animation"))
        Type=tp_Animation;
    else
    if (str==AnsiString("lights"))
        Type=tp_Lights;
    else
    if (str==AnsiString("switch"))
        Type=tp_Switch;
    else
    if (str==AnsiString("dynvel"))
        Type=tp_DynVel;
    else
    if (str==AnsiString("trackvel"))
        Type=tp_TrackVel;
    else
    if (str==AnsiString("multiple"))
        Type=tp_Multiple;
    else
    if (str==AnsiString("addvalues"))
        Type=tp_AddValues;
    else
    if (str==AnsiString("copyvalues"))
        Type=tp_CopyValues;
    else
    if (str==AnsiString("whois"))
        Type=tp_WhoIs;
    else
    if (str==AnsiString("logvalues"))
        Type=tp_LogValues;
    else
        Type=tp_Unknown;

    parser->getTokens();
    *parser >> fDelay;

    parser->getTokens();
    *parser >> token;
    str=AnsiString(token.c_str());

    if (str!="none") asNodeName=str; //nazwa obiektu powi¹zanego

    if (asName.SubString(1,5)=="none_")
     Type=tp_Ignored; //Ra: takie s¹ ignorowane

    switch (Type)
    {

        case tp_AddValues:
            Params[12].asInt=conditional_memadd; //dodawanko
        case tp_UpdateValues:
            if (Type==tp_UpdateValues) Params[12].asInt=0;
            parser->getTokens(1,false);  //case sensitive
            *parser >> token;
            str=AnsiString(token.c_str());
            Params[0].asText=new char[str.Length()+1];
            strcpy(Params[0].asText,str.c_str());
            if (str!=AnsiString("*"))       //*=nie brac tego pod uwage
              Params[12].asInt|=conditional_memstring;
            parser->getTokens();
            *parser >> token;
            str=AnsiString(token.c_str());
            if (str!=AnsiString("*"))       //*=nie brac tego pod uwage)
             {
              Params[12].asInt|=conditional_memval1;
              Params[1].asdouble=str.ToDouble();
             }
            else
             Params[1].asdouble=0;
            parser->getTokens();
            *parser >> token;
            str=AnsiString(token.c_str());
            if (str!=AnsiString("*"))       //*=nie brac tego pod uwage
             {
              Params[12].asInt|=conditional_memval2;
              Params[2].asdouble=str.ToDouble();
             }
            else
             Params[2].asdouble=0;
            parser->getTokens();
            *parser >> token;
        break;
        case tp_WhoIs:
        case tp_CopyValues:
        case tp_GetValues:
        case tp_LogValues:
            parser->getTokens(); //nazwa komórki pamiêci
            *parser >> token;
        break;
        case tp_PutValues:
            parser->getTokens(3);
            *parser >> Params[3].asdouble >> Params[4].asdouble >> Params[5].asdouble; //polozenie X,Y,Z
            if (org)
            {//przesuniêcie
             Params[3].asdouble+=org->x;
             Params[4].asdouble+=org->y;
             Params[5].asdouble+=org->z;
            }
            Params[12].asInt=0;
            parser->getTokens(1,false);  //komendy 'case sensitive'
            *parser >> token;
            str=AnsiString(token.c_str());
            if (str.SubString(1,19)=="PassengerStopPoint:")
             if (str.Pos("#")) str=str.SubString(1,str.Pos("#")-1); //obciêcie unikatowoœci
            Params[0].asText=new char[str.Length()+1];
            strcpy(Params[0].asText,str.c_str());
//            if (str!=AnsiString("*"))       //*=nie brac tego pod uwage
//              Params[12].asInt+=conditional_memstring;
            parser->getTokens();
            *parser >> token;
            str=AnsiString(token.c_str());
            try
            {Params[1].asdouble=str.ToDouble();}
            catch (...)
            {Params[1].asdouble=0.0;
             WriteLog("Error: number expected in PutValues event, found: "+str);
            }
            parser->getTokens();
            *parser >> token;
            str=AnsiString(token.c_str());
            try
            {Params[2].asdouble=str.ToDouble();}
            catch (...)
            {Params[2].asdouble=0.0;
             WriteLog("Error: number expected in PutValues event, found: "+str);
            }
            parser->getTokens();
            *parser >> token;
        break;
        case tp_Lights:
            i=0;
            do {
               parser->getTokens();
               *parser >> token;
               if (token.compare( "endevent" ) !=0 )
                {
                  str=AnsiString(token.c_str());
                  if (i<8)
                    Params[i].asInt=str.ToInt();
                  i++;
                }
               } while ( token.compare( "endevent" ) !=0 );

        break;
        case tp_Velocity:
            parser->getTokens();
            *parser >> token;
            str=AnsiString(token.c_str());
            Params[0].asdouble=str.ToDouble()*0.28;
            parser->getTokens(); *parser >> token;
        break;
        case tp_Sound:
//            Params[0].asRealSound->Init(asNodeName.c_str(),Parser->GetNextSymbol().ToDouble(),Parser->GetNextSymbol().ToDouble(),Parser->GetNextSymbol().ToDouble(),Parser->GetNextSymbol().ToDouble());
//McZapkie-070502: dzwiek przestrzenny (ale do poprawy)
//            Params[1].asdouble=Parser->GetNextSymbol().ToDouble();
//            Params[2].asdouble=Parser->GetNextSymbol().ToDouble();
//            Params[3].asdouble=Parser->GetNextSymbol().ToDouble(); //polozenie X,Y,Z - do poprawy!
            parser->getTokens();
            *parser >> Params[0].asInt; //0: wylaczyc, 1: wlaczyc; -1: wlaczyc zapetlone
            parser->getTokens(); *parser >> token;
        break;
        case tp_Exit:
            while ((ptr=strchr(asNodeName.c_str(),'_'))!=NULL)
                *ptr=' ';
            parser->getTokens(); *parser >> token;
        break;
        case tp_Disable:
            parser->getTokens(); *parser >> token;
        break;
        case tp_Animation:
            parser->getTokens();
            *parser >> token;
            Params[0].asInt=0; //nieznany typ
            if ( token.compare( "rotate" ) ==0 )
            {
                parser->getTokens();
                *parser >> token;
                Params[9].asText=new char[255]; //nazwa submodelu
                strcpy(Params[9].asText,token.c_str());
                Params[0].asInt=1;
                parser->getTokens(4);
                *parser >> Params[1].asdouble >> Params[2].asdouble >> Params[3].asdouble >> Params[4].asdouble;
            }
            else
            if ( token.compare( "translate" ) ==0 )
            {
                parser->getTokens();
                *parser >> token;
                Params[9].asText=new char[255]; //nazwa submodelu
                strcpy(Params[9].asText,token.c_str());
                Params[0].asInt=2;
                parser->getTokens(4);
                *parser >> Params[1].asdouble >> Params[2].asdouble >> Params[3].asdouble >> Params[4].asdouble;
            }
            parser->getTokens(); *parser >> token;
        break;
        case tp_Switch:
            parser->getTokens(); *parser >> Params[0].asInt;
            parser->getTokens(); *parser >> token;
        break;
        case tp_DynVel:
            parser->getTokens();
            *parser >> Params[0].asdouble; //McZapkie-090302 *0.28;
            parser->getTokens(); *parser >> token;
        break;
        case tp_TrackVel:
            parser->getTokens();
            *parser >> Params[0].asdouble; //McZapkie-090302 *0.28;
            parser->getTokens(); *parser >> token;
        break;
        case tp_Multiple:
            i=0;
            Params[8].asInt=0;

            parser->getTokens();
            *parser >> token;
            str=AnsiString(token.c_str());

            while (str!=AnsiString("endevent") && str!=AnsiString("condition"))
            {
             if ((str.SubString(1,5)!="none_")?(i<8):false)
             {//eventy rozpoczynaj¹ce siê od "none_" s¹ ignorowane
              Params[i].asText=new char[255];
              strcpy(Params[i].asText,str.c_str());
              i++;
             }
             else
              WriteLog("Event \""+str+"\" ignored in multiple \""+asName+"\"!");
             parser->getTokens();
             *parser >> token;
             str=AnsiString(token.c_str());
            }
            if (str==AnsiString("condition"))
            {
                Params[9].asText=new char[255];
                strcpy(Params[9].asText,asNodeName.c_str());
                parser->getTokens();
                *parser >> token;
                str=AnsiString(token.c_str());
                if (str==AnsiString("trackoccupied"))
                    Params[8].asInt=conditional_trackoccupied;
                if (str==AnsiString("trackfree"))
                    Params[8].asInt=conditional_trackfree;
                if (str==AnsiString("propability"))
                  {
                    Params[8].asInt=conditional_propability;
                    parser->getTokens();
                    *parser >> Params[10].asdouble;
                  }
                if  (str==AnsiString("memcompare"))
                  {
                    Params[8].asInt=0;
                    parser->getTokens(1,false);  //case sensitive
                    *parser >> token;
                    str=AnsiString(token.c_str());
                    Params[10].asText=new char[255];
                    strcpy(Params[10].asText,str.c_str());
                    if (str!=AnsiString("*"))       //*=nie brac command pod uwage
                     Params[8].asInt+=conditional_memstring;
                    parser->getTokens();
                    *parser >> token;
                    str=AnsiString(token.c_str());
                    if (str!=AnsiString("*"))       //*=nie brac val1 pod uwage
                     {
                      Params[11].asdouble=str.ToDouble();
                      Params[8].asInt+=conditional_memval1;
                     }
                    parser->getTokens();
                    *parser >> token;
                    str=AnsiString(token.c_str());
                    if (str!=AnsiString("*"))       //*=nie brac val2 pod uwage
                     {
                      Params[12].asdouble=str.ToDouble();
                      Params[8].asInt+=conditional_memval2;
                     }
                  }
                    parser->getTokens(); *parser >> token;

            }
        break;
        case tp_Ignored: //ignorowany
        case tp_Unknown: //nieznany
         do
         {parser->getTokens();
          *parser >> token;
          str=AnsiString(token.c_str());
         } while (str!="endevent");

         WriteLog("Event \""+asName+(Type==tp_Unknown?"\" has unknown type.":"\" is ignored."));
         break;
    }
}

void __fastcall TEvent::AddToQuery(TEvent *Event)
{//dodanie eventu do kolejki
 if (Next&&(Next->fStartTime<Event->fStartTime))
  Next->AddToQuery(Event); //sortowanie wg czasu
 else
 {//dodanie z przodu
  Event->Next=Next;
  Next=Event;
 }
}

//---------------------------------------------------------------------------

AnsiString __fastcall TEvent::CommandGet()
{//odczytanie komendy z eventu
 switch (Type)
 {//to siê wykonuje równie¿ sk³adu jad¹cego bez obs³ugi
  case tp_GetValues:
   return String(Params[9].asMemCell->szText);
  case tp_PutValues:
   return String(Params[0].asText);
 }
 return ""; //inne eventy siê nie licz¹
};
/* //siê sprawa komplikuje
vector3* __fastcall TEvent::PositionGet()
{//pobranie wspó³rzêdnych eventu
 switch (Type)
 {//to siê wykonuje równie¿ sk³adu jad¹cego bez obs³ugi
  case tp_GetValues:
   return &Params[8].asGroundNode->pCenter;
  case tp_PutValues:
   return String(Params[0].asText);
 }
 return NULL; //inne eventy siê nie licz¹
};
*/
