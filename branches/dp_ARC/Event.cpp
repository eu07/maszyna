//---------------------------------------------------------------------------

/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak and others

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

__fastcall TEvent::TEvent(AnsiString m)
{
 //asName=""; //czy nazwa eventu jest niezb�dna w tym przypadku? chyba nie
 evNext=evNext2=NULL;
 bEnabled=false; //false dla event�w u�ywanych do skanowania sygna��w (nie dodawane do kolejki)
 asNodeName=m; //nazwa obiektu powi�zanego
 iQueued=0; //nie zosta� dodany do kolejki
 //bIsHistory=false;
 fDelay=0;
 fStartTime=0; //0 nie ma sensu
 Type=m.IsEmpty()?tp_Unknown:tp_GetValues; //utworzenie niejawnego odczytu kom�rki pami�ci w torze
 for (int i=0;i<13;i++)
  Params[i].asPointer=NULL;
 evJoined=NULL; //nie ma kolejnego z t� sam� nazw�, usuwane s� wg listy Next2
 Activator=NULL;
 iFlags=0;
 //event niejawny jest tworzony przed faz� InitEvents, kt�ra podmienia nazw� kom�rki pami�ci na wska�nik
 //Current->Params[8].asGroundNode=m; //to si� ustawi w InitEvents
 //Current->Params[9].asMemCell=m->MemCell;
};

__fastcall TEvent::~TEvent()
{
 switch (Type)
 {//sprz�tanie
  case tp_Multiple:
   //SafeDeleteArray(Params[9].asText); //nie usuwa� - nazwa obiektu powi�zanego zamieniana na wska�nik
   if (iFlags&conditional_memstring) //o ile jest �a�cuch do por�wnania w memcompare
    SafeDeleteArray(Params[10].asText);
   break;
  case tp_UpdateValues:
  case tp_AddValues:
   SafeDeleteArray(Params[0].asText);
   if (iFlags&conditional_memstring) //o ile jest �a�cuch do por�wnania w memcompare
    SafeDeleteArray(Params[10].asText);
  break;
  case tp_Animation: //nic
   //SafeDeleteArray(Params[9].asText); //nie usuwa� - nazwa jest zamieniana na wska�nik do submodelu
   if (Params[0].asInt==4) //je�li z pliku VMD
    delete[] Params[8].asPointer; //zwolni� obszar
  case tp_GetValues: //nic
  break;
 }
 evJoined=NULL; //nie usuwa� podczepionych tutaj
};

void __fastcall TEvent::Init()
{

};

