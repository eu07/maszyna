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

#include    "system.hpp"
#include    "classes.hpp"
#pragma hdrstop

#include "Train.h"
#include "MdlMngr.h"
#include "Globals.h"
#include "Timer.h"

using namespace Timer;

__fastcall TCab::TCab()
{
  CabPos1.x=-1.0; CabPos1.y=1.0; CabPos1.z=1.0;
  CabPos2.x=1.0; CabPos2.y=1.0; CabPos2.z=-1.0;
  bEnabled=false;
  bOccupied=true;
  dimm_r=dimm_g=dimm_b= 1;
  intlit_r=intlit_g=intlit_b= 0;
  intlitlow_r=intlitlow_g=intlitlow_b= 0;
}

__fastcall TCab::Init(double Initx1,double Inity1,double Initz1,double Initx2,double Inity2,double Initz2,bool InitEnabled,bool InitOccupied)
{
  CabPos1.x=Initx1; CabPos1.y=Inity1; CabPos1.z=Initz1;
  CabPos2.x=Initx2; CabPos2.y=Inity2; CabPos2.z=Initz2;
  bEnabled=InitEnabled;
  bOccupied=InitOccupied;
}

__fastcall TCab::Load(TQueryParserComp *Parser)
{
  AnsiString str=Parser->GetNextSymbol().LowerCase();
  if (str==AnsiString("cablight"))
   {
     dimm_r=Parser->GetNextSymbol().ToDouble();
     dimm_g=Parser->GetNextSymbol().ToDouble();
     dimm_b=Parser->GetNextSymbol().ToDouble();
     intlit_r=Parser->GetNextSymbol().ToDouble();
     intlit_g=Parser->GetNextSymbol().ToDouble();
     intlit_b=Parser->GetNextSymbol().ToDouble();
     intlitlow_r=Parser->GetNextSymbol().ToDouble();
     intlitlow_g=Parser->GetNextSymbol().ToDouble();
     intlitlow_b=Parser->GetNextSymbol().ToDouble();
     str=Parser->GetNextSymbol().LowerCase();
   }
  CabPos1.x=str.ToDouble();
  CabPos1.y=Parser->GetNextSymbol().ToDouble();
  CabPos1.z=Parser->GetNextSymbol().ToDouble();
  CabPos2.x=Parser->GetNextSymbol().ToDouble();
  CabPos2.y=Parser->GetNextSymbol().ToDouble();
  CabPos2.z=Parser->GetNextSymbol().ToDouble();

  bEnabled=True;
  bOccupied=True;
}

__fastcall TCab::~TCab()
{
}


__fastcall TTrain::TTrain()
{
    ActiveUniversal4=false;
    ShowNextCurrent=false;
//McZapkie-240302 - przyda sie do tachometru
    fTachoVelocity= 0;
    fTachoCount= 0;
    fPPress=fNPress= 0;
    asMessage= "";
    fMechCroach=0.25;
    pMechShake= vector3(0,0,0);
    vMechMovement= vector3(0,0,0);
    pMechOffset= vector3(0,0,0);
    fBlinkTimer=0;
    fHaslerTimer=0;
    keybrakecount= 0;
    DynamicObject= NULL;
    iCabLightFlag= 0;
    pMechSittingPosition=vector3(0,0,0); //ABu: 180404
    LampkaUniversal3_st=false; //ABu: 030405
}

__fastcall TTrain::~TTrain()
{
}

bool __fastcall TTrain::Init(TDynamicObject *NewDynamicObject)
{

    DynamicObject= NewDynamicObject;

    if (DynamicObject->Mechanik==NULL)
     return false;
    if (DynamicObject->Mechanik->AIControllFlag==AIdriver)
      return false;

    DynamicObject->MechInside= true;

/*    iPozSzereg= 28;
    for (int i=1; i<DynamicObject->MoverParameters->MainCtrlPosNo; i++)
     {
      if (DynamicObject->MoverParameters->RList[i].Bn>1)
       {
        iPozSzereg= i-1;
        i= DynamicObject->MoverParameters->MainCtrlPosNo+1;
       }
     }
*/
    MechSpring.Init(0,500);
    vMechVelocity= vector3(0,0,0);
    pMechOffset= vector3(-0.4,3.3,5.5);
    fMechCroach=0.5;
    fMechSpringX= 1;
    fMechSpringY= 0.1;
    fMechSpringZ= 0.1;
    fMechMaxSpring= 0.15;
    fMechRoll= 0.05;
    fMechPitch= 0.1;

    if (!LoadMMediaFile(DynamicObject->asBaseDir+DynamicObject->MoverParameters->TypeName+".mmd"))
       { return false; }

//McZapkie: w razie wykolejenia
//    dsbDerailment= TSoundsManager::GetFromName("derail.wav");
//McZapkie: jazda luzem:
//    dsbRunningNoise= TSoundsManager::GetFromName("runningnoise.wav");

// McZapkie? - dzwieki slyszalne tylko wewnatrz kabiny - generowane przez obiekt sterowany:


//McZapkie-080302    sWentylatory.Init("wenton.wav","went.wav","wentoff.wav");
//McZapkie-010302
//    sCompressor.Init("compressor-start.wav","compressor.wav","compressor-stop.wav");

//    sHorn1.Init("horn1.wav",0.3);
//    sHorn2.Init("horn2.wav",0.3);

//  sHorn1.Init("horn1-start.wav","horn1.wav","horn1-stop.wav");
//  sHorn2.Init("horn2-start.wav","horn2.wav","horn2-stop.wav");

//  sConverter.Init("converter.wav",1.5); //NBMX obsluga przez AdvSound
  iCabn=0;
  return true;
}


