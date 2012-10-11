//---------------------------------------------------------------------------
/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak, Maciej Czapkiewicz and others

*/

#include    "system.hpp"
#include    "classes.hpp"
#pragma hdrstop

#include "Train.h"
#include "MdlMngr.h"
#include "Globals.h"
#include "Timer.h"
#include "Driver.h"
#include "Console.h"
//---------------------------------------------------------------------------

#pragma package(smart_init)

using namespace Timer;

__fastcall TCab::TCab()
{
  CabPos1.x=-1.0; CabPos1.y=1.0; CabPos1.z=1.0;
  CabPos2.x=1.0; CabPos2.y=1.0; CabPos2.z=-1.0;
  bEnabled=false;
  bOccupied=true;
  dimm_r=dimm_g=dimm_b=1;
  intlit_r=intlit_g=intlit_b=0;
  intlitlow_r=intlitlow_g=intlitlow_b=0;
}

void __fastcall TCab::Init(double Initx1,double Inity1,double Initz1,double Initx2,double Inity2,double Initz2,bool InitEnabled,bool InitOccupied)
{
  CabPos1.x=Initx1; CabPos1.y=Inity1; CabPos1.z=Initz1;
  CabPos2.x=Initx2; CabPos2.y=Inity2; CabPos2.z=Initz2;
  bEnabled=InitEnabled;
  bOccupied=InitOccupied;
}

void __fastcall TCab::Load(TQueryParserComp *Parser)
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
    fTachoVelocity=0;
    fTachoCount=0;
    fPPress=fNPress=0;

    //asMessage="";
    fMechCroach=0.25;
    pMechShake=vector3(0,0,0);
    vMechMovement=vector3(0,0,0);
    pMechOffset=vector3(0,0,0);
 fBlinkTimer=0;
 fHaslerTimer=0;
    keybrakecount=0;
    DynamicObject=NULL;
    iCabLightFlag=0;
    pMechSittingPosition=vector3(0,0,0); //ABu: 180404
    LampkaUniversal3_st=false; //ABu: 030405
 dsbNastawnikJazdy=NULL;
 dsbNastawnikBocz=NULL;
 dsbRelay=NULL;
 dsbPneumaticRelay=NULL;
 dsbSwitch=NULL;
 dsbPneumaticSwitch=NULL;
 dsbReverserKey=NULL; //hunter-121211
 dsbCouplerAttach=NULL;
 dsbCouplerDetach=NULL;
 dsbDieselIgnition=NULL;
 dsbDoorClose=NULL;
 dsbDoorOpen=NULL;
 dsbPantUp=NULL;
 dsbPantDown=NULL;
 dsbWejscie_na_bezoporow=NULL;
 dsbWejscie_na_drugi_uklad=NULL; //hunter-081211: poprawka literowki
 dsbHasler=NULL;
 dsbBuzzer=NULL;
 dsbSlipAlarm=NULL; //Bombardier 011010: alarm przy poslizgu dla 181/182
 dsbCouplerStretch=NULL;
 dsbBufferClamp=NULL;
}

__fastcall TTrain::~TTrain()
{
 if (DynamicObject)
  if (DynamicObject->Mechanik)
   DynamicObject->Mechanik->TakeControl(true); //likwidacja kabiny wymaga przejêcia przez AI
}

bool __fastcall TTrain::Init(TDynamicObject *NewDynamicObject)
{//powi¹zanie rêcznego sterowania kabin¹ z pojazdem
 //Global::pUserDynamic=NewDynamicObject; //pojazd renderowany bez trzêsienia
 DynamicObject=NewDynamicObject;
 if (DynamicObject->Mechanik==NULL)
  return false;
 //if (DynamicObject->Mechanik->AIControllFlag==AIdriver)
 // return false;
 DynamicObject->MechInside=true;

/*    iPozSzereg=28;
    for (int i=1; i<DynamicObject->MoverParameters->MainCtrlPosNo; i++)
     {
      if (DynamicObject->MoverParameters->RList[i].Bn>1)
       {
        iPozSzereg=i-1;
        i=DynamicObject->MoverParameters->MainCtrlPosNo+1;
       }
     }
*/
 MechSpring.Init(0,500);
 vMechVelocity=vector3(0,0,0);
 pMechOffset=vector3(-0.4,3.3,5.5);
 fMechCroach=0.5;
 fMechSpringX=1;
 fMechSpringY=0.1;
 fMechSpringZ=0.1;
 fMechMaxSpring=0.15;
 fMechRoll=0.05;
 fMechPitch=0.1;

 if (!LoadMMediaFile(DynamicObject->asBaseDir+DynamicObject->MoverParameters->TypeName+".mmd"))
  return false;

//McZapkie: w razie wykolejenia
//    dsbDerailment=TSoundsManager::GetFromName("derail.wav");
//McZapkie: jazda luzem:
//    dsbRunningNoise=TSoundsManager::GetFromName("runningnoise.wav");

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
  //Ra: taka proteza - przes³anie kierunku do cz³onów connected
  if (DynamicObject->MoverParameters->ActiveDir>0)
  {//by³o do przodu
   DynamicObject->MoverParameters->DirectionBackward();
   DynamicObject->MoverParameters->DirectionForward();
  }
  else if (DynamicObject->MoverParameters->ActiveDir<0)
  {DynamicObject->MoverParameters->DirectionForward();
   DynamicObject->MoverParameters->DirectionBackward();
  }
  return true;
}