void __fastcall TEvent::Conditions(cParser* parser,AnsiString s)
{//przetwarzanie warunk�w, wsp�lne dla Multiple i UpdateValues
 if (s=="condition")
 {//jesli nie "endevent"
  std::string token;
  AnsiString str;
  if (!asNodeName.IsEmpty())
  {//podczepienie �a�cucha, je�li nie jest pusty
   Params[9].asText=new char[asNodeName.Length()+1]; //usuwane i zamieniane na wska�nik
   strcpy(Params[9].asText,asNodeName.c_str());
  }
  parser->getTokens();
  *parser >> token;
  str=AnsiString(token.c_str());
  if (str==AnsiString("trackoccupied"))
   iFlags|=conditional_trackoccupied;
  else if (str==AnsiString("trackfree"))
   iFlags|=conditional_trackfree;
  else if (str==AnsiString("propability"))
  {
   iFlags|=conditional_propability;
   parser->getTokens();
   *parser >> Params[10].asdouble;
  }
  else if (str==AnsiString("memcompare"))
  {
   iFlags|=conditional_memcompare;
   parser->getTokens(1,false);  //case sensitive
   *parser >> token;
   str=AnsiString(token.c_str());
   if (str!="*") //"*" - nie brac command pod uwage
   {//zapami�tanie �a�cucha do por�wnania
    Params[10].asText=new char[255];
    strcpy(Params[10].asText,str.c_str());
    iFlags|=conditional_memstring;
   }
   parser->getTokens();
   *parser >> token;
   str=AnsiString(token.c_str());
   if (str!="*") //"*" - nie brac val1 pod uwage
   {
    Params[11].asdouble=str.ToDouble();
    iFlags|=conditional_memval1;
   }
   parser->getTokens();
   *parser >> token;
   str=AnsiString(token.c_str());
   if (str!=AnsiString("*")) //"*" - nie brac val2 pod uwage
   {
    Params[12].asdouble=str.ToDouble();
    iFlags|=conditional_memval2;
   }
  }
  parser->getTokens(); *parser >> token;
 }
};

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
   iFlags=update_memadd; //dodawanko
  case tp_UpdateValues:
   //if (Type==tp_UpdateValues) iFlags=0; //co modyfikowa�
   parser->getTokens(1,false);  //case sensitive
   *parser >> token;
   str=AnsiString(token.c_str());
   Params[0].asText=new char[str.Length()+1];
   strcpy(Params[0].asText,str.c_str());
   if (str!="*") //czy ma zosta� bez zmian?
    iFlags|=update_memstring;
   parser->getTokens();
   *parser >> token;
   str=AnsiString(token.c_str());
   if (str!="*") //czy ma zosta� bez zmian?
   {
    Params[1].asdouble=str.ToDouble();
    iFlags|=update_memval1;
   }
   else
    Params[1].asdouble=0;
   parser->getTokens();
   *parser >> token;
   str=AnsiString(token.c_str());
   if (str!="*") //czy ma zosta� bez zmian?
   {
    Params[2].asdouble=str.ToDouble();
    iFlags|=update_memval2;
   }
   else
    Params[2].asdouble=0;
   parser->getTokens();
   *parser >> token;
   Conditions(parser,token.c_str()); //sprawdzanie warunk�w
  break;
  case tp_CopyValues:
   Params[9].asText=NULL;
   iFlags=update_memstring|update_memval1|update_memval2; //normalanie trzy
   i=0;
   parser->getTokens();
   *parser >> token; //nazwa drugiej kom�rki (�r�d�owej)
   while (token.compare("endevent")!=0)
   {
    switch (++i)
    {//znaczenie kolejnych parametr�w
     case 1: //nazwa drugiej kom�rki (�r�d�owej)
      Params[9].asText=new char[token.length()+1]; //usuwane i zamieniane na wska�nik
      strcpy(Params[9].asText,token.c_str());
     break;
     case 2: //maska warto�ci
      iFlags=AnsiString(token.c_str()).ToIntDef(update_memstring|update_memval1|update_memval2);
     break;
    }
    parser->getTokens();
    *parser >> token;
   }
  break;
  case tp_WhoIs:
   iFlags=update_memstring|update_memval1|update_memval2; //normalanie trzy
   i=0;
   parser->getTokens();
   *parser >> token; //nazwa drugiej kom�rki (�r�d�owej)
   while (token.compare("endevent")!=0)
   {
    switch (++i)
    {//znaczenie kolejnych parametr�w
     case 1: //maska warto�ci
      iFlags=AnsiString(token.c_str()).ToIntDef(update_memstring|update_memval1|update_memval2);
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
   *parser >> Params[3].asdouble >> Params[4].asdouble >> Params[5].asdouble; //po�o�enie X,Y,Z
   if (org)
   {//przesuni�cie
    Params[3].asdouble+=org->x; //wsp�rz�dne w scenerii
    Params[4].asdouble+=org->y;
    Params[5].asdouble+=org->z;
   }
   //Params[12].asInt=0;
   parser->getTokens(1,false);  //komendy 'case sensitive'
   *parser >> token;
   str=AnsiString(token.c_str());
   if (str.SubString(1,19)=="PassengerStopPoint:")
   {if (str.Pos("#")) str=str.SubString(1,str.Pos("#")-1); //obci�cie unikatowo�ci
    bEnabled=false; //nie do kolejki (dla SetVelocity te�, ale jak jest do toru dowi�zany)
    Params[6].asCommand=cm_PassengerStopPoint;
   }
   else if (str=="SetVelocity")
   {bEnabled=false;
    Params[6].asCommand=cm_SetVelocity;
   }
   else if (str=="ShuntVelocity")
   {bEnabled=false;
    Params[6].asCommand=cm_ShuntVelocity;
   }
   else if (str=="SetProximityVelocity")
   {bEnabled=false;
    Params[6].asCommand=cm_SetProximityVelocity;
   }
   else if (str=="OutsideStation")
   {bEnabled=false; //ma by� skanowny, aby AI nie przekracza�o W5
    Params[6].asCommand=cm_OutsideStation;
   }
   else
    Params[6].asCommand=cm_Unknown;
   Params[0].asText=new char[str.Length()+1];
   strcpy(Params[0].asText,str.c_str());
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
      Params[i].asdouble=str.ToDouble(); //teraz mo�e mie� u�amek
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
   //Params[0].asRealSound->Init(asNodeName.c_str(),Parser->GetNextSymbol().ToDouble(),Parser->GetNextSymbol().ToDouble(),Parser->GetNextSymbol().ToDouble(),Parser->GetNextSymbol().ToDouble());
   //McZapkie-070502: dzwiek przestrzenny (ale do poprawy)
   //Params[1].asdouble=Parser->GetNextSymbol().ToDouble();
   //Params[2].asdouble=Parser->GetNextSymbol().ToDouble();
   //Params[3].asdouble=Parser->GetNextSymbol().ToDouble(); //polozenie X,Y,Z - do poprawy!
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
   Params[0].asInt=0; //na razie nieznany typ
   if (token.compare("rotate")==0)
   {//obr�t wzgl�dem osi
    parser->getTokens();
    *parser >> token;
    Params[9].asText=new char[255]; //nazwa submodelu
    strcpy(Params[9].asText,token.c_str());
    Params[0].asInt=1;
    parser->getTokens(4);
    *parser >> Params[1].asdouble >> Params[2].asdouble >> Params[3].asdouble >> Params[4].asdouble;
   }
   else
   if (token.compare("translate")==0)
   {//przesuw o wektor
    parser->getTokens();
    *parser >> token;
    Params[9].asText=new char[255]; //nazwa submodelu
    strcpy(Params[9].asText,token.c_str());
    Params[0].asInt=2;
    parser->getTokens(4);
    *parser >> Params[1].asdouble >> Params[2].asdouble >> Params[3].asdouble >> Params[4].asdouble;
   }
   else if (token.substr(token.length()-4,4)==".vmd") //na razie tu, mo�e b�dzie inaczej
   {//animacja z pliku VMD
    TFileStream *fs=new TFileStream("models\\"+AnsiString(token.c_str()),fmOpenRead);
    Params[7].asInt=fs->Size;
    Params[8].asPointer=new char[fs->Size];
    fs->Read(Params[8].asPointer,fs->Size); //wczytanie pliku
    delete fs;
    parser->getTokens();
    *parser >> token;
    Params[9].asText=new char[255]; //nazwa submodelu
    strcpy(Params[9].asText,token.c_str());
    Params[0].asInt=4; //rodzaj animacji
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
   ti=0; //flaga dla else
   parser->getTokens();
   *parser >> token;
   str=AnsiString(token.c_str());
   while (str!=AnsiString("endevent") && str!=AnsiString("condition"))
   {
    if ((str.SubString(1,5)!="none_")?(i<8):false)
    {//eventy rozpoczynaj�ce si� od "none_" s� ignorowane
     if (str!="else")
     {Params[i].asText=new char[255];
      strcpy(Params[i].asText,str.c_str());
      if (ti) iFlags|=conditional_else<<i; //oflagowanie dla event�w "else"
      i++;
     }
     else
      ti=!ti; //zmiana flagi dla s�owa "else"
    }
    else
     WriteLog("Event \""+str+"\" ignored in multiple \""+asName+"\"!");
    parser->getTokens();
    *parser >> token;
    str=AnsiString(token.c_str());
   }
   Conditions(parser,str); //sprawdzanie warunk�w
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
};

void __fastcall TEvent::AddToQuery(TEvent *e)
{//dodanie eventu do kolejki
 if (evNext?(e->fStartTime>=evNext->fStartTime):false)
  evNext->AddToQuery(e); //sortowanie wg czasu
 else
 {//dodanie z przodu
  e->evNext=evNext;
  evNext=e;
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

TCommandType __fastcall TEvent::Command()
{//odczytanie komendy z eventu
 switch (Type)
 {//to si� wykonuje r�wnie� dla sk�adu jad�cego bez obs�ugi
  case tp_GetValues:
   return Params[9].asMemCell->Command();
  case tp_PutValues:
   return Params[6].asCommand; //komenda zakodowana binarnie
 }
 return cm_Unknown; //inne eventy si� nie licz�
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

bool __fastcall TEvent::StopCommand()
{//
 if (Type==tp_GetValues)
  return Params[9].asMemCell->StopCommand(); //info o komendzie z kom�rki
 return false;
};

void __fastcall TEvent::StopCommandSent()
{
 if (Type==tp_GetValues)
  Params[9].asMemCell->StopCommandSent(); //komenda z kom�rki zosta�a wys�ana
};

void __fastcall TEvent::Append(TEvent *e)
{//doczepienie kolejnych z t� sam� nazw�
 if (evJoined)
  evJoined->Append(e); //rekurencja! - g�ra kilkana�cie event�w b�dzie potrzebne
 else
 {evJoined=e;
  e->bEnabled=true; //ten doczepiony mo�e by� tylko kolejkowany
 }
};