void __fastcall TTrain::OnKeyPress(int cKey)
{

  bool isEztOer;
  isEztOer=((DynamicObject->MoverParameters->TrainType=="ezt")&&(DynamicObject->MoverParameters->Mains)&&(DynamicObject->MoverParameters->BrakeSubsystem==ss_ESt)&&(DynamicObject->MoverParameters->ActiveDir!=0));

  if (GetAsyncKeyState(VK_SHIFT)<0)
   {
      if (cKey==Global::Keys[k_IncMainCtrlFAST])   //McZapkie-200702: szybkie przelaczanie na poz. bezoporowa
      {
         if (DynamicObject->MoverParameters->IncMainCtrl(2))
          {
              dsbNastawnik->SetCurrentPosition(0);
              dsbNastawnik->Play( 0, 0, 0 );
          }
      }
      else
      if (cKey==Global::Keys[k_DirectionBackward])
      {
      if (DynamicObject->MoverParameters->Radio==false)
          {
            dsbSwitch->SetVolume(DSBVOLUME_MAX);
            dsbSwitch->Play( 0, 0, 0 );
            DynamicObject->MoverParameters->Radio=true;
          }
      }
      else

      if (cKey==Global::Keys[k_DecMainCtrlFAST])
          if (DynamicObject->MoverParameters->DecMainCtrl(2))
          {
              dsbNastawnik->SetCurrentPosition(0);
              dsbNastawnik->Play( 0, 0, 0 );
          }
       else;
      else
      if (cKey==Global::Keys[k_IncScndCtrlFAST])
          if (DynamicObject->MoverParameters->IncScndCtrl(2))
          {
              dsbNastawnik->SetCurrentPosition(0);
              dsbNastawnik->Play( 0, 0, 0 );
          }
       else;
      else
      if (cKey==Global::Keys[k_DecScndCtrlFAST])
          if (DynamicObject->MoverParameters->DecScndCtrl(2))
          {
              dsbNastawnik->SetCurrentPosition(0);
              dsbNastawnik->Play( 0, 0, 0 );
          }
       else;
      else
      if (cKey==Global::Keys[k_IncLocalBrakeLevelFAST])
          if (DynamicObject->MoverParameters->IncLocalBrakeLevel(2));
          else;
      else
      if (cKey==Global::Keys[k_DecLocalBrakeLevelFAST])
          if (DynamicObject->MoverParameters->DecLocalBrakeLevel(2));
          else;
  // McZapkie-240302 - wlaczanie glownego obwodu klawiszem M+shift
      if (cKey==Global::Keys[k_Main])
      {
         MainOnButtonGauge.PutValue(1);
         if (DynamicObject->MoverParameters->MainSwitch(true))
           {
              if (DynamicObject->MoverParameters->EngineType==DieselEngine)
               dsbDieselIgnition->Play( 0, 0, 0 );
              else

               dsbNastawnik->Play( 0, 0, 0 );
               

           }
      }
      else
      if (cKey==Global::Keys[k_Battery])
      {
         if(((DynamicObject->MoverParameters->TrainType=="ezt") || (DynamicObject->MoverParameters->EngineType==ElectricSeriesMotor)|| (DynamicObject->MoverParameters->EngineType==DieselElectric))&&(DynamicObject->MoverParameters->Battery==false))
         {
         if ((DynamicObject->MoverParameters->BatterySwitch(true)))
           {
               dsbSwitch->Play( 0, 0, 0 );
               SetFlag(DynamicObject->MoverParameters->SecuritySystem.Status,s_active);
               SetFlag(DynamicObject->MoverParameters->SecuritySystem.Status,s_alarm) ;

           }
           }
      }
      else
      if (cKey==Global::Keys[k_StLinOff])
      {
         if(DynamicObject->MoverParameters->TrainType=="ezt")
         {
         if ((DynamicObject->MoverParameters->Signalling==false))
           {
               dsbSwitch->Play( 0, 0, 0 );
               DynamicObject->MoverParameters->Signalling=true;
           }
           }
      }
      else
      if (cKey==Global::Keys[k_Sand])
      {
         if(DynamicObject->MoverParameters->TrainType=="ezt")
         {
         if ((DynamicObject->MoverParameters->DoorSignalling==false))
           {
               dsbSwitch->Play( 0, 0, 0 );
               DynamicObject->MoverParameters->DoorSignalling=true;
           }
           }
      }
      else
      if (cKey==Global::Keys[k_BrakeProfile])   //McZapkie-240302-B: przelacznik opoznienia hamowania
      {//yB://ABu: male poprawki, zeby bylo mozna ustawic dowolny wagon
              int CouplNr=-2;
              if (!FreeFlyModeFlag)
              {
                  if(DynamicObject->MoverParameters->BrakeDelaySwitch(0))
                   {
                       dsbPneumaticRelay->SetVolume(DSBVOLUME_MAX);
                       dsbPneumaticRelay->Play( 0, 0, 0 );
                   }
              }
              else
              {
                 TDynamicObject *temp;
                 temp=(DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(),-1, 500, CouplNr));
                 if (temp==NULL)
                 {
                    CouplNr=-2;
                    temp=(DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(),1, 500, CouplNr));
                 }
                 if (temp!=NULL)
                 {
                    if (temp->MoverParameters->BrakeDelaySwitch(0))
                     {
                       dsbPneumaticRelay->SetVolume(DSBVOLUME_MAX);
                       dsbPneumaticRelay->Play( 0, 0, 0 );
                     }
                 }
              }
      }
      else
      if (cKey==Global::Keys[k_Converter])   //NBMX 14-09-2003: przetwornica wl
      {
        if ((DynamicObject->MoverParameters->PantFrontVolt) || (DynamicObject->MoverParameters->PantRearVolt) || (DynamicObject->MoverParameters->EnginePowerSource.SourceType!=CurrentCollector) || (!Global::bLiveTraction) || (DynamicObject->MoverParameters->EngineType!=DieselElectric))
           if(DynamicObject->MoverParameters->ConverterSwitch(true))
           {
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play( 0, 0, 0 );
           }
      }
      else
      if (cKey==Global::Keys[k_Compressor])   //NBMX 14-09-2003: sprezarka wl
      {
        if ((DynamicObject->MoverParameters->ConverterFlag) || (DynamicObject->MoverParameters->CompressorPower<2))
           if(DynamicObject->MoverParameters->CompressorSwitch(true))
           {
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play( 0, 0, 0 );
           }
      }
      else

      
  //McZapkie-240302 - wlaczanie automatycznego pilota (zadziala tylko w trybie debugmode)
      if (cKey==VkKeyScan('q'))
      {
        DynamicObject->Mechanik->Ready=false;
        DynamicObject->Mechanik->AIControllFlag= AIdriver;
        DynamicObject->Controller= AIdriver;
        DynamicObject->Mechanik->ChangeOrder(Prepare_engine);
        DynamicObject->Mechanik->JumpToNextOrder();
        DynamicObject->Mechanik->ChangeOrder(Obey_train);
        DynamicObject->Mechanik->JumpToFirstOrder();
  // czy dac ponizsze? to problematyczne
        DynamicObject->Mechanik->SetVelocity(DynamicObject->GetVelocity(),-1);
      }
      else
      if (cKey==Global::Keys[k_MaxCurrent])   //McZapkie-160502: F - wysoki rozruch
      {
           if ((DynamicObject->MoverParameters->EngineType==DieselElectric) && (DynamicObject->MoverParameters->ShuntModeAllow) && (DynamicObject->MoverParameters->MainCtrlPos==0))
           {
               DynamicObject->MoverParameters->ShuntMode=true;
           }

           if ((DynamicObject->MoverParameters->EngineType==DieselElectric) && (DynamicObject->MoverParameters->TrainType=="sp32") && (DynamicObject->MoverParameters->MainCtrlPos==0))
               DynamicObject->MoverParameters->ScndS=true;

           if(DynamicObject->MoverParameters->MaxCurrentSwitch(true))
           {
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play( 0, 0, 0 );
           }

           if(DynamicObject->MoverParameters->TrainType!="ezt")
             if(DynamicObject->MoverParameters->MinCurrentSwitch(true))
             {
                 dsbSwitch->SetVolume(DSBVOLUME_MAX);
                 dsbSwitch->Play( 0, 0, 0 );
             }
      }
      else
      if (cKey==Global::Keys[k_CurrentAutoRelay])  //McZapkie-241002: G - wlaczanie PSR
      {
           if(DynamicObject->MoverParameters->AutoRelaySwitch(true))
           {
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play( 0, 0, 0 );
           }
      }
      else
      if (cKey==Global::Keys[k_FailedEngineCutOff])  //McZapkie-060103: E - wylaczanie sekcji silnikow
      {
           if(DynamicObject->MoverParameters->CutOffEngine())
           {
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play( 0, 0, 0 );
           }
      }
      else
      if (cKey==Global::Keys[k_OpenLeft])   //NBMX 17-09-2003: otwieranie drzwi
      {
      if (DynamicObject->MoverParameters->ActiveCab==1)
        {
         if (DynamicObject->MoverParameters->DoorOpenCtrl==1)
           if(DynamicObject->MoverParameters->DoorLeft(true))
             {
                 dsbSwitch->SetVolume(DSBVOLUME_MAX);
                 dsbSwitch->Play( 0, 0, 0 );
                 dsbDoorOpen->SetCurrentPosition(0);
                 dsbDoorOpen->Play( 0, 0, 0 );
             }
        }
      if (DynamicObject->MoverParameters->ActiveCab<1)
        {
         if (DynamicObject->MoverParameters->DoorCloseCtrl==1)
           if(DynamicObject->MoverParameters->DoorRight(true))
           {
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play( 0, 0, 0 );
               dsbDoorOpen->SetCurrentPosition(0);
               dsbDoorOpen->Play( 0, 0, 0 );
           }
      }
      }
      else
      if (cKey==Global::Keys[k_OpenRight])   //NBMX 17-09-2003: otwieranie drzwi
      {
      if (DynamicObject->MoverParameters->ActiveCab==1)
      {
         if (DynamicObject->MoverParameters->DoorCloseCtrl==1)
           if(DynamicObject->MoverParameters->DoorRight(true))
           {
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play( 0, 0, 0 );
               dsbDoorOpen->SetCurrentPosition(0);
               dsbDoorOpen->Play( 0, 0, 0 );
           }
      }
      if (DynamicObject->MoverParameters->ActiveCab<1)
      {
         if (DynamicObject->MoverParameters->DoorOpenCtrl==1)
           if(DynamicObject->MoverParameters->DoorLeft(true))
             {
                 dsbSwitch->SetVolume(DSBVOLUME_MAX);
                 dsbSwitch->Play( 0, 0, 0 );
                 dsbDoorOpen->SetCurrentPosition(0);
                 dsbDoorOpen->Play( 0, 0, 0 );
             }
        }
      }
      else
      if (cKey==Global::Keys[k_PantFrontUp])   //Winger 160204: podn. przedn. pantografu
      {
      if ((DynamicObject->MoverParameters->ActiveCab==1) || ((DynamicObject->MoverParameters->ActiveCab<1)&&(DynamicObject->MoverParameters->TrainType!="et40")&&(DynamicObject->MoverParameters->TrainType!="et41")&&(DynamicObject->MoverParameters->TrainType!="et42")&&(DynamicObject->MoverParameters->TrainType!="ezt")))
         {
           DynamicObject->MoverParameters->PantFrontSP=false;
           if(DynamicObject->MoverParameters->PantFront(true))
            if (DynamicObject->MoverParameters->PantFrontStart!=1)
            {
                dsbSwitch->SetVolume(DSBVOLUME_MAX);
                dsbSwitch->Play( 0, 0, 0 );
            }
      }
      if ((DynamicObject->MoverParameters->ActiveCab<1)&&((DynamicObject->MoverParameters->TrainType=="et40")||(DynamicObject->MoverParameters->TrainType=="et41")||(DynamicObject->MoverParameters->TrainType=="et42")||(DynamicObject->MoverParameters->TrainType=="ezt")))
      {
           DynamicObject->MoverParameters->PantRearSP=false;
           if(DynamicObject->MoverParameters->PantRear(true))
            if(DynamicObject->MoverParameters->PantRearStart!=1)
            {
                dsbSwitch->SetVolume(DSBVOLUME_MAX);
                dsbSwitch->Play( 0, 0, 0 );
            }
      }
      }
      else
      if (cKey==Global::Keys[k_PantRearUp])   //Winger 160204: podn. tyln. pantografu
      {
      if ((DynamicObject->MoverParameters->ActiveCab==1) || ((DynamicObject->MoverParameters->ActiveCab<1)&&(DynamicObject->MoverParameters->TrainType!="et40")&&(DynamicObject->MoverParameters->TrainType!="et41")&&(DynamicObject->MoverParameters->TrainType!="et42")&&(DynamicObject->MoverParameters->TrainType!="ezt")))
      {
           DynamicObject->MoverParameters->PantRearSP=false;
           if(DynamicObject->MoverParameters->PantRear(true))
            if(DynamicObject->MoverParameters->PantRearStart!=1)
            {
                dsbSwitch->SetVolume(DSBVOLUME_MAX);
                dsbSwitch->Play( 0, 0, 0 );
            }
      }
      if ((DynamicObject->MoverParameters->ActiveCab<1)&&((DynamicObject->MoverParameters->TrainType=="et40")||(DynamicObject->MoverParameters->TrainType=="et41")||(DynamicObject->MoverParameters->TrainType=="et42")||(DynamicObject->MoverParameters->TrainType=="ezt")))
      {
           DynamicObject->MoverParameters->PantFrontSP=false;
           if(DynamicObject->MoverParameters->PantFront(true))
            if (DynamicObject->MoverParameters->PantFrontStart!=1)
            {
                dsbSwitch->SetVolume(DSBVOLUME_MAX);
                dsbSwitch->Play( 0, 0, 0 );
            }
      }
      }

      else
      if (cKey==Global::Keys[k_Heating])   //Winger 020304: ogrzewanie skladu - wlaczenie
      {
              if (!FreeFlyModeFlag)
              {
                   if ((DynamicObject->MoverParameters->Heating==false)&&((DynamicObject->MoverParameters->EngineType==ElectricSeriesMotor)||(DynamicObject->MoverParameters->ConverterFlag)))
           {
           DynamicObject->MoverParameters->Heating=true;
           dsbSwitch->SetVolume(DSBVOLUME_MAX);
           dsbSwitch->Play( 0, 0, 0 );
           }
      }
      else
              {
                 int CouplNr=-2;              
                 TDynamicObject *temp;
                 temp=(DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(),-1, 500, CouplNr));
                 if (temp==NULL)
                 {
                    CouplNr=-2;
                    temp=(DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(),1, 500, CouplNr));
                 }
                 if (temp!=NULL)
                 {
                    if (temp->MoverParameters->IncBrakeMult())
                     {
                       dsbSwitch->SetVolume(DSBVOLUME_MAX);
                       dsbSwitch->Play( 0, 0, 0 );
                     }
                 }
              }
      }
      else
      //ABu 060205: dzielo Wingera po malutkim liftingu:
      if (cKey==Global::Keys[k_LeftSign])   //lewe swiatlo - wlaczenie
      {
         if ((1+DynamicObject->MoverParameters->ActiveCab)==0)
         { //kabina 0
            if(((DynamicObject->MoverParameters->HeadSignalsFlag)&3)==0)
            {
               DynamicObject->MoverParameters->HeadSignalsFlag|=1;
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play( 0, 0, 0 );
               LeftLightButtonGauge.PutValue(1);
            }
            if(((DynamicObject->MoverParameters->HeadSignalsFlag)&3)==2)
            {
               DynamicObject->MoverParameters->HeadSignalsFlag&=(255-2);
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play( 0, 0, 0 );
               if(LeftEndLightButtonGauge.SubModel!=NULL)
               {
                  LeftEndLightButtonGauge.PutValue(0);
                  LeftLightButtonGauge.PutValue(0);
               }
               else
                  LeftLightButtonGauge.PutValue(0);
            }
         }
         else
         { //kabina 1
            if(((DynamicObject->MoverParameters->EndSignalsFlag)&3)==0)
            {
               DynamicObject->MoverParameters->EndSignalsFlag|=1;
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play( 0, 0, 0 );
               LeftLightButtonGauge.PutValue(1);
            }
            if(((DynamicObject->MoverParameters->EndSignalsFlag)&3)==2)
            {
               DynamicObject->MoverParameters->EndSignalsFlag&=(255-2);
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play( 0, 0, 0 );
               if(LeftEndLightButtonGauge.SubModel!=NULL)
               {
                  LeftEndLightButtonGauge.PutValue(0);
                  LeftLightButtonGauge.PutValue(0);
               }
               else
                  LeftLightButtonGauge.PutValue(0);
            }
         }
      }
      else
      if (cKey==Global::Keys[k_UpperSign])   //ABu 060205: swiatlo gorne - wlaczenie
      {
         if ((1+DynamicObject->MoverParameters->ActiveCab)==0)
         { //kabina 0
            if(((DynamicObject->MoverParameters->HeadSignalsFlag)&12)==0)
            {
               DynamicObject->MoverParameters->HeadSignalsFlag|=4;
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play( 0, 0, 0 );
               UpperLightButtonGauge.PutValue(1);
            }
         }
         else
         { //kabina 1
            if(((DynamicObject->MoverParameters->EndSignalsFlag)&12)==0)
            {
               DynamicObject->MoverParameters->EndSignalsFlag|=4;
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play( 0, 0, 0 );
               UpperLightButtonGauge.PutValue(1);
            }
         }
      }
      else
      if (cKey==Global::Keys[k_RightSign])   //Winger 070304: swiatla tylne (koncowki) - wlaczenie
      {
         if ((1+DynamicObject->MoverParameters->ActiveCab)==0)
         { //kabina 0
            if(((DynamicObject->MoverParameters->HeadSignalsFlag)&48)==0)
            {
               DynamicObject->MoverParameters->HeadSignalsFlag|=16;
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play( 0, 0, 0 );
               RightLightButtonGauge.PutValue(1);
            }
            if(((DynamicObject->MoverParameters->HeadSignalsFlag)&48)==32)
            {
               DynamicObject->MoverParameters->HeadSignalsFlag&=(255-32);
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play( 0, 0, 0 );
               if(RightEndLightButtonGauge.SubModel!=NULL)
               {
                  RightEndLightButtonGauge.PutValue(0);
                  RightLightButtonGauge.PutValue(0);
               }
               else
                  RightLightButtonGauge.PutValue(0);
            }
         }
         else
         { //kabina 1
            if(((DynamicObject->MoverParameters->EndSignalsFlag)&48)==0)
            {
               DynamicObject->MoverParameters->EndSignalsFlag|=16;
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play( 0, 0, 0 );
               RightLightButtonGauge.PutValue(1);
            }
            if(((DynamicObject->MoverParameters->EndSignalsFlag)&48)==32)
            {
               DynamicObject->MoverParameters->EndSignalsFlag&=(255-32);
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play( 0, 0, 0 );
               if(RightEndLightButtonGauge.SubModel!=NULL)
               {
                  RightEndLightButtonGauge.PutValue(0);
                  RightLightButtonGauge.PutValue(0);
               }
               else
                  RightLightButtonGauge.PutValue(0);
            }
         }
      }
      else
      if (cKey==Global::Keys[k_SmallCompressor])   //yB: pompa paliwa
      {
         if(DynamicObject->MoverParameters->EngineType==DieselElectric)
            DynamicObject->MoverParameters->diel_fuel= true;
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play( 0, 0, 0 );
      }
      else
      if (cKey==Global::Keys[k_EndSign])
      {
         int CouplNr=-1;
         TDynamicObject *tmp=NULL;
         tmp=DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(), 1, 500, CouplNr);
         if (tmp==NULL)
            tmp=DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(),-1, 500, CouplNr);
         if ((tmp!=NULL)&&(CouplNr!=-1))
         {
            if(CouplNr==0)
            {
               if(((tmp->MoverParameters->HeadSignalsFlag)&64)==0)
               {
                  tmp->MoverParameters->HeadSignalsFlag|=64;
                  dsbSwitch->SetVolume(DSBVOLUME_MAX);
                  dsbSwitch->Play( 0, 0, 0 );
               }
            }
            else
            {
               if(((tmp->MoverParameters->EndSignalsFlag)&64)==0)
               {
                  tmp->MoverParameters->EndSignalsFlag|=64;
                  dsbSwitch->SetVolume(DSBVOLUME_MAX);
                  dsbSwitch->Play( 0, 0, 0 );
               }
            }
         }
      }
   }
  else                   //McZapkie-240302 - klawisze bez shifta
   {
      if (cKey==Global::Keys[k_IncMainCtrl])
      {
          if (DynamicObject->MoverParameters->IncMainCtrl(1))
          {
              dsbNastawnik->SetCurrentPosition(0);
              dsbNastawnik->Play( 0, 0, 0 );
          }
         else;
      }
      else
      if (cKey==Global::Keys[k_FreeFlyMode])
      {
         if (FreeFlyModeFlag==false)
          FreeFlyModeFlag=true;
         else
          FreeFlyModeFlag=false;
         DynamicObject->ABuSetModelShake(vector3(0,0,0));
      }
      else
      if (cKey==Global::Keys[k_DecMainCtrl])
          if (DynamicObject->MoverParameters->DecMainCtrl(1))
          {
              dsbNastawnik->SetCurrentPosition(0);
              dsbNastawnik->Play( 0, 0, 0 );
          }
         else;
      else
      if (cKey==Global::Keys[k_IncScndCtrl])
//        if (MoverParameters->ScndCtrlPos<MoverParameters->ScndCtrlPosNo)
//         if(DynamicObject->MoverParameters->EnginePowerSource.SourceType==CurrentCollector)
          if (DynamicObject->MoverParameters->ShuntMode)
          {
            DynamicObject->MoverParameters->AnPos += (GetDeltaTime()/0.85f);
            if (DynamicObject->MoverParameters->AnPos > 1)
              DynamicObject->MoverParameters->AnPos = 1;
          }
          else
          if (DynamicObject->MoverParameters->IncScndCtrl(1))
          {
              dsbNastawnik->SetCurrentPosition(0);
              dsbNastawnik->Play( 0, 0, 0 );
          }
         else;
      else
      if (cKey==Global::Keys[k_DecScndCtrl])
  //       if(DynamicObject->MoverParameters->EnginePowerSource.SourceType==CurrentCollector)
          if (DynamicObject->MoverParameters->ShuntMode)
          {
            DynamicObject->MoverParameters->AnPos -= (GetDeltaTime()/0.55f);
            if (DynamicObject->MoverParameters->AnPos < 0)
              DynamicObject->MoverParameters->AnPos = 0;
          }
          else

          if (DynamicObject->MoverParameters->DecScndCtrl(1))
  //        if (MoverParameters->ScndCtrlPos>0)
          {
              dsbNastawnik->SetCurrentPosition(0);
              dsbNastawnik->Play( 0, 0, 0 );
          }
         else;
      else
      if (cKey==Global::Keys[k_IncLocalBrakeLevel])
          {//ABu: male poprawki, zeby bylo mozna przyhamowac dowolny wagon
              int CouplNr=-2;
              if (!FreeFlyModeFlag)
              {
              if (GetAsyncKeyState(VK_CONTROL)<0)
                if ((DynamicObject->MoverParameters->LocalBrake==ManualBrake)||(DynamicObject->MoverParameters->MBrake==true))
                {
                DynamicObject->MoverParameters->IncManualBrakeLevel(1);
                }
                else;
              else
              if (DynamicObject->MoverParameters->LocalBrake!=ManualBrake)
                {
                DynamicObject->MoverParameters->IncLocalBrakeLevel(1);
                }
              }
              else
              {
                 TDynamicObject *temp;
                 CouplNr=-2;
                 temp=(DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(),-1, 500, CouplNr));
                 if (temp==NULL)
                 {
                    CouplNr=-2;
                    temp=(DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(),1, 500, CouplNr));
                 }
                 if (temp!=NULL)
                 {
                 if (GetAsyncKeyState(VK_CONTROL)<0)
                   if ((temp->MoverParameters->LocalBrake==ManualBrake)||(temp->MoverParameters->MBrake==true))
                   {
                   temp->MoverParameters->IncManualBrakeLevel(1);
                   }
                   else;
                  else
                  if (temp->MoverParameters->LocalBrake!=ManualBrake)
                   {
                    if (temp->MoverParameters->IncLocalBrakeLevelFAST())
                    {
                       dsbPneumaticRelay->SetVolume(-80);
                       dsbPneumaticRelay->Play( 0, 0, 0 );
                    }
                   }
                   }
                   }
                   }
      else
      if (cKey==Global::Keys[k_DecLocalBrakeLevel])
          {//ABu: male poprawki, zeby bylo mozna odhamowac dowolny wagon
              int CouplNr=-2;
              if (!FreeFlyModeFlag)
              {
                if (GetAsyncKeyState(VK_CONTROL)<0)
                if ((DynamicObject->MoverParameters->LocalBrake==ManualBrake)||(DynamicObject->MoverParameters->MBrake==true))

                  {DynamicObject->MoverParameters->DecManualBrakeLevel(1); }
                 else;
                else
                 if (DynamicObject->MoverParameters->LocalBrake!=ManualBrake)
                  {DynamicObject->MoverParameters->DecLocalBrakeLevel(1);}
              }
              else
              {
                 TDynamicObject *temp;
                 temp=(DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(),-1, 500, CouplNr));
                 if (temp==NULL)
                 {
                    CouplNr=-2;
                    temp=(DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(),1, 500, CouplNr));
                 }
                 if (temp!=NULL)
                 {
                  if (GetAsyncKeyState(VK_CONTROL)<0)
                    if ((temp->MoverParameters->LocalBrake==ManualBrake)||(temp->MoverParameters->MBrake==true))
                 {temp->MoverParameters->DecManualBrakeLevel(1);}
                    else;
                else


                 if (temp->MoverParameters->LocalBrake!=ManualBrake)
                 {
                    if (temp->MoverParameters->DecLocalBrakeLevelFAST())
                    {
                       dsbPneumaticRelay->SetVolume(-80);
                       dsbPneumaticRelay->Play( 0, 0, 0 );
                    }
                 }


                 }
              }
          }
      else
      if (cKey==Global::Keys[k_IncBrakeLevel])
          if (DynamicObject->MoverParameters->IncBrakeLevel())
          {
           keybrakecount= 0;
           if ((isEztOer) && (DynamicObject->MoverParameters->BrakeCtrlPos<3))
            {
             dsbPneumaticSwitch->SetVolume(-10);
             dsbPneumaticSwitch->Play( 0, 0, 0 );
            }
           }
          else;
      else
      if (cKey==Global::Keys[k_DecBrakeLevel])
       {
         if ((DynamicObject->MoverParameters->BrakeCtrlPos>-1)|| (keybrakecount>1))
          {
            if (DynamicObject->MoverParameters->DecBrakeLevel());
              {
              if ((isEztOer) && (DynamicObject->MoverParameters->BrakeCtrlPos<2)&&(keybrakecount<=1))
              {
               dsbPneumaticSwitch->SetVolume(-10);
               dsbPneumaticSwitch->Play( 0, 0, 0 );
              }
             }
          }
           else keybrakecount+=1;
       }
      else
      if (cKey==Global::Keys[k_EmergencyBrake])
      {
          while (DynamicObject->MoverParameters->IncBrakeLevel());
          if(isEztOer)
          DynamicObject->MoverParameters->DecBrakeLevel();
          DynamicObject->MoverParameters->EmergencyBrakeFlag=!DynamicObject->MoverParameters->EmergencyBrakeFlag;
      }
      else
      if (cKey==Global::Keys[k_Brake3])
      {
          if ((isEztOer) && ((DynamicObject->MoverParameters->BrakeCtrlPos==1)||(DynamicObject->MoverParameters->BrakeCtrlPos==-1)))
            {
             dsbPneumaticSwitch->SetVolume(-10);
             dsbPneumaticSwitch->Play( 0, 0, 0 );
            }
          while (DynamicObject->MoverParameters->BrakeCtrlPos>DynamicObject->MoverParameters->BrakeCtrlPosNo-1-(DynamicObject->MoverParameters->BrakeHandle==FV4a?1:0) && DynamicObject->MoverParameters->DecBrakeLevel());
          while (DynamicObject->MoverParameters->BrakeCtrlPos<DynamicObject->MoverParameters->BrakeCtrlPosNo-1-(DynamicObject->MoverParameters->BrakeHandle==FV4a?1:0) && DynamicObject->MoverParameters->IncBrakeLevel());
      }
      else
      if (cKey==Global::Keys[k_Brake2])
      {
          if ((isEztOer) && ((DynamicObject->MoverParameters->BrakeCtrlPos==1)||(DynamicObject->MoverParameters->BrakeCtrlPos==-1)))
            {
             dsbPneumaticSwitch->SetVolume(-10);
             dsbPneumaticSwitch->Play( 0, 0, 0 );
            }
          while (DynamicObject->MoverParameters->BrakeCtrlPos>DynamicObject->MoverParameters->BrakeCtrlPosNo/2-(DynamicObject->MoverParameters->BrakeHandle==FV4a?1:0) && DynamicObject->MoverParameters->DecBrakeLevel());
          while (DynamicObject->MoverParameters->BrakeCtrlPos<DynamicObject->MoverParameters->BrakeCtrlPosNo/2-(DynamicObject->MoverParameters->BrakeHandle==FV4a?1:0) && DynamicObject->MoverParameters->IncBrakeLevel());

          if (GetAsyncKeyState(VK_CONTROL)<0)
            if ((DynamicObject->MoverParameters->BrakeSubsystem==ss_LSt)&&(DynamicObject->MoverParameters->BrakeSystem==Pneumatic))
              DynamicObject->MoverParameters->BrakeCtrlPos=-2;
      }
      else
      if (cKey==Global::Keys[k_Brake1])
      {
          if ((isEztOer)&& (DynamicObject->MoverParameters->BrakeCtrlPos!=1))
            {
             dsbPneumaticSwitch->SetVolume(-10);
             dsbPneumaticSwitch->Play( 0, 0, 0 );
            }
          while (DynamicObject->MoverParameters->BrakeCtrlPos>1 && DynamicObject->MoverParameters->DecBrakeLevel());
          while (DynamicObject->MoverParameters->BrakeCtrlPos<1 && DynamicObject->MoverParameters->IncBrakeLevel());
      }
      else
      if (cKey==Global::Keys[k_Brake0])
      {
          if ((isEztOer) && ((DynamicObject->MoverParameters->BrakeCtrlPos==1)||(DynamicObject->MoverParameters->BrakeCtrlPos==-1)))
            {
             dsbPneumaticSwitch->SetVolume(-10);
             dsbPneumaticSwitch->Play( 0, 0, 0 );
            }
          while (DynamicObject->MoverParameters->BrakeCtrlPos>0 && DynamicObject->MoverParameters->DecBrakeLevel());
          while (DynamicObject->MoverParameters->BrakeCtrlPos<0 && DynamicObject->MoverParameters->IncBrakeLevel());
      }
      else
      if (cKey==Global::Keys[k_WaveBrake])
      {
          if ((isEztOer) && (DynamicObject->MoverParameters->BrakeCtrlPos!=-1))
            {
             dsbPneumaticSwitch->SetVolume(-10);
             dsbPneumaticSwitch->Play( 0, 0, 0 );
            }
          while (DynamicObject->MoverParameters->BrakeCtrlPos>-1 && DynamicObject->MoverParameters->DecBrakeLevel());
          while (DynamicObject->MoverParameters->BrakeCtrlPos<-1 && DynamicObject->MoverParameters->IncBrakeLevel());
      }
      else
      if (cKey==Global::Keys[k_Czuwak])
      {
//          dsbBuzzer->Stop();
          if (DynamicObject->MoverParameters->SecuritySystemReset())
           if (fabs(SecurityResetButtonGauge.GetValue())<0.001)
            {
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play( 0, 0, 0 );
            }
          SecurityResetButtonGauge.PutValue(1);
      }
      else
      if (cKey==Global::Keys[k_AntiSlipping])
      {
        if(DynamicObject->MoverParameters->TrainType!="ezt")
         {
          if (DynamicObject->MoverParameters->AntiSlippingButton())
           if (fabs(AntiSlipButtonGauge.GetValue())<0.01)
            {
              //Dlaczego bylo '-50'???
              //dsbSwitch->SetVolume(-50);
              dsbSwitch->SetVolume(DSBVOLUME_MAX);
              dsbSwitch->Play( 0, 0, 0 );
            }
          AntiSlipButtonGauge.PutValue(1);
         }
      }
      else
      if (cKey==Global::Keys[k_Fuse])
      {
          FuseButtonGauge.PutValue(1);
          DynamicObject->MoverParameters->FuseOn();
      }
      else
  // McZapkie-240302 - zmiana kierunku: 'd' do przodu, 'r' do tylu
      if (cKey==Global::Keys[k_DirectionForward])
      {
        if (DynamicObject->MoverParameters->DirectionForward())
          {
            dsbSwitch->SetVolume(DSBVOLUME_MAX);
            dsbSwitch->Play( 0, 0, 0 );
          }
      }
      else
      if (cKey==Global::Keys[k_DirectionBackward])  //r
      {
        if (GetAsyncKeyState(VK_CONTROL)<0)
          if (DynamicObject->MoverParameters->Radio==true)
          {
            dsbSwitch->SetVolume(DSBVOLUME_MAX);
            dsbSwitch->Play( 0, 0, 0 );
            DynamicObject->MoverParameters->Radio=false;
          }
        else;
        else
        if (DynamicObject->MoverParameters->DirectionBackward())
          {
            dsbSwitch->SetVolume(DSBVOLUME_MAX);
            dsbSwitch->Play( 0, 0, 0 );
          }


      }
      else
  // McZapkie-240302 - wylaczanie glownego obwodu
      if (cKey==Global::Keys[k_Main])
      {
        MainOffButtonGauge.PutValue(1);
        if(DynamicObject->MoverParameters->MainSwitch(false))
           {
              dsbNastawnik->Play( 0, 0, 0 );
           }
      }
      else

     if (cKey==Global::Keys[k_Battery])
      {
        if((DynamicObject->MoverParameters->TrainType=="ezt") || (DynamicObject->MoverParameters->EngineType==ElectricSeriesMotor)|| (DynamicObject->MoverParameters->EngineType==DieselElectric))
        if(DynamicObject->MoverParameters->BatterySwitch(false))
           {
              dsbSwitch->Play( 0, 0, 0 );
              DynamicObject->MoverParameters->SecuritySystem.Status=0;
           }
      }

      if (cKey==Global::Keys[k_BrakeProfile])
      {//yB://ABu: male poprawki, zeby bylo mozna ustawic dowolny wagon
              int CouplNr=-2;
              if (!FreeFlyModeFlag)
              {
                  if (GetAsyncKeyState(VK_CONTROL)<0)
                   if(DynamicObject->MoverParameters->BrakeDelaySwitch(2))
                     {
                       dsbPneumaticRelay->SetVolume(DSBVOLUME_MAX);
                       dsbPneumaticRelay->Play( 0, 0, 0 );
                     }
                   else;
                  else
                   if(DynamicObject->MoverParameters->BrakeDelaySwitch(1))
                     {
                       dsbPneumaticRelay->SetVolume(DSBVOLUME_MAX);
                       dsbPneumaticRelay->Play( 0, 0, 0 );
                     }
              }
              else
              {
                 TDynamicObject *temp;
                 temp=(DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(),-1, 500, CouplNr));
                 if (temp==NULL)
                 {
                    CouplNr=-2;
                    temp=(DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(),1, 500, CouplNr));
                 }
                 if (temp!=NULL)
                 {
                  if (GetAsyncKeyState(VK_CONTROL)<0)
                    if (temp->MoverParameters->BrakeDelaySwitch(2))
                     {
                       dsbPneumaticRelay->SetVolume(DSBVOLUME_MAX);
                       dsbPneumaticRelay->Play( 0, 0, 0 );
                     }
                    else;
                  else
                    if (temp->MoverParameters->BrakeDelaySwitch(1))
                     {
                       dsbPneumaticRelay->SetVolume(DSBVOLUME_MAX);
                       dsbPneumaticRelay->Play( 0, 0, 0 );
                     }
                 }
              }

      }
      else
      if (cKey==Global::Keys[k_Converter])       //NBMX wyl przetwornicy
      {
           if(DynamicObject->MoverParameters->ConverterSwitch(false))
           {
            dsbSwitch->SetVolume(DSBVOLUME_MAX);
            dsbSwitch->Play( 0, 0, 0 );;
           }
      }
      else
      if (cKey==Global::Keys[k_Compressor])       //NBMX wyl sprezarka
      {
           if(DynamicObject->MoverParameters->CompressorSwitch(false))
           {
            dsbSwitch->SetVolume(DSBVOLUME_MAX);
            dsbSwitch->Play( 0, 0, 0 );;
           }
      }
      else
      if (cKey==Global::Keys[k_Releaser])   //odluzniacz
      {
       if (!FreeFlyModeFlag)
        {
         if((DynamicObject->MoverParameters->EngineType==ElectricSeriesMotor)||(DynamicObject->MoverParameters->EngineType==DieselElectric))
          if(DynamicObject->MoverParameters->TrainType!="ezt")
           if (DynamicObject->MoverParameters->BrakeCtrlPosNo>0)
            {
             ReleaserButtonGauge.PutValue(1);
             DynamicObject->MoverParameters->BrakeStatus|=4;
             if (DynamicObject->MoverParameters->BrakeReleaser())
              {
                 dsbPneumaticRelay->SetVolume(-80);
                 dsbPneumaticRelay->Play( 0, 0, 0 );
              }
            }
        }
       else
       {
         int CouplNr=-2;
         TDynamicObject *temp;
         temp=(DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(),-1, 500, CouplNr));
         if (temp==NULL)
          {
            CouplNr=-2;
            temp=(DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(),1, 500, CouplNr));
          }
         if (temp!=NULL)
          {
           if (temp->MoverParameters->BrakeReleaser())
            {
             dsbPneumaticRelay->SetVolume(DSBVOLUME_MAX);
             dsbPneumaticRelay->Play( 0, 0, 0 );
            }
          }
       }
      }
      else
  //McZapkie-240302 - wylaczanie automatycznego pilota (w trybie ~debugmode mozna tylko raz)
      if (cKey==VkKeyScan('q'))
      {
        DynamicObject->Mechanik->AIControllFlag= Humandriver;
        DynamicObject->Controller= Humandriver;
      }
      else
      if (cKey==Global::Keys[k_MaxCurrent])   //McZapkie-160502: f - niski rozruch
      {
           if ((DynamicObject->MoverParameters->EngineType==DieselElectric) && (DynamicObject->MoverParameters->ShuntModeAllow) && (DynamicObject->MoverParameters->MainCtrlPos==0))
           {
               DynamicObject->MoverParameters->ShuntMode=False;
           }

           if ((DynamicObject->MoverParameters->EngineType==DieselElectric) && (DynamicObject->MoverParameters->TrainType=="sp32") && (DynamicObject->MoverParameters->MainCtrlPos==0))
               DynamicObject->MoverParameters->ScndS=false;

           if(DynamicObject->MoverParameters->MaxCurrentSwitch(false))
           {
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play( 0, 0, 0 );

           }
           if(DynamicObject->MoverParameters->TrainType!="ezt")
             if(DynamicObject->MoverParameters->MinCurrentSwitch(false))
             {
                 dsbSwitch->SetVolume(DSBVOLUME_MAX);
                 dsbSwitch->Play( 0, 0, 0 );
             }
      }
      else
      if (cKey==Global::Keys[k_CurrentAutoRelay])  //McZapkie-241002: g - wylaczanie PSR
      {
           if(DynamicObject->MoverParameters->AutoRelaySwitch(false))
           {
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play( 0, 0, 0 );
           }
      }
      else
      if (cKey==Global::Keys[k_Sand])
      {
        if (DynamicObject->MoverParameters->TrainType!="ezt")
        {
          if (DynamicObject->MoverParameters->SandDoseOn())
           if (DynamicObject->MoverParameters->SandDose)
            {
              dsbPneumaticRelay->SetVolume(-30);
              dsbPneumaticRelay->Play( 0, 0, 0 );
            }
        }
        if (DynamicObject->MoverParameters->TrainType=="ezt")
        {
          if(DynamicObject->MoverParameters->DoorSignalling==true)
           {
              dsbSwitch->Play( 0, 0, 0 );
              DynamicObject->MoverParameters->DoorSignalling=false;
           }
        }
      }
      else
      if (cKey==Global::Keys[k_CabForward])
      {
         // DynamicObject->MoverParameters->CabDeactivisation();
          if (!CabChange(1))
           if (TestFlag(DynamicObject->MoverParameters->Couplers[0].CouplingFlag,ctrain_passenger))
           {
//TODO: przejscie do nastepnego pojazdu, wskaznik do niego: DynamicObject->MoverParameters->Couplers[0].Connected
              Global::changeDynObj=true;
           }
         // DynamicObject->MoverParameters->CabActivisation();
      }
      else
      if (cKey==Global::Keys[k_CabBackward])
      {
         // DynamicObject->MoverParameters->CabDeactivisation();
          if (!CabChange(-1))
           if (TestFlag(DynamicObject->MoverParameters->Couplers[1].CouplingFlag,ctrain_passenger))
          {
//TODO: przejscie do poprzedniego, wskaznik do niego: DynamicObject->MoverParameters->Couplers[1].Connected
              Global::changeDynObj=true;
          }
         // DynamicObject->MoverParameters->CabActivisation();
      }
      else
      if (cKey==Global::Keys[k_Couple])
      {//ABu051104: male zmiany, zeby mozna bylo laczyc odlegle wagony
       //da sie zoptymalizowac, ale nie ma na to czasu :(
        if (iCabn>0)
        {
          if (!FreeFlyModeFlag) //tryb 'kabinowy'
          {/*
            if (DynamicObject->MoverParameters->Couplers[iCabn-1].CouplingFlag==0)
            {
              if (DynamicObject->MoverParameters->Attach(iCabn-1,DynamicObject->MoverParameters->Couplers[iCabn-1].Connected,ctrain_coupler))
              {
                dsbCouplerAttach->SetVolume(DSBVOLUME_MAX);
                dsbCouplerAttach->Play( 0, 0, 0 );
                //ABu: aha, a guzik, nie dziala i nie bedzie, a przydalo by sie cos takiego:
                //DynamicObject->NextConnected=DynamicObject->MoverParameters->Couplers[iCabn-1].Connected;
                //DynamicObject->PrevConnected=DynamicObject->MoverParameters->Couplers[iCabn-1].Connected;
              }
            }
            else
            if (!TestFlag(DynamicObject->MoverParameters->Couplers[iCabn-1].CouplingFlag,ctrain_pneumatic))
            {
              //ABu021104: zeby caly czas bylo widac sprzegi:
              if (DynamicObject->MoverParameters->Attach(iCabn-1,DynamicObject->MoverParameters->Couplers[iCabn-1].Connected,DynamicObject->MoverParameters->Couplers[iCabn-1].CouplingFlag+ctrain_pneumatic))
              //if (DynamicObject->MoverParameters->Attach(iCabn-1,DynamicObject->MoverParameters->Couplers[iCabn-1].Connected,ctrain_pneumatic))
              {
                rsHiss.Play(1,DSBPLAY_LOOPING,true,DynamicObject->GetPosition());
              }
            }*/
          }
          else
          { //tryb freefly
            int CouplNr=-1;
            TDynamicObject *tmp=NULL;
            tmp=DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(), 1, 500, CouplNr);
            if (tmp==NULL)
            tmp=DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(),-1, 500, CouplNr);
            if ((tmp!=NULL)&&(CouplNr!=-1))
            {
              if (tmp->MoverParameters->Couplers[CouplNr].CouplingFlag==0)        //hak
              {
                if (tmp->MoverParameters->Attach(CouplNr,2,tmp->MoverParameters->Couplers[CouplNr].Connected,ctrain_coupler))
                {
                  //tmp->MoverParameters->Couplers[CouplNr].Render=true;
                  dsbCouplerAttach->SetVolume(DSBVOLUME_MAX);
                  dsbCouplerAttach->Play( 0, 0, 0 );
                }
              }
              else
              if (!TestFlag(tmp->MoverParameters->Couplers[CouplNr].CouplingFlag,ctrain_pneumatic))    //pneumatyka
              {
                if (tmp->MoverParameters->Attach(CouplNr,2,tmp->MoverParameters->Couplers[CouplNr].Connected,tmp->MoverParameters->Couplers[CouplNr].CouplingFlag+ctrain_pneumatic))
                {
                  rsHiss.Play(1,DSBPLAY_LOOPING,true,tmp->GetPosition());
                  DynamicObject->SetPneumatic(CouplNr,1);
                  tmp->SetPneumatic(CouplNr,1);
                }
              }
              else
              if (!TestFlag(tmp->MoverParameters->Couplers[CouplNr].CouplingFlag,ctrain_scndpneumatic))     //zasilajacy
              {
                if (tmp->MoverParameters->Attach(CouplNr,2,tmp->MoverParameters->Couplers[CouplNr].Connected,tmp->MoverParameters->Couplers[CouplNr].CouplingFlag+ctrain_scndpneumatic))
                {
//                  rsHiss.Play(1,DSBPLAY_LOOPING,true,tmp->GetPosition());
                  dsbCouplerDetach->SetVolume(DSBVOLUME_MAX);
                  dsbCouplerDetach->Play( 0, 0, 0 );
                  DynamicObject->SetPneumatic(CouplNr,0);
                  tmp->SetPneumatic(CouplNr,0);
                }
              }
              else
              if (!TestFlag(tmp->MoverParameters->Couplers[CouplNr].CouplingFlag,ctrain_controll))     //ukrotnionko
              {
                if (tmp->MoverParameters->Attach(CouplNr,2,tmp->MoverParameters->Couplers[CouplNr].Connected,tmp->MoverParameters->Couplers[CouplNr].CouplingFlag+ctrain_controll))
                {
                  dsbCouplerAttach->SetVolume(DSBVOLUME_MAX);
                  dsbCouplerAttach->Play( 0, 0, 0 );
                }
              }
             else
              if (!TestFlag(tmp->MoverParameters->Couplers[CouplNr].CouplingFlag,ctrain_passenger))     //mostek
              {
                if (tmp->MoverParameters->Attach(CouplNr,2,tmp->MoverParameters->Couplers[CouplNr].Connected,tmp->MoverParameters->Couplers[CouplNr].CouplingFlag+ctrain_passenger))
                {
//                  rsHiss.Play(1,DSBPLAY_LOOPING,true,tmp->GetPosition());
                  dsbCouplerDetach->SetVolume(DSBVOLUME_MAX);
                  dsbCouplerDetach->Play( 0, 0, 0 );
                  DynamicObject->SetPneumatic(CouplNr,0);
                  tmp->SetPneumatic(CouplNr,0);
                }
              }
            }
          }
        }
      }
      else
      if (cKey==Global::Keys[k_DeCouple])
      { //ABu051104: male zmiany, zeby mozna bylo rozlaczac odlegle wagony
        if(iCabn>0)
        {
          if(!FreeFlyModeFlag) //tryb 'kabinowy'
          {
            if (DynamicObject->MoverParameters->Couplers[iCabn-1].CouplingFlag>0)
            {
              if (DynamicObject->MoverParameters->Dettach(iCabn-1))
              {
                dsbCouplerDetach->SetVolume(DSBVOLUME_MAX);
                dsbCouplerDetach->Play( 0, 0, 0 );
//              DynamicObject->MoverParameters->Couplers[iCabn-1].Connected*->Dettach(iCabn==1 ? 2 : 1);
              }
            }
          }
          else
          { //tryb freefly
            int CouplNr=-1;
            TDynamicObject *tmp=NULL;
            tmp=DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(), 1, 500, CouplNr);
            if (tmp==NULL)
            tmp=DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(),-1, 500, CouplNr);
            if ((tmp!=NULL)&&(CouplNr!=-1))
            {
              if (tmp->MoverParameters->Couplers[CouplNr].CouplingFlag>0)
              {
                if (tmp->MoverParameters->Dettach(CouplNr))
                {
                  dsbCouplerDetach->SetVolume(DSBVOLUME_MAX);
                  dsbCouplerDetach->Play( 0, 0, 0 );
//                DynamicObject->MoverParameters->Couplers[iCabn-1].Connected*->Dettach(iCabn==1 ? 2 : 1);
                }
              }
            }
          }
        }
      }
      else
      if (cKey==Global::Keys[k_CloseLeft])   //NBMX 17-09-2003: zamykanie drzwi
      {
       if (DynamicObject->MoverParameters->ActiveCab==1)
        {
           if(DynamicObject->MoverParameters->DoorLeft(false))
           {
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play( 0, 0, 0 );
               dsbDoorClose->SetCurrentPosition(0);
               dsbDoorClose->Play( 0, 0, 0 );
           }
       }
       if (DynamicObject->MoverParameters->ActiveCab<1)
       {
           if(DynamicObject->MoverParameters->DoorRight(false))
           {
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play( 0, 0, 0 );
               dsbDoorClose->SetCurrentPosition(0);
               dsbDoorClose->Play( 0, 0, 0 );
           }
        }
      }
      if (cKey==Global::Keys[k_CloseRight])   //NBMX 17-09-2003: zamykanie drzwi
      {
      if (DynamicObject->MoverParameters->ActiveCab==1)
      {
           if(DynamicObject->MoverParameters->DoorRight(false))
           {
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play( 0, 0, 0 );
               dsbDoorClose->SetCurrentPosition(0);
               dsbDoorClose->Play( 0, 0, 0 );
           }
      }
      if (DynamicObject->MoverParameters->ActiveCab<1)
      {
      if(DynamicObject->MoverParameters->DoorLeft(false))
           {
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play( 0, 0, 0 );
               dsbDoorClose->SetCurrentPosition(0);
               dsbDoorClose->Play( 0, 0, 0 );
           }
      }
      }

      else
      if (cKey==Global::Keys[k_PantFrontDown])   //Winger 160204: opuszczanie prz. patyka
      {
      if ((DynamicObject->MoverParameters->ActiveCab==1) || ((DynamicObject->MoverParameters->ActiveCab<1)&&(DynamicObject->MoverParameters->TrainType!="et40")&&(DynamicObject->MoverParameters->TrainType!="et41")&&(DynamicObject->MoverParameters->TrainType!="et42")&&(DynamicObject->MoverParameters->TrainType!="ezt")))
          {
           if (DynamicObject->MoverParameters->PantFront(false))
            {
                dsbSwitch->SetVolume(DSBVOLUME_MAX);
                dsbSwitch->Play( 0, 0, 0 );
            }
      }
      if ((DynamicObject->MoverParameters->ActiveCab<1)&&((DynamicObject->MoverParameters->TrainType=="et40")||(DynamicObject->MoverParameters->TrainType=="et41")||(DynamicObject->MoverParameters->TrainType=="et42")||(DynamicObject->MoverParameters->TrainType=="ezt")))
      {
           if (DynamicObject->MoverParameters->PantRear(false))
            {
                dsbSwitch->SetVolume(DSBVOLUME_MAX);
                dsbSwitch->Play( 0, 0, 0 );
            }
      }
     }

      if (cKey==Global::Keys[k_PantRearDown])   //Winger 160204: opuszczanie tyl. patyka
      {
      if ((DynamicObject->MoverParameters->ActiveCab==1) || ((DynamicObject->MoverParameters->ActiveCab<1)&&(DynamicObject->MoverParameters->TrainType!="et40")&&(DynamicObject->MoverParameters->TrainType!="et41")&&(DynamicObject->MoverParameters->TrainType!="et42")&&(DynamicObject->MoverParameters->TrainType!="ezt")))
         {
           if (DynamicObject->MoverParameters->PantSwitchType=="impulse")
            PantFrontButtonOffGauge.PutValue(1);
            if (DynamicObject->MoverParameters->PantRear(false))
             {
                 dsbSwitch->SetVolume(DSBVOLUME_MAX);
                 dsbSwitch->Play( 0, 0, 0 );
             }
      }
      if ((DynamicObject->MoverParameters->ActiveCab<1)&&((DynamicObject->MoverParameters->TrainType=="et40")||(DynamicObject->MoverParameters->TrainType=="et41")||(DynamicObject->MoverParameters->TrainType=="et42")||(DynamicObject->MoverParameters->TrainType=="ezt")))
      {
          /* if (DynamicObject->MoverParameters->PantSwitchType=="impulse")
            PantRearButtonOffGauge.PutValue(1);  */
            if (DynamicObject->MoverParameters->PantFront(false))
             {
                 dsbSwitch->SetVolume(DSBVOLUME_MAX);
                 dsbSwitch->Play( 0, 0, 0 );
             }
      }
      }
      if (cKey==Global::Keys[k_Heating])   //Winger 020304: ogrzewanie - wylaczenie
      {
              if (!FreeFlyModeFlag)
              {
           if (DynamicObject->MoverParameters->Heating==true)
           {
           dsbSwitch->SetVolume(DSBVOLUME_MAX);
           dsbSwitch->Play( 0, 0, 0 );
           DynamicObject->MoverParameters->Heating=false;
           }
      }
      else
              {
                 int CouplNr=-2;
                 TDynamicObject *temp;
                 temp=(DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(),-1, 500, CouplNr));
                 if (temp==NULL)
                 {
                    CouplNr=-2;
                    temp=(DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(),1, 500, CouplNr));
                 }
                 if (temp!=NULL)
                 {
                    if (temp->MoverParameters->DecBrakeMult())
                     {
                       dsbSwitch->SetVolume(DSBVOLUME_MAX);
                       dsbSwitch->Play( 0, 0, 0 );
                     }
                 }
              }

      }
      else
      if (cKey==Global::Keys[k_LeftSign])   //ABu 060205: lewe swiatlo - wylaczenie
      {
         if ((1+DynamicObject->MoverParameters->ActiveCab)==0)
         { //kabina 0
            if(((DynamicObject->MoverParameters->HeadSignalsFlag)&3)==0)
            {
               DynamicObject->MoverParameters->HeadSignalsFlag|=2;
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play( 0, 0, 0 );
               if(LeftEndLightButtonGauge.SubModel!=NULL)
               {
                  LeftEndLightButtonGauge.PutValue(1);
                  LeftLightButtonGauge.PutValue(0);
               }
               else
                  LeftLightButtonGauge.PutValue(-1);
            }
            if(((DynamicObject->MoverParameters->HeadSignalsFlag)&3)==1)
            {
               DynamicObject->MoverParameters->HeadSignalsFlag&=(255-1);
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play( 0, 0, 0 );
               LeftLightButtonGauge.PutValue(0);
            }
         }
         else
         { //kabina 1
            if(((DynamicObject->MoverParameters->EndSignalsFlag)&3)==0)
            {
               DynamicObject->MoverParameters->EndSignalsFlag|=2;
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play( 0, 0, 0 );
               if(LeftEndLightButtonGauge.SubModel!=NULL)
               {
                  LeftEndLightButtonGauge.PutValue(1);
                  LeftLightButtonGauge.PutValue(0);
               }
               else
                  LeftLightButtonGauge.PutValue(-1);
            }
            if(((DynamicObject->MoverParameters->EndSignalsFlag)&3)==1)
            {
               DynamicObject->MoverParameters->EndSignalsFlag&=(255-1);
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play( 0, 0, 0 );
               LeftLightButtonGauge.PutValue(0);
            }
         }
      }
      else
      if (cKey==Global::Keys[k_UpperSign])   //ABu 060205: swiatlo gorne - wylaczenie
      {
         if ((1+DynamicObject->MoverParameters->ActiveCab)==0)
         { //kabina 0
            if(((DynamicObject->MoverParameters->HeadSignalsFlag)&12)==4)
            {
               DynamicObject->MoverParameters->HeadSignalsFlag&=(255-4);
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play( 0, 0, 0 );
               UpperLightButtonGauge.PutValue(0);
            }
         }
         else
         { //kabina 1
            if(((DynamicObject->MoverParameters->EndSignalsFlag)&12)==4)
            {
               DynamicObject->MoverParameters->EndSignalsFlag&=(255-4);
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play( 0, 0, 0 );
               UpperLightButtonGauge.PutValue(0);
            }
         }
      }
      else
      if (cKey==Global::Keys[k_EndSign])   //ABu 060205: koncowki - sciagniecie
      {
         int CouplNr=-1;
         TDynamicObject *tmp=NULL;
         tmp=DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(), 1, 500, CouplNr);
         if (tmp==NULL)
            tmp=DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(),-1, 500, CouplNr);
         if ((tmp!=NULL)&&(CouplNr!=-1))
         {
            if(CouplNr==0)
            {
               if(((tmp->MoverParameters->HeadSignalsFlag)&64)==64)
               {
                  tmp->MoverParameters->HeadSignalsFlag&=(255-64);
                  dsbSwitch->SetVolume(DSBVOLUME_MAX);
                  dsbSwitch->Play( 0, 0, 0 );
               }
            }
            else
            {
               if(((tmp->MoverParameters->EndSignalsFlag)&64)==64)
               {
                  tmp->MoverParameters->EndSignalsFlag&=(255-64);
                  dsbSwitch->SetVolume(DSBVOLUME_MAX);
                  dsbSwitch->Play( 0, 0, 0 );
               }
            }
         }
      }
      if (cKey==Global::Keys[k_RightSign])   //Winger 070304: swiatla tylne (koncowki) - wlaczenie
      {
         if ((1+DynamicObject->MoverParameters->ActiveCab)==0)
         { //kabina 0
            if(((DynamicObject->MoverParameters->HeadSignalsFlag)&48)==0)
            {
               DynamicObject->MoverParameters->HeadSignalsFlag|=32;
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play( 0, 0, 0 );
               if(RightEndLightButtonGauge.SubModel!=NULL)
               {
                  RightEndLightButtonGauge.PutValue(1);
                  RightLightButtonGauge.PutValue(0);
               }
            else
               RightLightButtonGauge.PutValue(-1);
            }
            if(((DynamicObject->MoverParameters->HeadSignalsFlag)&48)==16)
            {
               DynamicObject->MoverParameters->HeadSignalsFlag&=(255-16);
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play( 0, 0, 0 );
               RightLightButtonGauge.PutValue(0);
            }
         }
         else
         { //kabina 1
            if(((DynamicObject->MoverParameters->EndSignalsFlag)&48)==0)
            {
               DynamicObject->MoverParameters->EndSignalsFlag|=32;
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play( 0, 0, 0 );
               if(RightEndLightButtonGauge.SubModel!=NULL)
               {
                  RightEndLightButtonGauge.PutValue(1);
                  RightLightButtonGauge.PutValue(0);
               }
            else
               RightLightButtonGauge.PutValue(-1);
            }
            if(((DynamicObject->MoverParameters->EndSignalsFlag)&48)==16)
            {
               DynamicObject->MoverParameters->EndSignalsFlag&=(255-16);
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play( 0, 0, 0 );
               RightLightButtonGauge.PutValue(0);
            }
         }
      }
      else
      if (cKey==Global::Keys[k_StLinOff])   //Winger 110904: wylacznik st. liniowych
      {
         if(DynamicObject->MoverParameters->TrainType!="ezt")
          {
                   FuseButtonGauge.PutValue(1);
                   dsbSwitch->SetVolume(DSBVOLUME_MAX);
                   dsbSwitch->Play( 0, 0, 0 );
                if (DynamicObject->MoverParameters->MainCtrlPosNo>0)
                  {
                  DynamicObject->MoverParameters->StLinFlag=true;
                  dsbRelay->SetVolume(DSBVOLUME_MAX);
                  dsbRelay->Play( 0, 0, 0 );
                  }
          }
        if(DynamicObject->MoverParameters->TrainType=="ezt")
        {
          if(DynamicObject->MoverParameters->Signalling==true)
           {
              dsbSwitch->Play( 0, 0, 0 );
              DynamicObject->MoverParameters->Signalling=false;
           }
        }
      }
      else
      if (cKey==Global::Keys[k_SmallCompressor])   //yB: pompa paliwa
      {
         if(DynamicObject->MoverParameters->EngineType==DieselElectric)
            DynamicObject->MoverParameters->diel_fuel= false;
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play( 0, 0, 0 );
      }
      else
      {
  //McZapkie: poruszanie sie po kabinie, w updatemechpos zawarte sa wiezy

       double dt=Timer::GetDeltaTime();
       if (DynamicObject->MoverParameters->ActiveCab<0)
        fMechCroach=-0.5;
       else
        fMechCroach=0.5;
//        if (!GetAsyncKeyState(VK_SHIFT)<0)         // bez shifta
         {
          if (cKey==Global::Keys[k_MechLeft])
              vMechMovement.x+=fMechCroach;
          else
          if (cKey==Global::Keys[k_MechRight])
              vMechMovement.x-=fMechCroach;
          else
          if (cKey==Global::Keys[k_MechBackward])
              vMechMovement.z-=fMechCroach;
          else
          if (cKey==Global::Keys[k_MechForward])
              vMechMovement.z+=fMechCroach;
          else
          if (cKey==Global::Keys[k_MechUp])
              pMechOffset.y+=0.3; //McZapkie-120302 - wstawanie
          else
          if (cKey==Global::Keys[k_MechDown])
              pMechOffset.y-=0.3; //McZapkie-120302 - siadanie
         }
       }

  //    else
      if (DebugModeFlag)
      {
          if (cKey==VkKeyScan('['))
              DynamicObject->Move(100.0);
          else
          if (cKey==VkKeyScan(']'))
              DynamicObject->Move(-100.0);
      }
   }
}


