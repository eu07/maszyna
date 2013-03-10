//---------------------------------------------------------------------------

/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak, Maciej Czapkiewicz and others

*/


#include "system.hpp"
#include "classes.hpp"
#pragma hdrstop

#include "Driver.h"
#include "mctools.hpp"
#include "MemCell.h"
#include "Event.h"
#include "parser.h"

#include "Usefull.h"
#include "Globals.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------

__fastcall TMemCell::TMemCell(vector3 *p)
{
 fValue1=fValue2=0;
 szText=NULL;
 vPosition=*p; //ustawienie wsp�rz�dnych, bo do TGroundNode nie ma dost�pu
 bCommand=false; //komenda wys�ana
 OnSent=NULL;
}

__fastcall TMemCell::~TMemCell()
{
 SafeDeleteArray(szText);
}

void __fastcall TMemCell::Init()
{
}

void __fastcall TMemCell::UpdateValues(char *szNewText, double fNewValue1, double fNewValue2, int CheckMask)
{
 if (CheckMask&update_memadd)
 {//dodawanie warto�ci
  if (TestFlag(CheckMask,update_memstring))
   strcat(szText,szNewText);
  if (TestFlag(CheckMask,update_memval1))
   fValue1+=fNewValue1;
  if (TestFlag(CheckMask,update_memval2))
   fValue2+=fNewValue2;
 }
 else
 {
  if (TestFlag(CheckMask,update_memstring))
   strcpy(szText,szNewText);
  if (TestFlag(CheckMask,update_memval1))
   fValue1=fNewValue1;
  if (TestFlag(CheckMask,update_memval2))
   fValue2=fNewValue2;
 }
 if (TestFlag(CheckMask,update_memstring))
  CommandCheck();//je�li zmieniony tekst, pr�bujemy rozpozna� komend�
}

TCommandType __fastcall TMemCell::CommandCheck()
{//rozpoznanie komendy
 if (strcmp(szText,"SetVelocity")==0) //najpopularniejsze
 {eCommand=cm_SetVelocity;
  bCommand=false; //ta komenda nie jest wysy�ana
 }
 else if (strcmp(szText,"ShuntVelocity")==0) //w tarczach manewrowych
 {eCommand=cm_ShuntVelocity;
  bCommand=false; //ta komenda nie jest wysy�ana
 }
 else if (strcmp(szText,"Change_direction")==0) //zdarza si�
 {eCommand=cm_ChangeDirection;
  bCommand=true; //do wys�ania
 }
 else if (strcmp(szText,"OutsideStation")==0) //zdarza si�
 {eCommand=cm_OutsideStation;
  bCommand=false; //tego nie powinno by� w kom�rce
 }
 else if (strcmp(szText,"PassengerStopPoint:")==0) //TODO: por�wna� pocz�tki !!!
 {eCommand=cm_PassengerStopPoint;
  bCommand=false; //tego nie powinno by� w kom�rce
 }
 else if (strcmp(szText,"SetProximityVelocity")==0) //nie powinno tego by�
 {eCommand=cm_SetProximityVelocity;
  bCommand=false; //ta komenda nie jest wysy�ana
 }
 else
 {eCommand=cm_Unknown; //ci�g nierozpoznany (nie jest komend�)
  bCommand=true; //do wys�ania
 }
 return eCommand;
}

bool __fastcall TMemCell::Load(cParser *parser)
{
 std::string token;
 parser->getTokens(1,false);  //case sensitive
 *parser >> token;
 SafeDeleteArray(szText);
 szText=new char[256]; //musi by� bufor do ��czenia tekst�w
 strcpy(szText,token.c_str());
 parser->getTokens(2);
 *parser >> fValue1 >> fValue2;
 parser->getTokens();
 *parser >> token;
 asTrackName= AnsiString(token.c_str());
 parser->getTokens();
 *parser >> token;
 if (token.compare("endmemcell")!=0)
  Error("endmemcell statement missing");
 CommandCheck();
 return true;
}

void __fastcall TMemCell::PutCommand(TController *Mech, vector3 *Loc)
{//wys�anie zawarto�ci kom�rki do AI
 if (Mech)
  Mech->PutCommand(szText,fValue1,fValue2,Loc);
}

bool __fastcall TMemCell::Compare(char *szTestText,double fTestValue1,double fTestValue2,int CheckMask)
{//por�wnanie zawarto�ci kom�rki pami�ci z podanymi warto�ciami
 if (TestFlag(CheckMask,conditional_memstring))
 {//por�wna� teksty
  char *pos=StrPos(szTestText,"*"); //zwraca wska�nik na pozycj� albo NULL
  if (pos)
  {//por�wnanie fragmentu �a�cucha
   int i=pos-szTestText; //ilo�� por�wnywanych znak�w
   if (i) //je�li nie jest pierwszym znakiem
    if (AnsiString(szTestText,i)!=AnsiString(szText,i))
     return false; //pocz�tki o d�ugo�ci (i) s� r�ne
  }
  else
   if (AnsiString(szTestText)!=AnsiString(szText))
    return false; //���cuchy s� r�ne
 }
 //tekst zgodny, por�wna� reszt�
 return ((!TestFlag(CheckMask,conditional_memval1) || (fValue1==fTestValue1)) &&
         (!TestFlag(CheckMask,conditional_memval2) || (fValue2==fTestValue2)));
};

bool __fastcall TMemCell::Render()
{
 return true;
}

bool __fastcall TMemCell::IsVelocity()
{//sprawdzenie, czy event odczytu tej kom�rki ma by� do skanowania, czy do kolejkowania
 if (eCommand==cm_SetVelocity) return true;
 if (eCommand==cm_ShuntVelocity) return true;
 return (eCommand==cm_SetProximityVelocity);
};

void __fastcall TMemCell::StopCommandSent()
{//
 if (!bCommand) return;
 bCommand=false;
 if (OnSent) //je�li jest event
  Global::AddToQuery(OnSent,NULL);
};

void __fastcall TMemCell::AssignEvents(TEvent *e)
{//powi�zanie eventu
 OnSent=e;
};