void __fastcall TTrain::OnKeyPress(int cKey)
{

  bool isEztOer;
  isEztOer=(DynamicObject->MoverParameters->TrainType==dt_EZT)&&(DynamicObject->MoverParameters->Mains)&&(DynamicObject->MoverParameters->BrakeSubsystem==Oerlikon)&&(DynamicObject->MoverParameters->ActiveDir!=0);

  if (GetAsyncKeyState(VK_SHIFT)<0)
   {//wciœniêty [Shift]
      if (cKey==Global::Keys[k_IncMainCtrlFAST])   //McZapkie-200702: szybkie przelaczanie na poz. bezoporowa
      {
          if (DynamicObject->MoverParameters->IncMainCtrl(2))
          {
              dsbNastawnikJazdy->SetCurrentPosition(0);
              dsbNastawnikJazdy->Play(0,0,0);
          }
      }
      else
      if (cKey==Global::Keys[k_DecMainCtrlFAST])
          if (DynamicObject->MoverParameters->DecMainCtrl(2))
          {
              dsbNastawnikJazdy->SetCurrentPosition(0);
              dsbNastawnikJazdy->Play(0,0,0);
          }
       else;
      else
      if (cKey==Global::Keys[k_IncScndCtrlFAST])
          if (DynamicObject->MoverParameters->IncScndCtrl(2))
          {
           if (dsbNastawnikBocz) //hunter-081211
             {
              dsbNastawnikBocz->SetCurrentPosition(0);
              dsbNastawnikBocz->Play(0,0,0);
             }
           else if (!dsbNastawnikBocz)
             {
              dsbNastawnikJazdy->SetCurrentPosition(0);
              dsbNastawnikJazdy->Play(0,0,0);
             }
          }
       else;
      else
      if (cKey==Global::Keys[k_DecScndCtrlFAST])
          if (DynamicObject->MoverParameters->DecScndCtrl(2))
          {
           if (dsbNastawnikBocz) //hunter-081211
             {
              dsbNastawnikBocz->SetCurrentPosition(0);
              dsbNastawnikBocz->Play(0,0,0);
             }
           else if (!dsbNastawnikBocz)
             {
              dsbNastawnikJazdy->SetCurrentPosition(0);
              dsbNastawnikJazdy->Play(0,0,0);
             }
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
      //-----------
      //hunter-141211: wyl. szybki zalaczony przeniesiony do TTrain::Update()
      /* if (cKey==Global::Keys[k_Main])
      {
         MainOnButtonGauge.PutValue(1);
         if (DynamicObject->MoverParameters->MainSwitch(true))
           {
              if (DynamicObject->MoverParameters->EngineType==DieselEngine)
               dsbDieselIgnition->Play(0,0,0);
              else
               dsbNastawnikJazdy->Play(0,0,0);
           }
      }
      else */

      if (cKey==Global::Keys[k_Main])
      {
           if (fabs(MainOnButtonGauge.GetValue())<0.001)
           {
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play(0,0,0);
           }
      }
      else
      //-----------

      if (cKey==Global::Keys[k_BrakeProfile])   //McZapkie-240302-B: przelacznik opoznienia hamowania
      {//yB://ABu: male poprawki, zeby bylo mozna ustawic dowolny wagon
              int CouplNr=-2;
              if (!FreeFlyModeFlag)
              {
                  if (DynamicObject->MoverParameters->BrakeDelaySwitch(0))
                   {
                       dsbPneumaticRelay->SetVolume(DSBVOLUME_MAX);
                       dsbPneumaticRelay->Play(0,0,0);
                   }
              }
              else
              {
                 TDynamicObject *temp;
                 temp=(DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(),-1, 1500, CouplNr));
                 if (temp==NULL)
                 {
                    CouplNr=-2;
                    temp=(DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(),1, 1500, CouplNr));
                 }
                 if (temp)
                 {
                    if (temp->MoverParameters->BrakeDelaySwitch(0))
                     {
                       dsbPneumaticRelay->SetVolume(DSBVOLUME_MAX);
                       dsbPneumaticRelay->Play(0,0,0);
                     }
                 }
              }
      }
      else
      //-----------
      //hunter-261211: przetwornica i sprzezarka przeniesione do TTrain::Update()
      /* if (cKey==Global::Keys[k_Converter])   //NBMX 14-09-2003: przetwornica wl
      {
        if ((DynamicObject->MoverParameters->PantFrontVolt) || (DynamicObject->MoverParameters->PantRearVolt) || (DynamicObject->MoverParameters->EnginePowerSource.SourceType!=CurrentCollector) || (!Global::bLiveTraction))
           if (DynamicObject->MoverParameters->ConverterSwitch(true))
           {
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play(0,0,0);
           }
      }
      else
      if (cKey==Global::Keys[k_Compressor])   //NBMX 14-09-2003: sprezarka wl
      {
        if ((DynamicObject->MoverParameters->ConverterFlag) || (DynamicObject->MoverParameters->CompressorPower<2))
           if (DynamicObject->MoverParameters->CompressorSwitch(true))
           {
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play(0,0,0);
           }
      }
      else */

      if (cKey==Global::Keys[k_Converter])
       {
        if (ConverterButtonGauge.GetValue()==0)
         {
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play(0,0,0);
         }
       }
      else
      if ((cKey==Global::Keys[k_Compressor])&&((DynamicObject->MoverParameters->EngineType==ElectricSeriesMotor)||(DynamicObject->MoverParameters->TrainType==dt_EZT))) //hunter-110212: poprawka dla EZT
       {
        if (CompressorButtonGauge.GetValue()==0)
         {
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play(0,0,0);
         }
       }
      else
      //-----------

      if (cKey==Global::Keys[k_SmallCompressor])   //Winger 160404: mala sprezarka wl
      {
//           if (DynamicObject->MoverParameters->CompressorSwitch(true))
//           {
               DynamicObject->MoverParameters->PantCompFlag=true;
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play(0,0,0);
//           }
      }
      else if (cKey==VkKeyScan('q')) //ze Shiftem - w³¹czenie AI
      {//McZapkie-240302 - wlaczanie automatycznego pilota (zadziala tylko w trybie debugmode)
       if (DynamicObject->Mechanik)
        DynamicObject->Mechanik->TakeControl(true);
      }
      else if (cKey==Global::Keys[k_MaxCurrent])   //McZapkie-160502: F - wysoki rozruch
      {
           if ((DynamicObject->MoverParameters->EngineType==DieselElectric) && (DynamicObject->MoverParameters->ShuntModeAllow) && (DynamicObject->MoverParameters->MainCtrlPos==0))
           {
               DynamicObject->MoverParameters->ShuntMode=True;
           }

           if (DynamicObject->MoverParameters->MaxCurrentSwitch(true))
           {
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play(0,0,0);
           }

           if (DynamicObject->MoverParameters->TrainType!=dt_EZT)
             if (DynamicObject->MoverParameters->MinCurrentSwitch(true))
             {
                 dsbSwitch->SetVolume(DSBVOLUME_MAX);
                 dsbSwitch->Play(0,0,0);
             }
      }
      else
      if (cKey==Global::Keys[k_CurrentAutoRelay])  //McZapkie-241002: G - wlaczanie PSR
      {
           if (DynamicObject->MoverParameters->AutoRelaySwitch(true))
           {
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play(0,0,0);
           }
      }
      else
      if (cKey==Global::Keys[k_FailedEngineCutOff])  //McZapkie-060103: E - wylaczanie sekcji silnikow
      {
           if (DynamicObject->MoverParameters->CutOffEngine())
           {
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play(0,0,0);
           }
      }
      else
      if (cKey==Global::Keys[k_OpenLeft])   //NBMX 17-09-2003: otwieranie drzwi
      {
         if (DynamicObject->MoverParameters->DoorOpenCtrl==1)
           if (DynamicObject->MoverParameters->CabNo<0?DynamicObject->MoverParameters->DoorRight(true):DynamicObject->MoverParameters->DoorLeft(true))
             {
                 dsbSwitch->SetVolume(DSBVOLUME_MAX);
                 dsbSwitch->Play(0,0,0);
                 dsbDoorOpen->SetCurrentPosition(0);
                 dsbDoorOpen->Play(0,0,0);
             }
      }
      else
      if (cKey==Global::Keys[k_OpenRight])   //NBMX 17-09-2003: otwieranie drzwi
      {
         if (DynamicObject->MoverParameters->DoorCloseCtrl==1)
           if (DynamicObject->MoverParameters->CabNo<0?DynamicObject->MoverParameters->DoorLeft(true):DynamicObject->MoverParameters->DoorRight(true))
           {
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play(0,0,0);
               dsbDoorOpen->SetCurrentPosition(0);
               dsbDoorOpen->Play(0,0,0);
           }
      }
      else
      //-----------
      //hunter-131211: dzwiek dla przelacznika universala podniesionego
      if (cKey==Global::Keys[k_Univ3])
      {
           if (Universal3ButtonGauge.GetValue()==0)
           {
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play(0,0,0);
           }
       if (Console::Pressed(VK_CONTROL))
       {//z [Ctrl] zapalamy albo gasimy œwiate³ko w kabinie
        if (iCabLightFlag<2) ++iCabLightFlag; //zapalenie
       }
      }
      else
      //-----------
      if (cKey==Global::Keys[k_PantFrontUp])   //Winger 160204: podn. przedn. pantografu
      {
           DynamicObject->MoverParameters->PantFrontSP=false;
           if (DynamicObject->MoverParameters->PantFront(true))
            if (DynamicObject->MoverParameters->PantFrontStart!=1)
            {
                dsbSwitch->SetVolume(DSBVOLUME_MAX);
                dsbSwitch->Play(0,0,0);
            }
      }
      else
      if (cKey==Global::Keys[k_PantRearUp])   //Winger 160204: podn. tyln. pantografu
      {
           DynamicObject->MoverParameters->PantRearSP=false;
           if (DynamicObject->MoverParameters->PantRear(true))
            if (DynamicObject->MoverParameters->PantRearStart!=1)
            {
                dsbSwitch->SetVolume(DSBVOLUME_MAX);
                dsbSwitch->Play(0,0,0);
            }
      }
      else
//      if (cKey==Global::Keys[k_Active])   //yB 300407: przelacznik rozrzadu
//      {
//        if (DynamicObject->MoverParameters->CabActivisation())
//           {
//            dsbSwitch->SetVolume(DSBVOLUME_MAX);
//            dsbSwitch->Play(0,0,0);
//           }
//      }
//      else
      if (cKey==Global::Keys[k_Heating])   //Winger 020304: ogrzewanie skladu - wlaczenie
      {
              if (!FreeFlyModeFlag)
              {
                   if ((DynamicObject->MoverParameters->Heating==false)&&((DynamicObject->MoverParameters->EngineType==ElectricSeriesMotor)||(DynamicObject->MoverParameters->ConverterFlag)))
                   {
                   DynamicObject->MoverParameters->Heating=true;
                   dsbSwitch->SetVolume(DSBVOLUME_MAX);
                   dsbSwitch->Play(0,0,0);
                   }
              }
/*
              else
              {Ra: przeniesione do World.cpp
                 int CouplNr=-2;
                 TDynamicObject *temp;
                 temp=(DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(),-1, 1500, CouplNr));
                 if (temp==NULL)
                 {
                    CouplNr=-2;
                    temp=(DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(),1, 1500, CouplNr));
                 }
                 if (temp)
                 {
                    if (temp->MoverParameters->IncBrakeMult())
                     {
                       dsbSwitch->SetVolume(DSBVOLUME_MAX);
                       dsbSwitch->Play(0,0,0);
                     }
                 }
              }
*/
      }
      else
      //ABu 060205: dzielo Wingera po malutkim liftingu:
      if (cKey==Global::Keys[k_LeftSign]) //lewe swiatlo - w³¹czenie
      {
      if ((GetAsyncKeyState(VK_CONTROL)<0)&&(RearLeftLightButtonGauge.SubModel))  //hunter-230112 - z controlem zapala z tylu
      {
      //------------------------------
       if (DynamicObject->MoverParameters->ActiveCab==1)
       {//kabina 1
        if (((DynamicObject->iLights[1])&3)==0)
        {
         DynamicObject->iLights[1]|=1;
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         RearLeftLightButtonGauge.PutValue(1);
        }
        if (((DynamicObject->iLights[1])&3)==2)
        {
         DynamicObject->iLights[1]&=(255-2);
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         if (RearLeftEndLightButtonGauge.SubModel)
         {
          RearLeftEndLightButtonGauge.PutValue(0);
          RearLeftLightButtonGauge.PutValue(0);
         }
         else
          RearLeftLightButtonGauge.PutValue(0);
        }
       }
       else
       {//kabina -1
        if (((DynamicObject->iLights[0])&3)==0)
        {
         DynamicObject->iLights[0]|=1;
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         RearLeftLightButtonGauge.PutValue(1);
        }
        if (((DynamicObject->iLights[0])&3)==2)
        {
         DynamicObject->iLights[0]&=(255-2);
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         if (RearLeftEndLightButtonGauge.SubModel)
         {
          RearLeftEndLightButtonGauge.PutValue(0);
          RearLeftLightButtonGauge.PutValue(0);
         }
         else
          RearLeftLightButtonGauge.PutValue(0);
        }
       }
      //----------------------
      }
      else
      {
       if (DynamicObject->MoverParameters->ActiveCab==1)
       {//kabina 1
        if (((DynamicObject->iLights[0])&3)==0)
        {
         DynamicObject->iLights[0]|=1;
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         LeftLightButtonGauge.PutValue(1);
        }
        if (((DynamicObject->iLights[0])&3)==2)
        {
         DynamicObject->iLights[0]&=(255-2);
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         if (LeftEndLightButtonGauge.SubModel)
         {
          LeftEndLightButtonGauge.PutValue(0);
          LeftLightButtonGauge.PutValue(0);
         }
         else
          LeftLightButtonGauge.PutValue(0);
        }
       }
       else
       {//kabina -1
        if (((DynamicObject->iLights[1])&3)==0)
        {
         DynamicObject->iLights[1]|=1;
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         LeftLightButtonGauge.PutValue(1);
        }
        if (((DynamicObject->iLights[1])&3)==2)
        {
         DynamicObject->iLights[1]&=(255-2);
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         if (LeftEndLightButtonGauge.SubModel)
         {
          LeftEndLightButtonGauge.PutValue(0);
          LeftLightButtonGauge.PutValue(0);
         }
         else
          LeftLightButtonGauge.PutValue(0);
        }
       }
      } //-----------
      }
      else
      if (cKey==Global::Keys[k_UpperSign]) //ABu 060205: œwiat³o górne - w³¹czenie
      {
      if ((GetAsyncKeyState(VK_CONTROL)<0)&&(RearUpperLightButtonGauge.SubModel))  //hunter-230112 - z controlem zapala z tylu
      {
      //------------------------------
       if ((DynamicObject->MoverParameters->ActiveCab)==1)
       { //kabina 1
        if (((DynamicObject->iLights[1])&12)==0)
        {
         DynamicObject->iLights[1]|=4;
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         RearUpperLightButtonGauge.PutValue(1);
        }
       }
       else
       {//kabina -1
        if (((DynamicObject->iLights[0])&12)==0)
        {
         DynamicObject->iLights[0]|=4;
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         RearUpperLightButtonGauge.PutValue(1);
        }
       }
      } //------------------------------
      else
      {
       if ((DynamicObject->MoverParameters->ActiveCab)==1)
       { //kabina 1
        if (((DynamicObject->iLights[0])&12)==0)
        {
         DynamicObject->iLights[0]|=4;
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         UpperLightButtonGauge.PutValue(1);
        }
       }
       else
       {//kabina -1
        if (((DynamicObject->iLights[1])&12)==0)
        {
         DynamicObject->iLights[1]|=4;
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         UpperLightButtonGauge.PutValue(1);
        }
       }
      }
      }
      else
      if (cKey==Global::Keys[k_RightSign])   //Winger 070304: swiatla tylne (koncowki) - wlaczenie
      {
      if ((GetAsyncKeyState(VK_CONTROL)<0)&&(RearRightLightButtonGauge.SubModel))  //hunter-230112 - z controlem zapala z tylu
      {
      //------------------------------
       if (DynamicObject->MoverParameters->ActiveCab==1)
       {//kabina 1
        if (((DynamicObject->iLights[1])&48)==0)
        {
         DynamicObject->iLights[1]|=16;
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         RearRightLightButtonGauge.PutValue(1);
        }
        if (((DynamicObject->iLights[1])&48)==32)
        {
         DynamicObject->iLights[1]&=(255-32);
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         if (RearRightEndLightButtonGauge.SubModel)
         {
          RearRightEndLightButtonGauge.PutValue(0);
          RearRightLightButtonGauge.PutValue(0);
         }
         else
          RearRightLightButtonGauge.PutValue(0);
        }
       }
       else
       {//kabina -1
        if (((DynamicObject->iLights[0])&48)==0)
        {
         DynamicObject->iLights[0]|=16;
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         RearRightLightButtonGauge.PutValue(1);
        }
        if (((DynamicObject->iLights[0])&48)==32)
        {
         DynamicObject->iLights[0]&=(255-32);
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         if (RearRightEndLightButtonGauge.SubModel)
         {
          RearRightEndLightButtonGauge.PutValue(0);
          RearRightLightButtonGauge.PutValue(0);
         }
         else
          RearRightLightButtonGauge.PutValue(0);
        }
       }
      } //------------------------------
      else
      {
       if (DynamicObject->MoverParameters->ActiveCab==1)
       {//kabina 1
        if (((DynamicObject->iLights[0])&48)==0)
        {
         DynamicObject->iLights[0]|=16;
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         RightLightButtonGauge.PutValue(1);
        }
        if (((DynamicObject->iLights[0])&48)==32)
        {
         DynamicObject->iLights[0]&=(255-32);
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         if (RightEndLightButtonGauge.SubModel)
         {
          RightEndLightButtonGauge.PutValue(0);
          RightLightButtonGauge.PutValue(0);
         }
         else
          RightLightButtonGauge.PutValue(0);
        }
       }
       else
       {//kabina -1
        if (((DynamicObject->iLights[1])&48)==0)
        {
         DynamicObject->iLights[1]|=16;
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         RightLightButtonGauge.PutValue(1);
        }
        if (((DynamicObject->iLights[1])&48)==32)
        {
         DynamicObject->iLights[1]&=(255-32);
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         if (RightEndLightButtonGauge.SubModel)
         {
          RightEndLightButtonGauge.PutValue(0);
          RightLightButtonGauge.PutValue(0);
         }
         else
          RightLightButtonGauge.PutValue(0);
        }
       }
      }
      }
      if (cKey==Global::Keys[k_EndSign])
      {//Ra: umieszczenie tego w obs³udze kabiny jest nieco bez sensu
       int CouplNr=-1;
       TDynamicObject *tmp;
       tmp=DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(), 1, 1500, CouplNr);
       if (tmp==NULL)
        tmp=DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(),-1, 1500, CouplNr);
       if (tmp&&(CouplNr!=-1))
       {
        int mask=(GetAsyncKeyState(VK_CONTROL)<0)?2+32:64;
        if (CouplNr==0)
        {
         if (((tmp->iLights[0])&mask)!=mask)
         {
          tmp->iLights[0]|=mask;
          dsbSwitch->SetVolume(DSBVOLUME_MAX); //Ra: ten dŸwiêk tu to przegiêcie
          dsbSwitch->Play(0,0,0);
         }
        }
        else
        {
         if (((tmp->iLights[1])&mask)!=mask)
         {
          tmp->iLights[1]|=mask;
          dsbSwitch->SetVolume(DSBVOLUME_MAX); //Ra: ten dŸwiêk tu to przegiêcie
          dsbSwitch->Play(0,0,0);
         }
        }
       }
      }
   }
  else //McZapkie-240302 - klawisze bez shifta
   {
      if (cKey==Global::Keys[k_IncMainCtrl])
      {
          if (DynamicObject->MoverParameters->IncMainCtrl(1))
          {
              dsbNastawnikJazdy->SetCurrentPosition(0);
              dsbNastawnikJazdy->Play(0,0,0);
          }
         else;
      }
/*
      else if (cKey==Global::Keys[k_FreeFlyMode])
      {//Ra: to tutaj trochê bruŸdzi
       FreeFlyModeFlag=!FreeFlyModeFlag;
       DynamicObject->ABuSetModelShake(vector3(0,0,0));
      }
*/
      else if (cKey==Global::Keys[k_DecMainCtrl])
          if (DynamicObject->MoverParameters->DecMainCtrl(1))
          {
              dsbNastawnikJazdy->SetCurrentPosition(0);
              dsbNastawnikJazdy->Play(0,0,0);
          }
         else;
      else
      if (cKey==Global::Keys[k_IncScndCtrl])
//        if (MoverParameters->ScndCtrlPos<MoverParameters->ScndCtrlPosNo)
//         if (DynamicObject->MoverParameters->EnginePowerSource.SourceType==CurrentCollector)
          if (DynamicObject->MoverParameters->ShuntMode)
          {
            DynamicObject->MoverParameters->AnPos+=(GetDeltaTime()/0.85f);
            if (DynamicObject->MoverParameters->AnPos > 1)
              DynamicObject->MoverParameters->AnPos=1;
          }
          else
          if (DynamicObject->MoverParameters->IncScndCtrl(1))
          {
           if (dsbNastawnikBocz) //hunter-081211
             {
              dsbNastawnikBocz->SetCurrentPosition(0);
              dsbNastawnikBocz->Play(0,0,0);
             }
           else if (!dsbNastawnikBocz)
             {
              dsbNastawnikJazdy->SetCurrentPosition(0);
              dsbNastawnikJazdy->Play(0,0,0);
             }
          }
         else;
      else
      if (cKey==Global::Keys[k_DecScndCtrl])
  //       if (DynamicObject->MoverParameters->EnginePowerSource.SourceType==CurrentCollector)
          if (DynamicObject->MoverParameters->ShuntMode)
          {
            DynamicObject->MoverParameters->AnPos-=(GetDeltaTime()/0.55f);
            if (DynamicObject->MoverParameters->AnPos < 0)
              DynamicObject->MoverParameters->AnPos=0;
          }
          else

          if (DynamicObject->MoverParameters->DecScndCtrl(1))
  //        if (MoverParameters->ScndCtrlPos>0)
          {
           if (dsbNastawnikBocz) //hunter-081211
             {
              dsbNastawnikBocz->SetCurrentPosition(0);
              dsbNastawnikBocz->Play(0,0,0);
             }
           else if (!dsbNastawnikBocz)
             {
              dsbNastawnikJazdy->SetCurrentPosition(0);
              dsbNastawnikJazdy->Play(0,0,0);
             }
          }
         else;
      else
      if (cKey==Global::Keys[k_IncLocalBrakeLevel])
          {//ABu: male poprawki, zeby bylo mozna przyhamowac dowolny wagon
              int CouplNr=-2;
              if (!FreeFlyModeFlag)
              {
                 if (DynamicObject->MoverParameters->IncLocalBrakeLevel(1));
                 else;
              }
              else
              {
                 TDynamicObject *temp;
                 CouplNr=-2;
                 temp=(DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(),-1, 1500, CouplNr));
                 if (temp==NULL)
                 {
                    CouplNr=-2;
                    temp=(DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(),1, 1500, CouplNr));
                 }
                 if (temp)
                 {
                    if (temp->MoverParameters->IncLocalBrakeLevelFAST())
                    {
                       dsbPneumaticRelay->SetVolume(-80);
                       dsbPneumaticRelay->Play(0,0,0);
                    }
                    else;
                 }
              }
          }
      else
      if (cKey==Global::Keys[k_DecLocalBrakeLevel])
          {//ABu: male poprawki, zeby bylo mozna odhamowac dowolny wagon
              int CouplNr=-2;
              if (!FreeFlyModeFlag)
              {
                 if (DynamicObject->MoverParameters->DecLocalBrakeLevel(1));
                 else;
              }
              else
              {
                 TDynamicObject *temp;
                 temp=(DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(),-1, 1500, CouplNr));
                 if (temp==NULL)
                 {
                    CouplNr=-2;
                    temp=(DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(),1, 1500, CouplNr));
                 }
                 if (temp)
                 {
                    if (temp->MoverParameters->DecLocalBrakeLevelFAST())
                    {
                       dsbPneumaticRelay->SetVolume(-80);
                       dsbPneumaticRelay->Play(0,0,0);
                    }
                    else;
                 }
              }
          }
      else
      if (cKey==Global::Keys[k_IncBrakeLevel])
       //if (DynamicObject->MoverParameters->IncBrakeLevel())
       if (DynamicObject->MoverParameters->BrakeLevelAdd(Global::fBrakeStep)) //nieodpowiedni warunek; true, jeœli mo¿na dalej krêciæ
          {
           keybrakecount=0;
           if ((isEztOer) && (DynamicObject->MoverParameters->BrakeCtrlPos<3))
            {
             dsbPneumaticSwitch->SetVolume(-10);
             dsbPneumaticSwitch->Play(0,0,0);
            }
           }
          else;
      else
      if (cKey==Global::Keys[k_DecBrakeLevel])
       {
         if ((DynamicObject->MoverParameters->BrakeCtrlPos>-1) || (keybrakecount>1))
          {

            if ((isEztOer) && (DynamicObject->MoverParameters->Mains) && (DynamicObject->MoverParameters->BrakeCtrlPos!=-1))
            {
             dsbPneumaticSwitch->SetVolume(-10);
             dsbPneumaticSwitch->Play(0,0,0);
            }
            //DynamicObject->MoverParameters->DecBrakeLevel();
            DynamicObject->MoverParameters->BrakeLevelAdd(-Global::fBrakeStep);

             /*
			 if ((isEztOer) && (DynamicObject->MoverParameters->BrakeCtrlPos<2))
              {
               dsbPneumaticSwitch->SetVolume(-10);
               dsbPneumaticSwitch->Play(0,0,0);
              }
			  */
          }
           else keybrakecount+=1;
       }
      else
      if (cKey==Global::Keys[k_EmergencyBrake])
      {
       //while (DynamicObject->MoverParameters->IncBrakeLevel());
       DynamicObject->MoverParameters->BrakeLevelSet(DynamicObject->MoverParameters->BrakeCtrlPosNo);
       DynamicObject->MoverParameters->EmergencyBrakeFlag=true;
      }
      else
      if (cKey==Global::Keys[k_Brake3])
      {
          if ((isEztOer) && ((DynamicObject->MoverParameters->BrakeCtrlPos==1)||(DynamicObject->MoverParameters->BrakeCtrlPos==-1)))
            {
             dsbPneumaticSwitch->SetVolume(-10);
             dsbPneumaticSwitch->Play(0,0,0);
            }
       //while (DynamicObject->MoverParameters->BrakeCtrlPos>DynamicObject->MoverParameters->BrakeCtrlPosNo-1 && DynamicObject->MoverParameters->DecBrakeLevel());
       //while (DynamicObject->MoverParameters->BrakeCtrlPos<DynamicObject->MoverParameters->BrakeCtrlPosNo-1 && DynamicObject->MoverParameters->IncBrakeLevel());
       DynamicObject->MoverParameters->BrakeLevelSet(DynamicObject->MoverParameters->BrakeCtrlPosNo-1);
      }
      else
      if (cKey==Global::Keys[k_Brake2])
      {
       if ((isEztOer) && ((DynamicObject->MoverParameters->BrakeCtrlPos==1)||(DynamicObject->MoverParameters->BrakeCtrlPos==-1)))
       {
        dsbPneumaticSwitch->SetVolume(-10);
        dsbPneumaticSwitch->Play(0,0,0);
       }
       //while (DynamicObject->MoverParameters->BrakeCtrlPos>DynamicObject->MoverParameters->BrakeCtrlPosNo/2 && DynamicObject->MoverParameters->DecBrakeLevel());
       //while (DynamicObject->MoverParameters->BrakeCtrlPos<DynamicObject->MoverParameters->BrakeCtrlPosNo/2 && DynamicObject->MoverParameters->IncBrakeLevel());
       DynamicObject->MoverParameters->BrakeLevelSet(DynamicObject->MoverParameters->BrakeCtrlPosNo/2);
       if (GetAsyncKeyState(VK_CONTROL)<0)
        if ((DynamicObject->MoverParameters->BrakeSubsystem==Oerlikon)&&(DynamicObject->MoverParameters->BrakeSystem==Pneumatic))
         //DynamicObject->MoverParameters->BrakeCtrlPos=-2;
         DynamicObject->MoverParameters->BrakeLevelSet(-2);
      }
      else
      if (cKey==Global::Keys[k_Brake1])
      {
          if ((isEztOer)&& (DynamicObject->MoverParameters->BrakeCtrlPos!=1))
            {
             dsbPneumaticSwitch->SetVolume(-10);
             dsbPneumaticSwitch->Play(0,0,0);
            }
       //while (DynamicObject->MoverParameters->BrakeCtrlPos>1 && DynamicObject->MoverParameters->DecBrakeLevel());
       //while (DynamicObject->MoverParameters->BrakeCtrlPos<1 && DynamicObject->MoverParameters->IncBrakeLevel());
       DynamicObject->MoverParameters->BrakeLevelSet(1);
      }
      else
      if (cKey==Global::Keys[k_Brake0])
      {
          if ((isEztOer) && ((DynamicObject->MoverParameters->BrakeCtrlPos==1)||(DynamicObject->MoverParameters->BrakeCtrlPos==-1)))
            {
             dsbPneumaticSwitch->SetVolume(-10);
             dsbPneumaticSwitch->Play(0,0,0);
            }
       //while (DynamicObject->MoverParameters->BrakeCtrlPos>0 && DynamicObject->MoverParameters->DecBrakeLevel());
       //while (DynamicObject->MoverParameters->BrakeCtrlPos<0 && DynamicObject->MoverParameters->IncBrakeLevel());
       DynamicObject->MoverParameters->BrakeLevelSet(0);
      }
      else
      if (cKey==Global::Keys[k_WaveBrake])
      {
          if ((isEztOer) && (DynamicObject->MoverParameters->Mains) && (DynamicObject->MoverParameters->BrakeCtrlPos!=-1))
            {
             dsbPneumaticSwitch->SetVolume(-10);
             dsbPneumaticSwitch->Play(0,0,0);
            }
       //while (DynamicObject->MoverParameters->BrakeCtrlPos>-1 && DynamicObject->MoverParameters->DecBrakeLevel());
       //while (DynamicObject->MoverParameters->BrakeCtrlPos<-1 && DynamicObject->MoverParameters->IncBrakeLevel());
       DynamicObject->MoverParameters->BrakeLevelSet(-1);
      }
      else
      //---------------
      //hunter-131211: zbicie czuwaka przeniesione do TTrain::Update()
      /* if (cKey==Global::Keys[k_Czuwak]))
      {
//          dsbBuzzer->Stop();
          if (DynamicObject->MoverParameters->SecuritySystemReset())
           if (fabs(SecurityResetButtonGauge.GetValue())<0.001)
            {
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play(0,0,0);
            }
          SecurityResetButtonGauge.PutValue(1);
      }
      else */

      if (cKey==Global::Keys[k_Czuwak])
      {
       if (fabs(SecurityResetButtonGauge.GetValue())<0.001)
        {
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play(0,0,0);
        }
      }
      else
      //---------------
      //hunter-221211: hamulec przeciwposlizgowy przeniesiony do TTrain::Update()
      if (cKey==Global::Keys[k_AntiSlipping])
      {
        if (DynamicObject->MoverParameters->BrakeSystem!=ElectroPneumatic)
         {
          //if (DynamicObject->MoverParameters->AntiSlippingButton())
           if (fabs(AntiSlipButtonGauge.GetValue())<0.001)
            {
              //Dlaczego bylo '-50'???
              //dsbSwitch->SetVolume(-50);
              dsbSwitch->SetVolume(DSBVOLUME_MAX);
              dsbSwitch->Play(0,0,0);
            }
          //AntiSlipButtonGauge.PutValue(1);
         }
      }
      else
      //---------------

      if (cKey==Global::Keys[k_Fuse])
      {
       if (GetAsyncKeyState(VK_CONTROL)<0)  //z controlem
        {
         ConverterFuseButtonGauge.PutValue(1);   //hunter-261211
         if ((DynamicObject->MoverParameters->Mains==false)&&(ConverterButtonGauge.GetValue()==0))
          DynamicObject->MoverParameters->ConvOvldFlag=false;
        }
       else
        {
         FuseButtonGauge.PutValue(1);
         DynamicObject->MoverParameters->FuseOn();
        }
      }
      else
  // McZapkie-240302 - zmiana kierunku: 'd' do przodu, 'r' do tylu
      if (cKey==Global::Keys[k_DirectionForward])
      {
        if (DynamicObject->MoverParameters->DirectionForward())
          {
           //------------
           //hunter-121211: dzwiek kierunkowego
            if (dsbReverserKey)
             {
              dsbReverserKey->SetCurrentPosition(0);
              dsbReverserKey->SetVolume(DSBVOLUME_MAX);
              dsbReverserKey->Play(0,0,0);
             }
            else if (!dsbReverserKey)
             {
              dsbSwitch->SetVolume(DSBVOLUME_MAX);
              dsbSwitch->Play(0,0,0);
             }
           //------------

          }
      }
      else
      if (cKey==Global::Keys[k_DirectionBackward])  //r
      {
        if (DynamicObject->MoverParameters->DirectionBackward())
          {
           //------------
           //hunter-121211: dzwiek kierunkowego
            if (dsbReverserKey)
             {
              dsbReverserKey->SetCurrentPosition(0);
              dsbReverserKey->SetVolume(DSBVOLUME_MAX);
              dsbReverserKey->Play(0,0,0);
             }
            else if (!dsbReverserKey)
             {
              dsbSwitch->SetVolume(DSBVOLUME_MAX);
              dsbSwitch->Play(0,0,0);
             }
           //------------
          }
      }
      else
  // McZapkie-240302 - wylaczanie glownego obwodu
      //-----------
      //hunter-141211: wyl. szybki wylaczony przeniesiony do TTrain::Update()
      if (cKey==Global::Keys[k_Main])
      {
           if (fabs(MainOffButtonGauge.GetValue())<0.001)
           {
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play(0,0,0);
           }
      }
      else
      //-----------