bool __fastcall TTrain::UpdateMechPosition(double dt)
{

    DynamicObject->vFront= DynamicObject->GetDirection();

    DynamicObject->vUp= vWorldUp;
    DynamicObject->vFront.Normalize();
    DynamicObject->vLeft= CrossProduct(DynamicObject->vUp,DynamicObject->vFront);
    DynamicObject->vUp= CrossProduct(DynamicObject->vFront,DynamicObject->vLeft);
    matrix4x4 mat;

    double a1,a2,atmp;
    a1= DegToRad(DynamicObject->Axle1.GetRoll());
    a2= DegToRad(DynamicObject->Axle4.GetRoll());
    atmp=(a1+a2);
    if (DynamicObject->ABuGetDirection()<0) atmp=-atmp;
//    mat.Rotation( (DegToRad(Axle1.GetRoll()+Axle4.GetRoll()))*0.5f, vFront );
    mat.Rotation( atmp*0.5f, DynamicObject->vFront );

    DynamicObject->vUp= mat*DynamicObject->vUp;
    DynamicObject->vLeft= mat*DynamicObject->vLeft;


//    matrix4x4 mat;
    mat.Identity();

    mat.BasisChange(DynamicObject->vLeft,DynamicObject->vUp,DynamicObject->vFront);
    DynamicObject->mMatrix= Inverse(mat);

    vector3 pNewMechPosition;
//McZapkie: najpierw policze pozycje w/m kabiny

    //McZapkie: poprawka starego bledu
//    dt=Timer::GetDeltaTime();

    //ABu: rzucamy kabina tylko przy duzym FPS!
    //Mala histereza, zeby bez przerwy nie przelaczalo przy FPS~17
    //Granice mozna ustalic doswiadczalnie. Ja proponuje 14:20
    double r1,r2,r3;
    int iVel= DynamicObject->GetVelocity();
    if (iVel>150) iVel=150;
    if (!Global::slowmotion)
    {
      if (!(random((GetFPS()+1)/15)>0))
      {
         if ((iVel>0) && (random(155-iVel)<16))
         {
            r1= (double(random(iVel*2)-iVel)/((iVel*2)*4))*fMechSpringX;
            r2= (double(random(iVel*2)-iVel)/((iVel*2)*4))*fMechSpringY;
            r3= (double(random(iVel*2)-iVel)/((iVel*2)*4))*fMechSpringZ;
            MechSpring.ComputateForces(vector3(r1,r2,r3),pMechShake);
//          MechSpring.ComputateForces(vector3(double(random(200)-100)/200,double(random(200)-100)/200,double(random(200)-100)/500),pMechShake);
         }
         else
            MechSpring.ComputateForces(vector3(-DynamicObject->MoverParameters->AccN*dt,DynamicObject->MoverParameters->AccV*dt*10,-DynamicObject->MoverParameters->AccS*dt),pMechShake);
      }
      vMechVelocity-= (MechSpring.vForce2+vMechVelocity*100)*(fMechSpringX+fMechSpringY+fMechSpringZ)/(200);

      //McZapkie:
      pMechShake+= vMechVelocity*dt;
    }
    else
      pMechShake-=pMechShake*Min0R(dt,1);

    pMechOffset+= vMechMovement*dt;
    if ((pMechShake.y>fMechMaxSpring) || (pMechShake.y<-fMechMaxSpring))
     vMechVelocity.y=-vMechVelocity.y;
    //ABu011104: 5*pMechShake.y, zeby ladnie pudlem rzucalo :)
    pNewMechPosition= pMechOffset+vector3(pMechShake.x,5*pMechShake.y,pMechShake.z);
    vMechMovement= vMechMovement/2;
//numer kabiny (-1: kabina B)
    iCabn= (DynamicObject->MoverParameters->ActiveCab==-1 ? 2 : DynamicObject->MoverParameters->ActiveCab);
    if (!DebugModeFlag)
//sprawdzaj wiezy
     {
//McZapkie: TODO: zmieniac tu 1 na nr kabiny

      if (pNewMechPosition.x<Cabine[iCabn].CabPos1.x) pNewMechPosition.x=Cabine[iCabn].CabPos1.x;
      if (pNewMechPosition.x>Cabine[iCabn].CabPos2.x) pNewMechPosition.x=Cabine[iCabn].CabPos2.x;
      if (pNewMechPosition.z<Cabine[iCabn].CabPos1.z) pNewMechPosition.z=Cabine[iCabn].CabPos1.z;
      if (pNewMechPosition.z>Cabine[iCabn].CabPos2.z) pNewMechPosition.z=Cabine[iCabn].CabPos2.z;
      if (pNewMechPosition.y>Cabine[iCabn].CabPos1.y+1.8) pNewMechPosition.y=Cabine[iCabn].CabPos1.y+1.8;
      if (pNewMechPosition.y<Cabine[iCabn].CabPos1.y+0.5) pNewMechPosition.y=Cabine[iCabn].CabPos2.y+0.5;

      if (pMechOffset.x<Cabine[iCabn].CabPos1.x) pMechOffset.x=Cabine[iCabn].CabPos1.x;
      if (pMechOffset.x>Cabine[iCabn].CabPos2.x) pMechOffset.x=Cabine[iCabn].CabPos2.x;
      if (pMechOffset.z<Cabine[iCabn].CabPos1.z) pMechOffset.z=Cabine[iCabn].CabPos1.z;
      if (pMechOffset.z>Cabine[iCabn].CabPos2.z) pMechOffset.z=Cabine[iCabn].CabPos2.z;
      if (pMechOffset.y>Cabine[iCabn].CabPos1.y+1.8) pMechOffset.y=Cabine[iCabn].CabPos1.y+1.8;
      if (pMechOffset.y<Cabine[iCabn].CabPos1.y+0.5) pMechOffset.y=Cabine[iCabn].CabPos2.y+0.5;
      }
    pMechPosition= DynamicObject->mMatrix*pNewMechPosition;
    pMechPosition+= DynamicObject->GetPosition();






}

