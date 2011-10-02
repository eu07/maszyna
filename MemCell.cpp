//---------------------------------------------------------------------------

/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak, Maciej Czapkiewicz and others

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


#include "system.hpp"
#include "classes.hpp"
#pragma hdrstop

//#include "Mover.hpp"
#include "Driver.h"
#include "mctools.hpp"
#include "MemCell.h"
#include "Event.h"
#include "parser.h"

#include "Usefull.h"

//---------------------------------------------------------------------------

__fastcall TMemCell::TMemCell()
{
    fValue1=fValue2= 0;
    szText= NULL;
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
 if (CheckMask&conditional_memadd)
 {//dodawanie wartoœci
  if (TestFlag(CheckMask,conditional_memstring))
   strcat(szText,szNewText);
  if (TestFlag(CheckMask,conditional_memval1))
   fValue1+=fNewValue1;
  if (TestFlag(CheckMask,conditional_memval2))
   fValue2+=fNewValue2;
 }
 else
 {
  if (TestFlag(CheckMask,conditional_memstring))
   strcpy(szText,szNewText);
  if (TestFlag(CheckMask,conditional_memval1))
   fValue1= fNewValue1;
  if (TestFlag(CheckMask,conditional_memval2))
   fValue2= fNewValue2;
 }
}

bool __fastcall TMemCell::Load(cParser *parser)
{
    std::string token;
    parser->getTokens(1,false);  //case sensitive
    *parser >> token;
    SafeDeleteArray(szText);
    szText= new char[256];
    strcpy(szText,token.c_str());
    parser->getTokens(2);
    *parser >> fValue1 >> fValue2;
    parser->getTokens();
    *parser >> token;
    asTrackName= AnsiString(token.c_str());
    parser->getTokens();
    *parser >> token;
    if (token.compare( "endmemcell" ) != 0)
     Error("endmemcell statement missing");
    return true;
}

void __fastcall TMemCell::PutCommand(TController *Mech, TLocation &Loc)
{//wys³anie zawartoœci komórki do AI
 if (Mech)
  Mech->PutCommand(szText,fValue1,fValue2,Loc);
}

bool __fastcall TMemCell::Compare(char *szTestText,double fTestValue1,double fTestValue2,int CheckMask)
{//porównanie zawartoœci komórki pamiêci z podanymi wartoœciami
 if (TestFlag(CheckMask,conditional_memstring))
 {//porównaæ teksty
  char *pos=StrPos(szTestText,"*"); //zwraca wskaŸnik na pozycjê albo NULL
  if (pos)
  {//porównanie fragmentu ³añcucha
   int i=pos-szTestText; //iloœæ porównywanych znaków
   if (i) //jeœli nie jest pierwszym znakiem
    if (AnsiString(szTestText,i)!=AnsiString(szText,i))
     return false; //pocz¹tki o d³ugoœci (i) s¹ ró¿ne
  }
  else
   if (AnsiString(szTestText)!=AnsiString(szText))
    return false; //³¹ñcuchy s¹ ró¿ne
 }
 //tekst zgodny, porównaæ resztê
 return ((!TestFlag(CheckMask,conditional_memval1) || (fValue1==fTestValue1)) &&
         (!TestFlag(CheckMask,conditional_memval2) || (fValue2==fTestValue2)));
};

bool __fastcall TMemCell::Render()
{
    return true;
}

//---------------------------------------------------------------------------

#pragma package(smart_init)