//      if (cKey==Global::Keys[k_Active])   //yB 300407: przelacznik rozrzadu
//      {
//        if (DynamicObject->MoverParameters->CabDeactivisation())
//           {
//            dsbSwitch->SetVolume(DSBVOLUME_MAX);
//            dsbSwitch->Play(0,0,0);
//           }
//      }
//      else
      if (cKey==Global::Keys[k_BrakeProfile])
      {//yB://ABu: male poprawki, zeby bylo mozna ustawic dowolny wagon
              int CouplNr=-2;
              if (!FreeFlyModeFlag)
              {
                  if (GetAsyncKeyState(VK_CONTROL)<0)
                   if (DynamicObject->MoverParameters->BrakeDelaySwitch(2))
                     {
                       dsbPneumaticRelay->SetVolume(DSBVOLUME_MAX);
                       dsbPneumaticRelay->Play(0,0,0);
                     }
                   else;
                  else
                   if (DynamicObject->MoverParameters->BrakeDelaySwitch(1))
                     {
                       dsbPneumaticRelay->SetVolume(DSBVOLUME_MAX);
                       dsbPneumaticRelay->Play(0,0,0);
                     }
              }
              else
              {
                 TDynamicObject *temp;
                 temp=(DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(),-1, 1500, CouplNr));
                 if (temp==NULL)
                 {
                    CouplNr=-2;
                    temp=(DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(),1, 1500, CouplNr));
                 }
                 if (temp)
                 {
                  if (GetAsyncKeyState(VK_CONTROL)<0)
                    if (temp->MoverParameters->BrakeDelaySwitch(2))
                     {
                       dsbPneumaticRelay->SetVolume(DSBVOLUME_MAX);
                       dsbPneumaticRelay->Play(0,0,0);
                     }
                    else;
                  else
                    if (temp->MoverParameters->BrakeDelaySwitch(1))
                     {
                       dsbPneumaticRelay->SetVolume(DSBVOLUME_MAX);
                       dsbPneumaticRelay->Play(0,0,0);
                     }
                 }
              }

      }
      else
      //-----------
      //hunter-261211: przetwornica i sprzezarka przeniesione do TTrain::Update()
      if (cKey==Global::Keys[k_Converter])
       {
        if (ConverterButtonGauge.GetValue()!=0)
         {
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play(0,0,0);
         }
       }
      else
      if ((cKey==Global::Keys[k_Compressor])&&((DynamicObject->MoverParameters->EngineType==ElectricSeriesMotor)||(DynamicObject->MoverParameters->TrainType==dt_EZT))) //hunter-110212: poprawka dla EZT
       {
        if (CompressorButtonGauge.GetValue()!=0)
         {
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play(0,0,0);
         }
       }
      else
      //-----------
      if (cKey==Global::Keys[k_Releaser])   //odluzniacz
      {
       if (!FreeFlyModeFlag)
       {
        if ((DynamicObject->MoverParameters->EngineType==ElectricSeriesMotor)||(DynamicObject->MoverParameters->EngineType==DieselElectric))
         if (DynamicObject->MoverParameters->TrainType!=dt_EZT)
          if (DynamicObject->MoverParameters->BrakeCtrlPosNo>0)
          {
           ReleaserButtonGauge.PutValue(1);
           if (DynamicObject->MoverParameters->BrakeReleaser())
           {
            dsbPneumaticRelay->SetVolume(-80);
            dsbPneumaticRelay->Play(0,0,0);
           }
          }
       }
/* //OdluŸniacz przeniesiony do World.cpp
       else
       {//Ra: odluŸnianie dowolnego pojazdu przy kamerze, by³o tylko we w³asnym sk³adzie
        TDynamicObject *temp=Global::DynamicNearest();
        int CouplNr=-2;
        TDynamicObject *temp;
        temp=(DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(),-1, 1500, CouplNr));
        if (temp==NULL)
        {
          CouplNr=-2;
          temp=(DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(),1, 1500, CouplNr));
        }
        if (temp)
        {
         if (temp->MoverParameters->BrakeReleaser())
         {
          dsbPneumaticRelay->SetVolume(DSBVOLUME_MAX);
          dsbPneumaticRelay->Play(0,0,0);
         }
        }
       }
*/
      }
  //McZapkie-240302 - wylaczanie automatycznego pilota (w trybie ~debugmode mozna tylko raz)
      else if (cKey==VkKeyScan('q')) //bez Shift
      {
       if (DynamicObject->Mechanik)
        DynamicObject->Mechanik->TakeControl(false);
      }
      else
      if (cKey==Global::Keys[k_MaxCurrent])   //McZapkie-160502: f - niski rozruch
      {
           if ((DynamicObject->MoverParameters->EngineType==DieselElectric) && (DynamicObject->MoverParameters->ShuntModeAllow) && (DynamicObject->MoverParameters->MainCtrlPos==0))
           {
               DynamicObject->MoverParameters->ShuntMode=False;
           }

           if (DynamicObject->MoverParameters->MaxCurrentSwitch(false))
           {
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play(0,0,0);

           }
           if (DynamicObject->MoverParameters->TrainType!=dt_EZT)
             if (DynamicObject->MoverParameters->MinCurrentSwitch(false))
             {
                 dsbSwitch->SetVolume(DSBVOLUME_MAX);
                 dsbSwitch->Play(0,0,0);
             }
      }
      else
      if (cKey==Global::Keys[k_CurrentAutoRelay])  //McZapkie-241002: g - wylaczanie PSR
      {
           if (DynamicObject->MoverParameters->AutoRelaySwitch(false))
           {
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play(0,0,0);
           }
      }
      else
      //hunter-201211: piasecznica poprawiona oraz przeniesiona do TTrain::Update()
      /*
      if (cKey==Global::Keys[k_Sand])
      {
          if (DynamicObject->MoverParameters->SandDoseOn())
           if (DynamicObject->MoverParameters->SandDose)
            {
              dsbPneumaticRelay->SetVolume(-30);
              dsbPneumaticRelay->Play(0,0,0);
            }
      }
      else
      */
      if (cKey==Global::Keys[k_CabForward])
      {
       DynamicObject->MoverParameters->CabDeactivisation();
       if (!CabChange(1))
        if (TestFlag(DynamicObject->MoverParameters->Couplers[0].CouplingFlag,ctrain_passenger))
        {
//TODO: przejscie do nastepnego pojazdu, wskaznik do niego: DynamicObject->MoverParameters->Couplers[0].Connected
         Global::changeDynObj=true;
        }
       DynamicObject->MoverParameters->CabActivisation();
      }
      else if (cKey==Global::Keys[k_CabBackward])
      {
       DynamicObject->MoverParameters->CabDeactivisation();
       if (!CabChange(-1))
        if (TestFlag(DynamicObject->MoverParameters->Couplers[1].CouplingFlag,ctrain_passenger))
        {
//TODO: przejscie do poprzedniego, wskaznik do niego: DynamicObject->MoverParameters->Couplers[1].Connected
         Global::changeDynObj=true;
        }
       DynamicObject->MoverParameters->CabActivisation();
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
                dsbCouplerAttach->Play(0,0,0);
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
           int CouplNr=-1; //normalnie ¿aden ze sprzêgów
           TDynamicObject *tmp;
           tmp=DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(),1,1500,CouplNr);
           if (tmp==NULL)
            tmp=DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(),-1,1500,CouplNr);
           if (tmp&&(CouplNr!=-1))
           {
            if (tmp->MoverParameters->Couplers[CouplNr].CouplingFlag==0) //najpierw hak
            {
             if (tmp->MoverParameters->Attach(CouplNr,2,tmp->MoverParameters->Couplers[CouplNr].Connected,ctrain_coupler))
             {
              //tmp->MoverParameters->Couplers[CouplNr].Render=true; //pod³¹czony sprzêg bêdzie widoczny
              if (DynamicObject->Mechanik) //na wszelki wypadek
               DynamicObject->Mechanik->CheckVehicles(); //aktualizacja flag kierunku w sk³adzie
              dsbCouplerAttach->SetVolume(DSBVOLUME_MAX);
              dsbCouplerAttach->Play(0,0,0);
             }
            }
            else
            if (!TestFlag(tmp->MoverParameters->Couplers[CouplNr].CouplingFlag,ctrain_pneumatic))    //pneumatyka
            {
             if (tmp->MoverParameters->Attach(CouplNr,2,tmp->MoverParameters->Couplers[CouplNr].Connected,tmp->MoverParameters->Couplers[CouplNr].CouplingFlag+ctrain_pneumatic))
             {
              rsHiss.Play(1,DSBPLAY_LOOPING,true,tmp->GetPosition());
              DynamicObject->SetPneumatic(CouplNr,1); //Ra: to mi siê nie podoba !!!!
              tmp->SetPneumatic(CouplNr,1);
             }
            }
            else
            if (!TestFlag(tmp->MoverParameters->Couplers[CouplNr].CouplingFlag,ctrain_scndpneumatic))     //zasilajacy
            {
             if (tmp->MoverParameters->Attach(CouplNr,2,tmp->MoverParameters->Couplers[CouplNr].Connected,tmp->MoverParameters->Couplers[CouplNr].CouplingFlag+ctrain_scndpneumatic))
             {
//              rsHiss.Play(1,DSBPLAY_LOOPING,true,tmp->GetPosition());
              dsbCouplerDetach->SetVolume(DSBVOLUME_MAX);
              dsbCouplerDetach->Play(0,0,0);
              DynamicObject->SetPneumatic(CouplNr,0); //Ra: to mi siê nie podoba !!!!
              tmp->SetPneumatic(CouplNr,0);
             }
            }
            else
            if (!TestFlag(tmp->MoverParameters->Couplers[CouplNr].CouplingFlag,ctrain_controll))     //ukrotnionko
            {
             if (tmp->MoverParameters->Attach(CouplNr,2,tmp->MoverParameters->Couplers[CouplNr].Connected,tmp->MoverParameters->Couplers[CouplNr].CouplingFlag+ctrain_controll))
             {
              dsbCouplerAttach->SetVolume(DSBVOLUME_MAX);
              dsbCouplerAttach->Play(0,0,0);
             }
            }
           }
          }
        }
      }
      else
      if (cKey==Global::Keys[k_DeCouple])
      { //ABu051104: male zmiany, zeby mozna bylo rozlaczac odlegle wagony
        if (iCabn>0)
        {
          if (!FreeFlyModeFlag) //tryb 'kabinowy' (pozwala równie¿ roz³¹czyæ sprzêgi zablokowane)
          {
           if (DynamicObject->DettachStatus(iCabn-1)<0) //jeœli jest co odczepiæ
            if (DynamicObject->Dettach(iCabn-1)) //iCab==1:przód,iCab==2:ty³
            {
             dsbCouplerDetach->SetVolume(DSBVOLUME_MAX); //w kabinie ten dŸwiêk?
             dsbCouplerDetach->Play(0,0,0);
            }
          }
          else
          {//tryb freefly
           int CouplNr=-1;
           TDynamicObject *tmp;
           tmp=DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(),1,1500,CouplNr);
           if (tmp==NULL)
            tmp=DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(),-1,1500,CouplNr);
           if (tmp&&(CouplNr!=-1))
           {
            if ((tmp->MoverParameters->Couplers[CouplNr].CouplingFlag&ctrain_depot)==0) //je¿eli sprzêg niezablokowany
             if (tmp->DettachStatus(CouplNr)<0) //jeœli jest co odczepiæ i siê da
              if (!tmp->Dettach(CouplNr))
              {//dŸwiêk odczepiania
               dsbCouplerDetach->SetVolume(DSBVOLUME_MAX);
               dsbCouplerDetach->Play(0,0,0);
              }
           }
          }
          if (DynamicObject->Mechanik) //na wszelki wypadek
           DynamicObject->Mechanik->CheckVehicles(); //aktualizacja skrajnych pojazdów w sk³adzie
        }
      }
      else
      if (cKey==Global::Keys[k_CloseLeft])   //NBMX 17-09-2003: zamykanie drzwi
      {
           if (DynamicObject->MoverParameters->CabNo<0?DynamicObject->MoverParameters->DoorRight(false):DynamicObject->MoverParameters->DoorLeft(false))
//           if (DynamicObject->MoverParameters->DoorLeft(false))
           {
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play(0,0,0);
               dsbDoorClose->SetCurrentPosition(0);
               dsbDoorClose->Play(0,0,0);
           }
      }
      else if (cKey==Global::Keys[k_CloseRight])   //NBMX 17-09-2003: zamykanie drzwi
      {
           if (DynamicObject->MoverParameters->CabNo<0?DynamicObject->MoverParameters->DoorLeft(false):DynamicObject->MoverParameters->DoorRight(false))
//           if (DynamicObject->MoverParameters->DoorRight(false))
           {
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play(0,0,0);
               dsbDoorClose->SetCurrentPosition(0);
               dsbDoorClose->Play(0,0,0);
           }
      }
      else
      //-----------
      //hunter-131211: dzwiek dla przelacznika universala
      if (cKey==Global::Keys[k_Univ3])
      {
           if (Universal3ButtonGauge.GetValue()!=0)
           {
               dsbSwitch->SetVolume(DSBVOLUME_MAX);
               dsbSwitch->Play(0,0,0);
           }
       if (Console::Pressed(VK_CONTROL))
       {//z [Ctrl] zapalamy albo gasimy œwiate³ko w kabinie
        if (iCabLightFlag) --iCabLightFlag; //gaszenie
       }
      }
      else
      //-----------
      if (cKey==Global::Keys[k_PantFrontDown])   //Winger 160204: opuszczanie prz. patyka
      {
           if (DynamicObject->MoverParameters->PantFront(false))
            {
                dsbSwitch->SetVolume(DSBVOLUME_MAX);
                dsbSwitch->Play(0,0,0);
            }
      }
      else if (cKey==Global::Keys[k_PantRearDown])   //Winger 160204: opuszczanie tyl. patyka
      {
           if (DynamicObject->MoverParameters->PantSwitchType=="impulse")
            PantFrontButtonOffGauge.PutValue(1);
            if (DynamicObject->MoverParameters->PantRear(false))
             {
                 dsbSwitch->SetVolume(DSBVOLUME_MAX);
                 dsbSwitch->Play(0,0,0);
             }
      }
      else if (cKey==Global::Keys[k_Heating])   //Winger 020304: ogrzewanie - wylaczenie
      {
       if (!FreeFlyModeFlag)
       {
        if (DynamicObject->MoverParameters->Heating==true)
        {
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         DynamicObject->MoverParameters->Heating=false;
        }
       }
/*
       else
       {//Ra: przeniesione do World.cpp
        int CouplNr=-2;
        TDynamicObject *temp;
        temp=(DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(),-1, 1500, CouplNr));
        if (temp==NULL)
        {
         CouplNr=-2;
         temp=(DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(),1, 1500, CouplNr));
        }
        if (temp)
        {
         if (temp->MoverParameters->DecBrakeMult())
         {
          dsbSwitch->SetVolume(DSBVOLUME_MAX);
          dsbSwitch->Play(0,0,0);
         }
        }
       }
*/
      }
      else
      if (cKey==Global::Keys[k_LeftSign])   //ABu 060205: lewe swiatlo - wylaczenie
      {
      if ((GetAsyncKeyState(VK_CONTROL)<0)&&(RearLeftLightButtonGauge.SubModel))  //hunter-230112 - z controlem gasi z tylu
      {
      //------------------------------
       if (DynamicObject->MoverParameters->ActiveCab==1)
       {//kabina 1
        if (((DynamicObject->iLights[1])&3)==0)
        {
         DynamicObject->iLights[1]|=2;
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         if (RearLeftEndLightButtonGauge.SubModel)
         {
          RearLeftEndLightButtonGauge.PutValue(1);
          RearLeftLightButtonGauge.PutValue(0);
         }
         else
          RearLeftLightButtonGauge.PutValue(-1);
        }
        if (((DynamicObject->iLights[1])&3)==1)
        {
         DynamicObject->iLights[1]&=(255-1);
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         RearLeftLightButtonGauge.PutValue(0);
        }
       }
       else
       {//kabina -1
        if (((DynamicObject->iLights[0])&3)==0)
        {
         DynamicObject->iLights[0]|=2;
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         if (RearLeftEndLightButtonGauge.SubModel)
         {
          RearLeftEndLightButtonGauge.PutValue(1);
          RearLeftLightButtonGauge.PutValue(0);
         }
         else
          RearLeftLightButtonGauge.PutValue(-1);
        }
        if (((DynamicObject->iLights[1])&3)==1)
        {
         DynamicObject->iLights[1]&=(255-1);
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         LeftLightButtonGauge.PutValue(0);
        }
       }
      } //------------------------------
      else
      {
       if (DynamicObject->MoverParameters->ActiveCab==1)
       {//kabina 1
        if (((DynamicObject->iLights[0])&3)==0)
        {
         DynamicObject->iLights[0]|=2;
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         if (LeftEndLightButtonGauge.SubModel)
         {
          LeftEndLightButtonGauge.PutValue(1);
          LeftLightButtonGauge.PutValue(0);
         }
         else
          LeftLightButtonGauge.PutValue(-1);
        }
        if (((DynamicObject->iLights[0])&3)==1)
        {
         DynamicObject->iLights[0]&=(255-1);
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         LeftLightButtonGauge.PutValue(0);
        }
       }
       else
       {//kabina -1
        if (((DynamicObject->iLights[1])&3)==0)
        {
         DynamicObject->iLights[1]|=2;
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         if (LeftEndLightButtonGauge.SubModel)
         {
          LeftEndLightButtonGauge.PutValue(1);
          LeftLightButtonGauge.PutValue(0);
         }
         else
          LeftLightButtonGauge.PutValue(-1);
        }
        if (((DynamicObject->iLights[1])&3)==1)
        {
         DynamicObject->iLights[1]&=(255-1);
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         LeftLightButtonGauge.PutValue(0);
        }
       }
      }
      }
      else
      if (cKey==Global::Keys[k_UpperSign]) //ABu 060205: œwiat³o górne - wy³¹czenie
      {
      if ((GetAsyncKeyState(VK_CONTROL)<0)&&(RearUpperLightButtonGauge.SubModel))  //hunter-230112 - z controlem gasi z tylu
      {
      //------------------------------
       if (DynamicObject->MoverParameters->ActiveCab==1)
       {//kabina 1
        if (((DynamicObject->iLights[1])&12)==4)
        {
         DynamicObject->iLights[1]&=(255-4);
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         RearUpperLightButtonGauge.PutValue(0);
        }
       }
       else
       {//kabina -1
        if (((DynamicObject->iLights[0])&12)==4)
        {
         DynamicObject->iLights[0]&=(255-4);
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         RearUpperLightButtonGauge.PutValue(0);
        }
       }
      } //------------------------------
      else
      {
       if (DynamicObject->MoverParameters->ActiveCab==1)
       {//kabina 1
        if (((DynamicObject->iLights[0])&12)==4)
        {
         DynamicObject->iLights[0]&=(255-4);
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         UpperLightButtonGauge.PutValue(0);
        }
       }
       else
       {//kabina -1
        if (((DynamicObject->iLights[1])&12)==4)
        {
         DynamicObject->iLights[1]&=(255-4);
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         UpperLightButtonGauge.PutValue(0);
        }
       }
      }
      }
      else
      if (cKey==Global::Keys[k_EndSign])   //ABu 060205: koncowki - sciagniecie
      {
       int CouplNr=-1;
       TDynamicObject *tmp;
       tmp=DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(), 1, 500, CouplNr);
       if (tmp==NULL)
        tmp=DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(),-1, 500, CouplNr);
       if (tmp&&(CouplNr!=-1))
       {
        if (CouplNr==0)
        {
         if ((tmp->iLights[0])&(2+32+64))
         {
          tmp->iLights[0]&=~(2+32+64);
          dsbSwitch->SetVolume(DSBVOLUME_MAX); //Ra: ten dŸwiêk tu to przegiêcie
          dsbSwitch->Play(0,0,0);
         }
        }
        else
        {
         if ((tmp->iLights[1])&(2+32+64))
         {
          tmp->iLights[1]&=~(2+32+64);
          dsbSwitch->SetVolume(DSBVOLUME_MAX);
          dsbSwitch->Play(0,0,0);
         }
        }
       }
      }
      if (cKey==Global::Keys[k_RightSign])   //Winger 070304: swiatla tylne (koncowki) - wlaczenie
      {
      if ((GetAsyncKeyState(VK_CONTROL)<0)&&(RearRightLightButtonGauge.SubModel))  //hunter-230112 - z controlem gasi z tylu
      {
      //------------------------------
       if (DynamicObject->MoverParameters->ActiveCab==1)
       {//kabina 1 (od strony 0)
        if (((DynamicObject->iLights[1])&48)==0)
        {
         DynamicObject->iLights[1]|=32;
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         if (RearRightEndLightButtonGauge.SubModel)
         {
          RearRightEndLightButtonGauge.PutValue(1);
          RearRightLightButtonGauge.PutValue(0);
         }
        else
         RearRightLightButtonGauge.PutValue(-1);
        }
        if (((DynamicObject->iLights[1])&48)==16)
        {
         DynamicObject->iLights[1]&=(255-16);
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         RearRightLightButtonGauge.PutValue(0);
        }
       }
       else
       {//kabina -1
        if (((DynamicObject->iLights[0])&48)==0)
        {
         DynamicObject->iLights[0]|=32;
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         if (RearRightEndLightButtonGauge.SubModel)
         {
          RearRightEndLightButtonGauge.PutValue(1);
          RearRightLightButtonGauge.PutValue(0);
         }
        else
         RearRightLightButtonGauge.PutValue(-1);
        }
        if (((DynamicObject->iLights[0])&48)==16)
        {
         DynamicObject->iLights[0]&=(255-16);
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         RearRightLightButtonGauge.PutValue(0);
        }
       }
      } //------------------------------
      else
      {
       if (DynamicObject->MoverParameters->ActiveCab==1)
       { //kabina 0
        if (((DynamicObject->iLights[0])&48)==0)
        {
         DynamicObject->iLights[0]|=32;
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         if (RightEndLightButtonGauge.SubModel)
         {
          RightEndLightButtonGauge.PutValue(1);
          RightLightButtonGauge.PutValue(0);
         }
        else
         RightLightButtonGauge.PutValue(-1);
        }
        if (((DynamicObject->iLights[0])&48)==16)
        {
         DynamicObject->iLights[0]&=(255-16);
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         RightLightButtonGauge.PutValue(0);
        }
       }
       else
       {//kabina -1
        if (((DynamicObject->iLights[1])&48)==0)
        {
         DynamicObject->iLights[1]|=32;
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         if (RightEndLightButtonGauge.SubModel)
         {
          RightEndLightButtonGauge.PutValue(1);
          RightLightButtonGauge.PutValue(0);
         }
        else
         RightLightButtonGauge.PutValue(-1);
        }
        if (((DynamicObject->iLights[1])&48)==16)
        {
         DynamicObject->iLights[1]&=(255-16);
         dsbSwitch->SetVolume(DSBVOLUME_MAX);
         dsbSwitch->Play(0,0,0);
         RightLightButtonGauge.PutValue(0);
        }
       }
      }
      }
      else
      if (cKey==Global::Keys[k_StLinOff])   //Winger 110904: wylacznik st. liniowych
      {
       if (DynamicObject->MoverParameters->TrainType!=dt_EZT)
       {
        StLinOffButtonGauge.PutValue(1); //Ra: by³o Fuse...
        dsbSwitch->SetVolume(DSBVOLUME_MAX);
        dsbSwitch->Play(0,0,0);
        if (DynamicObject->MoverParameters->MainCtrlPosNo>0)
        {
         DynamicObject->MoverParameters->StLinFlag=true;
         dsbRelay->SetVolume(DSBVOLUME_MAX);
         dsbRelay->Play(0,0,0);
        }
       }
      }
      else
      {
  //McZapkie: poruszanie sie po kabinie, w updatemechpos zawarte sa wiezy

       //double dt=Timer::GetDeltaTime();
       if (DynamicObject->MoverParameters->ActiveCab<0)
        fMechCroach=-0.5;
       else
        fMechCroach=0.5;
//        if (!GetAsyncKeyState(VK_SHIFT)<0)         // bez shifta
         if (!Console::Pressed(VK_CONTROL)) //gdy [Ctrl] zwolniony (dodatkowe widoki)
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
              pMechOffset.y+=0.2; //McZapkie-120302 - wstawanie
          else
          if (cKey==Global::Keys[k_MechDown])
              pMechOffset.y-=0.2; //McZapkie-120302 - siadanie
         }
       }

  //    else
      if (DebugModeFlag)
      {//przesuwanie sk³adu o 100m
       TDynamicObject *d=DynamicObject;
       if (cKey==VkKeyScan('['))
       {while (d)
        {d->Move(100.0);
         d=d->Next(); //pozosta³e te¿
        }
        d=DynamicObject->Prev();
        while (d)
        {d->Move(100.0);
         d=d->Prev(); //w drug¹ stronê te¿
        }
       }
       else
       if (cKey==VkKeyScan(']'))
       {while (d)
        {d->Move(-100.0);
         d=d->Next(); //pozosta³e te¿
        }
        d=DynamicObject->Prev();
        while (d)
        {d->Move(-100.0);
         d=d->Prev(); //w drug¹ stronê te¿
        }
       }
      }
    if (cKey==VkKeyScan('-'))
    {//zmniejszenie numeru kana³u radiowego
     if (iRadioChannel>0) --iRadioChannel; //0=wy³¹czony
    }
    else if (cKey==VkKeyScan('='))
    {//zmniejszenie numeru kana³u radiowego
     if (iRadioChannel<8) ++iRadioChannel; //0=wy³¹czony
    }
   }
}