//#include "dbgForm.h"

bool __fastcall TTrain::Update()
{

 DWORD stat;

 double dt= Timer::GetDeltaTime();

 if (DynamicObject->mdKabina)
  {
    tor= DynamicObject->GetTrack(); //McZapkie-180203

//McZapkie: predkosc wyswietlana na tachometrze brana jest z obrotow kol
    float maxtacho=3;
    fTachoVelocity= abs(11.31*DynamicObject->MoverParameters->WheelDiameter*DynamicObject->MoverParameters->nrot);
    if (fTachoVelocity>1) //McZapkie-270503: podkrecanie tachometru
     {
      if (fTachoCount<maxtacho)
       fTachoCount+=dt;
     }
    else
     if (fTachoCount>0)
      fTachoCount-=dt;

    double vol=0;
//    int freq=1;
    double dfreq=1.0;

//McZapkie-280302 - syczenie
      if (rsHiss.AM!=0)
       {
          fPPress=(fPPress+Max0R(DynamicObject->MoverParameters->dpLocalValve,DynamicObject->MoverParameters->dpMainValve))/2;
          if (fPPress>0)
           {
            vol=2*rsHiss.AM*fPPress;
           }
          fNPress=(fNPress+Min0R(DynamicObject->MoverParameters->dpLocalValve,DynamicObject->MoverParameters->dpMainValve))/2;
          if (fNPress<0 && -fNPress>fPPress)
           {
            vol=-2*rsHiss.AM*fNPress;
           }
          if (vol>0.1)
           {
            rsHiss.Play(vol,DSBPLAY_LOOPING,true,DynamicObject->GetPosition());
           }
          else
           {
            rsHiss.Stop();
           }
       }

//Winger-160404 - syczenie pomocniczego (luzowanie)
/*      if (rsSBHiss.AM!=0)
       {
          fSPPress=(DynamicObject->MoverParameters->LocalBrakeRatio())-(DynamicObject->MoverParameters->LocalBrakePos);
          if (fSPPress>0)
           {
            vol=2*rsSBHiss.AM*fSPPress;
           }
          if (vol>0.1)
           {
            rsSBHiss.Play(vol,DSBPLAY_LOOPING,true,DynamicObject->GetPosition());
           }
          else
           {
            rsSBHiss.Stop();
           }
       }
*/
//szum w czasie jazdy
    vol=0.0;
    dfreq=1.0;
    if (rsRunningNoise.AM!=0)
     {
       if (DynamicObject->GetVelocity()!=0)
        {
          if (!TestFlag(DynamicObject->MoverParameters->DamageFlag,dtrain_wheelwear)) //McZpakie-221103: halas zalezny od kola
           {
             dfreq= rsRunningNoise.FM*DynamicObject->MoverParameters->Vel+rsRunningNoise.FA;
             vol= rsRunningNoise.AM*DynamicObject->MoverParameters->Vel+rsRunningNoise.AA;
              switch (tor->eEnvironment)
              {
                  case e_tunnel:
                   {
                    vol*=3;
                    dfreq*=0.95;
                   }
                  break;
                  case e_canyon:
                   {
                    vol*=1.1;
                   }
                  break;
                  case e_bridge:
                   {
                    vol*=2;
                    dfreq*=0.98;
                   }
                  break;
              }

           }
          else                                                   //uszkodzone kolo (podkucie)
           if (fabs(DynamicObject->MoverParameters->nrot)>0.01)
           {
             dfreq= rsRunningNoise.FM*DynamicObject->MoverParameters->Vel+rsRunningNoise.FA;
             vol= rsRunningNoise.AM*DynamicObject->MoverParameters->Vel+rsRunningNoise.AA;
              switch (tor->eEnvironment)
              {
                  case e_tunnel:
                   {
                    vol*=2;
                   }
                  break;
                  case e_canyon:
                   {
                    vol*=1.1;
                   }
                  break;
                  case e_bridge:
                   {
                    vol*=1.5;
                   }
                  break;
              }
           }
          if (fabs(DynamicObject->MoverParameters->nrot)>0.01)
           vol*=1+DynamicObject->MoverParameters->UnitBrakeForce/(1+DynamicObject->MoverParameters->MaxBrakeForce); //hamulce wzmagaja halas
          vol= vol*(20.0+tor->iDamageFlag)/21;
          rsRunningNoise.AdjFreq(dfreq,0);
          rsRunningNoise.Play(vol, DSBPLAY_LOOPING, true, DynamicObject->GetPosition());
        }
       else
        rsRunningNoise.Stop();
     }

    if (rsBrake.AM!=0)
     {
      if ((!DynamicObject->MoverParameters->SlippingWheels) && (DynamicObject->MoverParameters->UnitBrakeForce>10.0) && (DynamicObject->GetVelocity()>0.01))
       {
        vol=rsBrake.AA+rsBrake.AM*(DynamicObject->GetVelocity()*100+DynamicObject->MoverParameters->UnitBrakeForce);
        dfreq=rsBrake.FA+rsBrake.FM*DynamicObject->GetVelocity();
        rsBrake.AdjFreq(dfreq,0);
        rsBrake.Play(vol,DSBPLAY_LOOPING,true,DynamicObject->GetPosition());
       }
      else
       {
         rsBrake.Stop();
       }
      }


    if (rsEngageSlippery.AM!=0)
     {
      if /*((fabs(DynamicObject->MoverParameters->dizel_engagedeltaomega)>0.2) && */ (DynamicObject->MoverParameters->dizel_engage>0.1)
       {
        if (fabs(DynamicObject->MoverParameters->dizel_engagedeltaomega)>0.2)
         {
           dfreq=rsEngageSlippery.FA+rsEngageSlippery.FM*fabs(DynamicObject->MoverParameters->dizel_engagedeltaomega);
           vol=rsEngageSlippery.AA+rsEngageSlippery.AM*(DynamicObject->MoverParameters->dizel_engage);
         }
        else
         {
          dfreq=1; //rsEngageSlippery.FA+0.7*rsEngageSlippery.FM*(fabs(DynamicObject->MoverParameters->enrot)+DynamicObject->MoverParameters->nmax);
          if (DynamicObject->MoverParameters->dizel_engage>0.2)
           vol=rsEngageSlippery.AA+0.2*rsEngageSlippery.AM*(DynamicObject->MoverParameters->enrot/DynamicObject->MoverParameters->nmax);
          else
           vol=0;
         }
        rsEngageSlippery.AdjFreq(dfreq,0);
        rsEngageSlippery.Play(vol,DSBPLAY_LOOPING,true,DynamicObject->GetPosition());
       }
      else
       {
         rsEngageSlippery.Stop();
       }
      }

   rsFadeSound.Play(1,DSBPLAY_LOOPING,true,DynamicObject->GetPosition());

// McZapkie! - to wazne - SoundFlag wystawiane jest przez moje moduly
// gdy zachodza pewne wydarzenia komentowane dzwiekiem.
// Mysle ze wystarczy sprawdzac a potem zerowac SoundFlag tutaj
// a nie w DynObject - gdyby cos poszlo zle to po co szarpac dzwiekiem co 10ms.

  if ((TestFlag(DynamicObject->MoverParameters->SoundFlag,sound_relay))&&((!DynamicObject->MoverParameters->DynamicBrakeFlag)||(DynamicObject->MoverParameters->DynamicBrakeType!=dbrake_passive))) // przekaznik - gdy bezpiecznik, automatyczny rozruch itp
  {
    if (DynamicObject->MoverParameters->EventFlag || TestFlag(DynamicObject->MoverParameters->SoundFlag,sound_loud))
    {
     DynamicObject->MoverParameters->EventFlag=False;
     dsbRelay->SetVolume(DSBVOLUME_MAX);
    }
    else
     {
     dsbRelay->SetVolume(-40);
     }
     if (!TestFlag(DynamicObject->MoverParameters->SoundFlag,sound_manyrelay))
       dsbRelay->Play( 0, 0, 0 );
     else
      {
        if (TestFlag(DynamicObject->MoverParameters->SoundFlag,sound_loud))
         dsbWejscie_na_bezoporow->Play( 0, 0, 0 );
        else
         dsbWescie_na_drugi_uklad->Play( 0, 0, 0 );
      }
  }
  // potem dorobic bufory, sprzegi jako RealSound.
  if (TestFlag(DynamicObject->MoverParameters->SoundFlag,sound_bufferclamp)) // zderzaki uderzaja o siebie

   {
    if (TestFlag(DynamicObject->MoverParameters->SoundFlag,sound_loud))
     dsbBufferClamp->SetVolume(DSBVOLUME_MAX);
    else
     dsbBufferClamp->SetVolume(-20);
    dsbBufferClamp->Play( 0, 0, 0 );


 }
  if (TestFlag(DynamicObject->MoverParameters->SoundFlag,sound_couplerstretch)) // sprzegi sie rozciagaja
      {
      if (TestFlag(DynamicObject->MoverParameters->SoundFlag,sound_loud))
       {
       dsbCouplerStretch->SetVolume(DSBVOLUME_MAX);
       }
     else
     dsbCouplerStretch->SetVolume(-20);
     dsbCouplerStretch->Play( 0, 0, 0 );
     }

  if (DynamicObject->MoverParameters->SoundFlag==0)
   if (DynamicObject->MoverParameters->EventFlag)
    if (TestFlag(DynamicObject->MoverParameters->DamageFlag,dtrain_wheelwear))
     {
      if (rsRunningNoise.AM!=0)
       {
        rsRunningNoise.Stop();
        float aa=rsRunningNoise.AA;
        float am=rsRunningNoise.AM;
        float fa=rsRunningNoise.FA;
        float fm=rsRunningNoise.FM;
        rsRunningNoise.Init("lomotpodkucia.wav",-1,0,0,0);    //MC: zmiana szumu na lomot
        if (rsRunningNoise.AM==1)
         rsRunningNoise.AM=am;
        rsRunningNoise.AA=0.7;
        rsRunningNoise.FA=fa;
        rsRunningNoise.FM-fm;
       }
      DynamicObject->MoverParameters->EventFlag=False;
     }

  DynamicObject->MoverParameters->SoundFlag=0;
/*
    for (int b=0; b<2; b++) //MC: aby zerowac stukanie przekaznikow w czlonie silnikowym
     if (TestFlag(DynamicObject->MoverParameters->Couplers[b].CouplingFlag,ctrain_controll))
      if (DynamicObject->MoverParameters->Couplers[b].Connected.Power>0.01)
       DynamicObject->MoverParameters->Couplers[b]->Connected->SoundFlag=0;
*/

    // McZapkie! - koniec obslugi dzwiekow z mover.pas

//McZapkie-030402: poprawione i uzupelnione amperomierze
if (!ShowNextCurrent)
{
    if (I1Gauge.SubModel!=NULL)
     {
      I1Gauge.UpdateValue(DynamicObject->MoverParameters->ShowCurrent(1));
      I1Gauge.Update();
     }
    if (I2Gauge.SubModel!=NULL)
     {
      I2Gauge.UpdateValue(DynamicObject->MoverParameters->ShowCurrent(2));
      I2Gauge.Update();
     }
    if (I3Gauge.SubModel!=NULL)
     {
      I3Gauge.UpdateValue(DynamicObject->MoverParameters->ShowCurrent(3));
      I3Gauge.Update();
     }
    if (ItotalGauge.SubModel!=NULL)
     {
      ItotalGauge.UpdateValue(DynamicObject->MoverParameters->ShowCurrent(0));
      ItotalGauge.Update();
     }
     if (I1GaugeB.SubModel!=NULL)
     {
      I1GaugeB.UpdateValue(DynamicObject->MoverParameters->ShowCurrent(1));
      I1GaugeB.Update();
     }
    if (I2GaugeB.SubModel!=NULL)
     {
      I2GaugeB.UpdateValue(DynamicObject->MoverParameters->ShowCurrent(2));
      I2GaugeB.Update();
     }
    if (I3GaugeB.SubModel!=NULL)
     {
      I3GaugeB.UpdateValue(DynamicObject->MoverParameters->ShowCurrent(3));
      I3GaugeB.Update();
     }
    if (ItotalGaugeB.SubModel!=NULL)
    {
      if (DynamicObject->MoverParameters->TrainType!="ezt")

       {
        ItotalGaugeB.UpdateValue(DynamicObject->MoverParameters->ShowCurrent(0));
        ItotalGaugeB.Update();
        }
      else
        {
          if(((TestFlag(DynamicObject->MoverParameters->Couplers[1].CouplingFlag,ctrain_controll))
          && (DynamicObject->MoverParameters->ActiveCab>0)))
          {
           ItotalGaugeB.UpdateValue(DynamicObject->NextConnected->MoverParameters->ShowCurrent(0)); //Winger czy to nie jest zle? *DynamicObject->MoverParameters->Mains);
           ItotalGaugeB.Update();
           }
           if(((TestFlag(DynamicObject->MoverParameters->Couplers[0].CouplingFlag,ctrain_controll))
           && (DynamicObject->MoverParameters->ActiveCab<0)))
           {
           ItotalGaugeB.UpdateValue(DynamicObject->PrevConnected->MoverParameters->ShowCurrent(0)); //Winger czy to nie jest zle? *DynamicObject->MoverParameters->Mains);
           ItotalGaugeB.Update();
           }
         }

   }  }
else
{
   //ABu 100205: prad w nastepnej lokomotywie, przycisk w ET41
   TDynamicObject *tmp;
   tmp=NULL;
   if (DynamicObject->NextConnected)
      if ((DynamicObject->NextConnected->MoverParameters->TrainType=="ET41")
         && TestFlag(DynamicObject->MoverParameters->Couplers[1].CouplingFlag,ctrain_controll))
         tmp=DynamicObject->NextConnected;
   if (DynamicObject->PrevConnected)
      if ((DynamicObject->PrevConnected->MoverParameters->TrainType=="ET41")
         && TestFlag(DynamicObject->MoverParameters->Couplers[0].CouplingFlag,ctrain_controll))
         tmp=DynamicObject->PrevConnected;
   //tmp=DynamicObject->NextConnected;
   if(tmp!=NULL)
   {
      if (I1Gauge.SubModel!=NULL)
      {
         I1Gauge.UpdateValue(tmp->MoverParameters->ShowCurrent(1)*1.05);
         I1Gauge.Update();
      }
      if (I2Gauge.SubModel!=NULL)
      {
         I2Gauge.UpdateValue(tmp->MoverParameters->ShowCurrent(2)*1.05);
         I2Gauge.Update();
      }
      if (I3Gauge.SubModel!=NULL)
      {
         I3Gauge.UpdateValue(tmp->MoverParameters->ShowCurrent(3)*1.05);
         I3Gauge.Update();
      }
      if (ItotalGauge.SubModel!=NULL)
      {
         ItotalGauge.UpdateValue(tmp->MoverParameters->ShowCurrent(0)*1.05);
         ItotalGauge.Update();
      }
      if (I1GaugeB.SubModel!=NULL)
      {
         I1GaugeB.UpdateValue(tmp->MoverParameters->ShowCurrent(1)*1.05);
         I1GaugeB.Update();
      }
      if (I2GaugeB.SubModel!=NULL)
      {
         I2GaugeB.UpdateValue(tmp->MoverParameters->ShowCurrent(2)*1.05);
         I2GaugeB.Update();
      }
      if (I3GaugeB.SubModel!=NULL)
      {
         I3GaugeB.UpdateValue(tmp->MoverParameters->ShowCurrent(3)*1.05);
         I3GaugeB.Update();
      }
      if (ItotalGaugeB.SubModel!=NULL)
      {
         ItotalGaugeB.UpdateValue(tmp->MoverParameters->ShowCurrent(0)*1.05);
         ItotalGaugeB.Update();
      }
   }
   else
   {
      if (I1Gauge.SubModel!=NULL)
      {
         I1Gauge.UpdateValue(0);
         I1Gauge.Update();
      }
      if (I2Gauge.SubModel!=NULL)
      {
         I2Gauge.UpdateValue(0);
         I2Gauge.Update();
      }
      if (I3Gauge.SubModel!=NULL)
      {
         I3Gauge.UpdateValue(0);
         I3Gauge.Update();
      }
      if (ItotalGauge.SubModel!=NULL)
      {
         ItotalGauge.UpdateValue(0);
         ItotalGauge.Update();
      }
      if (I1GaugeB.SubModel!=NULL)
      {
         I1GaugeB.UpdateValue(0);
         I1GaugeB.Update();
      }
      if (I2GaugeB.SubModel!=NULL)
      {
         I2GaugeB.UpdateValue(0);
         I2GaugeB.Update();
      }
      if (I3GaugeB.SubModel!=NULL)
      {
         I3GaugeB.UpdateValue(0);
         I3GaugeB.Update();
      }
      if (ItotalGaugeB.SubModel!=NULL)
        {
           ItotalGaugeB.UpdateValue(0);
           ItotalGaugeB.Update();
        }



   }
}

//youBy - prad w drugim czlonie: galaz lub calosc
{
   TDynamicObject *tmp;
   tmp=NULL;
   if (DynamicObject->NextConnected)
      if ((TestFlag(DynamicObject->MoverParameters->Couplers[1].CouplingFlag,ctrain_controll)) && (DynamicObject->MoverParameters->ActiveCab==1))
         tmp=DynamicObject->NextConnected;
   if (DynamicObject->PrevConnected)
      if ((TestFlag(DynamicObject->MoverParameters->Couplers[0].CouplingFlag,ctrain_controll)) && (DynamicObject->MoverParameters->ActiveCab==-1))
         tmp=DynamicObject->PrevConnected;
   //tmp=DynamicObject->NextConnected;
   if(tmp!=NULL)
    if (tmp->MoverParameters->Power>0)
     {
      if (I1BGauge.SubModel!=NULL)
      {
         I1BGauge.UpdateValue(tmp->MoverParameters->ShowCurrent(1));
         I1BGauge.Update();
      }
      if (I2BGauge.SubModel!=NULL)
      {
         I2BGauge.UpdateValue(tmp->MoverParameters->ShowCurrent(2));
         I2BGauge.Update();
      }
      if (I3BGauge.SubModel!=NULL)
      {
         I3BGauge.UpdateValue(tmp->MoverParameters->ShowCurrent(3));
         I3BGauge.Update();
      }
      if (ItotalBGauge.SubModel!=NULL)
      {
         ItotalBGauge.UpdateValue(tmp->MoverParameters->ShowCurrent(0));
         ItotalBGauge.Update();
      }
     
     }
}
//McZapkie-240302    VelocityGauge.UpdateValue(DynamicObject->GetVelocity());
    if (VelocityGauge.SubModel!=NULL)
     {

      fHaslerTimer+=dt;
      if(fHaslerTimer>fHaslerTime)
       {
        VelocityGauge.UpdateValue(fTachoVelocity>2?fTachoVelocity+0.5-random(5)/2:0);
        fHaslerTimer-=fHaslerTime;
       }
      VelocityGauge.Update();
     }
    if (VelocityGaugeB.SubModel!=NULL)
     {
      VelocityGaugeB.UpdateValue(fTachoVelocity);
      VelocityGaugeB.Update();
     }
//McZapkie-300302: zegarek
    if (ClockMInd.SubModel!=NULL)
     {
      ClockMInd.UpdateValue(GlobalTime->mm);
      ClockMInd.Update();
      ClockHInd.UpdateValue(GlobalTime->hh+GlobalTime->mm/60.0);
      ClockHInd.Update();
     }

    if (CylHamGauge.SubModel!=NULL)
     {
      CylHamGauge.UpdateValue(DynamicObject->MoverParameters->BrakePress);
      CylHamGauge.Update();
     }
    if (CylHamGaugeB.SubModel!=NULL)
     {
      CylHamGaugeB.UpdateValue(DynamicObject->MoverParameters->BrakePress);
      CylHamGaugeB.Update();
     }
    if (PrzGlGauge.SubModel!=NULL)
     {
      PrzGlGauge.UpdateValue(DynamicObject->MoverParameters->PipePress);
      PrzGlGauge.Update();
     }
    if (PrzGlGaugeB.SubModel!=NULL)
     {
      PrzGlGaugeB.UpdateValue(DynamicObject->MoverParameters->PipePress);
      PrzGlGaugeB.Update();
     }
// McZapkie! - zamiast pojemnosci cisnienie
    if (ZbGlGauge.SubModel!=NULL)
     {
      ZbGlGauge.UpdateValue(DynamicObject->MoverParameters->Compressor);
      ZbGlGauge.Update();
     }
    if (ZbGlGaugeB.SubModel!=NULL)
     {
      ZbGlGaugeB.UpdateValue(DynamicObject->MoverParameters->Compressor);
      ZbGlGaugeB.Update();
     }
    if (ZbWyrGauge.SubModel!=NULL)
     {
      ZbWyrGauge.UpdateValue(DynamicObject->MoverParameters->LimPipePress);
      ZbWyrGauge.Update();
     }
    if (ZbWyrGaugeB.SubModel!=NULL)
     {
      ZbWyrGaugeB.UpdateValue(DynamicObject->MoverParameters->LimPipePress);
      ZbWyrGaugeB.Update();
     }


    if (HVoltageGauge.SubModel!=NULL)
     {
      if ((DynamicObject->MoverParameters->TrainType!="ezt")&&(DynamicObject->MoverParameters->EngineType!=DieselElectric))
      {
      HVoltageGauge.UpdateValue(DynamicObject->MoverParameters->RunningTraction.TractionVoltage); //Winger czy to nie jest zle? *DynamicObject->MoverParameters->Mains);
      HVoltageGauge.Update();
      }
      if (DynamicObject->MoverParameters->TrainType=="ezt")
      {
      if(((TestFlag(DynamicObject->MoverParameters->Couplers[1].CouplingFlag,ctrain_controll))
       && (DynamicObject->MoverParameters->ActiveCab>0)))
        {
        HVoltageGauge.UpdateValue(DynamicObject->NextConnected->MoverParameters->RunningTraction.TractionVoltage); //Winger czy to nie jest zle? *DynamicObject->MoverParameters->Mains);
        HVoltageGauge.Update();
        }
      if(((TestFlag(DynamicObject->MoverParameters->Couplers[0].CouplingFlag,ctrain_controll))
       && (DynamicObject->MoverParameters->ActiveCab<0)))
       {
        HVoltageGauge.UpdateValue(DynamicObject->PrevConnected->MoverParameters->RunningTraction.TractionVoltage); //Winger czy to nie jest zle? *DynamicObject->MoverParameters->Mains);
        HVoltageGauge.Update();
       }
      }
       if (DynamicObject->MoverParameters->EngineType==DieselElectric)
        HVoltageGauge.UpdateValue(DynamicObject->MoverParameters->Voltage);
      HVoltageGauge.Update();

     }
//youBy - napiecie na silnikach
    if (EngineVoltage.SubModel!=NULL)
     {
      if (DynamicObject->MoverParameters->DynamicBrakeFlag)
       { EngineVoltage.UpdateValue(abs(DynamicObject->MoverParameters->Im*5)); }
      else
        {
         int x;
         if((DynamicObject->MoverParameters->TrainType=="et42")&&(DynamicObject->MoverParameters->Imax==DynamicObject->MoverParameters->ImaxHi))
         x=1;else x=2;
         if ((DynamicObject->MoverParameters->RList[DynamicObject->MoverParameters->MainCtrlActualPos].Mn>0) && (abs(DynamicObject->MoverParameters->Im)>0))
         { EngineVoltage.UpdateValue((x*(DynamicObject->MoverParameters->RunningTraction.TractionVoltage-DynamicObject->MoverParameters->RList[DynamicObject->MoverParameters->MainCtrlActualPos].R*abs(DynamicObject->MoverParameters->Im))/DynamicObject->MoverParameters->RList[DynamicObject->MoverParameters->MainCtrlActualPos].Mn)); }
         else
          { EngineVoltage.UpdateValue(0); }}
      EngineVoltage.Update();
     }

//Winger 140404 - woltomierz NN
    if (LVoltageGauge.SubModel!=NULL)
     {
      if (DynamicObject->MoverParameters->Battery==true)
      {LVoltageGauge.UpdateValue(DynamicObject->MoverParameters->BatteryVoltage);}
      else
      {LVoltageGauge.UpdateValue(0);}
      LVoltageGauge.Update();
     }

    if (DynamicObject->MoverParameters->EngineType==DieselElectric)
     {
      if (enrot1mGauge.SubModel!=NULL)
       {
        enrot1mGauge.UpdateValue(DynamicObject->MoverParameters->ShowEngineRotation(1));
        enrot1mGauge.Update();
       }
      if (enrot2mGauge.SubModel!=NULL)
       {
        enrot2mGauge.UpdateValue(DynamicObject->MoverParameters->ShowEngineRotation(2));
        enrot2mGauge.Update();
       }
      if (OPressGauge.SubModel!=NULL)
       {
        OPressGauge.UpdateValue(DynamicObject->MoverParameters->EnLiqs[el_oil][el_act][el_P]);
        OPressGauge.Update();
       }
      if (OTempGauge.SubModel!=NULL)
       {
        OTempGauge.UpdateValue(DynamicObject->MoverParameters->EnLiqs[el_oil][el_act][el_T]);
        OTempGauge.Update();
       }
      if (WTempGauge.SubModel!=NULL)
       {
        WTempGauge.UpdateValue(DynamicObject->MoverParameters->EnLiqs[el_water][el_act][el_T]);
        WTempGauge.Update();
       }
      if (I1Gauge.SubModel!=NULL)
       {
        I1Gauge.UpdateValue(abs(DynamicObject->MoverParameters->Im));
        I1Gauge.Update();
       }
      if (I2Gauge.SubModel!=NULL)
       {
        I2Gauge.UpdateValue(abs(DynamicObject->MoverParameters->Im));
        I2Gauge.Update();
       }
      if (HVoltageGauge.SubModel!=NULL)
       {
        HVoltageGauge.UpdateValue(DynamicObject->MoverParameters->Voltage);
        HVoltageGauge.Update();
       }

     }



    if (DynamicObject->MoverParameters->EngineType==DieselEngine)
     {
      if (enrot1mGauge.SubModel!=NULL)
       {
        enrot1mGauge.UpdateValue(DynamicObject->MoverParameters->ShowEngineRotation(1));
        enrot1mGauge.Update();
       }
      if (enrot2mGauge.SubModel!=NULL)
         {
          enrot2mGauge.UpdateValue(DynamicObject->MoverParameters->ShowEngineRotation(2));
          enrot2mGauge.Update();
         }
      if (enrot3mGauge.SubModel!=NULL)
       if (DynamicObject->MoverParameters->Couplers[1].Connected!=NULL)
         {
          enrot3mGauge.UpdateValue(DynamicObject->MoverParameters->ShowEngineRotation(3));
          enrot3mGauge.Update();
         }
      if (engageratioGauge.SubModel!=NULL)
       {
        engageratioGauge.UpdateValue(DynamicObject->MoverParameters->dizel_engage);
        engageratioGauge.Update();
       }
      if (maingearstatusGauge.SubModel!=NULL)
       {
        if (DynamicObject->MoverParameters->Mains)
         maingearstatusGauge.UpdateValue(1.1-fabs(DynamicObject->MoverParameters->dizel_automaticgearstatus));
        else
         maingearstatusGauge.UpdateValue(0);
        maingearstatusGauge.Update();
       }
      if (IgnitionKeyGauge.SubModel!=NULL)
       {
        IgnitionKeyGauge.UpdateValue(DynamicObject->MoverParameters->dizel_enginestart);
        IgnitionKeyGauge.Update();
       }
     }

    if ( DynamicObject->MoverParameters->SlippingWheels )
     {
       double veldiff=(DynamicObject->GetVelocity()-fTachoVelocity)/DynamicObject->MoverParameters->Vmax;
       if (veldiff<0)
        {
          if (abs(DynamicObject->MoverParameters->Im)>10.0)
            btLampkaPoslizg.TurnOn();
          rsSlippery.Play(-rsSlippery.AM*veldiff+rsSlippery.AA,DSBPLAY_LOOPING,true,DynamicObject->GetPosition());
        }
       else
        {
         if ((DynamicObject->MoverParameters->UnitBrakeForce>100.0) && (DynamicObject->GetVelocity()>1.0))
           {
             rsSlippery.Play(rsSlippery.AM*veldiff+rsSlippery.AA,DSBPLAY_LOOPING,true,DynamicObject->GetPosition());
           }
        }
     }
    else
     {
        btLampkaPoslizg.TurnOff();
        rsSlippery.Stop();
     }

    if ( DynamicObject->MoverParameters->Mains )
     {
        btLampkaWylSzybki.TurnOn();
        if ( DynamicObject->MoverParameters->ResistorsFlagCheck())
          btLampkaOpory.TurnOn();
        else
          btLampkaOpory.TurnOff();
        if (( DynamicObject->MoverParameters->ResistorsFlagCheck()) || ( DynamicObject->MoverParameters->MainCtrlActualPos==0))
          btLampkaBezoporowa.TurnOn();
        else
          btLampkaBezoporowa.TurnOff();    //Do EU04                                                                                                         //0.36
        if ( (DynamicObject->MoverParameters->Itot!=0) || (DynamicObject->MoverParameters->BrakePress > 0.2) || ( DynamicObject->MoverParameters->PipePress < 0.0 ))
          btLampkaStyczn.TurnOff();     //
        else
        if (DynamicObject->MoverParameters->BrakePress < 0.1)
           btLampkaStyczn.TurnOn();      //mozna prowadzic rozruch
         if (((TestFlag(DynamicObject->MoverParameters->Couplers[1].CouplingFlag,ctrain_controll)) && (DynamicObject->MoverParameters->ActiveCab==1)) ||
        ((TestFlag(DynamicObject->MoverParameters->Couplers[0].CouplingFlag,ctrain_controll)) && (DynamicObject->MoverParameters->ActiveCab==-1)))
           btLampkaUkrotnienie.TurnOn();
        else
           btLampkaUkrotnienie.TurnOff();
         if ((TestFlag(DynamicObject->MoverParameters->BrakeStatus,+b_Rused+b_Ractive)))//Lampka drugiego stopnia hamowania

           btLampkaHamPosp.TurnOn();
        else
           btLampkaHamPosp.TurnOff();

        if ( DynamicObject->MoverParameters->FuseFlagCheck() )
          btLampkaNadmSil.TurnOn();
        else
          btLampkaNadmSil.TurnOff();

        if ( DynamicObject->MoverParameters->Imax==DynamicObject->MoverParameters->ImaxHi )
          btLampkaWysRozr.TurnOn();
        else
          btLampkaWysRozr.TurnOff();
        if ((( DynamicObject->MoverParameters->ScndCtrlActualPos > 0) ||  ( (DynamicObject->MoverParameters->RList[DynamicObject->MoverParameters->MainCtrlActualPos].ScndAct!=0)&&(DynamicObject->MoverParameters->RList[DynamicObject->MoverParameters->MainCtrlActualPos].ScndAct!=255)))&&(!DynamicObject->MoverParameters->DelayCtrlFlag))
          btLampkaBoczniki.TurnOn();
        else
          btLampkaBoczniki.TurnOff();
        if ( DynamicObject->MoverParameters->ActiveDir!=0 ) //napiecie na nastawniku hamulcowym

         { btLampkaNapNastHam.TurnOn();
          //DynamicObject->MoverParameters->UpdateBatteryVoltage:=0.01;
          }
        else
         { btLampkaNapNastHam.TurnOff(); }

        if ( DynamicObject->MoverParameters->CompressorFlag==true ) //mutopsitka dziala
         { btLampkaSprezarka.TurnOn(); }
        else
         { btLampkaSprezarka.TurnOff(); }
        //boczniki
        if (DynamicObject->MoverParameters->EngineType==ElectricSeriesMotor)
         {
        char scp;
        scp=DynamicObject->MoverParameters->RList[DynamicObject->MoverParameters->MainCtrlActualPos].ScndAct;
          scp=(scp==255?0:scp);
        if ((DynamicObject->MoverParameters->ScndCtrlActualPos>0)||(DynamicObject->MoverParameters->ScndInMain)&&(scp>0))
         { btLampkaBocznikI.TurnOn(); }
        else
         { btLampkaBocznikI.TurnOff(); }
         }
        else
         {
          btLampkaBocznikI.TurnOff();
          btLampkaBocznikII.TurnOff();
          btLampkaBocznikIII.TurnOff();
          btLampkaBocznikIV.TurnOff();
          switch ( DynamicObject->MoverParameters->ScndCtrlPos )
           {
            case 1: { btLampkaBocznikI.TurnOn(); } break;
            case 2: { btLampkaBocznikII.TurnOn(); } break;
            case 3: { btLampkaBocznikIII.TurnOn(); } break;
            case 4: { btLampkaBocznikIV.TurnOn(); } break;
           }
         }
/*
        { //sprezarka w drugim wozie
        bool comptemp=false;
        if (DynamicObject->NextConnected)
           if (TestFlag(DynamicObject->MoverParameters->Couplers[1].CouplingFlag,ctrain_controll))
              comptemp=DynamicObject->NextConnected->MoverParameters->CompressorFlag;
        if ((DynamicObject->PrevConnected) && (!comptemp))
           if (TestFlag(DynamicObject->MoverParameters->Couplers[0].CouplingFlag,ctrain_controll))
              comptemp=DynamicObject->PrevConnected->MoverParameters->CompressorFlag;
             if (comptemp)
         { btLampkaSprezarkaB.TurnOn(); }
        else
         { btLampkaSprezarkaB.TurnOff(); }
        }
*/
     }
    else  //wylaczone
     {
        btLampkaWylSzybki.TurnOff();
        btLampkaOpory.TurnOff();
        btLampkaStyczn.TurnOff();
        btLampkaNadmSil.TurnOff();
        btLampkaUkrotnienie.TurnOff();
        btLampkaHamPosp.TurnOff();
        btLampkaBoczniki.TurnOff();
        btLampkaNapNastHam.TurnOff();
        btLampkaSprezarka.TurnOff();
        btLampkaBezoporowa.TurnOff();
     }
if ( DynamicObject->MoverParameters->Signalling==true )
  {

  if ((DynamicObject->MoverParameters->BrakePress>=0.145f)&&(DynamicObject->MoverParameters->Battery==true)&&(DynamicObject->MoverParameters->Signalling==true))
     { btLampkaHamowanie1zes.TurnOn(); }
  if (DynamicObject->MoverParameters->BrakePress<0.075f)
     { btLampkaHamowanie1zes.TurnOff(); }
  }
  else
  {
  btLampkaHamowanie1zes.TurnOff();
  }
if ( DynamicObject->MoverParameters->DoorSignalling==true)
  {
  if ((DynamicObject->MoverParameters->DoorBlockedFlag())&& (DynamicObject->MoverParameters->Battery==true))
     { btLampkaBlokadaDrzwi.TurnOn(); }
  else
     { btLampkaBlokadaDrzwi.TurnOff(); }
  }
  else
  { btLampkaBlokadaDrzwi.TurnOff(); }




{ //yB - wskazniki drugiego czlonu
   TDynamicObject *tmp;
   tmp=NULL;
   if ((TestFlag(DynamicObject->MoverParameters->Couplers[1].CouplingFlag,ctrain_controll)) && (DynamicObject->MoverParameters->ActiveCab>0))
      tmp=DynamicObject->NextConnected;
   if ((TestFlag(DynamicObject->MoverParameters->Couplers[0].CouplingFlag,ctrain_controll)) && (DynamicObject->MoverParameters->ActiveCab<0))
      tmp=DynamicObject->PrevConnected;

   if(tmp!=NULL)
    if ( tmp->MoverParameters->Mains )
      {
        btLampkaWylSzybkiB.TurnOn();
        if ( tmp->MoverParameters->ResistorsFlagCheck())
          btLampkaOporyB.TurnOn();
        else
          btLampkaOporyB.TurnOff();
        if (( tmp->MoverParameters->ResistorsFlagCheck()) || ( tmp->MoverParameters->MainCtrlActualPos==0))
          btLampkaBezoporowaB.TurnOn();
        else
          btLampkaBezoporowaB.TurnOff();    //Do EU04
        if ( (tmp->MoverParameters->Itot!=0) || (tmp->MoverParameters->BrakePress > 0.2) || ( tmp->MoverParameters->PipePress < 0.36 ))
          btLampkaStycznB.TurnOff();     //
        else
        if (tmp->MoverParameters->BrakePress < 0.1)
           btLampkaStycznB.TurnOn();      //mozna prowadzic rozruch

        if ( tmp->MoverParameters->CompressorFlag==true ) //mutopsitka dziala
         { btLampkaSprezarkaB.TurnOn(); }
        else
         { btLampkaSprezarkaB.TurnOff(); }
        if (( tmp->MoverParameters->BrakePress>=0.145f )&&(DynamicObject->MoverParameters->Battery==true)&&(DynamicObject->MoverParameters->Signalling==true))
         { btLampkaHamowanie2zes.TurnOn(); }
        if(( tmp->MoverParameters->BrakePress<0.075f )|| (DynamicObject->MoverParameters->Battery==false)||(DynamicObject->MoverParameters->Signalling==false))
         { btLampkaHamowanie2zes.TurnOff(); }
        if (tmp->MoverParameters->ConverterFlag==true)
        btLampkaNadmPrzetwB.TurnOff();
    else
        btLampkaNadmPrzetwB.TurnOn();
      }

    else
      {
        btLampkaWylSzybkiB.TurnOff();
        btLampkaOporyB.TurnOff();
        btLampkaStycznB.TurnOff();
        btLampkaSprezarkaB.TurnOff();
        btLampkaBezoporowaB.TurnOff();
        btLampkaHamowanie2zes.TurnOff();
        btLampkaNadmPrzetwB.TurnOn();
      }
  // if(tmp!=NULL)


} //**************************************************** */
if (DynamicObject->MoverParameters->Battery==true)
{
 if ((((DynamicObject->MoverParameters->BrakePress>=0.01f) || (DynamicObject->MoverParameters->DynamicBrakeFlag)) && (DynamicObject->MoverParameters->TrainType!="ezt")) || ((DynamicObject->MoverParameters->TrainType=="ezt") && (DynamicObject->MoverParameters->BrakePress>=0.2f)&&(DynamicObject->MoverParameters->Signalling==true)))
     { btLampkaHamienie.TurnOn(); }
    else
     { btLampkaHamienie.TurnOff(); }

     
//KURS90

     if (abs(DynamicObject->MoverParameters->Im)>=350)
     { btLampkaMaxSila.TurnOn(); }
    else
    { btLampkaMaxSila.TurnOff(); }
    if (abs(DynamicObject->MoverParameters->Im)>=450)
     { btLampkaPrzekrMaxSila.TurnOn(); }
    else
     { btLampkaPrzekrMaxSila.TurnOff(); }
     if (DynamicObject->MoverParameters->Radio==true)
     { btLampkaRadio.TurnOn(); }
    else
    { btLampkaRadio.TurnOff(); }
    if (DynamicObject->MoverParameters->ManualBrakePos>0)
     { btLampkaHamulecReczny.TurnOn(); }
    else
    { btLampkaHamulecReczny.TurnOff(); }
// NBMX wrzesien 2003 - drzwi oraz sygnal odjazdu
    if (DynamicObject->MoverParameters->DoorLeftOpened)
      btLampkaDoorLeft.TurnOn();
    else
      btLampkaDoorLeft.TurnOff();

    if (DynamicObject->MoverParameters->DoorRightOpened)
      btLampkaDoorRight.TurnOn();
    else
      btLampkaDoorRight.TurnOff();
}
else
{
btLampkaHamienie.TurnOff();
btLampkaMaxSila.TurnOff();
btLampkaPrzekrMaxSila.TurnOff();
btLampkaRadio.TurnOff();
btLampkaHamulecReczny.TurnOff();
btLampkaDoorLeft.TurnOff();
btLampkaDoorRight.TurnOff();
}
//McZapkie-080602: obroty (albo translacje) regulatorow
    if (MainCtrlGauge.SubModel!=NULL)
     {
      if (DynamicObject->MoverParameters->CoupledCtrl)
       MainCtrlGauge.UpdateValue(double(DynamicObject->MoverParameters->MainCtrlPos+DynamicObject->MoverParameters->ScndCtrlPos));
      else
       MainCtrlGauge.UpdateValue(double(DynamicObject->MoverParameters->MainCtrlPos*(DynamicObject->MoverParameters->TrainType=="sp32"&&DynamicObject->MoverParameters->ScndS?-1:1)));
      MainCtrlGauge.Update();
     }
    if (MainCtrlActGauge.SubModel!=NULL)
     {
      if (DynamicObject->MoverParameters->CoupledCtrl)
       MainCtrlActGauge.UpdateValue(double(DynamicObject->MoverParameters->MainCtrlActualPos+DynamicObject->MoverParameters->ScndCtrlActualPos));
      else
       MainCtrlActGauge.UpdateValue(double(DynamicObject->MoverParameters->MainCtrlActualPos));
      MainCtrlActGauge.Update();
     }
    if (ScndCtrlGauge.SubModel!=NULL)
     {
      ScndCtrlGauge.UpdateValue(double(DynamicObject->MoverParameters->ScndCtrlPos-(DynamicObject->MoverParameters->TrainType=="et42"&&DynamicObject->MoverParameters->DynamicBrakeFlag)));
      ScndCtrlGauge.Update();
     }
    if (DirKeyGauge.SubModel!=NULL)
     {
      if (DynamicObject->MoverParameters->TrainType!="ezt")
        DirKeyGauge.UpdateValue(double(DynamicObject->MoverParameters->ActiveDir));
      else
        DirKeyGauge.UpdateValue(double(DynamicObject->MoverParameters->ActiveDir)+
                                double(DynamicObject->MoverParameters->Imin==
                                       DynamicObject->MoverParameters->IminHi));


      DirKeyGauge.Update();
     }
    if (BrakeCtrlGauge.SubModel!=NULL)
     {
      BrakeCtrlGauge.UpdateValue(double(DynamicObject->MoverParameters->BrakeCtrlPos));
      BrakeCtrlGauge.Update();
     }
    if (LocalBrakeGauge.SubModel!=NULL)
     {
      LocalBrakeGauge.UpdateValue(double(DynamicObject->MoverParameters->LocalBrakePos));
      LocalBrakeGauge.Update();
     }
    if (ManualBrakeGauge.SubModel!=NULL)
     {
      ManualBrakeGauge.UpdateValue(double(DynamicObject->MoverParameters->ManualBrakePos));
      ManualBrakeGauge.Update();
     }
    if (BrakeProfileCtrlGauge.SubModel!=NULL)
     {
      BrakeProfileCtrlGauge.UpdateValue(double(DynamicObject->MoverParameters->BrakeDelayFlag==2?0.5:DynamicObject->MoverParameters->BrakeDelayFlag));
      BrakeProfileCtrlGauge.Update();
     }

    if (MaxCurrentCtrlGauge.SubModel!=NULL)
     {
      MaxCurrentCtrlGauge.UpdateValue(double(DynamicObject->MoverParameters->Imax==DynamicObject->MoverParameters->ImaxHi));
      MaxCurrentCtrlGauge.Update();
     }

// NBMX wrzesien 2003 - drzwi
    if (DoorLeftButtonGauge.SubModel!=NULL)
     {
     if (DynamicObject->MoverParameters->DoorLeftOpened)
       {
         DoorLeftButtonGauge.PutValue(1);
       }
     else DoorLeftButtonGauge.PutValue(0);
      DoorLeftButtonGauge.Update();
     }
    if (DoorRightButtonGauge.SubModel!=NULL)
     {
     if (DynamicObject->MoverParameters->DoorRightOpened)
       {
         DoorRightButtonGauge.PutValue(1);
       }
     else DoorRightButtonGauge.PutValue(0);
      DoorRightButtonGauge.Update();
     }
    if (DepartureSignalButtonGauge.SubModel!=NULL)
     {
//      DepartureSignalButtonGauge.UpdateValue(double());
      DepartureSignalButtonGauge.Update();
     }

//NBMX dzwignia sprezarki
    if (CompressorButtonGauge.SubModel!=NULL)
     {
     if (DynamicObject->MoverParameters->CompressorAllow)
       {
         CompressorButtonGauge.PutValue(1);
       }
     else CompressorButtonGauge.PutValue(0);
      CompressorButtonGauge.Update();
     }

    if (MainButtonGauge.SubModel!=NULL)
     {
      MainButtonGauge.Update();
     }
 if (RadioButtonGauge.SubModel!=NULL)
    {
      if (DynamicObject->MoverParameters->Radio)
          {
          RadioButtonGauge.PutValue(1);

          }
      else
          {
          RadioButtonGauge.PutValue(0);
          }
    RadioButtonGauge.Update();
    }

    if (ConverterButtonGauge.SubModel!=NULL)
     {
      ConverterButtonGauge.Update();
     }
    if (ConverterOffButtonGauge.SubModel!=NULL)
     {
      ConverterOffButtonGauge.Update();
     }


     if(((DynamicObject->MoverParameters->HeadSignalsFlag)==0)
      &&((DynamicObject->MoverParameters->EndSignalsFlag)==0))
     {
        RightLightButtonGauge.PutValue(0);
        LeftLightButtonGauge.PutValue(0);
        UpperLightButtonGauge.PutValue(0);
        RightEndLightButtonGauge.PutValue(0);
        LeftEndLightButtonGauge.PutValue(0);
     }
     if(((DynamicObject->MoverParameters->HeadSignalsFlag&1)==1)
      ||((DynamicObject->MoverParameters->EndSignalsFlag&1)==1))
        LeftLightButtonGauge.PutValue(1);
     if(((DynamicObject->MoverParameters->HeadSignalsFlag&16)==16)
      ||((DynamicObject->MoverParameters->EndSignalsFlag&16)==16))
        RightLightButtonGauge.PutValue(1);
     if(((DynamicObject->MoverParameters->HeadSignalsFlag&4)==4)
      ||((DynamicObject->MoverParameters->EndSignalsFlag&4)==4))
        UpperLightButtonGauge.PutValue(1);

     if(((DynamicObject->MoverParameters->HeadSignalsFlag&2)==2)
      ||((DynamicObject->MoverParameters->EndSignalsFlag&2)==2))
        if(LeftEndLightButtonGauge.SubModel!=NULL)
        {
           LeftEndLightButtonGauge.PutValue(1);
           LeftLightButtonGauge.PutValue(0);
        }
        else
           LeftLightButtonGauge.PutValue(-1);

     if(((DynamicObject->MoverParameters->HeadSignalsFlag&32)==32)
      ||((DynamicObject->MoverParameters->EndSignalsFlag&32)==32))
        if(RightEndLightButtonGauge.SubModel!=NULL)
        {
           RightEndLightButtonGauge.PutValue(1);
           RightLightButtonGauge.PutValue(0);
        }
        else
           RightLightButtonGauge.PutValue(-1);


//Winger 010304 - pantografy
    if (PantFrontButtonGauge.SubModel!=NULL)
    {
      if (DynamicObject->MoverParameters->PantFrontUp)
          PantFrontButtonGauge.PutValue(1);
      else
          PantFrontButtonGauge.PutValue(0);
    PantFrontButtonGauge.Update();
    }
    if (PantRearButtonGauge.SubModel!=NULL)
    {
      if (DynamicObject->MoverParameters->PantRearUp)
          PantRearButtonGauge.PutValue(1);
      else
          PantRearButtonGauge.PutValue(0);
     PantRearButtonGauge.Update();
     }
    if (PantFrontButtonOffGauge.SubModel!=NULL)
    {
     PantFrontButtonOffGauge.Update();
    }
//Winger 020304 - ogrzewanie
    if (TrainHeatingButtonGauge.SubModel!=NULL)
    {
      if (DynamicObject->MoverParameters->Heating)
          {
          TrainHeatingButtonGauge.PutValue(1);
          if (DynamicObject->MoverParameters->ConverterFlag==true)
           btLampkaOgrzewanieSkladu.TurnOn();
          }
      else
          {
          TrainHeatingButtonGauge.PutValue(0);
          btLampkaOgrzewanieSkladu.TurnOff();
          }
    TrainHeatingButtonGauge.Update();
    }
    if (SignallingButtonGauge.SubModel!=NULL)
    {
      if (DynamicObject->MoverParameters->Signalling)
          {
          SignallingButtonGauge.PutValue(1);

          }
      else
          {
          SignallingButtonGauge.PutValue(0);
          }
    SignallingButtonGauge.Update();
    }
    if (DoorSignallingButtonGauge.SubModel!=NULL)
    {
      if (DynamicObject->MoverParameters->DoorSignalling)
          {
          DoorSignallingButtonGauge.PutValue(1);

          }
      else
          {
          DoorSignallingButtonGauge.PutValue(0);
          }
    DoorSignallingButtonGauge.Update();
    }

    if (DynamicObject->MoverParameters->ConverterFlag==true)
        btLampkaNadmPrzetw.TurnOff();
    else
        btLampkaNadmPrzetw.TurnOn();

//McZapkie-141102: SHP i czuwak, TODO: sygnalizacja kabinowa
    if (DynamicObject->MoverParameters->SecuritySystem.Status>0)
     {
       if (fBlinkTimer>fCzuwakBlink)
           fBlinkTimer= -fCzuwakBlink;
       else
           fBlinkTimer+= dt;
       if (TestFlag(DynamicObject->MoverParameters->SecuritySystem.Status,s_aware))
        {
         if (fBlinkTimer>0)
          btLampkaCzuwaka.TurnOn();
         else
          btLampkaCzuwaka.TurnOff();
        }
        else btLampkaCzuwaka.TurnOff();
       if (TestFlag(DynamicObject->MoverParameters->SecuritySystem.Status,s_active))
        {
//         if (fBlinkTimer>0)
          btLampkaSHP.TurnOn();
//         else
//          btLampkaSHP.TurnOff();
        }
        else btLampkaSHP.TurnOff();
       if (TestFlag(DynamicObject->MoverParameters->SecuritySystem.Status,s_alarm))
        {
          dsbBuzzer->GetStatus(&stat);
          if (!(stat&DSBSTATUS_PLAYING))
             dsbBuzzer->Play( 0, 0, DSBPLAY_LOOPING );
        }
       else
        {
         dsbBuzzer->GetStatus(&stat);
         if (stat&DSBSTATUS_PLAYING)
           dsbBuzzer->Stop();
        }
     }
    else //wylaczone
     {
       btLampkaCzuwaka.TurnOff();
       btLampkaSHP.TurnOff();
       dsbBuzzer->GetStatus(&stat);
       if (stat&DSBSTATUS_PLAYING)
         dsbBuzzer->Stop();
     }


    if ( Pressed(Global::Keys[k_Horn]) )
     {
      if (Pressed(VK_SHIFT))
         {
         SetFlag(DynamicObject->MoverParameters->WarningSignal,2);
         DynamicObject->MoverParameters->WarningSignal&=(255-1);
         if (HornButtonGauge.SubModel!=NULL)
            HornButtonGauge.UpdateValue(1);
         }
      else
         {
         SetFlag(DynamicObject->MoverParameters->WarningSignal,1);
         DynamicObject->MoverParameters->WarningSignal&=(255-2);
         if (HornButtonGauge.SubModel!=NULL)
            HornButtonGauge.UpdateValue(-1);
         }
     }
    else
     {
        DynamicObject->MoverParameters->WarningSignal=0;
        if (HornButtonGauge.SubModel!=NULL)
           HornButtonGauge.UpdateValue(0);
     }

    if ( Pressed(Global::Keys[k_Horn2]) )
     if (Global::Keys[k_Horn2]!=Global::Keys[k_Horn])
     {
        SetFlag(DynamicObject->MoverParameters->WarningSignal,2);
     }

     if ( Pressed(Global::Keys[k_Univ1]) )
     {
        if (!DebugModeFlag)
        {
           if(Universal1ButtonGauge.SubModel!=NULL)
           if (Pressed(VK_SHIFT))
           {
              Universal1ButtonGauge.IncValue(dt/2);
           }
           else
           {
              Universal1ButtonGauge.DecValue(dt/2);
           }
        }
     }

     if ( Pressed(Global::Keys[k_SmallCompressor]))   //Winger 160404: mala sprezarka wl
          {
           if((DynamicObject->MoverParameters->PantPress<0.55))
           {
               DynamicObject->MoverParameters->PantCompFlag= true;
               //dsbSwitch->SetVolume(DSBVOLUME_MAX);
               //dsbSwitch->Play( 0, 0, 0 );
           }
           else
           {
            DynamicObject->MoverParameters->PantCompFlag= false;
            }
      }
      if ( !Pressed(Global::Keys[k_SmallCompressor]))
      {
            DynamicObject->MoverParameters->PantCompFlag= false;
            }
     if ( Pressed(Global::Keys[k_Univ2]) )
     {
        if (!DebugModeFlag)
        {
           if(Universal2ButtonGauge.SubModel!=NULL)
           if (Pressed(VK_SHIFT))
           {
              Universal2ButtonGauge.IncValue(dt/2);
           }
           else
           {
              Universal2ButtonGauge.DecValue(dt/2);
           }
        }
     }
     if ( Pressed(Global::Keys[k_Univ3]) )
     {
        if(Universal3ButtonGauge.SubModel!=NULL)
        if (Pressed(VK_SHIFT))
        {
           Universal3ButtonGauge.UpdateValue(1);
           if(btLampkaUniversal3.Active())
              LampkaUniversal3_st=true;
        }
        else
        {
           Universal3ButtonGauge.UpdateValue(0);
           if(btLampkaUniversal3.Active())
              LampkaUniversal3_st=false;
        }
     }

     if (( !Pressed(Global::Keys[k_DecBrakeLevel]) )&&( !Pressed(Global::Keys[k_WaveBrake]) )&&(DynamicObject->MoverParameters->BrakeCtrlPos==-1)&&(DynamicObject->MoverParameters->BrakeSubsystem==ss_LSt)&&(DynamicObject->MoverParameters->BrakeSystem==ElectroPneumatic)&&(DynamicObject->Controller!= AIdriver))
     {
     //DynamicObject->MoverParameters->BrakeCtrlPos=(DynamicObject->MoverParameters->BrakeCtrlPos)+1;
     DynamicObject->MoverParameters->IncBrakeLevel();
     keybrakecount = 0;
       if ((DynamicObject->MoverParameters->TrainType=="ezt")&&(DynamicObject->MoverParameters->Mains)&&(DynamicObject->MoverParameters->ActiveDir!=0))
       {
       dsbPneumaticSwitch->SetVolume(-10);
       dsbPneumaticSwitch->Play( 0, 0, 0 );
       }
     }
     //ABu030405 obsluga lampki uniwersalnej:
     if((LampkaUniversal3_st)&&(btLampkaUniversal3.Active()))
     {
        if(LampkaUniversal3_typ==0)
           btLampkaUniversal3.TurnOn();
        else
        if((LampkaUniversal3_typ==1)&&(DynamicObject->MoverParameters->Mains==true))
           btLampkaUniversal3.TurnOn();
        else
        if((LampkaUniversal3_typ==2)&&(DynamicObject->MoverParameters->ConverterFlag==true))
           btLampkaUniversal3.TurnOn();
        else
           btLampkaUniversal3.TurnOff();
     }
     else
     {
        if(btLampkaUniversal3.Active())
           btLampkaUniversal3.TurnOff();
     }

     if ( Pressed(Global::Keys[k_Univ4]) )
     {
        if(Universal4ButtonGauge.SubModel!=NULL)
        if (Pressed(VK_SHIFT))
        {
           ActiveUniversal4=true;
           //Universal4ButtonGauge.UpdateValue(1);
        }
        else
        {
           ActiveUniversal4=false;
           //Universal4ButtonGauge.UpdateValue(0);
        }
     }



    if ((DynamicObject->MoverParameters->BrakeSystem==ElectroPneumatic)&&(DynamicObject->MoverParameters->BrakeSubsystem==Knorr))
     if ( Pressed(Global::Keys[k_AntiSlipping] ) )
      {
       AntiSlipButtonGauge.UpdateValue(1);
       if(DynamicObject->MoverParameters->SwitchEPBrake(1))
        {
         dsbPneumaticSwitch->SetVolume(-10);
         dsbPneumaticSwitch->Play( 0, 0, 0 );
        }
      }
     else
      {
       if(DynamicObject->MoverParameters->SwitchEPBrake(0))
        {
         dsbPneumaticSwitch->SetVolume(-10);
         dsbPneumaticSwitch->Play( 0, 0, 0 );
        }
      }
    if ( Pressed(Global::Keys[k_DepartureSignal]) )
     {
      DepartureSignalButtonGauge.PutValue(1);
      btLampkaDepartureSignal.TurnOn();
      DynamicObject->MoverParameters->DepartureSignal=true;
     }
     else
     {
       btLampkaDepartureSignal.TurnOff();
       DynamicObject->MoverParameters->DepartureSignal=false;
     }

if ( Pressed(Global::Keys[k_Converter]) )                //[]
     {
      if (Pressed(VK_SHIFT))
         ConverterButtonGauge.PutValue(1);
      else
      {
         ConverterButtonGauge.PutValue(0);
         ConverterOffButtonGauge.PutValue(1);
      }
     }
    else
      if (DynamicObject->MoverParameters->ConvSwitchType=="impulse")
      {
         ConverterButtonGauge.PutValue(0);
         ConverterOffButtonGauge.PutValue(0);
      }

if ( Pressed(Global::Keys[k_Main]) )                //[]
     {
      if (Pressed(VK_SHIFT))
         MainButtonGauge.PutValue(1);
      else
         MainButtonGauge.PutValue(0);
      }
    else
         MainButtonGauge.PutValue(0);

if ( Pressed(Global::Keys[k_CurrentNext]))
   {
      if(ShowNextCurrent==false)
      {
         if(NextCurrentButtonGauge.SubModel!=NULL)
         {
            NextCurrentButtonGauge.UpdateValue(1);
            dsbSwitch->SetVolume(DSBVOLUME_MAX);
            dsbSwitch->Play( 0, 0, 0 );
            ShowNextCurrent=true;
         }
      }
   }
else
   {
      if(ShowNextCurrent==true)
      {
         if(NextCurrentButtonGauge.SubModel!=NULL)
         {
            NextCurrentButtonGauge.UpdateValue(0);
            dsbSwitch->SetVolume(DSBVOLUME_MAX);
            dsbSwitch->Play( 0, 0, 0 );
            ShowNextCurrent=false;
         }
      }
   }

//Winger 010304  PantAllDownButton
    if ( Pressed(Global::Keys[k_PantFrontUp]) )
     {
      if (Pressed(VK_SHIFT))
      PantFrontButtonGauge.PutValue(1);
      else
      PantAllDownButtonGauge.PutValue(1);
     }
     else
      if (DynamicObject->MoverParameters->PantSwitchType=="impulse")
      {
      PantFrontButtonGauge.PutValue(0);
      PantAllDownButtonGauge.PutValue(0);
      }

    if ( Pressed(Global::Keys[k_PantRearUp]) )
     {
      if (Pressed(VK_SHIFT))
      PantRearButtonGauge.PutValue(1);
      else
      PantFrontButtonOffGauge.PutValue(1);
     }
     else
      if (DynamicObject->MoverParameters->PantSwitchType=="impulse")
      {
      PantRearButtonGauge.PutValue(0);
      PantFrontButtonOffGauge.PutValue(0);
      }


    if (Pressed(Global::Keys[k_Releaser]));   //odluzniacz
     else
      DynamicObject->MoverParameters->BrakeStatus&=251;
 /*if ((DynamicObject->MoverParameters->Mains) && (DynamicObject->MoverParameters->EngineType==ElectricSeriesMotor))
    {
//tu dac w przyszlosci zaleznosc od wlaczenia przetwornicy
    if (DynamicObject->MoverParameters->ConverterFlag)          //NBMX -obsluga przetwornicy
    {
    //glosnosc zalezna od nap. sieci
    //-2000 do 0
     long tmpVol;
     int trackVol;
     trackVol = 3550-2000;
     if ( DynamicObject->MoverParameters->CompressorFlag==true )
      {
       sConverter.Volume(5);
      }
     else
      {
       tmpVol=DynamicObject->MoverParameters->RunningTraction.TractionVoltage-2000;
      }
     sConverter.Volume(10);

       if (!sConverter.Playing())
         sConverter.TurnOn();
    }
    else                        //wyl przetwornicy
    sConverter.TurnOff();
   }
   else
   {
      if (sConverter.Playing())
       sConverter.TurnOff();
   }
   sConverter.Update();

   */
//  if (fabs(DynamicObject->GetVelocity())>0.5)
    if (fTachoCount>maxtacho)
    {
        dsbHasler->GetStatus(&stat);
        if (!(stat&DSBSTATUS_PLAYING))
            dsbHasler->Play( 0, 0, DSBPLAY_LOOPING );
    }
   else
    {
     if (fTachoCount<1)
      {
       dsbHasler->GetStatus(&stat);
       if (stat&DSBSTATUS_PLAYING)
          dsbHasler->Stop();
      }
    }


// koniec mieszania z dzwiekami

// guziki:
    MainOffButtonGauge.Update();
    MainOnButtonGauge.Update();
    MainButtonGauge.Update();
    SecurityResetButtonGauge.Update();
    ReleaserButtonGauge.Update();
    AntiSlipButtonGauge.Update();
    FuseButtonGauge.Update();
    StLinOffButtonGauge.Update();
    RadioButtonGauge.Update();
    DepartureSignalButtonGauge.Update();
    PantFrontButtonGauge.Update();
    PantRearButtonGauge.Update();
    PantFrontButtonOffGauge.Update();
    UpperLightButtonGauge.Update();
    LeftLightButtonGauge.Update();
    RightLightButtonGauge.Update();
    LeftEndLightButtonGauge.Update();
    RightEndLightButtonGauge.Update();
    PantAllDownButtonGauge.Update();
    ConverterButtonGauge.Update();
    ConverterOffButtonGauge.Update();
    TrainHeatingButtonGauge.Update();
    SignallingButtonGauge.Update();
    NextCurrentButtonGauge.Update();
    HornButtonGauge.Update();
    Universal1ButtonGauge.Update();
    Universal2ButtonGauge.Update();
    Universal3ButtonGauge.Update();
    if (ActiveUniversal4)
       Universal4ButtonGauge.PermIncValue(dt);
    Universal4ButtonGauge.Update();
    MainOffButtonGauge.UpdateValue(0);
    MainOnButtonGauge.UpdateValue(0);
    SecurityResetButtonGauge.UpdateValue(0);
    ReleaserButtonGauge.UpdateValue(0);
    AntiSlipButtonGauge.UpdateValue(0);
    DepartureSignalButtonGauge.UpdateValue(0);
    FuseButtonGauge.UpdateValue(0);
  }
 return true; //(DynamicObject->Update(dt));
}  //koniec update


