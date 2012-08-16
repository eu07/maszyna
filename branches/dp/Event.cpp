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
#include "Globals.h"
#include "Ground.h"
#pragma package(smart_init)

__fastcall TEvent::TEvent()
{
 Next=Next2=NULL;
 bEnabled=false; //false dla event�w u�ywanych do skanowania sygna��w (nie dodawane do kolejki)
 asNodeName="";
 bLaunched=false;
 bIsHistory=false;
 fDelay=0;
 fStartTime=0;
 Type=tp_Unknown;
 for (int i=0;i<13;i++)
  Params[i].asPointer=NULL;
}

__fastcall TEvent::~TEvent()
{
 switch (Type)
 {//sprz�tanie
  case tp_Multiple:
   //SafeDeleteArray(Params[9].asText); //nie usuwa� - nazwa obiektu powi�zanego zamieniana na wska�nik
   if (Params[8].asInt&conditional_memstring) //o ile jest �a�cuch do por�wnania w memcompare
    SafeDeleteArray(Params[10].asText);
   break;
  case tp_UpdateValues:
  case tp_AddValues:
   SafeDeleteArray(Params[0].asText);
  break;
  case tp_Animation: //nic
   //SafeDeleteArray(Params[9].asText); //nie usuwa� - nazwa jest zamieniana na wska�nik do submodelu
  case tp_GetValues: //nic
  break;
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

    bEnabled=true; //zmieniane na false dla event�w u�ywanych do skanowania sygna��w

    parser->getTokens();
    *parser >> token;
    asName=AnsiString(token.c_str()).LowerCase(); //u�ycie parametr�w mo�e dawa� wielkie

    parser->getTokens();
    *parser >> token;
    str=AnsiString(token.c_str());

    if (str==AnsiString("exit"))
     Type=tp_Exit;
    else if (str==AnsiString("updatevalues"))
     Type=tp_UpdateValues;
    else if (str==AnsiString("getvalues"))
     Type=tp_GetValues;
    else if (str==AnsiString("putvalues"))
     Type=tp_PutValues;
    else if (str==AnsiString("disable"))
     Type=tp_Disable;
    else if (str==AnsiString("sound"))
     Type=tp_Sound;
    else if (str==AnsiString("velocity"))
     Type=tp_Velocity;
    else if (str==AnsiString("animation"))
     Type=tp_Animation;
    else if (str==AnsiString("lights"))
     Type=tp_Lights;
    else if (str==AnsiString("visible"))
     Type=tp_Visible; //zmiana wy�wietlania obiektu
    else if (str==AnsiString("switch"))
     Type=tp_Switch;
    else if (str==AnsiString("dynvel"))
     Type=tp_DynVel;
    else if (str==AnsiString("trackvel"))
     Type=tp_TrackVel;
    else if (str==AnsiString("multiple"))
     Type=tp_Multiple;
    else if (str==AnsiString("addvalues"))
     Type=tp_AddValues;
    else if (str==AnsiString("copyvalues"))
     Type=tp_CopyValues;
    else if (str==AnsiString("whois"))
     Type=tp_WhoIs;
    else if (str==AnsiString("logvalues"))
     Type=tp_LogValues;
    else
     Type=tp_Unknown;

    parser->getTokens();
    *parser >> fDelay;

    parser->getTokens();
    *parser >> token;
    str=AnsiString(token.c_str());

    if (str!="none") asNodeName=str; //nazwa obiektu powi�zanego

    if (asName.SubString(1,5)=="none_")
     Type=tp_Ignored; //Ra: takie s� ignorowane

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
            parser->getTokens(); //nazwa kom�rki pami�ci
            *parser >> token;
        break;
        case tp_PutValues:
         parser->getTokens(3);
         *parser >> Params[3].asdouble >> Params[4].asdouble >> Params[5].asdouble; //polozenie X,Y,Z
         if (org)
         {//przesuni�cie
          Params[3].asdouble+=org->x; //wsp�rz�dne w scenerii
          Params[4].asdouble+=org->y;
          Params[5].asdouble+=org->z;
         }
         Params[12].asInt=0;
         parser->getTokens(1,false);  //komendy 'case sensitive'
         *parser >> token;
         str=AnsiString(token.c_str());
         if (str.SubString(1,19)=="PassengerStopPoint:")
         {if (str.Pos("#")) str=str.SubString(1,str.Pos("#")-1); //obci�cie unikatowo�ci
          bEnabled=false; //nie do kolejki (dla SetVelocity te�, ale jak jest do toru dowi�zany) 
         }
         if (str=="SetVelocity") bEnabled=false;
         if (str=="ShuntVelocity") bEnabled=false;
         Params[0].asText=new char[str.Length()+1];
         strcpy(Params[0].asText,str.c_str());
//         if (str!=AnsiString("*"))       //*=nie brac tego pod uwage
//           Params[12].asInt+=conditional_memstring;
         parser->getTokens();
         *parser >> token;
         str=AnsiString(token.c_str());
         if (str=="none")
          Params[1].asdouble=0.0;
         else
          try
          {Params[1].asdouble=str.ToDouble();}
          catch (...)
          {Params[1].asdouble=0.0;
           WriteLog("Error: number expected in PutValues event, found: "+str);
          }
         parser->getTokens();
         *parser >> token;
         str=AnsiString(token.c_str());
         if (str=="none")
          Params[2].asdouble=0.0;
         else
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
         do
         {
          parser->getTokens();
          *parser >> token;
          if (token.compare("endevent")!=0)
          {
           str=AnsiString(token.c_str());
           if (i<8)
            Params[i].asInt=str.ToInt();
           i++;
          }
         }
         while (token.compare("endevent")!=0);
         break;
        case tp_Visible: //zmiana wy�wietlania obiektu
         parser->getTokens();
         *parser >> token;
         str=AnsiString(token.c_str());
         Params[0].asInt=str.ToInt();
         parser->getTokens(); *parser >> token;
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
          {//eventy rozpoczynaj�ce si� od "none_" s� ignorowane
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
          if (!asNodeName.IsEmpty())
          {//podczepienie �a�cucha, je�li nie jest pusty
           Params[9].asText=new char[asNodeName.Length()+1]; //usuwane i zamieniane na wska�nik
           strcpy(Params[9].asText,asNodeName.c_str());
          }
          parser->getTokens();
          *parser >> token;
          str=AnsiString(token.c_str());
          if (str==AnsiString("trackoccupied"))
           Params[8].asInt=conditional_trackoccupied;
          else if (str==AnsiString("trackfree"))
           Params[8].asInt=conditional_trackfree;
          else if (str==AnsiString("propability"))
          {
           Params[8].asInt=conditional_propability;
           parser->getTokens();
           *parser >> Params[10].asdouble;
          }
          else if (str==AnsiString("memcompare"))
          {
           Params[8].asInt=0;
           parser->getTokens(1,false);  //case sensitive
           *parser >> token;
           str=AnsiString(token.c_str());
           if (str!=AnsiString("*")) //"*" - nie brac command pod uwage
           {//zapami�tanie �a�cucha do por�wnania
            Params[10].asText=new char[255];
            strcpy(Params[10].asText,str.c_str());
            Params[8].asInt|=conditional_memstring;
           }
           parser->getTokens();
           *parser >> token;
           str=AnsiString(token.c_str());
           if (str!=AnsiString("*")) //"*" - nie brac val1 pod uwage
           {
            Params[11].asdouble=str.ToDouble();
            Params[8].asInt|=conditional_memval1;
           }
           parser->getTokens();
           *parser >> token;
           str=AnsiString(token.c_str());
           if (str!=AnsiString("*")) //"*" - nie brac val2 pod uwage
           {
            Params[12].asdouble=str.ToDouble();
            Params[8].asInt|=conditional_memval2;
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
 //if (Type!=tp_Unknown)
  //if (Type!=tp_Ignored)
   //if (asName.Pos("onstart")) //event uruchamiany automatycznie po starcie
    //Global::pGround->AddToQuery(this,NULL); //dodanie do kolejki
    //return true; //doda� do kolejki
 //return false;
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
 {//to si� wykonuje r�wnie� sk�adu jad�cego bez obs�ugi
  case tp_GetValues:
   return String(Params[9].asMemCell->Text());
  case tp_PutValues:
   return String(Params[0].asText);
 }
 return ""; //inne eventy si� nie licz�
};

double __fastcall TEvent::ValueGet(int n)
{//odczytanie komendy z eventu
 n&=1; //tylko 1 albo 2 jest prawid�owy
 switch (Type)
 {//to si� wykonuje r�wnie� sk�adu jad�cego bez obs�ugi
  case tp_GetValues:
   return n?Params[9].asMemCell->Value1():Params[9].asMemCell->Value2();
  case tp_PutValues:
   return Params[2-n].asdouble;
 }
 return 0.0; //inne eventy si� nie licz�
};

vector3 __fastcall TEvent::PositionGet()
{//pobranie wsp�rz�dnych eventu
 switch (Type)
 {//
  case tp_GetValues:
   return Params[9].asMemCell->Position(); //wsp�rz�dne pod��czonej kom�rki pami�ci
  case tp_PutValues:
   return vector3(Params[3].asdouble,Params[4].asdouble,Params[5].asdouble);
 }
 return vector3(0,0,0); //inne eventy si� nie licz�
};

