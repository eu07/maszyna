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

#include "system.hpp"
#include "classes.hpp"

#include "opengl/glew.h"
#include "opengl/glut.h"

#pragma hdrstop

#include "Timer.h"
#include "Texture.h"
#include "Ground.h"
#include "Globals.h"
#include "Event.h"
#include "EvLaunch.h"
#include "TractionPower.h"
#include "Traction.h"
#include "Track.h"
#include "RealSound.h"
#include "AnimModel.h"
#include "MemCell.h"
#include "mtable.hpp"
#include "DynObj.h"
#include "Data.h"
#include "world.h"
#include "parser.h" //Tolaris-010603
#include "Driver.h"
#include "Train.h"
//---------------------------------------------------------------------------

#pragma package(smart_init)

TWorld *W;
TTrain *T;



bool __fastcall TGround::READADDFILE(AnsiString FILENAME, TDynamicObject *DYN)
{

AnsiString DYNOBJBDIR =  Global::g___APPDIR + DYN->asBaseDir;

FILENAME =  AnsiString(DYNOBJBDIR + DYN->MoverParameters->Name + ".add");

                 AnsiString mdventil1="";
                 AnsiString mdventil2="";
                 AnsiString mdhaslerA="";
                 AnsiString mdhaslerB="";
                 AnsiString mdczuwakA="";
                 AnsiString mdczuwakB="";
                 AnsiString mdclock_1="";
                 AnsiString mdclock_2="";
                 AnsiString mdbogey_A="";
                 AnsiString mdbogey_B="";
                 AnsiString mdpanto1A="";
                 AnsiString mdpanto1B="";
                 AnsiString mdpanto2A="";
                 AnsiString mdpanto2B="";
                 AnsiString mddirtab1="";
                 AnsiString mddirtab2="";
                 AnsiString mdfotel1A="";
                 AnsiString mdfotel2A="";
                 AnsiString mdfotel1B="";
                 AnsiString mdfotel2B="";
                 AnsiString mdogszyb1="";
                 AnsiString mdogszyb2="";
                 AnsiString mdswsockA="";
                 AnsiString mdswsockB="";
                 AnsiString mdwindowN="";
                 AnsiString mdzgrnczA="";
                 AnsiString mdzgrnczB="";
                 AnsiString mdnastawA="";
                 AnsiString mdnastawB="";
                 AnsiString mdstolikA="";
                 AnsiString mdstolikB="";
                 AnsiString mdstatic1="";
                 AnsiString mdstatic2="";
                 AnsiString mdstatic3="";
                 AnsiString mdstatic4="";
                 AnsiString mdstatic5="";
                 AnsiString mdstatic6="";
                 AnsiString mdstatic7="";
                 AnsiString mdstatic8="";
                 AnsiString mdstatic9="";
                 AnsiString mdstatic0="";

                 AnsiString txkabinaA="";
                 AnsiString txkabinaB="";

                 AnsiString test, strl;
                 TStringList *ADDFILE;

                 DYNOBJBDIR = Global::g___APPDIR + DYN->asBaseDir;
                 
                 if (FileExists( FILENAME ))
                     {

                      int ADDFILELC;
                      ADDFILE = new TStringList;
                      ADDFILE->Clear();
                      ADDFILE->LoadFromFile( FILENAME );
                      ADDFILELC = ADDFILE->Count;
                      WriteLog("ADD FILE EXIST!!!");
                      WriteLog(AnsiString(DYNOBJBDIR + DYN->MoverParameters->Name + ".add"));
                      AnsiString WHAT, T3D, TEX;
                      for (int i=0; i<ADDFILELC; i++)
                           {
                            strl = ADDFILE->Strings[i];
                            test = strl.SubString(1, 10);

                            if (test == "mdventil1:") mdventil1 = strl.SubString(12, 255);
                            if (test == "mdventil2:") mdventil2 = strl.SubString(12, 255);
                            if (test == "mdhaslerA:") mdhaslerA = strl.SubString(12, 255);
                            if (test == "mdhaslerB:") mdhaslerB = strl.SubString(12, 255);
                            if (test == "mdczuwakA:") mdczuwakA = strl.SubString(12, 255);
                            if (test == "mdczuwakB:") mdczuwakB = strl.SubString(12, 255);
                            if (test == "mdbogey_A:") mdbogey_A = strl.SubString(12, 255);
                            if (test == "mdbogey_B:") mdbogey_B = strl.SubString(12, 255);
                            if (test == "mdclock_1:") mdclock_1 = strl.SubString(12, 255);
                            if (test == "mdclock_2:") mdclock_2 = strl.SubString(12, 255);
                            if (test == "mdpanto1A:") mdpanto1A = strl.SubString(12, 255);
                            if (test == "mdpanto1B:") mdpanto1B = strl.SubString(12, 255);
                            if (test == "mdfotel1A:") mdfotel1A = strl.SubString(12, 255);
                            if (test == "mdfotel2A:") mdfotel2A = strl.SubString(12, 255);
                            if (test == "mdfotel1B:") mdfotel1B = strl.SubString(12, 255);
                            if (test == "mdfotel2B:") mdfotel2B = strl.SubString(12, 255);
                            if (test == "mddirtab1:") mddirtab1 = strl.SubString(12, 255);
                            if (test == "mddirtab2:") mddirtab2 = strl.SubString(12, 255);
                            if (test == "mdwindown:") mdwindowN = strl.SubString(12, 255);
                            if (test == "mdogszyb1:") mdogszyb1 = strl.SubString(12, 255);
                            if (test == "mdogszyb2:") mdogszyb2 = strl.SubString(12, 255);
                            if (test == "mdswsockA:") mdswsockA = strl.SubString(12, 255);
                            if (test == "mdswsockB:") mdswsockB = strl.SubString(12, 255);
                            if (test == "mdplog__A:") mdzgrnczA = strl.SubString(12, 255);
                            if (test == "mdplog__B:") mdzgrnczB = strl.SubString(12, 255);
                            if (test == "mdnastawA:") mdnastawA = strl.SubString(12, 255);
                            if (test == "mdnastawB:") mdnastawB = strl.SubString(12, 255);
                            if (test == "mdstolikA:") mdstolikA = strl.SubString(12, 255);
                            if (test == "mdstolikB:") mdstolikB = strl.SubString(12, 255);
                            if (test == "mdstatic1:") mdstatic1 = strl.SubString(12, 255);
                            if (test == "mdstatic2:") mdstatic2 = strl.SubString(12, 255);
                            if (test == "mdstatic3:") mdstatic3 = strl.SubString(12, 255);
                            if (test == "mdstatic4:") mdstatic4 = strl.SubString(12, 255);
                            if (test == "mdstatic5:") mdstatic5 = strl.SubString(12, 255);
                            if (test == "mdstatic6:") mdstatic6 = strl.SubString(12, 255);
                            if (test == "mdstatic7:") mdstatic7 = strl.SubString(12, 255);
                            if (test == "mdstatic8:") mdstatic8 = strl.SubString(12, 255);
                            if (test == "mdstatic9:") mdstatic9 = strl.SubString(12, 255);
                            if (test == "mdstatic0:") mdstatic0 = strl.SubString(12, 255);

                            if (test == "txkabinaA:") txkabinaA = strl.SubString(12, 255);
                            if (test == "txkabinaB:") txkabinaB = strl.SubString(12, 255);
                           }
                       delete ADDFILE;    




                      DYN->SRJ1token = DYN->directtabletoken;                   // SLUZBOWY ROZKLAD JAZDY, RELACJA 'DO'
                      DYN->SRJ2token = DYN->directtabletoken;                   // SLUZBOWY ROZKLAD JAZDY, RELACJA 'Z'
                      DYN->associatedtrain = DYN->directtabletoken;             // PRZYPISANIE RELACJI DO WAGONU

                      W->RenderLoader(77, "WCZYTYWANIE MODELI SKLADOWYCH POJAZDU");
                      Global::asCurrentTexturePath = DYN->asBaseDir;
                      
                      if (mdventil1 != "") DYN->mdVentilator1 = TModelsManager::GetModel(AnsiString(DYN->asBaseDir  + mdventil1).c_str(),true);
                      if (mdventil2 != "") DYN->mdVentilator2 = TModelsManager::GetModel(AnsiString(DYN->asBaseDir  + mdventil2).c_str(),true);
                      if (mdclock_1 != "") DYN->mdClock1      = TModelsManager::GetModel(AnsiString(DYN->asBaseDir  + mdclock_1).c_str(),true);
                      if (mdclock_2 != "") DYN->mdClock2      = TModelsManager::GetModel(AnsiString(DYN->asBaseDir  + mdclock_2).c_str(),true);
                      if (mddirtab1 != "") DYN->mdDIRTABLE1   = TModelsManager::GetModel(AnsiString(DYN->asBaseDir  + mddirtab1).c_str(),true);
                      if (mddirtab2 != "") DYN->mdDIRTABLE2   = TModelsManager::GetModel(AnsiString(DYN->asBaseDir  + mddirtab2).c_str(),true);

                      if (mdstatic1 != "") DYN->mdSTATIC01   = TModelsManager::GetModel(AnsiString(DYN->asBaseDir  + mdstatic1).c_str(),true);
                      if (mdstatic2 != "") DYN->mdSTATIC02   = TModelsManager::GetModel(AnsiString(DYN->asBaseDir  + mdstatic2).c_str(),true);
                      if (mdstatic3 != "") DYN->mdSTATIC03   = TModelsManager::GetModel(AnsiString(DYN->asBaseDir  + mdstatic3).c_str(),true);
                      if (mdstatic4 != "") DYN->mdSTATIC04   = TModelsManager::GetModel(AnsiString(DYN->asBaseDir  + mdstatic4).c_str(),true);
                      if (mdstatic5 != "") DYN->mdSTATIC05   = TModelsManager::GetModel(AnsiString(DYN->asBaseDir  + mdstatic5).c_str(),true);
                      if (mdstatic6 != "") DYN->mdSTATIC06   = TModelsManager::GetModel(AnsiString(DYN->asBaseDir  + mdstatic6).c_str(),true);
                      if (mdstatic7 != "") DYN->mdSTATIC07   = TModelsManager::GetModel(AnsiString(DYN->asBaseDir  + mdstatic7).c_str(),true);
                      if (mdstatic8 != "") DYN->mdSTATIC08   = TModelsManager::GetModel(AnsiString(DYN->asBaseDir  + mdstatic8).c_str(),true);
                      if (mdstatic9 != "") DYN->mdSTATIC09   = TModelsManager::GetModel(AnsiString(DYN->asBaseDir  + mdstatic9).c_str(),true);
                      if (mdstatic0 != "") DYN->mdSTATIC10   = TModelsManager::GetModel(AnsiString(DYN->asBaseDir  + mdstatic0).c_str(),true);


                      if (mdclock_1 != "") DYN->mdClock1->Init();
                      if (mdclock_2 != "") DYN->mdClock2->Init();
                      if (mdventil1 != "") DYN->mdVentilator1->Init();
                      if (mdventil2 != "") DYN->mdVentilator2->Init();
                      if (mddirtab1 != "") DYN->mdDIRTABLE1->Init();
                      if (mddirtab2 != "") DYN->mdDIRTABLE2->Init();
                      if (mdstatic1 != "") DYN->mdSTATIC01->Init();
                      if (mdstatic2 != "") DYN->mdSTATIC02->Init();
                      if (mdstatic3 != "") DYN->mdSTATIC03->Init();
                      if (mdstatic4 != "") DYN->mdSTATIC04->Init();
                      if (mdstatic5 != "") DYN->mdSTATIC05->Init();
                      if (mdstatic6 != "") DYN->mdSTATIC06->Init();
                      if (mdstatic7 != "") DYN->mdSTATIC07->Init();
                      if (mdstatic8 != "") DYN->mdSTATIC08->Init();
                      if (mdstatic9 != "") DYN->mdSTATIC09->Init();
                      if (mdstatic0 != "") DYN->mdSTATIC10->Init();

                     }

}



//---------------------------------------------------------------------------
#pragma package(smart_init)