//McZapkie-090902: zmiana kabiny 1->0->2 i z powrotem
bool TTrain::CabChange(int iDirection)
{
   if (DynamicObject->MoverParameters->ChangeCab(iDirection))
    {
      if (InitializeCab(DynamicObject->MoverParameters->ActiveCab,DynamicObject->asBaseDir+DynamicObject->MoverParameters->TypeName+".mmd"))
       {
         VelocityGauge.PutValue(DynamicObject->MoverParameters->Vel);
         return true;
       }
      else return false;
    }
   else return false;
  return false;
}

//McZapkie-310302
//wczytywanie pliku z danymi multimedialnymi (dzwieki, kontrolki, kabiny)
bool __fastcall TTrain::LoadMMediaFile(AnsiString asFileName)
{
    double dSDist;
    TFileStream *fs;
    fs= new TFileStream(asFileName , fmOpenRead	| fmShareCompat	);
    AnsiString str= "";
//    DecimalSeparator= '.';
    int size= fs->Size;
    str.SetLength(size);
    fs->Read(str.c_str(),size);
    str+= "";
    delete fs;
    TQueryParserComp *Parser;
    Parser= new TQueryParserComp(NULL);
    Parser->TextToParse= str;
//    Parser->LoadStringToParse(asFile);
    Parser->First();
    str="";
    dsbPneumaticSwitch= TSoundsManager::GetFromName("silence1.wav");
    while ((!Parser->EndOfFile) && (str!=AnsiString("internaldata:")))
    {
        str= Parser->GetNextSymbol().LowerCase();
    }
    if (str==AnsiString("internaldata:"))
    {
     while (!Parser->EndOfFile)
      {
        str= Parser->GetNextSymbol().LowerCase();
//SEKCJA DZWIEKOW
        if (str==AnsiString("ctrl:"))                    //nastawnik:
         {
          str= Parser->GetNextSymbol().LowerCase();
          dsbNastawnik= TSoundsManager::GetFromName(str.c_str());
         }
        else
        if (str==AnsiString("buzzer:"))                    //bzyczek shp:
         {
          str= Parser->GetNextSymbol().LowerCase();
          dsbBuzzer= TSoundsManager::GetFromName(str.c_str());
         }
        else
        if (str==AnsiString("tachoclock:"))                 //cykanie rejestratora:
         {
          str= Parser->GetNextSymbol().LowerCase();
          dsbHasler= TSoundsManager::GetFromName(str.c_str());
         }
        else
        if (str==AnsiString("switch:"))                   //przelaczniki:
         {
          str= Parser->GetNextSymbol().LowerCase();
          dsbSwitch= TSoundsManager::GetFromName(str.c_str());
         }
        else
        if (str==AnsiString("pneumaticswitch:"))                   //stycznik EP:
         {
          str= Parser->GetNextSymbol().LowerCase();
          dsbPneumaticSwitch= TSoundsManager::GetFromName(str.c_str());
         }
        else
        if (str==AnsiString("relay:"))                   //styczniki itp:
         {
          str= Parser->GetNextSymbol().LowerCase();
          dsbRelay= TSoundsManager::GetFromName(str.c_str());
          dsbWejscie_na_bezoporow= TSoundsManager::GetFromName("wejscie_na_bezoporow.wav");
          dsbWescie_na_drugi_uklad= TSoundsManager::GetFromName("wescie_na_drugi_uklad.wav");
         }
        else
        if (str==AnsiString("pneumaticrelay:"))           //wylaczniki pneumatyczne:
         {
          str= Parser->GetNextSymbol().LowerCase();
          dsbPneumaticRelay= TSoundsManager::GetFromName(str.c_str());
         }
        else
        if (str==AnsiString("couplerattach:"))           //laczenie:
         {
          str= Parser->GetNextSymbol().LowerCase();
          dsbCouplerAttach= TSoundsManager::GetFromName(str.c_str());
          dsbCouplerStretch= TSoundsManager::GetFromName("en57_couplerstretch.wav"); //McZapkie-090503: PROWIZORKA!!!
         }
        else
        if (str==AnsiString("couplerdetach:"))           //rozlaczanie:
         {
          str= Parser->GetNextSymbol().LowerCase();
          dsbCouplerDetach= TSoundsManager::GetFromName(str.c_str());
          dsbBufferClamp= TSoundsManager::GetFromName("en57_bufferclamp.wav"); //McZapkie-090503: PROWIZORKA!!!
         }
        else
        if (str==AnsiString("ignition:"))           //rozlaczanie:
         {
          str= Parser->GetNextSymbol().LowerCase();
          dsbDieselIgnition= TSoundsManager::GetFromName(str.c_str());
         }
        else
        if (str==AnsiString("brakesound:"))                    //hamowanie zwykle:
         {
          str= Parser->GetNextSymbol();
          rsBrake.Init(str.c_str(),-1,0,0,0);
          rsBrake.AM=Parser->GetNextSymbol().ToDouble()/(1+DynamicObject->MoverParameters->MaxBrakeForce*1000);
          rsBrake.AA=Parser->GetNextSymbol().ToDouble();
          rsBrake.FM=Parser->GetNextSymbol().ToDouble()/(1+DynamicObject->MoverParameters->Vmax);
          rsBrake.FA=Parser->GetNextSymbol().ToDouble();
         }
        else
        if (str==AnsiString("slipperysound:"))                    //sanie:
         {
          str= Parser->GetNextSymbol();
          rsSlippery.Init(str.c_str(),-1,0,0,0);
          rsSlippery.AM=Parser->GetNextSymbol().ToDouble()/(1+DynamicObject->MoverParameters->Vmax);
          rsSlippery.AA=Parser->GetNextSymbol().ToDouble();
          rsSlippery.FM=0.0;
          rsSlippery.FA=1.0;
         }
        else
        if (str==AnsiString("airsound:"))                    //syk:
         {
          str= Parser->GetNextSymbol();
          rsHiss.Init(str.c_str(),-1,0,0,0);
          rsHiss.AM=Parser->GetNextSymbol().ToDouble();
          rsHiss.AA=Parser->GetNextSymbol().ToDouble();
          rsHiss.FM=0.0;
          rsHiss.FA=1.0;
         }
        else
        if (str==AnsiString("fadesound:"))                    //syk:
         {
          str= Parser->GetNextSymbol();
          rsFadeSound.Init(str.c_str(),-1,0,0,0);
          rsFadeSound.AM=1.0;
          rsFadeSound.AA=1.0;
          rsFadeSound.FM=1.0;
          rsFadeSound.FA=1.0;
         }
        else
        if (str==AnsiString("localbrakesound:"))                    //syk:
         {
          str= Parser->GetNextSymbol();
          rsSBHiss.Init(str.c_str(),-1,0,0,0);
          rsSBHiss.AM=Parser->GetNextSymbol().ToDouble();
          rsSBHiss.AA=Parser->GetNextSymbol().ToDouble();
          rsSBHiss.FM=0.0;
          rsSBHiss.FA=1.0;
         }
        else
        if (str==AnsiString("runningnoise:"))                    //szum podczas jazdy:
         {
          str= Parser->GetNextSymbol();
          rsRunningNoise.Init(str.c_str(),-1,0,0,0);
          rsRunningNoise.AM=Parser->GetNextSymbol().ToDouble()/(1+DynamicObject->MoverParameters->Vmax);
          rsRunningNoise.AA=Parser->GetNextSymbol().ToDouble();
          rsRunningNoise.FM=Parser->GetNextSymbol().ToDouble()/(1+DynamicObject->MoverParameters->Vmax);
          rsRunningNoise.FA=Parser->GetNextSymbol().ToDouble();
         }
        else
        if (str==AnsiString("engageslippery:"))                    //tarcie tarcz sprzegla:
         {
          str= Parser->GetNextSymbol();
          rsEngageSlippery.Init(str.c_str(),-1,0,0,0);
          rsEngageSlippery.AM=Parser->GetNextSymbol().ToDouble();
          rsEngageSlippery.AA=Parser->GetNextSymbol().ToDouble();
          rsEngageSlippery.FM=Parser->GetNextSymbol().ToDouble()/(1+DynamicObject->MoverParameters->nmax);
          rsEngageSlippery.FA=Parser->GetNextSymbol().ToDouble();
         }
        else
        if (str==AnsiString("mechspring:"))                    //parametry bujania kamery:
         {
          str= Parser->GetNextSymbol();
          MechSpring.Init(0,str.ToDouble(),Parser->GetNextSymbol().ToDouble());
          fMechSpringX= Parser->GetNextSymbol().ToDouble();
          fMechSpringY= Parser->GetNextSymbol().ToDouble();
          fMechSpringZ= Parser->GetNextSymbol().ToDouble();
          fMechMaxSpring= Parser->GetNextSymbol().ToDouble();
          fMechRoll= Parser->GetNextSymbol().ToDouble();
          fMechPitch= Parser->GetNextSymbol().ToDouble();
         }
                 else
        if (str==AnsiString("pantographup:"))           //podniesienie patyka:
         {
          str= Parser->GetNextSymbol().LowerCase();
          dsbPantUp= TSoundsManager::GetFromName(str.c_str());
         }
                 else
        if (str==AnsiString("pantographdown:"))           //podniesienie patyka:
         {
          str= Parser->GetNextSymbol().LowerCase();
          dsbPantDown= TSoundsManager::GetFromName(str.c_str());
         }
                 else
        if (str==AnsiString("doorclose:"))           //zamkniecie drzwi:
         {
          str= Parser->GetNextSymbol().LowerCase();
          dsbDoorClose= TSoundsManager::GetFromName(str.c_str());
         }
                 else
        if (str==AnsiString("dooropen:"))           //wotwarcie drzwi:
         {
          str= Parser->GetNextSymbol().LowerCase();
          dsbDoorOpen= TSoundsManager::GetFromName(str.c_str());
          dsbCouplerStretch= TSoundsManager::GetFromName("en57_couplers.wav");
          dsbBufferClamp= TSoundsManager::GetFromName("en57_couplers.wav");
         }
      }

    }
    else return false; //nie znalazl sekcji internal
   if (!InitializeCab(DynamicObject->MoverParameters->ActiveCab,asFileName)) //zle zainicjowana kabina
    return false;
   else
    {
      if (DynamicObject->Controller==Humandriver)
       DynamicObject->bDisplayCab= true;             //McZapkie-030303: mozliwosc wyswietlania kabiny, w przyszlosci dac opcje w mmd
      return true;
    }
}