void __fastcall TTrain::UpdateMechPosition(double dt)
{//Ra: mechanik powinien byæ telepany niezale¿nie od pozycji pojazdu
 //Ra: trzeba zrobiæ model bujania g³ow¹ i wczepiæ go do pojazdu

 //DynamicObject->vFront=DynamicObject->GetDirection(); //to jest ju¿ policzone

 //Ra: tu by siê przyda³o uwzglêdniæ rozk³ad si³:
 // - na postoju horyzont prosto, kabina skosem
 // - przy szybkiej jeŸdzie kabina prosto, horyzont pochylony

 vector3 pNewMechPosition;
 //McZapkie: najpierw policzê pozycjê w/m kabiny

 //McZapkie: poprawka starego bledu
 //dt=Timer::GetDeltaTime();

 //ABu: rzucamy kabina tylko przy duzym FPS!
 //Mala histereza, zeby bez przerwy nie przelaczalo przy FPS~17
 //Granice mozna ustalic doswiadczalnie. Ja proponuje 14:20
 double r1,r2,r3;
 int iVel=DynamicObject->GetVelocity();
 if (iVel>150) iVel=150;
 if (!Global::iSlowMotion) //musi byæ pe³na prêdkoœæ
 {
   if (!(random((GetFPS()+1)/15)>0))
   {
      if ((iVel>0) && (random(155-iVel)<16))
      {
         r1=(double(random(iVel*2)-iVel)/((iVel*2)*4))*fMechSpringX;
         r2=(double(random(iVel*2)-iVel)/((iVel*2)*4))*fMechSpringY;
         r3=(double(random(iVel*2)-iVel)/((iVel*2)*4))*fMechSpringZ;
         MechSpring.ComputateForces(vector3(r1,r2,r3),pMechShake);
 //      MechSpring.ComputateForces(vector3(double(random(200)-100)/200,double(random(200)-100)/200,double(random(200)-100)/500),pMechShake);
      }
      else
         MechSpring.ComputateForces(vector3(-DynamicObject->MoverParameters->AccN*dt,DynamicObject->MoverParameters->AccV*dt*10,-DynamicObject->MoverParameters->AccS*dt),pMechShake);
   }
   vMechVelocity-=(MechSpring.vForce2+vMechVelocity*100)*(fMechSpringX+fMechSpringY+fMechSpringZ)/(200);

   //McZapkie:
   pMechShake+=vMechVelocity*dt;
 }
 else
   pMechShake-=pMechShake*Min0R(dt,1);

 pMechOffset+=vMechMovement*dt;
 if ((pMechShake.y>fMechMaxSpring) || (pMechShake.y<-fMechMaxSpring))
  vMechVelocity.y=-vMechVelocity.y;
 //ABu011104: 5*pMechShake.y, zeby ladnie pudlem rzucalo :)
 pNewMechPosition=pMechOffset+vector3(pMechShake.x,5*pMechShake.y,pMechShake.z);
 vMechMovement=vMechMovement/2;
 //numer kabiny (-1: kabina B)
 iCabn=(DynamicObject->MoverParameters->ActiveCab==-1 ? 2 : DynamicObject->MoverParameters->ActiveCab);
 if (!DebugModeFlag)
 {//sprawdzaj wiêzy //Ra: nie tu!
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
 pMechPosition=DynamicObject->mMatrix*pNewMechPosition;
 pMechPosition+=DynamicObject->GetPosition();
};

//#include "dbgForm.h"

bool __fastcall TTrain::Update()
{
 DWORD stat;
 double dt=Timer::GetDeltaTime();
 if (DynamicObject->mdKabina)
 {//Ra: TODO: odczyty klawiatury/pulpitu nie powinny byæ uzale¿nione od istnienia modelu kabiny 
  tor=DynamicObject->GetTrack(); //McZapkie-180203
  //McZapkie: predkosc wyswietlana na tachometrze brana jest z obrotow kol
  float maxtacho=3;
  fTachoVelocity=abs(11.31*DynamicObject->MoverParameters->WheelDiameter*DynamicObject->MoverParameters->nrot);
  if (fTachoVelocity>1) //McZapkie-270503: podkrecanie tachometru
  {
   if (fTachoCount<maxtacho)
    fTachoCount+=dt;
  }
  else
   if (fTachoCount>0)
    fTachoCount-=dt;

/* Ra: to by trzeba by³o przemyœleæ, zmienione na szybko problemy robi
  //McZapkie: predkosc wyswietlana na tachometrze brana jest z obrotow kol
  double vel=fabs(11.31*DynamicObject->MoverParameters->WheelDiameter*DynamicObject->MoverParameters->nrot);
  if (iSekunda!=floor(GlobalTime->mr)||(vel<1.0))
  {fTachoVelocity=vel;
   if (fTachoVelocity>1.0) //McZapkie-270503: podkrecanie tachometru
   {
    if (fTachoCount<maxtacho)
     fTachoCount+=dt;
   }
   else
    if (fTachoCount>0)
     fTachoCount-=dt;
   if (DynamicObject->MoverParameters->TrainType==dt_EZT)
    //dla EZT wskazówka porusza siê niestabilnie
    if (fTachoVelocity>7.0)
    {fTachoVelocity=floor(0.5+fTachoVelocity+random(5)-random(5)); //*floor(0.2*fTachoVelocity);
     if (fTachoVelocity<0.0) fTachoVelocity=0.0;
    }
   iSekunda=floor(GlobalTime->mr);
  }
*/

  //------------------
  //hunter-261211: nadmiarowy przetwornicy i ogrzewania
  if (DynamicObject->MoverParameters->ConverterFlag==true)
   {
    fConverterTimer+=dt;
     if ((DynamicObject->MoverParameters->CompressorFlag==true)&&(DynamicObject->MoverParameters->CompressorPower!=0)&&((DynamicObject->MoverParameters->EngineType==ElectricSeriesMotor)||(DynamicObject->MoverParameters->TrainType==dt_EZT))&&(DynamicObject->Controller==Humandriver)) //hunter-110212: poprawka dla EZT
      {
       if (fConverterTimer<fConverterPrzekaznik)
        {
         DynamicObject->MoverParameters->ConvOvldFlag=true;
         DynamicObject->MoverParameters->MainSwitch(false);
        }
       else if (fConverterTimer>=fConverterPrzekaznik)
        DynamicObject->MoverParameters->CompressorSwitch(true);
      }
   }
  else
   fConverterTimer=0;
  //------------------


  double vol=0;
//    int freq=1;
   double dfreq;

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
             dfreq=rsRunningNoise.FM*DynamicObject->MoverParameters->Vel+rsRunningNoise.FA;
             vol=rsRunningNoise.AM*DynamicObject->MoverParameters->Vel+rsRunningNoise.AA;
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
             dfreq=rsRunningNoise.FM*DynamicObject->MoverParameters->Vel+rsRunningNoise.FA;
             vol=rsRunningNoise.AM*DynamicObject->MoverParameters->Vel+rsRunningNoise.AA;
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
          vol=vol*(20.0+tor->iDamageFlag)/21;
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

  if (TestFlag(DynamicObject->MoverParameters->SoundFlag,sound_relay)) // przekaznik - gdy bezpiecznik, automatyczny rozruch itp
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
       dsbRelay->Play(0,0,0);
     else
      {
        if (TestFlag(DynamicObject->MoverParameters->SoundFlag,sound_loud))
         dsbWejscie_na_bezoporow->Play(0,0,0);
        else
         dsbWejscie_na_drugi_uklad->Play(0,0,0);
      }
  }
  // potem dorobic bufory, sprzegi jako RealSound.
  if (TestFlag(DynamicObject->MoverParameters->SoundFlag,sound_bufferclamp)) // zderzaki uderzaja o siebie
   {
    if (TestFlag(DynamicObject->MoverParameters->SoundFlag,sound_loud))
     dsbBufferClamp->SetVolume(DSBVOLUME_MAX);
    else
     dsbBufferClamp->SetVolume(-20);
    dsbBufferClamp->Play(0,0,0);
  }
  if (dsbCouplerStretch)
   if (TestFlag(DynamicObject->MoverParameters->SoundFlag,sound_couplerstretch)) // sprzegi sie rozciagaja
   {
    if (TestFlag(DynamicObject->MoverParameters->SoundFlag,sound_loud))
     dsbCouplerStretch->SetVolume(DSBVOLUME_MAX);
    else
     dsbCouplerStretch->SetVolume(-20);
    dsbCouplerStretch->Play(0,0,0);
   }

  if (DynamicObject->MoverParameters->SoundFlag==0)
   if (DynamicObject->MoverParameters->EventFlag)
    if (TestFlag(DynamicObject->MoverParameters->DamageFlag,dtrain_wheelwear))
     {
      if (rsRunningNoise.AM!=0)
       {
        rsRunningNoise.Stop();
        //float aa=rsRunningNoise.AA;
        float am=rsRunningNoise.AM;
        float fa=rsRunningNoise.FA;
        float fm=rsRunningNoise.FM;
        rsRunningNoise.Init("lomotpodkucia.wav",-1,0,0,0,true);    //MC: zmiana szumu na lomot
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

//if (!DebugModeFlag) try{//podobno to tutaj sypie DDS //trykacz i tak nie dzia³a

//McZapkie-030402: poprawione i uzupelnione amperomierze
if (!ShowNextCurrent)
{   if (I1Gauge.SubModel)
     {
      I1Gauge.UpdateValue(DynamicObject->MoverParameters->ShowCurrent(1));
      I1Gauge.Update();
     }
    if (I2Gauge.SubModel)
     {
      I2Gauge.UpdateValue(DynamicObject->MoverParameters->ShowCurrent(2));
      I2Gauge.Update();
     }
    if (I3Gauge.SubModel)
     {
      I3Gauge.UpdateValue(DynamicObject->MoverParameters->ShowCurrent(3));
      I3Gauge.Update();
     }
    if (ItotalGauge.SubModel)
     {
      ItotalGauge.UpdateValue(DynamicObject->MoverParameters->ShowCurrent(0));
      ItotalGauge.Update();
     }
     if (I1GaugeB.SubModel)
     {
      I1GaugeB.UpdateValue(DynamicObject->MoverParameters->ShowCurrent(1));
      I1GaugeB.Update();
     }
    if (I2GaugeB.SubModel)
     {
      I2GaugeB.UpdateValue(DynamicObject->MoverParameters->ShowCurrent(2));
      I2GaugeB.Update();
     }
    if (I3GaugeB.SubModel)
     {
      I3GaugeB.UpdateValue(DynamicObject->MoverParameters->ShowCurrent(3));
      I3GaugeB.Update();
     }
    if (ItotalGaugeB.SubModel)
     {
      ItotalGaugeB.UpdateValue(DynamicObject->MoverParameters->ShowCurrent(0));
      ItotalGaugeB.Update();
     }
}
else
{
   //ABu 100205: prad w nastepnej lokomotywie, przycisk w ET41
   TDynamicObject *tmp;
   tmp=NULL;
   if (DynamicObject->NextConnected) //pojazd od strony sprzêgu 1
      if ((DynamicObject->NextConnected->MoverParameters->TrainType==dt_ET41)
         && TestFlag(DynamicObject->MoverParameters->Couplers[1].CouplingFlag,ctrain_controll))
         tmp=DynamicObject->NextConnected;
   if (DynamicObject->PrevConnected) //pojazd od strony sprzêgu 0
      if ((DynamicObject->PrevConnected->MoverParameters->TrainType==dt_ET41)
         && TestFlag(DynamicObject->MoverParameters->Couplers[0].CouplingFlag,ctrain_controll))
         tmp=DynamicObject->PrevConnected;
   if (tmp)
   {
      if (I1Gauge.SubModel)
      {
         I1Gauge.UpdateValue(tmp->MoverParameters->ShowCurrent(1)*1.05);
         I1Gauge.Update();
      }
      if (I2Gauge.SubModel)
      {
         I2Gauge.UpdateValue(tmp->MoverParameters->ShowCurrent(2)*1.05);
         I2Gauge.Update();
      }
      if (I3Gauge.SubModel)
      {
         I3Gauge.UpdateValue(tmp->MoverParameters->ShowCurrent(3)*1.05);
         I3Gauge.Update();
      }
      if (ItotalGauge.SubModel)
      {
         ItotalGauge.UpdateValue(tmp->MoverParameters->ShowCurrent(0)*1.05);
         ItotalGauge.Update();
      }
      if (I1GaugeB.SubModel)
      {
         I1GaugeB.UpdateValue(tmp->MoverParameters->ShowCurrent(1)*1.05);
         I1GaugeB.Update();
      }
      if (I2GaugeB.SubModel)
      {
         I2GaugeB.UpdateValue(tmp->MoverParameters->ShowCurrent(2)*1.05);
         I2GaugeB.Update();
      }
      if (I3GaugeB.SubModel)
      {
         I3GaugeB.UpdateValue(tmp->MoverParameters->ShowCurrent(3)*1.05);
         I3GaugeB.Update();
      }
      if (ItotalGaugeB.SubModel)
      {
         ItotalGaugeB.UpdateValue(tmp->MoverParameters->ShowCurrent(0)*1.05);
         ItotalGaugeB.Update();
      }
   }
   else
   {
      if (I1Gauge.SubModel)
      {
         I1Gauge.UpdateValue(0);
         I1Gauge.Update();
      }
      if (I2Gauge.SubModel)
      {
         I2Gauge.UpdateValue(0);
         I2Gauge.Update();
      }
      if (I3Gauge.SubModel)
      {
         I3Gauge.UpdateValue(0);
         I3Gauge.Update();
      }
      if (ItotalGauge.SubModel)
      {
         ItotalGauge.UpdateValue(0);
         ItotalGauge.Update();
      }
      if (I1GaugeB.SubModel)
      {
         I1GaugeB.UpdateValue(0);
         I1GaugeB.Update();
      }
      if (I2GaugeB.SubModel)
      {
         I2GaugeB.UpdateValue(0);
         I2GaugeB.Update();
      }
      if (I3GaugeB.SubModel)
      {
         I3GaugeB.UpdateValue(0);
         I3GaugeB.Update();
      }
      if (ItotalGaugeB.SubModel)
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
   if (tmp)
    if (tmp->MoverParameters->Power>0)
     {
      if (I1BGauge.SubModel)
      {
         I1BGauge.UpdateValue(tmp->MoverParameters->ShowCurrent(1));
         I1BGauge.Update();
      }
      if (I2BGauge.SubModel)
      {
         I2BGauge.UpdateValue(tmp->MoverParameters->ShowCurrent(2));
         I2BGauge.Update();
      }
      if (I3BGauge.SubModel)
      {
         I3BGauge.UpdateValue(tmp->MoverParameters->ShowCurrent(3));
         I3BGauge.Update();
      }
      if (ItotalBGauge.SubModel)
      {
         ItotalBGauge.UpdateValue(tmp->MoverParameters->ShowCurrent(0));
         ItotalBGauge.Update();
      }
     }
}

//}catch(...){WriteLog("!!!! Problem z amperomierzami");}; //trykacz i tak nie dzia³a

//McZapkie-240302    VelocityGauge.UpdateValue(DynamicObject->GetVelocity());
    //fHaslerTimer+=dt;
    //if (fHaslerTimer>fHaslerTime)
    {//Ra: ryzykowne jest to, gdy¿ mo¿e siê nie uaktualniaæ prêdkoœæ
     //Ra: prêdkoœæ siê powinna zaokr¹glaæ tam gdzie siê liczy fTachoVelocity
     if (VelocityGauge.SubModel)
     {//ZiomalCl: wskazanie Haslera w kabinie A ze zwloka czasowa oraz odpowiednia tolerancja
      //Nalezy sie zastanowic na przyszlosc nad rozroznieniem predkosciomierzy (dokladnosc wskazan, zwloka czasowa wskazania, inne funkcje)
      //ZiomalCl: W ezt typu stare EN57 wskazania haslera sa mniej dokladne (linka)
      //VelocityGauge.UpdateValue(fTachoVelocity>2?fTachoVelocity+0.5-random(DynamicObject->MoverParameters->TrainType==dt_EZT?5:2)/2:0);
      VelocityGauge.UpdateValue(fTachoVelocity);
      VelocityGauge.Update();
     }
     if (VelocityGaugeB.SubModel)
     {//ZiomalCl: wskazanie Haslera w kabinie B ze zwloka czasowa oraz odpowiednia tolerancja
      //Nalezy sie zastanowic na przyszlosc nad rozroznieniem predkosciomierzy (dokladnosc wskazan, zwloka czasowa wskazania, inne funkcje)
      //VelocityGaugeB.UpdateValue(fTachoVelocity>2?fTachoVelocity+0.5-random(DynamicObject->MoverParameters->TrainType==dt_EZT?5:2)/2:0);
      VelocityGaugeB.UpdateValue(fTachoVelocity);
      VelocityGaugeB.Update();
     }
     //fHaslerTimer-=fHaslerTime; //1.2s (???)
    }
//McZapkie-300302: zegarek
    if (ClockMInd.SubModel)
     {
      ClockSInd.UpdateValue(int(GlobalTime->mr));
      ClockSInd.Update();     
      ClockMInd.UpdateValue(GlobalTime->mm);
      ClockMInd.Update();
      ClockHInd.UpdateValue(GlobalTime->hh+GlobalTime->mm/60.0);
      ClockHInd.Update();
     }

    if (CylHamGauge.SubModel)
     {
      CylHamGauge.UpdateValue(DynamicObject->MoverParameters->BrakePress);
      CylHamGauge.Update();
     }
    if (CylHamGaugeB.SubModel)
     {
      CylHamGaugeB.UpdateValue(DynamicObject->MoverParameters->BrakePress);
      CylHamGaugeB.Update();
     }
    if (PrzGlGauge.SubModel)
     {
      PrzGlGauge.UpdateValue(DynamicObject->MoverParameters->PipePress);
      PrzGlGauge.Update();
     }
    if (PrzGlGaugeB.SubModel)
     {
      PrzGlGaugeB.UpdateValue(DynamicObject->MoverParameters->PipePress);
      PrzGlGaugeB.Update();
     }
// McZapkie! - zamiast pojemnosci cisnienie
    if (ZbGlGauge.SubModel)
     {
      ZbGlGauge.UpdateValue(DynamicObject->MoverParameters->Compressor);
      ZbGlGauge.Update();
     }
    if (ZbGlGaugeB.SubModel)
     {
      ZbGlGaugeB.UpdateValue(DynamicObject->MoverParameters->Compressor);
      ZbGlGaugeB.Update();
     }
     
    if (HVoltageGauge.SubModel)
     {
      if (DynamicObject->MoverParameters->EngineType!=DieselElectric)
        HVoltageGauge.UpdateValue(DynamicObject->MoverParameters->RunningTraction.TractionVoltage); //Winger czy to nie jest zle? *DynamicObject->MoverParameters->Mains);
      else
        HVoltageGauge.UpdateValue(DynamicObject->MoverParameters->Voltage);
      HVoltageGauge.Update();
     }

//youBy - napiecie na silnikach
    if (EngineVoltage.SubModel)
     {
      if (DynamicObject->MoverParameters->DynamicBrakeFlag)
       { EngineVoltage.UpdateValue(abs(DynamicObject->MoverParameters->Im*5)); }
      else
        {
         int x;
         if ((DynamicObject->MoverParameters->TrainType==dt_ET42)&&(DynamicObject->MoverParameters->Imax==DynamicObject->MoverParameters->ImaxHi))
         x=1;else x=2;
         if (DynamicObject->MoverParameters->RList[DynamicObject->MoverParameters->MainCtrlActualPos].Mn>0)
          { EngineVoltage.UpdateValue((x*(DynamicObject->MoverParameters->RunningTraction.TractionVoltage-DynamicObject->MoverParameters->RList[DynamicObject->MoverParameters->MainCtrlActualPos].R*abs(DynamicObject->MoverParameters->Im))/DynamicObject->MoverParameters->RList[DynamicObject->MoverParameters->MainCtrlActualPos].Mn)); }
         else
          { EngineVoltage.UpdateValue(0); }}
      EngineVoltage.Update();
     }

//Winger 140404 - woltomierz NN
    if (LVoltageGauge.SubModel)
     {
      LVoltageGauge.UpdateValue(DynamicObject->MoverParameters->BatteryVoltage);
      LVoltageGauge.Update();
     }

    if (DynamicObject->MoverParameters->EngineType==DieselElectric)
     {
      if (enrot1mGauge.SubModel)
       {
        enrot1mGauge.UpdateValue(DynamicObject->MoverParameters->ShowEngineRotation(1));
        enrot1mGauge.Update();
       }
      if (enrot2mGauge.SubModel)
       {
        enrot2mGauge.UpdateValue(DynamicObject->MoverParameters->ShowEngineRotation(2));
        enrot2mGauge.Update();
       }
      if (I1Gauge.SubModel)
       {
        I1Gauge.UpdateValue(abs(DynamicObject->MoverParameters->Im));
        I1Gauge.Update();
       }
      if (I2Gauge.SubModel)
       {
        I2Gauge.UpdateValue(abs(DynamicObject->MoverParameters->Im));
        I2Gauge.Update();
       }
      if (HVoltageGauge.SubModel)
       {
        HVoltageGauge.UpdateValue(DynamicObject->MoverParameters->Voltage);
        HVoltageGauge.Update();
       }

     }



    if (DynamicObject->MoverParameters->EngineType==DieselEngine)
     {
      if (enrot1mGauge.SubModel)
       {
        enrot1mGauge.UpdateValue(DynamicObject->MoverParameters->ShowEngineRotation(1));
        enrot1mGauge.Update();
       }
      if (enrot2mGauge.SubModel)
         {
          enrot2mGauge.UpdateValue(DynamicObject->MoverParameters->ShowEngineRotation(2));
          enrot2mGauge.Update();
         }
      if (enrot3mGauge.SubModel)
       if (DynamicObject->MoverParameters->Couplers[1].Connected)
         {
          enrot3mGauge.UpdateValue(DynamicObject->MoverParameters->ShowEngineRotation(3));
          enrot3mGauge.Update();
         }
      if (engageratioGauge.SubModel)
       {
        engageratioGauge.UpdateValue(DynamicObject->MoverParameters->dizel_engage);
        engageratioGauge.Update();
       }
      if (maingearstatusGauge.SubModel)
       {
        if (DynamicObject->MoverParameters->Mains)
         maingearstatusGauge.UpdateValue(1.1-fabs(DynamicObject->MoverParameters->dizel_automaticgearstatus));
        else
         maingearstatusGauge.UpdateValue(0);
        maingearstatusGauge.Update();
       }
      if (IgnitionKeyGauge.SubModel)
       {
        IgnitionKeyGauge.UpdateValue(DynamicObject->MoverParameters->dizel_enginestart);
        IgnitionKeyGauge.Update();
       }
     }

    if  (DynamicObject->MoverParameters->SlippingWheels)
    {
     double veldiff=(DynamicObject->GetVelocity()-fTachoVelocity)/DynamicObject->MoverParameters->Vmax;
     if (veldiff<0)
     {
      if (fabs(DynamicObject->MoverParameters->Im)>10.0)
       btLampkaPoslizg.TurnOn();
      rsSlippery.Play(-rsSlippery.AM*veldiff+rsSlippery.AA,DSBPLAY_LOOPING,true,DynamicObject->GetPosition());
      if (DynamicObject->MoverParameters->TrainType==dt_181) //alarm przy poslizgu dla 181/182 - BOMBARDIER
       if (dsbSlipAlarm) dsbSlipAlarm->Play(0,0,DSBPLAY_LOOPING);
     }
     else
     {
      if ((DynamicObject->MoverParameters->UnitBrakeForce>100.0) && (DynamicObject->GetVelocity()>1.0))
      {
       rsSlippery.Play(rsSlippery.AM*veldiff+rsSlippery.AA,DSBPLAY_LOOPING,true,DynamicObject->GetPosition());
       if (DynamicObject->MoverParameters->TrainType==dt_181)
        if (dsbSlipAlarm) dsbSlipAlarm->Stop();
      }
     }
    }
    else
    {
     btLampkaPoslizg.TurnOff();
     rsSlippery.Stop();
     if (DynamicObject->MoverParameters->TrainType==dt_181)
      if (dsbSlipAlarm) dsbSlipAlarm->Stop();
    }

    if ( DynamicObject->MoverParameters->Mains )
     {
        btLampkaWylSzybki.TurnOn();
        if ( DynamicObject->MoverParameters->ResistorsFlagCheck())
          btLampkaOpory.TurnOn();
        else
          btLampkaOpory.TurnOff();
        if ( (DynamicObject->MoverParameters->Itot!=0) || (DynamicObject->MoverParameters->BrakePress > 0.2) || ( DynamicObject->MoverParameters->PipePress < 0.36 ))
          btLampkaStyczn.TurnOff();     //
        else
        if (DynamicObject->MoverParameters->BrakePress < 0.1)
           btLampkaStyczn.TurnOn();      //mozna prowadzic rozruch

        //hunter-111211: wylacznik cisnieniowy
        if (DynamicObject->MoverParameters->TrainType!=dt_EZT)
         if (((DynamicObject->MoverParameters->BrakePress > 0.2) || ( DynamicObject->MoverParameters->PipePress < 0.36 )) && ( DynamicObject->MoverParameters->MainCtrlPos != 0 ))
          DynamicObject->MoverParameters->StLinFlag=true;
        //-------

        //hunter-121211: lampka zanikowo-pradowego wentylatorow:
        if ((DynamicObject->MoverParameters->RventRot<5.0) && (DynamicObject->MoverParameters->ResistorsFlagCheck()))
          btLampkaNadmWent.TurnOn();
        else
          btLampkaNadmWent.TurnOff();
        //-------

        if ( DynamicObject->MoverParameters->FuseFlagCheck() )
          btLampkaNadmSil.TurnOn();
        else
          btLampkaNadmSil.TurnOff();

        if ( DynamicObject->MoverParameters->Imax==DynamicObject->MoverParameters->ImaxHi )
          btLampkaWysRozr.TurnOn();
        else
          btLampkaWysRozr.TurnOff();

        if ( DynamicObject->MoverParameters->ActiveDir!=0 ) //napiecie na nastawniku hamulcowym
         { btLampkaNapNastHam.TurnOn(); }
        else
         { btLampkaNapNastHam.TurnOff(); }

        if ( DynamicObject->MoverParameters->CompressorFlag==true ) //mutopsitka dziala
         { btLampkaSprezarka.TurnOn(); }
        else
         { btLampkaSprezarka.TurnOff(); }
        //boczniki
        unsigned char scp; //Ra: dopisa³em "unsigned" 
        scp=DynamicObject->MoverParameters->RList[DynamicObject->MoverParameters->MainCtrlActualPos].ScndAct;
        scp=(scp==255?0:scp);
        if ((DynamicObject->MoverParameters->ScndCtrlActualPos>0)||(DynamicObject->MoverParameters->ScndInMain)&&(scp>0))
         { btLampkaBocznikI.TurnOn(); }
        else
         { btLampkaBocznikI.TurnOff(); }
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

        btLampkaNapNastHam.TurnOff();
        btLampkaSprezarka.TurnOff();
     }

{ //yB - wskazniki drugiego czlonu
   TDynamicObject *tmp;
   tmp=NULL;
   if ((TestFlag(DynamicObject->MoverParameters->Couplers[1].CouplingFlag,ctrain_controll)) && (DynamicObject->MoverParameters->ActiveCab>0))
      tmp=DynamicObject->NextConnected;
   if ((TestFlag(DynamicObject->MoverParameters->Couplers[0].CouplingFlag,ctrain_controll)) && (DynamicObject->MoverParameters->ActiveCab<0))
      tmp=DynamicObject->PrevConnected;

   if (tmp)
    if ( tmp->MoverParameters->Mains )
      {
        btLampkaWylSzybkiB.TurnOn();
        if ( tmp->MoverParameters->ResistorsFlagCheck())
          btLampkaOporyB.TurnOn();
        else
          btLampkaOporyB.TurnOff();
        if ( (tmp->MoverParameters->Itot!=0) || (tmp->MoverParameters->BrakePress > 0.2) || ( tmp->MoverParameters->PipePress < 0.36 ))
          btLampkaStycznB.TurnOff();     //
        else
        if (tmp->MoverParameters->BrakePress < 0.1)
           btLampkaStycznB.TurnOn();      //mozna prowadzic rozruch

        //-----------------
        //hunter-271211: brak jazdy w drugim czlonie, gdy w pierwszym tez nie ma (i odwrotnie)
        if (tmp->MoverParameters->TrainType!=dt_EZT)
         if (((tmp->MoverParameters->BrakePress > 0.2) || ( tmp->MoverParameters->PipePress < 0.36 )) && ( tmp->MoverParameters->MainCtrlPos != 0 ))
          {
           tmp->MoverParameters->MainCtrlActualPos=0; //inaczej StLinFlag nie zmienia sie na false w drugim pojezdzie
           //tmp->MoverParameters->StLinFlag=true;
           DynamicObject->MoverParameters->StLinFlag=true;
          }
        if (DynamicObject->MoverParameters->StLinFlag==true)
         tmp->MoverParameters->MainCtrlActualPos=0; //tmp->MoverParameters->StLinFlag=true;

        //-----------------
        //hunter-271211: sygnalizacja poslizgu w pierwszym pojezdzie, gdy wystapi w drugim
        if  (tmp->MoverParameters->SlippingWheels)
         btLampkaPoslizg.TurnOn();
        else
         btLampkaPoslizg.TurnOff();
        //-----------------


        if ( tmp->MoverParameters->CompressorFlag==true ) //mutopsitka dziala
         { btLampkaSprezarkaB.TurnOn(); }
        else
         { btLampkaSprezarkaB.TurnOff(); }

      }
    else  //wylaczone
      {
        btLampkaWylSzybkiB.TurnOff();
        btLampkaOporyB.TurnOff();
        btLampkaStycznB.TurnOff();
        btLampkaSprezarkaB.TurnOff();
      }

   //hunter-261211: jakis stary kod (i niezgodny z prawda), zahaszowalem
   //if (tmp)
   // if (tmp->MoverParameters->ConverterFlag==true)
   //     btLampkaNadmPrzetwB.TurnOff();
   // else
   //     btLampkaNadmPrzetwB.TurnOn();

} //**************************************************** */

    if ((DynamicObject->MoverParameters->BrakePress>=0.05f*DynamicObject->MoverParameters->MaxBrakePress) || (DynamicObject->MoverParameters->DynamicBrakeFlag))
       btLampkaHamienie.TurnOn();
    else
       btLampkaHamienie.TurnOff();

// NBMX wrzesien 2003 - drzwi oraz sygnal odjazdu
    if (DynamicObject->MoverParameters->DoorLeftOpened)
      btLampkaDoorLeft.TurnOn();
    else
      btLampkaDoorLeft.TurnOff();

    if (DynamicObject->MoverParameters->DoorRightOpened)
      btLampkaDoorRight.TurnOn();
    else
      btLampkaDoorRight.TurnOff();

    if (DynamicObject->MoverParameters->ActiveDir>0)
     btLampkaForward.TurnOn(); //jazda do przodu
    else
     btLampkaForward.TurnOff();

    if (DynamicObject->MoverParameters->ActiveDir<0)
     btLampkaBackward.TurnOn(); //jazda do ty³u
    else
     btLampkaBackward.TurnOff();

//McZapkie-080602: obroty (albo translacje) regulatorow
    if (MainCtrlGauge.SubModel)
     {
      if (DynamicObject->MoverParameters->CoupledCtrl)
       MainCtrlGauge.UpdateValue(double(DynamicObject->MoverParameters->MainCtrlPos+DynamicObject->MoverParameters->ScndCtrlPos));
      else
       MainCtrlGauge.UpdateValue(double(DynamicObject->MoverParameters->MainCtrlPos));
      MainCtrlGauge.Update();
     }
    if (MainCtrlActGauge.SubModel)
     {
      if (DynamicObject->MoverParameters->CoupledCtrl)
       MainCtrlActGauge.UpdateValue(double(DynamicObject->MoverParameters->MainCtrlActualPos+DynamicObject->MoverParameters->ScndCtrlActualPos));
      else
       MainCtrlActGauge.UpdateValue(double(DynamicObject->MoverParameters->MainCtrlActualPos));
      MainCtrlActGauge.Update();
     }
    if (ScndCtrlGauge.SubModel)
     {//Ra: od byte odejmowane boolean i konwertowane potem na double?
      ScndCtrlGauge.UpdateValue(double(DynamicObject->MoverParameters->ScndCtrlPos-((DynamicObject->MoverParameters->TrainType==dt_ET42)&&DynamicObject->MoverParameters->DynamicBrakeFlag)));
      ScndCtrlGauge.Update();
     }
    if (DirKeyGauge.SubModel)
     {
      if (DynamicObject->MoverParameters->TrainType!=dt_EZT)
        DirKeyGauge.UpdateValue(double(DynamicObject->MoverParameters->ActiveDir));
      else
        DirKeyGauge.UpdateValue(double(DynamicObject->MoverParameters->ActiveDir)+
                                double(DynamicObject->MoverParameters->Imin==
                                       DynamicObject->MoverParameters->IminHi));
      DirKeyGauge.Update();
     }
    if (BrakeCtrlGauge.SubModel)
    {if (DynamicObject->Mechanik?(DynamicObject->Mechanik->AIControllFlag?false:Global::iFeedbackMode==4):false) //nie blokujemy AI
     {//Ra: nie najlepsze miejsce, ale na pocz¹tek gdzieœ to daæ trzeba
      double b=Console::AnalogGet(0); //odczyt z pulpitu i modyfikacja pozycji kranu
      if ((b>=0.0))//&&(DynamicObject->MoverParameters->BrakeHandle==FV4a))
      {b=(((Global::fCalibrateIn[0][3]*b)+Global::fCalibrateIn[0][2])*b+Global::fCalibrateIn[0][1])*b+Global::fCalibrateIn[0][0];
       if (b<-2.0) b=-2.0; else if (b>DynamicObject->MoverParameters->BrakeCtrlPosNo) b=DynamicObject->MoverParameters->BrakeCtrlPosNo;
       BrakeCtrlGauge.UpdateValue(b); //przesów bez zaokr¹glenia
       DynamicObject->MoverParameters->BrakeLevelSet(b);
      }
      //else //standardowa prodedura z kranem powi¹zanym z klawiatur¹
      // BrakeCtrlGauge.UpdateValue(double(DynamicObject->MoverParameters->BrakeCtrlPos));
     }
     //else //standardowa prodedura z kranem powi¹zanym z klawiatur¹
     // BrakeCtrlGauge.UpdateValue(double(DynamicObject->MoverParameters->BrakeCtrlPos));
     BrakeCtrlGauge.UpdateValue(DynamicObject->MoverParameters->fBrakeCtrlPos);
     BrakeCtrlGauge.Update();
    }
    if (LocalBrakeGauge.SubModel)
    {if (DynamicObject->Mechanik?(DynamicObject->Mechanik->AIControllFlag?false:Global::iFeedbackMode==4):false) //nie blokujemy AI
     {//Ra: nie najlepsze miejsce, ale na pocz¹tek gdzieœ to daæ trzeba
      double b=Console::AnalogGet(1); //odczyt z pulpitu i modyfikacja pozycji kranu
      if (b>=0.0)
      {b=(((Global::fCalibrateIn[1][3]*b)+Global::fCalibrateIn[1][2])*b+Global::fCalibrateIn[1][1])*b+Global::fCalibrateIn[1][0];
       if (b<0.0) b=0.0; else if (b>LocalBrakePosNo) b=LocalBrakePosNo;
       LocalBrakeGauge.UpdateValue(b); //przesów bez zaokr¹glenia
       DynamicObject->MoverParameters->LocalBrakePos=int(b); //sposób zaokr¹glania jest do ustalenia
      }
      else //standardowa prodedura z kranem powi¹zanym z klawiatur¹
       LocalBrakeGauge.UpdateValue(double(DynamicObject->MoverParameters->LocalBrakePos));
     }
     else //standardowa prodedura z kranem powi¹zanym z klawiatur¹
      LocalBrakeGauge.UpdateValue(double(DynamicObject->MoverParameters->LocalBrakePos));
     LocalBrakeGauge.Update();
    }

    if (BrakeProfileCtrlGauge.SubModel)
    {
     BrakeProfileCtrlGauge.UpdateValue(double(DynamicObject->MoverParameters->BrakeDelayFlag==2?0.5:DynamicObject->MoverParameters->BrakeDelayFlag));
     BrakeProfileCtrlGauge.Update();
    }

    if (MaxCurrentCtrlGauge.SubModel)
     {
      MaxCurrentCtrlGauge.UpdateValue(double(DynamicObject->MoverParameters->Imax==DynamicObject->MoverParameters->ImaxHi));
      MaxCurrentCtrlGauge.Update();
     }

// NBMX wrzesien 2003 - drzwi
    if (DoorLeftButtonGauge.SubModel)
    {
      if (DynamicObject->MoverParameters->DoorLeftOpened)
        DoorLeftButtonGauge.PutValue(1);
      else
        DoorLeftButtonGauge.PutValue(0);
      DoorLeftButtonGauge.Update();
    }
    if (DoorRightButtonGauge.SubModel)
    {
      if (DynamicObject->MoverParameters->DoorRightOpened)
        DoorRightButtonGauge.PutValue(1);
      else
        DoorRightButtonGauge.PutValue(0);
      DoorRightButtonGauge.Update();
    }
    if (DepartureSignalButtonGauge.SubModel)
     {
//      DepartureSignalButtonGauge.UpdateValue(double());
      DepartureSignalButtonGauge.Update();
     }

//NBMX dzwignia sprezarki
//NBMX dzwignia sprezarki
    if (CompressorButtonGauge.SubModel)  //hunter-261211: poprawka
      CompressorButtonGauge.Update();
    if (MainButtonGauge.SubModel)
       MainButtonGauge.Update();
    if (RadioButtonGauge.SubModel)
     {
      RadioButtonGauge.PutValue(1);
      RadioButtonGauge.Update();
     }
    if (ConverterButtonGauge.SubModel)
      ConverterButtonGauge.Update();
    if (ConverterOffButtonGauge.SubModel)
      ConverterOffButtonGauge.Update();

    if (((DynamicObject->iLights[0])==0)
      &&((DynamicObject->iLights[1])==0))
     {
        RightLightButtonGauge.PutValue(0);
        LeftLightButtonGauge.PutValue(0);
        UpperLightButtonGauge.PutValue(0);
        RightEndLightButtonGauge.PutValue(0);
        LeftEndLightButtonGauge.PutValue(0);
     }

     //---------
     //hunter-101211: poprawka na zle obracajace sie przelaczniki
     /*
     if (((DynamicObject->iLights[0]&1)==1)
      ||((DynamicObject->iLights[1]&1)==1))
        LeftLightButtonGauge.PutValue(1);
     if (((DynamicObject->iLights[0]&16)==16)
      ||((DynamicObject->iLights[1]&16)==16))
        RightLightButtonGauge.PutValue(1);
     if (((DynamicObject->iLights[0]&4)==4)
      ||((DynamicObject->iLights[1]&4)==4))
        UpperLightButtonGauge.PutValue(1);

     if (((DynamicObject->iLights[0]&2)==2)
      ||((DynamicObject->iLights[1]&2)==2))
        if (LeftEndLightButtonGauge.SubModel)
        {
           LeftEndLightButtonGauge.PutValue(1);
           LeftLightButtonGauge.PutValue(0);
        }
        else
           LeftLightButtonGauge.PutValue(-1);

     if (((DynamicObject->iLights[0]&32)==32)
      ||((DynamicObject->iLights[1]&32)==32))
        if (RightEndLightButtonGauge.SubModel)
        {
           RightEndLightButtonGauge.PutValue(1);
           RightLightButtonGauge.PutValue(0);
        }
        else
           RightLightButtonGauge.PutValue(-1);
      */

     //--------------
     //hunter-230112

     //REFLEKTOR LEWY
     //glowne oswietlenie
     if ((DynamicObject->iLights[0]&1)==1)
      if ((DynamicObject->MoverParameters->ActiveCab)==1)
        LeftLightButtonGauge.PutValue(1);
      else if ((DynamicObject->MoverParameters->ActiveCab)==-1)
        RearLeftLightButtonGauge.PutValue(1);

     if ((DynamicObject->iLights[1]&1)==1)
      if ((DynamicObject->MoverParameters->ActiveCab)==-1)
        LeftLightButtonGauge.PutValue(1);
      else if ((DynamicObject->MoverParameters->ActiveCab)==1)
        RearLeftLightButtonGauge.PutValue(1);


     //koncowki
     if ((DynamicObject->iLights[0]&2)==2)
      if ((DynamicObject->MoverParameters->ActiveCab)==1)
       {
        if (LeftEndLightButtonGauge.SubModel)
        {
           LeftEndLightButtonGauge.PutValue(1);
           LeftLightButtonGauge.PutValue(0);
        }
        else
           LeftLightButtonGauge.PutValue(-1);
       }
      else if ((DynamicObject->MoverParameters->ActiveCab)==-1)
       {
        if (RearLeftEndLightButtonGauge.SubModel)
        {
           RearLeftEndLightButtonGauge.PutValue(1);
           RearLeftLightButtonGauge.PutValue(0);
        }
        else
           RearLeftLightButtonGauge.PutValue(-1);
       }

     if ((DynamicObject->iLights[1]&2)==2)
      if ((DynamicObject->MoverParameters->ActiveCab)==-1)
      {
        if (LeftEndLightButtonGauge.SubModel)
        {
           LeftEndLightButtonGauge.PutValue(1);
           LeftLightButtonGauge.PutValue(0);
        }
        else
           LeftLightButtonGauge.PutValue(-1);
      }
      else if ((DynamicObject->MoverParameters->ActiveCab)==1)
      {
        if (RearLeftEndLightButtonGauge.SubModel)
        {
           RearLeftEndLightButtonGauge.PutValue(1);
           RearLeftLightButtonGauge.PutValue(0);
        }
        else
           RearLeftLightButtonGauge.PutValue(-1);
      }
     //--------------
     //REFLEKTOR GORNY
     if ((DynamicObject->iLights[0]&4)==4)
      if ((DynamicObject->MoverParameters->ActiveCab)==1)
        UpperLightButtonGauge.PutValue(1);
      else if ((DynamicObject->MoverParameters->ActiveCab)==-1)
        RearUpperLightButtonGauge.PutValue(1);

     if ((DynamicObject->iLights[1]&4)==4)
      if ((DynamicObject->MoverParameters->ActiveCab)==-1)
        UpperLightButtonGauge.PutValue(1);
      else if ((DynamicObject->MoverParameters->ActiveCab)==1)
        RearUpperLightButtonGauge.PutValue(1);
     //--------------
     //REFLEKTOR PRAWY
     //glowne oswietlenie
     if ((DynamicObject->iLights[0]&16)==16)
      if ((DynamicObject->MoverParameters->ActiveCab)==1)
        RightLightButtonGauge.PutValue(1);
      else if ((DynamicObject->MoverParameters->ActiveCab)==-1)
        RearRightLightButtonGauge.PutValue(1);

     if ((DynamicObject->iLights[1]&16)==16)
      if ((DynamicObject->MoverParameters->ActiveCab)==-1)
        RightLightButtonGauge.PutValue(1);
      else if ((DynamicObject->MoverParameters->ActiveCab)==1)
        RearRightLightButtonGauge.PutValue(1);


     //koncowki
     if ((DynamicObject->iLights[0]&32)==32)
      if ((DynamicObject->MoverParameters->ActiveCab)==1)
       {
        if (RightEndLightButtonGauge.SubModel)
        {
           RightEndLightButtonGauge.PutValue(1);
           RightLightButtonGauge.PutValue(0);
        }
        else
           RightLightButtonGauge.PutValue(-1);
       }
      else if ((DynamicObject->MoverParameters->ActiveCab)==-1)
       {
        if (RearRightEndLightButtonGauge.SubModel)
        {
           RearRightEndLightButtonGauge.PutValue(1);
           RearRightLightButtonGauge.PutValue(0);
        }
        else
           RearRightLightButtonGauge.PutValue(-1);
       }

     if ((DynamicObject->iLights[1]&32)==32)
      if ((DynamicObject->MoverParameters->ActiveCab)==-1)
      {
        if (RightEndLightButtonGauge.SubModel)
        {
           RightEndLightButtonGauge.PutValue(1);
           RightLightButtonGauge.PutValue(0);
        }
        else
           RightLightButtonGauge.PutValue(-1);
      }
      else if ((DynamicObject->MoverParameters->ActiveCab)==1)
      {
        if (RearRightEndLightButtonGauge.SubModel)
        {
           RearRightEndLightButtonGauge.PutValue(1);
           RearRightLightButtonGauge.PutValue(0);
        }
        else
           RearRightLightButtonGauge.PutValue(-1);
      }

    //---------
//Winger 010304 - pantografy
    if (PantFrontButtonGauge.SubModel)
    {
      if (DynamicObject->MoverParameters->PantFrontUp)
          PantFrontButtonGauge.PutValue(1);
      else
          PantFrontButtonGauge.PutValue(0);
      PantFrontButtonGauge.Update();
    }
    if (PantRearButtonGauge.SubModel)
    {
      if (DynamicObject->MoverParameters->PantRearUp)
          PantRearButtonGauge.PutValue(1);
      else
          PantRearButtonGauge.PutValue(0);
     PantRearButtonGauge.Update();
     }
    if (PantFrontButtonOffGauge.SubModel)
    {
     PantFrontButtonOffGauge.Update();
    }
//Winger 020304 - ogrzewanie
    if (TrainHeatingButtonGauge.SubModel)
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

    //----------
    //hunter-261211: jakis stary kod (i niezgodny z prawda),
    //zahaszowalem i poprawilem
    //if (DynamicObject->MoverParameters->ConverterFlag==true)
    //    btLampkaNadmPrzetw.TurnOff();
    //else
    //    btLampkaNadmPrzetw.TurnOn();
    if (DynamicObject->MoverParameters->ConvOvldFlag==true)
     btLampkaNadmPrzetw.TurnOn();
    else
     btLampkaNadmPrzetw.TurnOff();
    //----------


//McZapkie-141102: SHP i czuwak, TODO: sygnalizacja kabinowa
    if (DynamicObject->MoverParameters->SecuritySystem.Status>0)
     {
       if (fBlinkTimer>fCzuwakBlink)
           fBlinkTimer=-fCzuwakBlink;
       else
           fBlinkTimer+=dt;
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

    //******************************************
    //przelaczniki

    if (Console::Pressed(Global::Keys[k_Horn]))
     {
      if (Console::Pressed(VK_SHIFT))
         {
         SetFlag(DynamicObject->MoverParameters->WarningSignal,2);
         DynamicObject->MoverParameters->WarningSignal&=(255-1);
         if (HornButtonGauge.SubModel)
            HornButtonGauge.UpdateValue(1);
         }
      else
         {
         SetFlag(DynamicObject->MoverParameters->WarningSignal,1);
         DynamicObject->MoverParameters->WarningSignal&=(255-2);
         if (HornButtonGauge.SubModel)
            HornButtonGauge.UpdateValue(-1);
         }
     }
    else
     {
        DynamicObject->MoverParameters->WarningSignal=0;
        if (HornButtonGauge.SubModel)
           HornButtonGauge.UpdateValue(0);
     }

    if ( Console::Pressed(Global::Keys[k_Horn2]) )
     if (Global::Keys[k_Horn2]!=Global::Keys[k_Horn])
     {
        SetFlag(DynamicObject->MoverParameters->WarningSignal,2);
     }

     //----------------
     //hunter-141211: wyl. szybki zalaczony i wylaczony przeniesiony z OnKeyPress()
     if ( Console::Pressed(VK_SHIFT)&&Console::Pressed(Global::Keys[k_Main]) )
     {
          fMainRelayTimer+=dt;
          MainOnButtonGauge.PutValue(1);
          if (DynamicObject->MoverParameters->Mains!=true) //hunter-080812: poprawka
           DynamicObject->MoverParameters->ConverterSwitch(false);
          if (fMainRelayTimer>DynamicObject->MoverParameters->InitialCtrlDelay) //wlaczanie WSa z opoznieniem
            if (DynamicObject->MoverParameters->MainSwitch(true))
            {
              if (DynamicObject->MoverParameters->MainCtrlPos!=0) //zabezpieczenie, by po wrzuceniu pozycji przed wlaczonym
               DynamicObject->MoverParameters->StLinFlag=true; //WSem nie wrzucilo na ta pozycje po jego zalaczeniu

              if (DynamicObject->MoverParameters->EngineType==DieselEngine)
               dsbDieselIgnition->Play(0,0,0);
           }
     }
     else
     {
          if (ConverterButtonGauge.GetValue()!=0)    //po puszczeniu przycisku od WSa odpalanie potwora
           DynamicObject->MoverParameters->ConverterSwitch(true);

          fMainRelayTimer=0;
          MainOnButtonGauge.UpdateValue(0);
     }

     //---

     if ( !Console::Pressed(VK_SHIFT)&&Console::Pressed(Global::Keys[k_Main]) )
     {
          MainOffButtonGauge.PutValue(1);
          if (DynamicObject->MoverParameters->MainSwitch(false))
           dsbRelay->Play(0,0,0);
     }
     else
     {
          MainOffButtonGauge.UpdateValue(0);
     }

     /* if (cKey==Global::Keys[k_Main])     //z shiftem
      {
         MainOnButtonGauge.PutValue(1);
         if (DynamicObject->MoverParameters->MainSwitch(true))
           {
              if (DynamicObject->MoverParameters->MainCtrlPos!=0) //hunter-131211: takie zabezpieczenie
               DynamicObject->MoverParameters->StLinFlag=true;

              if (DynamicObject->MoverParameters->EngineType==DieselEngine)
               dsbDieselIgnition->Play(0,0,0);
              else
               dsbNastawnikJazdy->Play(0,0,0);
           }
      }
      else */

      /* if (cKey==Global::Keys[k_Main])    //bez shifta
      {
        MainOffButtonGauge.PutValue(1);
        if (DynamicObject->MoverParameters->MainSwitch(false))
           {
              dsbNastawnikJazdy->Play(0,0,0);
           }
      }
      else */

     //----------------
     //hunter-131211: czuwak przeniesiony z OnKeyPress
     if ( Console::Pressed(Global::Keys[k_Czuwak]) )
     {
      SecurityResetButtonGauge.PutValue(1);
      if ((DynamicObject->MoverParameters->SecuritySystem.Status&s_aware)&&
          (DynamicObject->MoverParameters->SecuritySystem.Status&s_active))
       {
        DynamicObject->MoverParameters->SecuritySystem.SystemTimer=0;
        DynamicObject->MoverParameters->SecuritySystem.Status-=s_aware;
        DynamicObject->MoverParameters->SecuritySystem.VelocityAllowed=-1;
        CAflag=1;
       }
      else if (CAflag!=1)
        DynamicObject->MoverParameters->SecuritySystemReset();
     }
     else
     {
      SecurityResetButtonGauge.UpdateValue(0);
      CAflag=0;
     }
     //-----------------
     //hunter-201211: piasecznica przeniesiona z OnKeyPress, wlacza sie tylko,
     //gdy trzymamy przycisk, a nie tak jak wczesniej (raz nacisnelo sie 's'
     //i sypala caly czas)
      /*
      if (cKey==Global::Keys[k_Sand])
      {
          if (DynamicObject->MoverParameters->SandDoseOn())
           if (DynamicObject->MoverParameters->SandDose)
            {
              dsbPneumaticRelay->SetVolume(-30);
              dsbPneumaticRelay->Play(0,0,0);
            }
      }
     */


     if ( Console::Pressed(Global::Keys[k_Sand]) )
     {
      DynamicObject->MoverParameters->SandDose=true;
      //DynamicObject->MoverParameters->SandDoseOn(true);
     }
     else
     {
      DynamicObject->MoverParameters->SandDose=false;
      //DynamicObject->MoverParameters->SandDoseOn(false);
      //dsbPneumaticRelay->SetVolume(-30);
      //dsbPneumaticRelay->Play(0,0,0);
     }

     //-----------------
     //hunter-221211: hamowanie przy poslizgu
     if ( Console::Pressed(Global::Keys[k_AntiSlipping]) )
     {
      if (DynamicObject->MoverParameters->BrakeSystem!=ElectroPneumatic)
      {
       AntiSlipButtonGauge.PutValue(1);
       DynamicObject->MoverParameters->AntiSlippingBrake();
      }
     }
     else
     {
      AntiSlipButtonGauge.UpdateValue(0);
     }
     //-----------------
     //hunter-261211: przetwornica i sprezarka
     if ( Console::Pressed(VK_SHIFT)&&Console::Pressed(Global::Keys[k_Converter]) )   //NBMX 14-09-2003: przetwornica wl
      {
        ConverterButtonGauge.PutValue(1);
        if ((DynamicObject->MoverParameters->PantFrontVolt) || (DynamicObject->MoverParameters->PantRearVolt) || (DynamicObject->MoverParameters->EnginePowerSource.SourceType!=CurrentCollector) || (!Global::bLiveTraction))
         DynamicObject->MoverParameters->ConverterSwitch(true);
        if ((DynamicObject->MoverParameters->EngineType!=ElectricSeriesMotor)&&(DynamicObject->MoverParameters->TrainType!=dt_EZT)) //hunter-110212: poprawka dla EZT
         DynamicObject->MoverParameters->CompressorSwitch(true);
      }
     else
      {
       if (DynamicObject->MoverParameters->ConvSwitchType=="impulse")
        {
         ConverterButtonGauge.PutValue(0);
         ConverterOffButtonGauge.PutValue(0);
        }
      }

     if ( Console::Pressed(VK_SHIFT)&&Console::Pressed(Global::Keys[k_Compressor])&&((DynamicObject->MoverParameters->EngineType==ElectricSeriesMotor)||(DynamicObject->MoverParameters->TrainType==dt_EZT)) )   //NBMX 14-09-2003: sprezarka wl
      { //hunter-110212: poprawka dla EZT
        CompressorButtonGauge.PutValue(1);
        DynamicObject->MoverParameters->CompressorSwitch(true);
      }

     if ( !Console::Pressed(VK_SHIFT)&&Console::Pressed(Global::Keys[k_Converter]) )   //NBMX 14-09-2003: przetwornica wl
      {
        ConverterButtonGauge.PutValue(0);
        ConverterOffButtonGauge.PutValue(1);
        DynamicObject->MoverParameters->ConverterSwitch(false);
      }

     if ( !Console::Pressed(VK_SHIFT)&&Console::Pressed(Global::Keys[k_Compressor])&&((DynamicObject->MoverParameters->EngineType==ElectricSeriesMotor)||(DynamicObject->MoverParameters->TrainType==dt_EZT)) )   //NBMX 14-09-2003: sprezarka wl
      { //hunter-110212: poprawka dla EZT
        CompressorButtonGauge.PutValue(0);
        DynamicObject->MoverParameters->CompressorSwitch(false);
      }

      /*
      bez szifta
      if (cKey==Global::Keys[k_Converter])       //NBMX wyl przetwornicy
      {
           if (DynamicObject->MoverParameters->ConverterSwitch(false))
           {
            dsbSwitch->SetVolume(DSBVOLUME_MAX);
            dsbSwitch->Play(0,0,0);;
           }
      }
      else
      if (cKey==Global::Keys[k_Compressor])       //NBMX wyl sprezarka
      {
           if (DynamicObject->MoverParameters->CompressorSwitch(false))
           {
            dsbSwitch->SetVolume(DSBVOLUME_MAX);
            dsbSwitch->Play(0,0,0);;
           }
      }
      else
      */
     //-----------------

     if ( Console::Pressed(Global::Keys[k_Univ1]) )
     {
        if (!DebugModeFlag)
        {
           if (Universal1ButtonGauge.SubModel)
           if (Console::Pressed(VK_SHIFT))
           {
              Universal1ButtonGauge.IncValue(dt/2);
           }
           else
           {
              Universal1ButtonGauge.DecValue(dt/2);
           }
        }
     }
     if ( Console::Pressed(Global::Keys[k_Univ2]) )
     {
        if (!DebugModeFlag)
        {
           if (Universal2ButtonGauge.SubModel)
           if (Console::Pressed(VK_SHIFT))
           {
              Universal2ButtonGauge.IncValue(dt/2);
           }
           else
           {
              Universal2ButtonGauge.DecValue(dt/2);
           }
        }
     }
     if ( Console::Pressed(Global::Keys[k_Univ3]) )
     {
       if (Universal3ButtonGauge.SubModel)
        if (Console::Pressed(VK_CONTROL))
        {//z [Ctrl] zapalamy albo gasimy œwiate³ko w kabinie
/* //tutaj jest bez sensu, trzeba reagowaæ na wciskanie klawisza!
         if (Console::Pressed(VK_SHIFT))
         {//zapalenie
          if (iCabLightFlag<2) ++iCabLightFlag;
         }
         else
         {//gaszenie
          if (iCabLightFlag) --iCabLightFlag;
         }
*/
        }
        else
        {//bez [Ctrl] prze³¹czamy coœtem
         if (Console::Pressed(VK_SHIFT))
         {
          Universal3ButtonGauge.PutValue(1);  //hunter-131211: z UpdateValue na PutValue - by zachowywal sie jak pozostale przelaczniki
          if (btLampkaUniversal3.Active())
           LampkaUniversal3_st=true;
         }
         else
         {
          Universal3ButtonGauge.PutValue(0);  //hunter-131211: z UpdateValue na PutValue - by zachowywal sie jak pozostale przelaczniki
          if (btLampkaUniversal3.Active())
           LampkaUniversal3_st=false;
         }
        }
     }
     //ABu030405 obsluga lampki uniwersalnej:
     if ((LampkaUniversal3_st)&&(btLampkaUniversal3.Active()))
     {
        if (LampkaUniversal3_typ==0)
           btLampkaUniversal3.TurnOn();
        else
        if ((LampkaUniversal3_typ==1)&&(DynamicObject->MoverParameters->Mains==true))
           btLampkaUniversal3.TurnOn();
        else
        if ((LampkaUniversal3_typ==2)&&(DynamicObject->MoverParameters->ConverterFlag==true))
           btLampkaUniversal3.TurnOn();
        else
           btLampkaUniversal3.TurnOff();
     }
     else
     {
        if (btLampkaUniversal3.Active())
           btLampkaUniversal3.TurnOff();
     }

     if (Console::Pressed(Global::Keys[k_Univ4]))
     {
        if (Universal4ButtonGauge.SubModel)
        if (Console::Pressed(VK_SHIFT))
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

     // Odskakiwanie hamulce EP
     if ((!Console::Pressed(Global::Keys[k_DecBrakeLevel]))&&(!Console::Pressed(Global::Keys[k_WaveBrake]) )&&(DynamicObject->MoverParameters->BrakeCtrlPos==-1)&&(DynamicObject->MoverParameters->BrakeSubsystem==Oerlikon)&&(DynamicObject->MoverParameters->BrakeSystem==ElectroPneumatic)&&(DynamicObject->Controller!=AIdriver))
     {
     //DynamicObject->MoverParameters->BrakeCtrlPos=(DynamicObject->MoverParameters->BrakeCtrlPos)+1;
     //DynamicObject->MoverParameters->IncBrakeLevel();
     DynamicObject->MoverParameters->BrakeLevelSet(DynamicObject->MoverParameters->BrakeCtrlPos+1);
     keybrakecount=0;
       if ((DynamicObject->MoverParameters->TrainType==dt_EZT)&&(DynamicObject->MoverParameters->Mains)&&(DynamicObject->MoverParameters->ActiveDir!=0))
       {
       dsbPneumaticSwitch->SetVolume(-10);
       dsbPneumaticSwitch->Play(0,0,0);
       }
     }

    //Ra: przeklejka z SPKS - p³ynne poruszanie hamulcem
    //if ((DynamicObject->MoverParameters->BrakeHandle==FV4a)&&(Console::Pressed(Global::Keys[k_IncBrakeLevel])))
    if ((Console::Pressed(Global::Keys[k_IncBrakeLevel])))
     {
      if (Console::Pressed(VK_CONTROL))
       {
        //DynamicObject->MoverParameters->BrakeCtrlPos2-=dt/20.0;
        //if (DynamicObject->MoverParameters->BrakeCtrlPos2<-1.5) DynamicObject->MoverParameters->BrakeCtrlPos2=-1.5;
       }
      else
       {
        //DynamicObject->MoverParameters->BrakeCtrlPosR+=(DynamicObject->MoverParameters->BrakeCtrlPosR>DynamicObject->MoverParameters->BrakeCtrlPosNo?0:dt*2);
        //DynamicObject->MoverParameters->BrakeCtrlPos= floor(DynamicObject->MoverParameters->BrakeCtrlPosR+0.499);
       }
     }
    //if ((DynamicObject->MoverParameters->BrakeHandle==FV4a)&&(Console::Pressed(Global::Keys[k_DecBrakeLevel])))
    if ((Console::Pressed(Global::Keys[k_DecBrakeLevel])))
     {
      if (Console::Pressed(VK_CONTROL))
       {
        //DynamicObject->MoverParameters->BrakeCtrlPos2+=(DynamicObject->MoverParameters->BrakeCtrlPos2>2?0:dt/20.0);
        //if (DynamicObject->MoverParameters->BrakeCtrlPos2<-3) DynamicObject->MoverParameters->BrakeCtrlPos2=-3;
        //DynamicObject->MoverParameters->BrakeLevelAdd(DynamicObject->MoverParameters->fBrakeCtrlPos<-1?0:dt*2);
       }
      else
       {
        //DynamicObject->MoverParameters->BrakeCtrlPosR-=(DynamicObject->MoverParameters->BrakeCtrlPosR<-1?0:dt*2);
        //DynamicObject->MoverParameters->BrakeCtrlPos= floor(DynamicObject->MoverParameters->BrakeCtrlPosR+0.499);
        //DynamicObject->MoverParameters->BrakeLevelAdd(DynamicObject->MoverParameters->fBrakeCtrlPos<-1?0:-dt*2);
       }
     }

//    bool kEP;
//    kEP=(DynamicObject->MoverParameters->BrakeSubsystem==Knorr)||(DynamicObject->MoverParameters->BrakeSubsystem==Hik)||(DynamicObject->MoverParameters->BrakeSubsystem==Kk);
    if ((DynamicObject->MoverParameters->BrakeSystem==ElectroPneumatic)&&((DynamicObject->MoverParameters->BrakeSubsystem==Knorr)||(DynamicObject->MoverParameters->BrakeSubsystem==Hik)||(DynamicObject->MoverParameters->BrakeSubsystem==Kk)))
     if (Console::Pressed(Global::Keys[k_AntiSlipping]))                             //kEP
//     if (GetAsyncKeyState(VK_RETURN)<0)
      {
       AntiSlipButtonGauge.UpdateValue(1);
       if (DynamicObject->MoverParameters->SwitchEPBrake(1))
        {
         dsbPneumaticSwitch->SetVolume(-10);
         dsbPneumaticSwitch->Play(0,0,0);
        }
      }
     else
      {
       if (DynamicObject->MoverParameters->SwitchEPBrake(0))
        {
         dsbPneumaticSwitch->SetVolume(-10);
         dsbPneumaticSwitch->Play(0,0,0);
        }
      }

    if ( Console::Pressed(Global::Keys[k_DepartureSignal]) )
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

if ( Console::Pressed(Global::Keys[k_Main]) )                //[]
     {
      if (Console::Pressed(VK_SHIFT))
         MainButtonGauge.PutValue(1);
      else
         MainButtonGauge.PutValue(0);
      }
    else
         MainButtonGauge.PutValue(0);

if ( Console::Pressed(Global::Keys[k_CurrentNext]))
   {
      if (ShowNextCurrent==false)
      {
         if (NextCurrentButtonGauge.SubModel)
         {
            NextCurrentButtonGauge.UpdateValue(1);
            dsbSwitch->SetVolume(DSBVOLUME_MAX);
            dsbSwitch->Play(0,0,0);
            ShowNextCurrent=true;
         }
      }
   }
else
   {
      if (ShowNextCurrent==true)
      {
         if (NextCurrentButtonGauge.SubModel)
         {
            NextCurrentButtonGauge.UpdateValue(0);
            dsbSwitch->SetVolume(DSBVOLUME_MAX);
            dsbSwitch->Play(0,0,0);
            ShowNextCurrent=false;
         }
      }
   }

//Winger 010304  PantAllDownButton
    if ( Console::Pressed(Global::Keys[k_PantFrontUp]) )
     {
      if (Console::Pressed(VK_SHIFT))
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

    if ( Console::Pressed(Global::Keys[k_PantRearUp]) )
     {
      if (Console::Pressed(VK_SHIFT))
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

/*  if ((DynamicObject->MoverParameters->Mains) && (DynamicObject->MoverParameters->EngineType==ElectricSeriesMotor))
    {
//tu dac w przyszlosci zaleznosc od wlaczenia przetwornicy
    if (DynamicObject->MoverParameters->ConverterFlag)          //NBMX -obsluga przetwornicy
    {
    //glosnosc zalezna od nap. sieci
    //-2000 do 0
     long tmpVol;
     int trackVol;
     trackVol=3550-2000;
     if (DynamicObject->MoverParameters->RunningTraction.TractionVoltage<2000)
      {
       tmpVol=0;
      }
     else
      {
       tmpVol=DynamicObject->MoverParameters->RunningTraction.TractionVoltage-2000;
      }
     sConverter.Volume(-2000*(trackVol-tmpVol)/trackVol);

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
   if (FreeFlyModeFlag?false:fTachoCount>maxtacho)
   {
    dsbHasler->GetStatus(&stat);
    if (!(stat&DSBSTATUS_PLAYING))
     dsbHasler->Play(0,0,DSBPLAY_LOOPING );
   }
   else
   {
    if (FreeFlyModeFlag?true:fTachoCount<1)
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
    ConverterFuseButtonGauge.Update();    
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
    //hunter-230112
    RearUpperLightButtonGauge.Update();
    RearLeftLightButtonGauge.Update();
    RearRightLightButtonGauge.Update();
    RearLeftEndLightButtonGauge.Update();
    RearRightEndLightButtonGauge.Update();
    //------------
    PantAllDownButtonGauge.Update();
    ConverterButtonGauge.Update();
    ConverterOffButtonGauge.Update();
    TrainHeatingButtonGauge.Update();
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
    ConverterFuseButtonGauge.UpdateValue(0);    
  }
 return true; //(DynamicObject->Update(dt));
}  //koniec update


bool TTrain::CabChange(int iDirection)
{//McZapkie-090902: zmiana kabiny 1->0->2 i z powrotem
 if (DynamicObject->MoverParameters->ChangeCab(iDirection))
 {
  if (InitializeCab(DynamicObject->MoverParameters->ActiveCab,DynamicObject->asBaseDir+DynamicObject->MoverParameters->TypeName+".mmd"))
  {
   return true;
  }
  else return false;
 }
 // else return false;
 return false;
}

//McZapkie-310302
//wczytywanie pliku z danymi multimedialnymi (dzwieki, kontrolki, kabiny)
bool __fastcall TTrain::LoadMMediaFile(AnsiString asFileName)
{
    double dSDist;
    TFileStream *fs;
    fs=new TFileStream(asFileName , fmOpenRead	| fmShareCompat	);
    AnsiString str="";
//    DecimalSeparator='.';
    int size=fs->Size;
    str.SetLength(size);
    fs->Read(str.c_str(),size);
    str+="";
    delete fs;
    TQueryParserComp *Parser;
    Parser=new TQueryParserComp(NULL);
    Parser->TextToParse=str;
//    Parser->LoadStringToParse(asFile);
    Parser->First();
    str="";
    dsbPneumaticSwitch=TSoundsManager::GetFromName("silence1.wav",true);
    while ((!Parser->EndOfFile) && (str!=AnsiString("internaldata:")))
    {
        str=Parser->GetNextSymbol().LowerCase();
    }
    if (str==AnsiString("internaldata:"))
    {
     while (!Parser->EndOfFile)
      {
        str=Parser->GetNextSymbol().LowerCase();
//SEKCJA DZWIEKOW
        if (str==AnsiString("ctrl:"))                    //nastawnik:
         {
          str=Parser->GetNextSymbol().LowerCase();
          dsbNastawnikJazdy=TSoundsManager::GetFromName(str.c_str(),true);
         }
        else
        //---------------
        //hunter-081211: nastawnik bocznikowania
        if (str==AnsiString("ctrlscnd:"))                    //nastawnik bocznikowania:
         {
          str=Parser->GetNextSymbol().LowerCase();
          dsbNastawnikBocz=TSoundsManager::GetFromName(str.c_str(),true);
         }
        else
        //---------------
        //hunter-131211: dzwiek kierunkowego
        if (str==AnsiString("reverserkey:"))                    //nastawnik kierunkowy
         {
          str=Parser->GetNextSymbol().LowerCase();
          dsbReverserKey=TSoundsManager::GetFromName(str.c_str(),true);
         }
        else
        //---------------
        if (str==AnsiString("buzzer:"))                    //bzyczek shp:
         {
          str=Parser->GetNextSymbol().LowerCase();
          dsbBuzzer=TSoundsManager::GetFromName(str.c_str(),true);
         }
        else
        if (str==AnsiString("slipalarm:")) //Bombardier 011010: alarm przy poslizgu:
        {
         str=Parser->GetNextSymbol().LowerCase();
         dsbSlipAlarm=TSoundsManager::GetFromName(str.c_str(),true);
        }
        else
        if (str==AnsiString("tachoclock:"))                 //cykanie rejestratora:
         {
          str=Parser->GetNextSymbol().LowerCase();
          dsbHasler=TSoundsManager::GetFromName(str.c_str(),true);
         }
        else
        if (str==AnsiString("switch:"))                   //przelaczniki:
         {
          str=Parser->GetNextSymbol().LowerCase();
          dsbSwitch=TSoundsManager::GetFromName(str.c_str(),true);
         }
        else
        if (str==AnsiString("pneumaticswitch:"))                   //stycznik EP:
         {
          str=Parser->GetNextSymbol().LowerCase();
          dsbPneumaticSwitch=TSoundsManager::GetFromName(str.c_str(),true);
         }
        else
        //---------------
        //hunter-111211: wydzielenie wejscia na bezoporowa i na drugi uklad do pliku
        if (str==AnsiString("wejscie_na_bezoporow:"))
         {
          str=Parser->GetNextSymbol().LowerCase();
          dsbWejscie_na_bezoporow=TSoundsManager::GetFromName(str.c_str(),true);
         }
        else
        if (str==AnsiString("wejscie_na_drugi_uklad:"))
         {
          str=Parser->GetNextSymbol().LowerCase();
          dsbWejscie_na_drugi_uklad=TSoundsManager::GetFromName(str.c_str(),true);
         }
        else
        //---------------
        if (str==AnsiString("relay:"))                   //styczniki itp:
         {
          str=Parser->GetNextSymbol().LowerCase();
          dsbRelay=TSoundsManager::GetFromName(str.c_str(),true);
          if (!dsbWejscie_na_bezoporow) //hunter-111211: domyslne, gdy brak
           dsbWejscie_na_bezoporow=TSoundsManager::GetFromName("wejscie_na_bezoporow.wav",true);
          if (!dsbWejscie_na_drugi_uklad)
           dsbWejscie_na_drugi_uklad=TSoundsManager::GetFromName("wescie_na_drugi_uklad.wav",true);
         }
        else
        if (str==AnsiString("pneumaticrelay:"))           //wylaczniki pneumatyczne:
         {
          str=Parser->GetNextSymbol().LowerCase();
          dsbPneumaticRelay=TSoundsManager::GetFromName(str.c_str(),true);
         }
        else
        if (str==AnsiString("couplerattach:"))           //laczenie:
         {
          str=Parser->GetNextSymbol().LowerCase();
          dsbCouplerAttach=TSoundsManager::GetFromName(str.c_str(),true);
          dsbCouplerStretch=TSoundsManager::GetFromName("en57_couplerstretch.wav",true); //McZapkie-090503: PROWIZORKA!!!
         }
        else
        if (str==AnsiString("couplerdetach:"))           //rozlaczanie:
         {
          str=Parser->GetNextSymbol().LowerCase();
          dsbCouplerDetach=TSoundsManager::GetFromName(str.c_str(),true);
          dsbBufferClamp=TSoundsManager::GetFromName("en57_bufferclamp.wav",true); //McZapkie-090503: PROWIZORKA!!!
         }
        else
        if (str==AnsiString("ignition:"))           //rozlaczanie:
         {
          str=Parser->GetNextSymbol().LowerCase();
          dsbDieselIgnition=TSoundsManager::GetFromName(str.c_str(),true);
         }
        else
        if (str==AnsiString("brakesound:"))                    //hamowanie zwykle:
         {
          str=Parser->GetNextSymbol();
          rsBrake.Init(str.c_str(),-1,0,0,0,true);
          rsBrake.AM=Parser->GetNextSymbol().ToDouble()/(1+DynamicObject->MoverParameters->MaxBrakeForce*1000);
          rsBrake.AA=Parser->GetNextSymbol().ToDouble();
          rsBrake.FM=Parser->GetNextSymbol().ToDouble()/(1+DynamicObject->MoverParameters->Vmax);
          rsBrake.FA=Parser->GetNextSymbol().ToDouble();
         }
        else
        if (str==AnsiString("slipperysound:"))                    //sanie:
         {
          str=Parser->GetNextSymbol();
          rsSlippery.Init(str.c_str(),-1,0,0,0,true);
          rsSlippery.AM=Parser->GetNextSymbol().ToDouble()/(1+DynamicObject->MoverParameters->Vmax);
          rsSlippery.AA=Parser->GetNextSymbol().ToDouble();
          rsSlippery.FM=0.0;
          rsSlippery.FA=1.0;
         }
        else
        if (str==AnsiString("airsound:"))                    //syk:
         {
          str=Parser->GetNextSymbol();
          rsHiss.Init(str.c_str(),-1,0,0,0,true);
          rsHiss.AM=Parser->GetNextSymbol().ToDouble();
          rsHiss.AA=Parser->GetNextSymbol().ToDouble();
          rsHiss.FM=0.0;
          rsHiss.FA=1.0;
         }
        else
        if (str==AnsiString("fadesound:"))                    //syk:
         {
          str=Parser->GetNextSymbol();
          rsFadeSound.Init(str.c_str(),-1,0,0,0,true);
          rsFadeSound.AM=1.0;
          rsFadeSound.AA=1.0;
          rsFadeSound.FM=1.0;
          rsFadeSound.FA=1.0;
         }
        else
        if (str==AnsiString("localbrakesound:"))                    //syk:
         {
          str=Parser->GetNextSymbol();
          rsSBHiss.Init(str.c_str(),-1,0,0,0,true);
          rsSBHiss.AM=Parser->GetNextSymbol().ToDouble();
          rsSBHiss.AA=Parser->GetNextSymbol().ToDouble();
          rsSBHiss.FM=0.0;
          rsSBHiss.FA=1.0;
         }
        else
        if (str==AnsiString("runningnoise:"))                    //szum podczas jazdy:
         {
          str=Parser->GetNextSymbol();
          rsRunningNoise.Init(str.c_str(),-1,0,0,0,true);
          rsRunningNoise.AM=Parser->GetNextSymbol().ToDouble()/(1+DynamicObject->MoverParameters->Vmax);
          rsRunningNoise.AA=Parser->GetNextSymbol().ToDouble();
          rsRunningNoise.FM=Parser->GetNextSymbol().ToDouble()/(1+DynamicObject->MoverParameters->Vmax);
          rsRunningNoise.FA=Parser->GetNextSymbol().ToDouble();
         }
        else
        if (str==AnsiString("engageslippery:"))                    //tarcie tarcz sprzegla:
         {
          str=Parser->GetNextSymbol();
          rsEngageSlippery.Init(str.c_str(),-1,0,0,0,true);
          rsEngageSlippery.AM=Parser->GetNextSymbol().ToDouble();
          rsEngageSlippery.AA=Parser->GetNextSymbol().ToDouble();
          rsEngageSlippery.FM=Parser->GetNextSymbol().ToDouble()/(1+DynamicObject->MoverParameters->nmax);
          rsEngageSlippery.FA=Parser->GetNextSymbol().ToDouble();
         }
        else
        if (str==AnsiString("mechspring:"))                    //parametry bujania kamery:
         {
          str=Parser->GetNextSymbol();
          MechSpring.Init(0,str.ToDouble(),Parser->GetNextSymbol().ToDouble());
          fMechSpringX=Parser->GetNextSymbol().ToDouble();
          fMechSpringY=Parser->GetNextSymbol().ToDouble();
          fMechSpringZ=Parser->GetNextSymbol().ToDouble();
          fMechMaxSpring=Parser->GetNextSymbol().ToDouble();
          fMechRoll=Parser->GetNextSymbol().ToDouble();
          fMechPitch=Parser->GetNextSymbol().ToDouble();
         }
                 else
        if (str==AnsiString("pantographup:"))           //podniesienie patyka:
         {
          str=Parser->GetNextSymbol().LowerCase();
          dsbPantUp=TSoundsManager::GetFromName(str.c_str(),true);
         }
                 else
        if (str==AnsiString("pantographdown:"))           //podniesienie patyka:
         {
          str=Parser->GetNextSymbol().LowerCase();
          dsbPantDown=TSoundsManager::GetFromName(str.c_str(),true);
         }
                 else
        if (str==AnsiString("doorclose:"))           //zamkniecie drzwi:
         {
          str=Parser->GetNextSymbol().LowerCase();
          dsbDoorClose=TSoundsManager::GetFromName(str.c_str(),true);
         }
                 else
        if (str==AnsiString("dooropen:"))           //wotwarcie drzwi:
         {
          str=Parser->GetNextSymbol().LowerCase();
          dsbDoorOpen=TSoundsManager::GetFromName(str.c_str(),true);
         }
      }
    }
    else return false; //nie znalazl sekcji internal
 delete Parser; //Ra: jak siê coœ newuje, to trzeba zdeletowaæ, inaczej syf siê robi
 if (!InitializeCab(DynamicObject->MoverParameters->ActiveCab,asFileName)) //zle zainicjowana kabina
  return false;
 else
 {
  if (DynamicObject->Controller==Humandriver)
   DynamicObject->bDisplayCab=true;             //McZapkie-030303: mozliwosc wyswietlania kabiny, w przyszlosci dac opcje w mmd
  return true;
 }
}


bool TTrain::InitializeCab(int NewCabNo, AnsiString asFileName)
{
 bool parse=false;
 double dSDist;
 TFileStream *fs;
 fs=new TFileStream(asFileName,fmOpenRead|fmShareCompat);
 AnsiString str="";
 //DecimalSeparator='.';
 int size=fs->Size;
 str.SetLength(size);
 fs->Read(str.c_str(),size);
 str+="";
 delete fs;
 TQueryParserComp *Parser;
 Parser=new TQueryParserComp(NULL);
 Parser->TextToParse=str;
 //Parser->LoadStringToParse(asFile);
 Parser->First();
 str="";
 int cabindex=0;
 switch (NewCabNo)
 {//ustalenie numeru kabiny do wczytania
  case -1: cabindex=2; break;
  case 1: cabindex=1; break;
  case 0: cabindex=0;
 }
 AnsiString cabstr=AnsiString("cab")+cabindex+AnsiString("definition:");
 while ((!Parser->EndOfFile) && (AnsiCompareStr(str,cabstr)!=0))
  str=Parser->GetNextSymbol().LowerCase(); //szukanie kabiny
 if (cabindex==2)
  if (AnsiCompareStr(str,cabstr)!=0) //jeœli nie znaleziony wpis kabiny
  {//próba szukania kabiny 1
   cabstr=AnsiString("cab1definition:");
   Parser->First();
   while ((!Parser->EndOfFile) && (AnsiCompareStr(str,cabstr)!=0))
    str=Parser->GetNextSymbol().LowerCase(); //szukanie kabiny
  }
 if (AnsiCompareStr(str,cabstr)==0) //jeœli znaleziony wpis kabiny
 {
  Cabine[cabindex].Load(Parser);
  str=Parser->GetNextSymbol().LowerCase();
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
  str=Parser->GetNextSymbol().LowerCase();
  if (AnsiCompareStr(AnsiString("driver")+cabindex+AnsiString("sitpos:"),str)==0)     //ABu 180404 pozycja siedzaca maszynisty
  {
   pMechSittingPosition.x=Parser->GetNextSymbol().ToDouble();
   pMechSittingPosition.y=Parser->GetNextSymbol().ToDouble();
   pMechSittingPosition.z=Parser->GetNextSymbol().ToDouble();
   parse=true;
  }
  //else parse=false;
  while (!Parser->EndOfFile)
  {//ABu: wstawione warunki, wczesniej tylko to:
   //   str=Parser->GetNextSymbol().LowerCase();
   if (parse)
    str=Parser->GetNextSymbol().LowerCase();
   else
    parse=true;
   //inicjacja kabiny
   if (AnsiCompareStr(AnsiString("cab")+cabindex+AnsiString("model:"),str)==0)     //model kabiny
   {
    str=Parser->GetNextSymbol().LowerCase();
    if (str!=AnsiString("none"))
    {
     str=DynamicObject->asBaseDir+str;
     Global::asCurrentTexturePath=DynamicObject->asBaseDir; //bie¿¹ca sciezka do tekstur to dynamic/...
     TModel3d *k=TModelsManager::GetModel(str.c_str(),true); //szukaj kabinê jako oddzielny model
     Global::asCurrentTexturePath=AnsiString(szDefaultTexturePath); //z powrotem defaultowa sciezka do tekstur
     if (DynamicObject->mdKabina!=k)
      DynamicObject->mdKabina=k; //nowa kabina
     else
      break; //wyjœcie z pêtli, bo model zostaje bez zmian
    }
    else
     DynamicObject->mdKabina=DynamicObject->mdModel;   //McZapkie-170103: szukaj elementy kabiny w glownym modelu
    ActiveUniversal4=false;
    MainCtrlGauge.Clear();
    MainCtrlActGauge.Clear();
    ScndCtrlGauge.Clear();
    DirKeyGauge.Clear();
    BrakeCtrlGauge.Clear();
    LocalBrakeGauge.Clear();
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
    ConverterFuseButtonGauge.Clear();    
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

    VelocityGaugeB.Clear();
    I1GaugeB.Clear();
    I1Gauge.Output(5); //Ra: ustawienie kana³u analogowego komunikacji zwrotnej
    I2GaugeB.Clear();
    I3GaugeB.Clear();
    ItotalGaugeB.Clear();
    CylHamGaugeB.Clear();
    PrzGlGaugeB.Clear();
    ZbGlGaugeB.Clear();

    I1BGauge.Clear();
    I2BGauge.Clear();
    I3BGauge.Clear();
    ItotalBGauge.Clear();

    ClockSInd.Clear();
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
    btLampkaPoslizg.Clear(6);
    btLampkaStyczn.Clear(5);
    btLampkaNadmPrzetw.Clear(7);
    btLampkaPrzekRozn.Clear();
    btLampkaPrzekRoznPom.Clear();
    btLampkaNadmSil.Clear(4);
    btLampkaWylSzybki.Clear(3);
    btLampkaNadmWent.Clear(9);
    btLampkaNadmSpr.Clear(8);
    btLampkaOpory.Clear(2);
    btLampkaWysRozr.Clear(10);
    btLampkaUniversal3.Clear();
    btLampkaWentZaluzje.Clear();
    btLampkaOgrzewanieSkladu.Clear();
    btLampkaSHP.Clear(0);
    btLampkaCzuwaka.Clear(1);
    btLampkaDoorLeft.Clear();
    btLampkaDoorRight.Clear();
    btLampkaDepartureSignal.Clear();
    btLampkaRezerwa.Clear();
    btLampkaBocznikI.Clear();
    btLampkaBocznikII.Clear();
    btLampkaRadiotelefon.Clear();
    btLampkaHamienie.Clear();
    btLampkaSprezarka.Clear();
    btLampkaSprezarkaB.Clear();
    btLampkaNapNastHam.Clear();
    btLampkaJazda.Clear();
    btLampkaStycznB.Clear();
    btLampkaNadmPrzetwB.Clear();
    btLampkaWylSzybkiB.Clear();
    btLampkaForward.Clear();
    btLampkaBackward.Clear();
    LeftLightButtonGauge.Clear();
    RightLightButtonGauge.Clear();
    UpperLightButtonGauge.Clear();
    LeftEndLightButtonGauge.Clear();
    RightEndLightButtonGauge.Clear();
    //hunter-230112
    RearLeftLightButtonGauge.Clear();
    RearRightLightButtonGauge.Clear();
    RearUpperLightButtonGauge.Clear();
    RearLeftEndLightButtonGauge.Clear();
    RearRightEndLightButtonGauge.Clear();
   }
   //SEKCJA REGULATOROW
   else if (str==AnsiString("mainctrl:"))                    //nastawnik
   {
    if (!DynamicObject->mdKabina)
    {
     delete Parser;
     WriteLog("Cab not initialised!");
     return false;
    }
    else
     MainCtrlGauge.Load(Parser,DynamicObject->mdKabina);
   }
   else if (str==AnsiString("mainctrlact:"))                 //zabek pozycji aktualnej
    MainCtrlActGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("scndctrl:"))                    //bocznik
    ScndCtrlGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("dirkey:"))                    //klucz kierunku
    DirKeyGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("brakectrl:"))                    //hamulec zasadniczy
    BrakeCtrlGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("localbrake:"))                    //hamulec pomocniczy
    LocalBrakeGauge.Load(Parser,DynamicObject->mdKabina);
   //sekcja przelacznikow obrotowych
   else if (str==AnsiString("brakeprofile_sw:"))                    //przelacznik tow/osob
    BrakeProfileCtrlGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("maxcurrent_sw:"))                    //przelacznik rozruchu
    MaxCurrentCtrlGauge.Load(Parser,DynamicObject->mdKabina);
   //SEKCJA przyciskow sprezynujacych
   else if (str==AnsiString("main_off_bt:"))                    //przycisk wylaczajacy (w EU07 wyl szybki czerwony)
    MainOffButtonGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("main_on_bt:"))                    //przycisk wlaczajacy (w EU07 wyl szybki zielony)
    MainOnButtonGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("security_reset_bt:"))             //przycisk zbijajacy SHP/czuwak
    SecurityResetButtonGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("releaser_bt:"))                   //przycisk odluzniacza
    ReleaserButtonGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("releaser_bt:"))                   //przycisk odluzniacza
    ReleaserButtonGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("antislip_bt:"))                   //przycisk antyposlizgowy
    AntiSlipButtonGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("horn_bt:"))                       //dzwignia syreny
    HornButtonGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("fuse_bt:"))                       //bezp. nadmiarowy
    FuseButtonGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("converterfuse_bt:"))                       //hunter-261211: odblokowanie przekaznika nadm. przetw. i ogrz.
    ConverterFuseButtonGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("stlinoff_bt:"))                       //st. liniowe
    StLinOffButtonGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("door_left_sw:"))                       //drzwi lewe
    DoorLeftButtonGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("door_right_sw:"))                       //drzwi prawe
    DoorRightButtonGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("departure_signal_bt:"))                       //sygnal odjazdu
    DepartureSignalButtonGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("upperlight_sw:"))                       //swiatlo
    UpperLightButtonGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("leftlight_sw:"))                       //swiatlo
    LeftLightButtonGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("rightlight_sw:"))                       //swiatlo
    RightLightButtonGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("leftend_sw:"))                       //swiatlo
    LeftEndLightButtonGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("rightend_sw:"))                       //swiatlo
    RightEndLightButtonGauge.Load(Parser,DynamicObject->mdKabina);
   //---------------------
   //hunter-230112: przelaczniki swiatel tylnich
   else if (str==AnsiString("rearupperlight_sw:"))                       //swiatlo
    RearUpperLightButtonGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("rearleftlight_sw:"))                       //swiatlo
    RearLeftLightButtonGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("rearrightlight_sw:"))                       //swiatlo
    RearRightLightButtonGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("rearleftend_sw:"))                       //swiatlo
    RearLeftEndLightButtonGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("rearrightend_sw:"))                       //swiatlo
    RearRightEndLightButtonGauge.Load(Parser,DynamicObject->mdKabina);
   //------------------
   else if (str==AnsiString("compressor_sw:"))                       //sprezarka
    CompressorButtonGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("converter_sw:"))                       //przetwornica
    ConverterButtonGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("converteroff_sw:"))                       //przetwornica wyl
    ConverterOffButtonGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("main_sw:"))                       //wyl szybki (ezt)
    MainButtonGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("radio_sw:"))                       //radio
    RadioButtonGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("pantfront_sw:"))                       //patyk przedni
    PantFrontButtonGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("pantrear_sw:"))                       //patyk tylny
    PantRearButtonGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("pantfrontoff_sw:"))                       //patyk przedni w dol
    PantFrontButtonOffGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("pantalloff_sw:"))                       //patyk przedni w dol
    PantAllDownButtonGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("trainheating_sw:"))                       //grzanie skladu
    TrainHeatingButtonGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("nextcurrent_sw:"))                       //grzanie skladu
    NextCurrentButtonGauge.Load(Parser,DynamicObject->mdKabina);
    //ABu 090305: uniwersalne przyciski lub inne rzeczy
   else if (str==AnsiString("universal1:"))
    Universal1ButtonGauge.Load(Parser,DynamicObject->mdKabina,DynamicObject->mdModel);
   else if (str==AnsiString("universal2:"))
    Universal2ButtonGauge.Load(Parser,DynamicObject->mdKabina,DynamicObject->mdModel);
   else if (str==AnsiString("universal3:"))
    Universal3ButtonGauge.Load(Parser,DynamicObject->mdKabina,DynamicObject->mdModel);
   else if (str==AnsiString("universal4:"))
    Universal4ButtonGauge.Load(Parser,DynamicObject->mdKabina,DynamicObject->mdModel);
   //SEKCJA WSKAZNIKOW
   else if (str==AnsiString("tachometer:"))                    //predkosciomierz
    VelocityGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("hvcurrent1:"))                    //1szy amperomierz
    I1Gauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("hvcurrent2:"))                    //2gi amperomierz
    I2Gauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("hvcurrent3:"))                    //3ci amperomierz
    I3Gauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("hvcurrent:"))                     //amperomierz calkowitego pradu
    ItotalGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("brakepress:"))                    //manometr cylindrow hamulcowych
    CylHamGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("pipepress:"))                    //manometr przewodu hamulcowego
    PrzGlGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("compressor:"))                    //manometr sprezarki/zbiornika glownego
    ZbGlGauge.Load(Parser,DynamicObject->mdKabina);
   //*************************************************************
   //Sekcja zdublowanych wskaznikow dla dwustronnych kabin
   else if (str==AnsiString("tachometerb:"))                    //predkosciomierz
    VelocityGaugeB.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("hvcurrent1b:"))                    //1szy amperomierz
    I1GaugeB.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("hvcurrent2b:"))                    //2gi amperomierz
    I2GaugeB.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("hvcurrent3b:"))                    //3ci amperomierz
    I3GaugeB.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("hvcurrentb:"))                     //amperomierz calkowitego pradu
    ItotalGaugeB.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("brakepressb:"))                    //manometr cylindrow hamulcowych
    CylHamGaugeB.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("pipepressb:"))                    //manometr przewodu hamulcowego
    PrzGlGaugeB.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("compressorb:"))                    //manometr sprezarki/zbiornika glownego
    ZbGlGaugeB.Load(Parser,DynamicObject->mdKabina);
   //*************************************************************
   //yB - dla drugiej sekcji
   else if (str==AnsiString("hvbcurrent1:"))                    //1szy amperomierz
    I1BGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("hvbcurrent2:"))                    //2gi amperomierz
    I2BGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("hvbcurrent3:"))                    //3ci amperomierz
    I3BGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("hvbcurrent:"))                     //amperomierz calkowitego pradu
    ItotalBGauge.Load(Parser,DynamicObject->mdKabina);
   //*************************************************************
   else if (str==AnsiString("clock:"))                    //manometr sprezarki/zbiornika glownego
   {
    if (Parser->GetNextSymbol()==AnsiString("analog"))
     {
      //McZapkie-300302: zegarek
      ClockSInd.Init(DynamicObject->mdKabina->GetFromName("ClockShand"),gt_Rotate,0.016666667,0,0);
      ClockMInd.Init(DynamicObject->mdKabina->GetFromName("ClockMhand"),gt_Rotate,0.016666667,0,0);
      ClockHInd.Init(DynamicObject->mdKabina->GetFromName("ClockHhand"),gt_Rotate,0.083333333,0,0);
     }
   }
   else if (str==AnsiString("evoltage:"))                    //woltomierz napiecia silnikow
    EngineVoltage.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("hvoltage:"))                    //woltomierz wysokiego napiecia
    HVoltageGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("lvoltage:"))                    //woltomierz niskiego napiecia
    LVoltageGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("enrot1m:"))                    //obrotomierz
    enrot1mGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("enrot2m:"))
    enrot2mGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("enrot3m:"))
    enrot3mGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("engageratio:"))   //np. cisnienie sterownika przegla
    engageratioGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("maingearstatus:"))   //np. cisnienie sterownika skrzyni biegow
    maingearstatusGauge.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("ignitionkey:"))   //np. cisnienie sterownika skrzyni biegow
    IgnitionKeyGauge.Load(Parser,DynamicObject->mdKabina);
   //SEKCJA LAMPEK
   else if (str==AnsiString("i-slippery:"))
    btLampkaPoslizg.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("i-contactors:"))
    btLampkaStyczn.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("i-conv_ovld:"))
    btLampkaNadmPrzetw.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("i-diff_relay:"))
    btLampkaPrzekRozn.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("i-diff_relay2:"))
    btLampkaPrzekRoznPom.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("i-motor_ovld:"))
    btLampkaNadmSil.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("i-mainbreaker:"))
    btLampkaWylSzybki.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("i-vent_ovld:"))
    btLampkaNadmWent.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("i-comp_ovld:"))
    btLampkaNadmSpr.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("i-resistors:"))
    btLampkaOpory.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("i-highcurrent:"))
    btLampkaWysRozr.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("i-universal3:"))
   {
    btLampkaUniversal3.Load(Parser,DynamicObject->mdKabina,DynamicObject->mdModel);
    LampkaUniversal3_typ=0;
   }
   else if (str==AnsiString("i-universal3_M:"))
   {
    btLampkaUniversal3.Load(Parser,DynamicObject->mdKabina,DynamicObject->mdModel);
    LampkaUniversal3_typ=1;
   }
   else if (str==AnsiString("i-universal3_C:"))
   {
    btLampkaUniversal3.Load(Parser,DynamicObject->mdKabina,DynamicObject->mdModel);
    LampkaUniversal3_typ=2;
   }
   else if (str==AnsiString("i-vent_trim:"))
    btLampkaWentZaluzje.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("i-trainheating:"))
    btLampkaOgrzewanieSkladu.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("i-security_aware:"))
    btLampkaCzuwaka.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("i-security_cabsignal:"))
    btLampkaSHP.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("i-door_left:"))
    btLampkaDoorLeft.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("i-door_right:"))
    btLampkaDoorRight.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("i-departure_signal:"))
    btLampkaDepartureSignal.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("i-reserve:"))
    btLampkaRezerwa.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("i-scnd1:"))
    btLampkaBocznikI.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("i-scnd2:"))
    btLampkaBocznikII.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("i-braking:"))
    btLampkaHamienie.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("i-compressor:"))
    btLampkaSprezarka.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("i-compressorb:"))
    btLampkaSprezarkaB.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("i-voltbrake:"))
    btLampkaNapNastHam.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("i-mainbreakerb:"))
    btLampkaWylSzybkiB.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("i-resistorsb:"))
    btLampkaOporyB.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("i-contactorsb:"))
    btLampkaStycznB.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("i-conv_ovldb:"))
    btLampkaNadmPrzetwB.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("i-forward:"))
    btLampkaForward.Load(Parser,DynamicObject->mdKabina);
   else if (str==AnsiString("i-backward:"))
    btLampkaBackward.Load(Parser,DynamicObject->mdKabina);
   //btLampkaUnknown.Init("unknown",mdKabina,false);
  }
 }
 else
 {delete Parser;
  return false;
 }
 //ABu 050205: tego wczesniej nie bylo:
 delete Parser;
 if (DynamicObject->mdKabina)
 {
  DynamicObject->mdKabina->Init(); //obrócenie modelu oraz optymalizacja, równie¿ zapisanie binarnego
  return true;
 }
 return AnsiCompareStr(str,AnsiString("none"));
}

void __fastcall TTrain::MechStop()
{//likwidacja ruchu kamery w kabinie (po powrocie przez [F4])
 pMechPosition=vector3(0,0,0);
 pMechShake=vector3(0,0,0);
 vMechMovement=vector3(0,0,0);
 //pMechShake=vMechVelocity=vector3(0,0,0);
};

vector3 __fastcall TTrain::MirrorPosition(bool lewe)
{//zwraca wspó³rzêdne widoku kamery z lusterka
 switch (iCabn)
 {
  case 1: //przednia (1)
   return DynamicObject->mMatrix*vector3(lewe?Cabine[iCabn].CabPos2.x:Cabine[iCabn].CabPos1.x,1.5+Cabine[iCabn].CabPos1.y,Cabine[iCabn].CabPos2.z);
  case 2: //tylna (-1)
   return DynamicObject->mMatrix*vector3(lewe?Cabine[iCabn].CabPos1.x:Cabine[iCabn].CabPos2.x,1.5+Cabine[iCabn].CabPos1.y,Cabine[iCabn].CabPos1.z);
 }
 return DynamicObject->GetPosition(); //wspó³rzêdne œrodka pojazdu
};