bool TTrain::InitializeCab(int NewCabNo, AnsiString asFileName)
{
    bool parse=false;
    double dSDist;
    TFileStream *fs;
    fs= new TFileStream(asFileName , fmOpenRead	| fmShareCompat	);
    AnsiString str= "";
//    DecimalSeparator= '.';
    int size= fs->Size;
    str.SetLength(size);
    fs->Read(str.c_str(),size);
    str+= "";
    delete fs;
    TQueryParserComp *Parser;
    Parser= new TQueryParserComp(NULL);
    Parser->TextToParse= str;
//    Parser->LoadStringToParse(asFile);
    Parser->First();
    str="";
    int cabindex=0;
    switch (NewCabNo)
     {
       case -1: cabindex=2;
       break;
       case 1: cabindex=1;
       break;
       case 0: cabindex=0;
     }
    AnsiString cabstr=AnsiString("cab")+cabindex+AnsiString("definition:");
    while ((!Parser->EndOfFile) && (AnsiCompareStr(str,cabstr)!=0))
    {
        str= Parser->GetNextSymbol().LowerCase();
    }
    if (AnsiCompareStr(str,cabstr)==0)
    {
     Cabine[cabindex].Load(Parser);
     str= Parser->GetNextSymbol().LowerCase();
     if (AnsiCompareStr(AnsiString("driver")+cabindex+AnsiString("pos:"),str)==0)     //pozycja poczatkowa maszynisty
      {
        pMechOffset.x=Parser->GetNextSymbol().ToDouble();
        pMechOffset.y=Parser->GetNextSymbol().ToDouble();
        pMechOffset.z=Parser->GetNextSymbol().ToDouble();
        pMechSittingPosition.x=pMechOffset.x;
        pMechSittingPosition.y=pMechOffset.y;
        pMechSittingPosition.z=pMechOffset.z;
      }
     //ABu: pozycja siedzaca mechanika
     str= Parser->GetNextSymbol().LowerCase();
     if (AnsiCompareStr(AnsiString("driver")+cabindex+AnsiString("sitpos:"),str)==0)     //ABu 180404 pozycja siedzaca maszynisty
      {
        pMechSittingPosition.x=Parser->GetNextSymbol().ToDouble();
        pMechSittingPosition.y=Parser->GetNextSymbol().ToDouble();
        pMechSittingPosition.z=Parser->GetNextSymbol().ToDouble();
        parse=true;
      }
      else
      {
        parse=false;
      }
     while (!Parser->EndOfFile)
      {
        //ABu: wstawione warunki, wczesniej tylko to:
        //   str= Parser->GetNextSymbol().LowerCase();
        if(parse)
        {
           str= Parser->GetNextSymbol().LowerCase();
        }
        else
        {
           parse=true;
        }

    //inicjacja kabiny
        if (AnsiCompareStr(AnsiString("cab")+cabindex+AnsiString("model:"),str)==0)     //model kabiny
         {
           str= Parser->GetNextSymbol().LowerCase();
           if (str!=AnsiString("none"))
            {
              str= DynamicObject->asBaseDir+str;
              Global::asCurrentTexturePath= DynamicObject->asBaseDir;         //biezaca sciezka do tekstur to dynamic/...
              DynamicObject->mdKabina= TModelsManager::GetModel(str.c_str());                //szukaj kabine jako oddzielny model
              Global::asCurrentTexturePath= AnsiString(szDefaultTexturePath); //z powrotem defaultowa sciezka do tekstur
             }
           else
            DynamicObject->mdKabina= DynamicObject->mdModel;   //McZapkie-170103: szukaj elementy kabiny w glownym modelu
           ActiveUniversal4=false;
           MainCtrlGauge.Clear();
           MainCtrlActGauge.Clear();           
           ScndCtrlGauge.Clear();
           DirKeyGauge.Clear();
           BrakeCtrlGauge.Clear();
           LocalBrakeGauge.Clear();
           ManualBrakeGauge.Clear();
           BrakeProfileCtrlGauge.Clear();
           MaxCurrentCtrlGauge.Clear();
           MainOffButtonGauge.Clear();
           MainOnButtonGauge.Clear();
           SecurityResetButtonGauge.Clear();
           ReleaserButtonGauge.Clear();
           AntiSlipButtonGauge.Clear();
           HornButtonGauge.Clear();
           NextCurrentButtonGauge.Clear();
           Universal1ButtonGauge.Clear();
           Universal2ButtonGauge.Clear();
           Universal3ButtonGauge.Clear();
           Universal4ButtonGauge.Clear();
           FuseButtonGauge.Clear();
           StLinOffButtonGauge.Clear();
           DoorLeftButtonGauge.Clear();
           DoorRightButtonGauge.Clear();
           DepartureSignalButtonGauge.Clear();
           CompressorButtonGauge.Clear();
           ConverterButtonGauge.Clear();
           PantFrontButtonGauge.Clear();
           PantRearButtonGauge.Clear();
           PantFrontButtonOffGauge.Clear();
           PantAllDownButtonGauge.Clear();
           VelocityGauge.Clear();
           I1Gauge.Clear();
           I2Gauge.Clear();
           I3Gauge.Clear();
           ItotalGauge.Clear();
           CylHamGauge.Clear();
           PrzGlGauge.Clear();
           ZbGlGauge.Clear();
           ZbWyrGauge.Clear();

           VelocityGaugeB.Clear();
           I1GaugeB.Clear();
           I2GaugeB.Clear();
           I3GaugeB.Clear();
           ItotalGaugeB.Clear();
           CylHamGaugeB.Clear();
           PrzGlGaugeB.Clear();
           ZbGlGaugeB.Clear();
           ZbWyrGaugeB.Clear();

           I1BGauge.Clear();
           I2BGauge.Clear();
           I3BGauge.Clear();
           ItotalBGauge.Clear();

           ClockMInd.Clear();
           ClockHInd.Clear();
           EngineVoltage.Clear();
           HVoltageGauge.Clear();
           LVoltageGauge.Clear();
           enrot1mGauge.Clear();
           enrot2mGauge.Clear();
           enrot3mGauge.Clear();
           engageratioGauge.Clear();
           maingearstatusGauge.Clear();
           IgnitionKeyGauge.Clear();
           btLampkaPoslizg.Clear();
           btLampkaStyczn.Clear();
           btLampkaNadmPrzetw.Clear();
           btLampkaPrzekRozn.Clear();
           btLampkaPrzekRoznPom.Clear();
           btLampkaNadmSil.Clear();
           btLampkaUkrotnienie.Clear();
           btLampkaHamPosp.Clear();
           btLampkaWylSzybki.Clear();
           btLampkaNadmWent.Clear();
           btLampkaNadmSpr.Clear();
           btLampkaOpory.Clear();
           btLampkaBezoporowa.Clear();
           btLampkaBezoporowaB.Clear();
           btLampkaMaxSila.Clear();
           btLampkaPrzekrMaxSila.Clear();
           btLampkaRadio.Clear();
           btLampkaHamulecReczny.Clear();
           btLampkaWysRozr.Clear();
           btLampkaBlokadaDrzwi.Clear();
           btLampkaUniversal3.Clear();
           btLampkaWentZaluzje.Clear();
           btLampkaOgrzewanieSkladu.Clear();
           btLampkaSHP.Clear();
           btLampkaCzuwaka.Clear();
           btLampkaDoorLeft.Clear();
           btLampkaDoorRight.Clear();
           btLampkaDepartureSignal.Clear();
           btLampkaRezerwa.Clear();
           btLampkaBocznikI.Clear();
                      btLampkaBoczniki.Clear();
           btLampkaBocznikII.Clear();           
           btLampkaBocznikIII.Clear();
           btLampkaBocznikIV.Clear();                      
           btLampkaHamienie.Clear();
           btLampkaSprezarka.Clear();
           btLampkaSprezarkaB.Clear();
           btLampkaNapNastHam.Clear();
           btLampkaJazda.Clear();
           btLampkaStycznB.Clear();
           btLampkaHamowanie1zes.Clear();
           btLampkaHamowanie2zes.Clear();
           btLampkaNadmPrzetwB.Clear();
           btLampkaWylSzybkiB.Clear();
           LeftLightButtonGauge.Clear();
           RightLightButtonGauge.Clear();
           UpperLightButtonGauge.Clear();
           LeftEndLightButtonGauge.Clear();
           RightEndLightButtonGauge.Clear();
         }
        else
    //SEKCJA REGULATOROW
        if (str==AnsiString("mainctrl:"))                    //nastawnik
         {
           if (!DynamicObject->mdKabina)
            {
             Error("Cab not initialised!");
            }
           else
            {
             MainCtrlGauge.Load(Parser,DynamicObject->mdKabina);
            }
         }
        else
        if (str==AnsiString("mainctrlact:"))                 //zabek pozycji aktualnej
         {
           MainCtrlActGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else

        if (str==AnsiString("scndctrl:"))                    //bocznik
         {
           ScndCtrlGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("dirkey:"))                    //klucz kierunku
         {
           DirKeyGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("brakectrl:"))                    //hamulec zasadniczy
         {
           BrakeCtrlGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("localbrake:"))                    //hamulec pomocniczy
         {
           LocalBrakeGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("manualbrake:"))                    //hamulec reczny
         {
           ManualBrakeGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
//sekcja przelacznikow obrotowych
        if (str==AnsiString("brakeprofile_sw:"))                    //przelacznik tow/osob
         {
           BrakeProfileCtrlGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("maxcurrent_sw:"))                    //przelacznik rozruchu
         {
           MaxCurrentCtrlGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
    //SEKCJA przyciskow sprezynujacych
        if (str==AnsiString("main_off_bt:"))                    //przycisk wylaczajacy (w EU07 wyl szybki czerwony)
         {
           MainOffButtonGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("main_on_bt:"))                    //przycisk wlaczajacy (w EU07 wyl szybki zielony)
         {
           MainOnButtonGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("security_reset_bt:"))             //przycisk zbijajacy SHP/czuwak
         {
           SecurityResetButtonGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("releaser_bt:"))                   //przycisk odluzniacza
         {
           ReleaserButtonGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("releaser_bt:"))                   //przycisk odluzniacza
         {
           ReleaserButtonGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("antislip_bt:"))                   //przycisk antyposlizgowy
         {
           AntiSlipButtonGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("horn_bt:"))                       //dzwignia syreny
         {
           HornButtonGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("fuse_bt:"))                       //bezp. nadmiarowy
         {
           FuseButtonGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("stlinoff_bt:"))                       //st. liniowe
         {
           StLinOffButtonGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("door_left_sw:"))                       //drzwi lewe
         {
           DoorLeftButtonGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("door_right_sw:"))                       //drzwi prawe
         {
           DoorRightButtonGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("departure_signal_bt:"))                       //sygnal odjazdu
         {
           DepartureSignalButtonGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("upperlight_sw:"))                       //swiatlo
         {
           UpperLightButtonGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("leftlight_sw:"))                       //swiatlo
         {
           LeftLightButtonGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("rightlight_sw:"))                       //swiatlo
         {
           RightLightButtonGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("leftend_sw:"))                       //swiatlo
         {
           LeftEndLightButtonGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("rightend_sw:"))                       //swiatlo
         {
           RightEndLightButtonGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("compressor_sw:"))                       //sprezarka
         {
           CompressorButtonGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("converter_sw:"))                       //przetwornica
         {
           ConverterButtonGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("converteroff_sw:"))                       //przetwornica wyl
         {
           ConverterOffButtonGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("main_sw:"))                       //wyl szybki (ezt)
         {
           MainButtonGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("radio_sw:"))                       //radio
         {
           RadioButtonGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("pantfront_sw:"))                       //patyk przedni
         {
           PantFrontButtonGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("pantrear_sw:"))                       //patyk tylni
         {
           PantRearButtonGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("pantfrontoff_sw:"))                       //patyk przedni w dol
         {
           PantFrontButtonOffGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("pantalloff_sw:"))                       //patyk przedni w dol
         {
           PantAllDownButtonGauge.Load(Parser,DynamicObject->mdKabina);
         }
        if (str==AnsiString("trainheating_sw:"))                       //grzanie skladu
         {
           TrainHeatingButtonGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("signalling_sw:"))                       //Sygnalizacja hamowania
         {
           SignallingButtonGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("door_signalling_sw:"))                       //Sygnalizacja blokady drzwi
         {
           DoorSignallingButtonGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("nextcurrent_sw:"))                       //grzanie skladu
         {
           NextCurrentButtonGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        //ABu 090305: uniwersalne przyciski lub inne rzeczy
        if (str==AnsiString("universal1:"))
         {
           Universal1ButtonGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("universal2:"))
         {
           Universal2ButtonGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("universal3:"))
         {
           Universal3ButtonGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("universal4:"))
         {
           Universal4ButtonGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
//SEKCJA WSKAZNIKOW
        if (str==AnsiString("tachometer:"))                    //predkosciomierz
         {
           VelocityGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("hvcurrent1:"))                    //1szy amperomierz
         {
            I1Gauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("hvcurrent2:"))                    //2gi amperomierz
         {
            I2Gauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("hvcurrent3:"))                    //3ci amperomierz
         {
            I3Gauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("hvcurrent:"))                     //amperomierz calkowitego pradu
         {
            ItotalGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("brakepress:"))                    //manometr cylindrow hamulcowych
         {
            CylHamGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("pipepress:"))                    //manometr przewodu hamulcowego
         {
            PrzGlGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("limpipepress:"))                    //manometr przewodu hamulcowego
         {
            ZbWyrGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("compressor:"))                    //manometr sprezarki/zbiornika glownego
         {
            ZbGlGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        //*************************************************************
        //Sekcja zdublowanych wskaznikow dla dwustronnych kabin
        if (str==AnsiString("tachometerb:"))                    //predkosciomierz
         {
           VelocityGaugeB.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("hvcurrent1b:"))                    //1szy amperomierz
         {
            I1GaugeB.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("hvcurrent2b:"))                    //2gi amperomierz
         {
            I2GaugeB.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("hvcurrent3b:"))                    //3ci amperomierz
         {
            I3GaugeB.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("hvcurrentb:"))                     //amperomierz calkowitego pradu
         {
            ItotalGaugeB.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("brakepressb:"))                    //manometr cylindrow hamulcowych
         {
            CylHamGaugeB.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("pipepressb:"))                    //manometr przewodu hamulcowego
         {
            PrzGlGaugeB.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("limpipepressb:"))                    //manometr przewodu hamulcowego
         {
            ZbWyrGaugeB.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("compressorb:"))                    //manometr sprezarki/zbiornika glownego
         {
            ZbGlGaugeB.Load(Parser,DynamicObject->mdKabina);
         }
        else
        //*************************************************************
        //yB - dla drugiej sekcji
        if (str==AnsiString("hvbcurrent1:"))                    //1szy amperomierz
         {
            I1BGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("hvbcurrent2:"))                    //2gi amperomierz
         {
            I2BGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("hvbcurrent3:"))                    //3ci amperomierz
         {
            I3BGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("hvbcurrent:"))                     //amperomierz calkowitego pradu
         {
            ItotalBGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        //*************************************************************
        if (str==AnsiString("clock:"))                    //manometr sprezarki/zbiornika glownego
         {
          if (Parser->GetNextSymbol()==AnsiString("analog"))
           {
            //McZapkie-300302: zegarek
            ClockMInd.Init(DynamicObject->mdKabina->GetFromName("ClockMhand"),gt_Rotate,0.01667,0,0);
            ClockHInd.Init(DynamicObject->mdKabina->GetFromName("ClockHhand"),gt_Rotate,0.08333,0,0);
           }
         }
        else
        if (str==AnsiString("evoltage:"))                    //woltomierz napiecia silnikow
         {
            EngineVoltage.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("hvoltage:"))                    //woltomierz wysokiego napiecia
         {
            HVoltageGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("lvoltage:"))                    //woltomierz niskiego napiecia
         {
            LVoltageGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("enrot1m:"))                    //obrotomierz
         {
            enrot1mGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("enrot2m:"))
         {
            enrot2mGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("enrot3m:"))
         {
            enrot3mGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("otemp:"))                    //obrotomierz
         {
            OTempGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("opress:"))
         {
            OPressGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("wtemp:"))
         {
            WTempGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("engageratio:"))   //np. cisnienie sterownika przegla
         {
            engageratioGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("maingearstatus:"))   //np. cisnienie sterownika skrzyni biegow
         {
            maingearstatusGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("ignitionkey:"))   //np. cisnienie sterownika skrzyni biegow
         {
            IgnitionKeyGauge.Load(Parser,DynamicObject->mdKabina);
         }
        else
//SEKCJA LAMPEK
        if (str==AnsiString("i-maxft:"))
        {
           btLampkaMaxSila.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("i-maxftt:"))
         {
           btLampkaPrzekrMaxSila.Load(Parser,DynamicObject->mdKabina);
         }
        else
       if (str==AnsiString("i-radio:"))
         {
           btLampkaRadio.Load(Parser,DynamicObject->mdKabina);
         }
        else
      if (str==AnsiString("i-manual_brake:"))
         {
           btLampkaHamulecReczny.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("i-door_blocked:"))
         {
           btLampkaBlokadaDrzwi.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("i-slippery:"))
        {
           btLampkaPoslizg.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("i-contactors:"))
         {
           btLampkaStyczn.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("i-conv_ovld:"))
         {
           btLampkaNadmPrzetw.Load(Parser,DynamicObject->mdKabina);
         }
       else
        if (str==AnsiString("i-diff_relay:"))
         {
           btLampkaPrzekRozn.Load(Parser,DynamicObject->mdKabina);
         }
       else
        if (str==AnsiString("i-diff_relay2:"))
         {
           btLampkaPrzekRoznPom.Load(Parser,DynamicObject->mdKabina);
         }
       else
        if (str==AnsiString("i-motor_ovld:"))
         {
           btLampkaNadmSil.Load(Parser,DynamicObject->mdKabina);
         }
       else
         if (str==AnsiString("i-train_controll:"))
         {
           btLampkaUkrotnienie.Load(Parser,DynamicObject->mdKabina);
         }
       else
         if (str==AnsiString("i-brake_delay_r:"))
         {
           btLampkaHamPosp.Load(Parser,DynamicObject->mdKabina);
         }
       else
        if (str==AnsiString("i-mainbreaker:"))
         {
           btLampkaWylSzybki.Load(Parser,DynamicObject->mdKabina);
         }
       else
        if (str==AnsiString("i-vent_ovld:"))
         {
           btLampkaNadmWent.Load(Parser,DynamicObject->mdKabina);
         }
       else
        if (str==AnsiString("i-comp_ovld:"))
         {
           btLampkaNadmSpr.Load(Parser,DynamicObject->mdKabina);
         }
       else
        if (str==AnsiString("i-resistors:"))
         {
           btLampkaOpory.Load(Parser,DynamicObject->mdKabina);
         }
       else
        if (str==AnsiString("i-no_resistors:"))
         {
           btLampkaBezoporowa.Load(Parser,DynamicObject->mdKabina);
         }
       else
         if (str==AnsiString("i-no_resistors_b:"))
         {
           btLampkaBezoporowaB.Load(Parser,DynamicObject->mdKabina);
         }
       else
        if (str==AnsiString("i-highcurrent:"))
         {
           btLampkaWysRozr.Load(Parser,DynamicObject->mdKabina);
         }
       else
        if (str==AnsiString("i-universal3:"))
         {
           btLampkaUniversal3.Load(Parser,DynamicObject->mdKabina);
           LampkaUniversal3_typ=0;
         }
       else
       if (str==AnsiString("i-universal3_M:"))
         {
           btLampkaUniversal3.Load(Parser,DynamicObject->mdKabina);
           LampkaUniversal3_typ=1;
         }
       else
       if (str==AnsiString("i-universal3_C:"))
         {
           btLampkaUniversal3.Load(Parser,DynamicObject->mdKabina);
           LampkaUniversal3_typ=2;
         }
       else
        if (str==AnsiString("i-vent_trim:"))
         {
           btLampkaWentZaluzje.Load(Parser,DynamicObject->mdKabina);
         }
       else
        if (str==AnsiString("i-trainheating:"))
         {
           btLampkaOgrzewanieSkladu.Load(Parser,DynamicObject->mdKabina);
         }
       else
        if (str==AnsiString("i-security_aware:"))
         {
           btLampkaCzuwaka.Load(Parser,DynamicObject->mdKabina);
         }
       else
        if (str==AnsiString("i-security_cabsignal:"))
         {
           btLampkaSHP.Load(Parser,DynamicObject->mdKabina);
         }
       else
        if (str==AnsiString("i-door_left:"))
         {
           btLampkaDoorLeft.Load(Parser,DynamicObject->mdKabina);
         }
       else
        if (str==AnsiString("i-door_right:"))
         {
           btLampkaDoorRight.Load(Parser,DynamicObject->mdKabina);
         }
       else
        if (str==AnsiString("i-departure_signal:"))
         {
           btLampkaDepartureSignal.Load(Parser,DynamicObject->mdKabina);
         }
       else
        if (str==AnsiString("i-reserve:"))
         {
           btLampkaRezerwa.Load(Parser,DynamicObject->mdKabina);
         }
       else
        if (str==AnsiString("i-scnd1:"))
         {
           btLampkaBocznikI.Load(Parser,DynamicObject->mdKabina);
         }
       else
       if (str==AnsiString("i-scnd:"))
         {
           btLampkaBoczniki.Load(Parser,DynamicObject->mdKabina);
         }
       else
        if (str==AnsiString("i-scnd2:"))
         {
           btLampkaBocznikII.Load(Parser,DynamicObject->mdKabina);
         }
       else
        if (str==AnsiString("i-scnd3:"))
         {
           btLampkaBocznikIII.Load(Parser,DynamicObject->mdKabina);
         }
       else
        if (str==AnsiString("i-scnd4:"))
         {
           btLampkaBocznikIV.Load(Parser,DynamicObject->mdKabina);
         }         
       else
        if (str==AnsiString("i-braking:"))
         {
           btLampkaHamienie.Load(Parser,DynamicObject->mdKabina);
         }
       else
        if (str==AnsiString("i-braking-ezt:"))
         {
           btLampkaHamowanie1zes.Load(Parser,DynamicObject->mdKabina);
         }
       else
        if (str==AnsiString("i-braking-ezt2:"))
         {
           btLampkaHamowanie2zes.Load(Parser,DynamicObject->mdKabina);
         }
       else
        if (str==AnsiString("i-compressor:"))
         {
           btLampkaSprezarka.Load(Parser,DynamicObject->mdKabina);
         }
        if (str==AnsiString("i-compressorb:"))
         {
           btLampkaSprezarkaB.Load(Parser,DynamicObject->mdKabina);
         }
       else
        if (str==AnsiString("i-voltbrake:"))
         {
           btLampkaNapNastHam.Load(Parser,DynamicObject->mdKabina);
         }

        if (str==AnsiString("i-mainbreakerb:"))
         {
           btLampkaWylSzybkiB.Load(Parser,DynamicObject->mdKabina);
         }
       else
        if (str==AnsiString("i-resistorsb:"))
         {
           btLampkaOporyB.Load(Parser,DynamicObject->mdKabina);
         }
       else
        if (str==AnsiString("i-contactorsb:"))
         {
           btLampkaStycznB.Load(Parser,DynamicObject->mdKabina);
         }
        else
        if (str==AnsiString("i-conv_ovldb:"))
         {
           btLampkaNadmPrzetwB.Load(Parser,DynamicObject->mdKabina);
         }
//    btLampkaUnknown.Init("unknown",mdKabina,false);
       }
    }
   else return false;
   //ABu 050205: tego wczesniej nie bylo:
   delete Parser;
   if (!DynamicObject->mdKabina && !AnsiCompareStr(str,AnsiString("none"))) return false;
    else
     {
     return true;
     }
}



//---------------------------------------------------------------------------

#pragma package(smart_init)