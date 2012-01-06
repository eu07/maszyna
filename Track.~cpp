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

//nag³ówki identyczne w ka¿dym pliku...
#pragma hdrstop

#include "Track.h"
#include "Usefull.h"
#include "Texture.h"
#include "Timer.h"
#include "Globals.h"
#include "Ground.h"
#include "parser.h"
#include "Mover.hpp"
#include "DynObj.h"


#pragma package(smart_init)

//101206 Ra: trapezoidalne drogi i tory
//110720 Ra: rozprucie zwrotnicy i odcinki izolowane

TIsolated *TIsolated::pRoot=NULL;

__fastcall TSwitchExtension::TSwitchExtension(TTrack *owner)
{//na pocz¹tku wszystko puste
 CurrentIndex=0;
 pNexts[0]=NULL; //wskaŸniki do kolejnych odcinków ruchu
 pNexts[1]=NULL;
 pPrevs[0]=NULL;
 pPrevs[1]=NULL;
 fOffset1=fOffset2=fDesiredOffset1=fDesiredOffset2=0.0; //po³o¿enie zasadnicze
 pOwner=NULL;
 pNextAnim=NULL;
 bMovement=false; //nie potrzeba przeliczaæ fOffset1
 Segments[0]=new TSegment(owner);
 Segments[1]=new TSegment(owner);
 Segments[3]=NULL;
 Segments[4]=NULL;
 EventPlus=EventMinus=NULL;
}
__fastcall TSwitchExtension::~TSwitchExtension()
{//nie ma nic do usuwania
}

__fastcall TIsolated::TIsolated()
{//utworznie pustego
 TIsolated("none",NULL);
};
__fastcall TIsolated::TIsolated(const AnsiString &n,TIsolated *i)
{//utworznie obwodu izolowanego
 asName=n;
 pNext=i;
 iAxles=0;
 eBusy=eFree=NULL;
};

__fastcall TIsolated::~TIsolated()
{//usuwanie
/*
 TIsolated *p=pRoot;
 while (pRoot)
 {
  p=pRoot;
  p->pNext=NULL;
  delete p;
 }
*/
};

TIsolated* __fastcall TIsolated::Find(const AnsiString &n)
{//znalezienie obiektu albo utworzenie nowego
 TIsolated *p=pRoot;
 while (p)
 {//jeœli siê znajdzie, to podaæ wskaŸnik
  if (p->asName==n) return p;
  p=p->pNext;
 }
 pRoot=new TIsolated(n,pRoot);
 return pRoot;
};

void __fastcall TIsolated::Modify(int i,TDynamicObject *o)
{//dodanie lub odjêcie osi
 if (iAxles)
 {//grupa zajêta
  iAxles+=i;
  if (!iAxles)
   if (eFree)
    Global::pGround->AddToQuery(eFree,o); //dodanie zwolnienia do kolejki
 }
 else
 {//grupa by³a wolna
  iAxles+=i;
  if (iAxles)
   if (eBusy)
    Global::pGround->AddToQuery(eBusy,o); //dodanie zajêtoœci do kolejki
 }
};


__fastcall TTrack::TTrack(TGroundNode *g)
{//tworzenie nowego odcinka ruchu
 pNext=pPrev=NULL; //s¹siednie
 Segment=NULL; //dane odcinka
 SwitchExtension=NULL; //dodatkowe parametry zwrotnicy i obrotnicy
 TextureID1=0; //tekstura szyny
 fTexLength=4.0; //powtarzanie tekstury
 TextureID2=0; //tekstura podsypki albo drugiego toru zwrotnicy
 fTexHeight=0.6; //nowy profil podsypki ;)
 fTexWidth=0.9;
 fTexSlope=0.9;
 eType=tt_Normal; //domyœlnie zwyk³y
 iCategoryFlag=1; //1-tor, 2-droga, 4-rzeka, 8-samolot?
 fTrackWidth=1.435; //rozstaw toru, szerokoœæ nawierzchni
 fFriction=0.15; //wspó³czynnik tarcia
 fSoundDistance=-1;
 iQualityFlag=20;
 iDamageFlag=0;
 eEnvironment=e_flat;
 bVisible=true;
 iEvents=0; //Ra: flaga informuj¹ca o obecnoœci eventów
 Event0=NULL;
 Event1=NULL;
 Event2=NULL;
 Eventall0=NULL;
 Eventall1=NULL;
 Eventall2=NULL;
 fVelocity=-1; //ograniczenie prêdkoœci
 fTrackLength=100.0;
 fRadius=0; //promieñ wybranego toru zwrotnicy
 fRadiusTable[0]=0; //dwa promienie nawet dla prostego
 fRadiusTable[1]=0;
 iNumDynamics=0;
 ScannedFlag=false;
 DisplayListID=0;
 iTrapezoid=0; //parametry kszta³tu: 0-standard, 1-przechy³ka, 2-trapez, 3-oba
 pTraction=NULL; //drut zasilaj¹cy najbli¿szy Point1 toru
 fTexRatio1=1.0; //proporcja boków tekstury nawierzchni (¿eby zaoszczêdziæ na rozmiarach tekstur...)
 fTexRatio2=1.0; //proporcja boków tekstury chodnika (¿eby zaoszczêdziæ na rozmiarach tekstur...)
 iPrevDirection=0; //domyœlnie wirtualne odcinki do³¹czamy stron¹ od Point1
 iNextDirection=0;
 pIsolated=NULL;
 pMyNode=g; //Ra: proteza, ¿eby tor zna³ swoj¹ nazwê TODO: odziedziczyæ TTrack z TGroundNode
 asTrackName = "";
}

__fastcall TTrack::~TTrack()
{//likwidacja odcinka
 if (eType==tt_Normal)
  delete Segment; //dla zwrotnic nie usuwaæ tego (kopiowany)
 else
  SafeDelete(SwitchExtension);
}

void __fastcall TTrack::Init()
{//tworzenie pomocniczych danych

asTrackName = Global::asCurrentNodeName;
 switch (eType)
 {
  case tt_Switch:
  case tt_Cross:
   SwitchExtension=new TSwitchExtension(this);
  break;
  case tt_Normal:
   Segment=new TSegment(this);
  break;
  case tt_Turn: //oba potrzebne
   SwitchExtension=new TSwitchExtension(this);
   Segment=new TSegment(this);
  break;
 }
}

TTrack* __fastcall TTrack::NullCreate(int dir)
{//tworzenie toru wykolejaj¹cego od strony (dir)
 TGroundNode *tmp=new TGroundNode(); //node
 tmp->iType=TP_TRACK;
 TTrack* trk=new TTrack(tmp); //tor; UWAGA! obrotnica mo¿e generowaæ du¿e iloœci tego
 tmp->pTrack=trk;
 trk->bVisible=false; //nie potrzeba pokazywaæ, zreszt¹ i tak nie ma tekstur
 //trk->iTrapezoid=1; //s¹ przechy³ki do uwzglêdniania w rysowaniu
 trk->iCategoryFlag=iCategoryFlag&15; //taki sam typ
 trk->iDamageFlag=128; //wykolejenie
 trk->fVelocity=0.0; //koniec jazdy
 trk->Init(); //utworzenie segmentu
 double r1,r2;
 Segment->GetRolls(r1,r2); //pobranie przechy³ek na pocz¹tku toru
 vector3 p1,cv1,cv2,p2; //bêdziem tworzyæ trajektoriê lotu
 switch (dir)
 {//³¹czenie z nowym torem
  case 0:
   p1=Segment->FastGetPoint_0();
   p2=p1-450.0*Normalize(Segment->GetDirection1());
   trk->Segment->Init(p1,p2,5,-RadToDeg(r1),70.0); //bo prosty, kontrolne wyliczane przy zmiennej przechy³ce
   ConnectPrevPrev(trk,0);
   break;
  case 1:
   p1=Segment->FastGetPoint_1();
   p2=p1-450.0*Normalize(Segment->GetDirection2());
   trk->Segment->Init(p1,p2,5,RadToDeg(r2),70.0); //bo prosty, kontrolne wyliczane przy zmiennej przechy³ce
   ConnectNextPrev(trk,0);
   break;
  case 3: //na razie nie mo¿liwe
   p1=SwitchExtension->Segments[1]->FastGetPoint_1(); //koniec toru drugiego zwrotnicy
   p2=p1-450.0*Normalize(SwitchExtension->Segments[1]->GetDirection2()); //przed³u¿enie na wprost
   trk->Segment->Init(p1,p2,5,RadToDeg(r2),70.0); //bo prosty, kontrolne wyliczane przy zmiennej przechy³ce
   ConnectNextPrev(trk,0);
   //trk->ConnectPrevNext(trk,dir);
   SetConnections(1); //skopiowanie po³¹czeñ
   Switch(1); //bo siê prze³¹czy na 0, a to coœ chce siê przecie¿ wykoleiæ na bok
   break; //do drugiego zwrotnicy... nie zadzia³a?
 }
 //trzeba jeszcze dodaæ do odpowiedniego segmentu, aby siê renderowa³y z niego pojazdy
 tmp->pCenter=(0.5*(p1+p2)); //œrodek, aby siê mog³o wyœwietliæ
 //Ra: to poni¿ej to pora¿ka, ale na razie siê nie da inaczej
 TSubRect *r=Global::pGround->GetSubRect(tmp->pCenter.x,tmp->pCenter.z);
 r->NodeAdd(tmp); //dodanie toru do segmentu
 r->Sort(); //¿eby wyœwietla³ tabor z dodanego toru
 r->Release(); //usuniêcie skompilowanych zasobów
 return trk;
};

void __fastcall TTrack::ConnectPrevPrev(TTrack *pTrack,int typ)
{//³aczenie torów - Point1 w³asny do Point1 cudzego
 if (pTrack)
 {
  pPrev=pTrack;
  iPrevDirection=0;
  pTrack->pPrev=this;
  pTrack->iPrevDirection=0;
 }
}
void __fastcall TTrack::ConnectPrevNext(TTrack *pTrack,int typ)
{//³aczenie torów - Point1 w³asny do Point2 cudzego
 if (pTrack)
 {
  pPrev=pTrack;
  iPrevDirection=typ|1; //1:zwyk³y lub pierwszy zwrotnicy, 3:drugi zwrotnicy
  pTrack->pNext=this;
  pTrack->iNextDirection=0;
  if (bVisible)
   if (pTrack->bVisible)
    if (eType==tt_Normal) //jeœli ³¹czone s¹ dwa normalne
     if (pTrack->eType==tt_Normal)
      if ((fTrackWidth!=pTrack->fTrackWidth) //Ra: jeœli kolejny ma inne wymiary
       || (fTexHeight!=pTrack->fTexWidth)
       || (fTexWidth!=pTrack->fTexWidth)
       || (fTexSlope!=pTrack->fTexSlope))
       pTrack->iTrapezoid|=2; //to rysujemy potworka
 }
}
void __fastcall TTrack::ConnectNextPrev(TTrack *pTrack,int typ)
{//³aczenie torów - Point2 w³asny do Point1 cudzego
 if (pTrack)
 {
  pNext=pTrack;
  iNextDirection=0;
  pTrack->pPrev=this;
  pTrack->iPrevDirection=1;
  if (bVisible)
   if (pTrack->bVisible)
    if (eType==tt_Normal) //jeœli ³¹czone s¹ dwa normalne
     if (pTrack->eType==tt_Normal)
      if ((fTrackWidth!=pTrack->fTrackWidth) //Ra: jeœli kolejny ma inne wymiary
       || (fTexHeight!=pTrack->fTexWidth)
       || (fTexWidth!=pTrack->fTexWidth)
       || (fTexSlope!=pTrack->fTexSlope))
       iTrapezoid|=2; //to rysujemy potworka
 }
}
void __fastcall TTrack::ConnectNextNext(TTrack *pTrack,int typ)
{//³aczenie torów - Point2 w³asny do Point2 cudzego
 if (pTrack)
 {
  pNext=pTrack;
  iNextDirection=typ|1; //1:zwyk³y lub pierwszy zwrotnicy, 3:drugi zwrotnicy
  pTrack->pNext=this;
  pTrack->iNextDirection=1;
 }
}

vector3 __fastcall MakeCPoint(vector3 p,double d,double a1,double a2)
{
 vector3 cp=vector3(0,0,1);
 cp.RotateX(DegToRad(a2));
 cp.RotateY(DegToRad(a1));
 cp=cp*d+p;
 return cp;
}

vector3 __fastcall LoadPoint(cParser *parser)
{//pobranie wspó³rzêdnych punktu
 vector3 p;
 std::string token;
 parser->getTokens(3);
 *parser >> p.x >> p.y >> p.z;
 return p;
}

void __fastcall TTrack::Load(cParser *parser,vector3 pOrigin,AnsiString name)
{//pobranie obiektu trajektorii ruchu
 vector3 pt,vec,p1,p2,cp1,cp2;
 double a1,a2,r1,r2,d1,d2,a;
 AnsiString str;
 bool bCurve;
 int i;//,state; //Ra: teraz ju¿ nie ma pocz¹tkowego stanu zwrotnicy we wpisie
 std::string token;

 parser->getTokens();
 *parser >> token;
 str=AnsiString(token.c_str()); //typ toru

 if (str=="normal")
 {
  eType=tt_Normal;
  iCategoryFlag=1;
 }
 else if (str=="switch")
 {
  eType=tt_Switch;
  iCategoryFlag=1;
 }
 else if (str=="turn")
 {//Ra: to jest obrotnica
  eType=tt_Turn;
  iCategoryFlag=1;
 }
 else if (str=="road")
 {
  eType=tt_Normal;
  iCategoryFlag=2;
 }
 else if (str=="cross")
 {//Ra: to bêdzie skrzy¿owanie dróg
  eType=tt_Cross;
  iCategoryFlag=2;
 }
 else if (str=="river")
 {eType=tt_Normal;
  iCategoryFlag=4;
 }
 else if (str=="tributary")
 {eType=tt_Tributary;
  iCategoryFlag=4;
 }
 else
  eType=tt_Unknown;
 //if (DebugModeFlag)
 // WriteLog(str.c_str());
 parser->getTokens(4);
 *parser >> fTrackLength >> fTrackWidth >> fFriction >> fSoundDistance;
//    fTrackLength=Parser->GetNextSymbol().ToDouble();                       //track length 100502
//    fTrackWidth=Parser->GetNextSymbol().ToDouble();                        //track width
//    fFriction=Parser->GetNextSymbol().ToDouble();                          //friction coeff.
//    fSoundDistance=Parser->GetNextSymbol().ToDouble();   //snd
 parser->getTokens(2);
 *parser >> iQualityFlag >> iDamageFlag;
//    iQualityFlag=Parser->GetNextSymbol().ToInt();   //McZapkie: qualityflag
//    iDamageFlag=Parser->GetNextSymbol().ToInt();   //damage
 parser->getTokens();
 *parser >> token;
 str=AnsiString(token.c_str());  //environment
 if (str=="flat")
  eEnvironment=e_flat;
 else if (str=="mountains" || str=="mountain")
  eEnvironment=e_mountains;
 else if (str=="canyon")
  eEnvironment=e_canyon;
 else if (str=="tunnel")
  eEnvironment=e_tunnel;
 else if (str=="bridge")
  eEnvironment=e_bridge;
 else if (str=="bank")
  eEnvironment=e_bank;
 else
 {
  eEnvironment=e_unknown;
  Error("Unknown track environment: \""+str+"\"");
 }
 parser->getTokens();
 *parser >> token;
 bVisible=(token.compare( "vis" )==0);   //visible
 if (bVisible)
  {
   parser->getTokens();
   *parser >> token;
   str=AnsiString(token.c_str());   //railtex
   TextureID1=(str=="none"?0:TTexturesManager::GetTextureID(str.c_str(),(iCategoryFlag&1)?Global::iRailProFiltering:Global::iBallastFiltering));
   parser->getTokens();
   *parser >> fTexLength; //tex tile length
   if (fTexLength<0.01) fTexLength=4; //Ra: zabezpiecznie przed zawieszeniem
   parser->getTokens();
   *parser >> token;
   str=AnsiString(token.c_str());   //sub || railtex
   TextureID2=(str=="none"?0:TTexturesManager::GetTextureID(str.c_str(),(eType==tt_Normal)?Global::iBallastFiltering:Global::iRailProFiltering));
   parser->getTokens(3);
   *parser >> fTexHeight >> fTexWidth >> fTexSlope;
//      fTexHeight=Parser->GetNextSymbol().ToDouble(); //tex sub height
//      fTexWidth=Parser->GetNextSymbol().ToDouble(); //tex sub width
//      fTexSlope=Parser->GetNextSymbol().ToDouble(); //tex sub slope width
   if (iCategoryFlag&4)
    fTexHeight=-fTexHeight; //rzeki maj¹ wysokoœæ odwrotnie ni¿ drogi
  }
 //else if (DebugModeFlag) WriteLog("unvis");
 Init();
 double segsize=5.0; //d³ugoœæ odcinka segmentowania
 switch (eType)
 {//Ra: ³uki segmentowane co 5m albo 314-k¹tem foremnym
  case tt_Turn: //obrotnica jest prawie jak zwyk³y tor
  case tt_Normal:
   p1=LoadPoint(parser)+pOrigin; //pobranie wspó³rzêdnych P1
   parser->getTokens();
   *parser >> r1; //pobranie przechy³ki w P1
   cp1=LoadPoint(parser); //pobranie wspó³rzêdnych punktów kontrolnych
   cp2=LoadPoint(parser);
   p2=LoadPoint(parser)+pOrigin; //pobranie wspó³rzêdnych P2
   parser->getTokens(2);
   *parser >> r2 >> fRadius; //pobranie przechy³ki w P1 i promienia
   if (iCategoryFlag&1)
   {//zero na g³ówce szyny
    p1.y+=0.18;
    p2.y+=0.18;
    //na przechy³ce doliczyæ jeszcze pó³ przechy³ki
   }
   if (fRadius!=0) //gdy podany promieñ
      segsize=Min0R(5.0,0.2+fabs(fRadius)*0.02); //do 250m - 5, potem 1 co 50m

   if ((((p1+p1+p2)/3.0-p1-cp1).Length()<0.02)||(((p1+p2+p2)/3.0-p2+cp1).Length()<0.02))
    cp1=cp2=vector3(0,0,0); //"prostowanie" prostych z kontrolnymi, dok³adnoœæ 2cm

   if ((cp1==vector3(0,0,0)) && (cp2==vector3(0,0,0))) //Ra: hm, czasem dla prostego s¹ podane...
    Segment->Init(p1,p2,segsize,r1,r2); //gdy prosty, kontrolne wyliczane przy zmiennej przechy³ce
   else
    Segment->Init(p1,cp1+p1,cp2+p2,p2,segsize,r1,r2); //gdy ³uk (ustawia bCurve=true)
   if ((r1!=0)||(r2!=0)) iTrapezoid=1; //s¹ przechy³ki do uwzglêdniania w rysowaniu
   if (eType==tt_Turn) //obrotnica ma doklejkê
   {SwitchExtension=new TSwitchExtension(this); //zwrotnica ma doklejkê
    SwitchExtension->Segments[0]->Init(p1,p2,segsize); //kopia oryginalnego toru
   }
   else if (iCategoryFlag&2)
    if (TextureID1&&fTexLength)
    {//dla drogi trzeba ustaliæ proporcje boków nawierzchni
     float w,h;
     glBindTexture(GL_TEXTURE_2D,TextureID1);
     glGetTexLevelParameterfv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&w);
     glGetTexLevelParameterfv(GL_TEXTURE_2D,0,GL_TEXTURE_HEIGHT,&h);
     if (h!=0.0) fTexRatio1=w/h; //proporcja boków
     glBindTexture(GL_TEXTURE_2D,TextureID2);
     glGetTexLevelParameterfv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&w);
     glGetTexLevelParameterfv(GL_TEXTURE_2D,0,GL_TEXTURE_HEIGHT,&h);
     if (h!=0.0) fTexRatio2=w/h; //proporcja boków
    }
  break;

  case tt_Cross: //skrzy¿owanie dróg - 4 punkty z wektorami kontrolnymi
  case tt_Switch: //zwrotnica
  case tt_Tributary: //dop³yw
   //problemy z animacj¹ iglic powstaje, gdzy odcinek prosty ma zmienn¹ przechy³kê
   //wtedy dzieli siê na dodatkowe odcinki (po 0.2m, bo R=0) i animacjê diabli bior¹
   //Ra: na razie nie podejmujê siê przerabiania iglic

   SwitchExtension=new TSwitchExtension(this); //zwrotnica ma doklejkê

   p1=LoadPoint(parser)+pOrigin; //pobranie wspó³rzêdnych P1
   parser->getTokens();
   *parser >> r1;
   cp1=LoadPoint(parser);
   cp2=LoadPoint(parser);
   p2=LoadPoint(parser)+pOrigin; //pobranie wspó³rzêdnych P2
   parser->getTokens(2);
   *parser >> r2 >> fRadiusTable[0];
   if (iCategoryFlag&1)
   {//zero na g³ówce szyny
    p1.y+=0.18;
    p2.y+=0.18;
    //na przechy³ce doliczyæ jeszcze pó³ przechy³ki?
   }
   if (fRadiusTable[0]>0)
    segsize=Min0R(5.0,0.2+fRadiusTable[0]*0.02);
   else
    if (eType!=tt_Cross)
    {//jak promieñ zerowy, to przeliczamy punkty kontrolne
     cp1=(p1+p1+p2)/3.0-p1; //jak jest prosty, to siê zoptymalizuje
     cp2=(p1+p2+p2)/3.0-p2;
     segsize=5.0;
    } //u³omny prosty
   if (!(cp1==vector3(0,0,0)) && !(cp2==vector3(0,0,0)))
    SwitchExtension->Segments[0]->Init(p1,cp1+p1,cp2+p2,p2,segsize,r1,r2);
   else
    SwitchExtension->Segments[0]->Init(p1,p2,segsize,r1,r2);

   p1=LoadPoint(parser)+pOrigin; //pobranie wspó³rzêdnych P3
   parser->getTokens();
   *parser >> r1;
   cp1=LoadPoint(parser);
   cp2=LoadPoint(parser);
   p2=LoadPoint(parser)+pOrigin; //pobranie wspó³rzêdnych P4
   parser->getTokens(2);
   *parser >> r2 >> fRadiusTable[1];
   if (iCategoryFlag&1)
   {//zero na g³ówce szyny
    p1.y+=0.18;
    p2.y+=0.18;
    //na przechy³ce doliczyæ jeszcze pó³ przechy³ki?
   }

   if (fRadiusTable[1]>0)
      segsize=Min0R(5.0,0.2+fRadiusTable[1]*0.02);
   else
      segsize=5.0;

   if (!(cp1==vector3(0,0,0)) && !(cp2==vector3(0,0,0)))
       SwitchExtension->Segments[1]->Init(p1,cp1+p1,cp2+p2,p2,segsize,r1,r2);
   else
       SwitchExtension->Segments[1]->Init(p1,p2,segsize,r1,r2);

   Switch(0); //na sta³e w po³o¿eniu 0 - nie ma pocz¹tkowego stanu zwrotnicy we wpisie

   //Ra: zamieniæ póŸniej na iloczyn wektorowy
   {vector3 v1,v2;
    double a1,a2;
    v1=SwitchExtension->Segments[0]->FastGetPoint_1()-SwitchExtension->Segments[0]->FastGetPoint_0();
    v2=SwitchExtension->Segments[1]->FastGetPoint_1()-SwitchExtension->Segments[1]->FastGetPoint_0();
    a1=atan2(v1.x,v1.z);
    a2=atan2(v2.x,v2.z);
    a2=a2-a1;
    while (a2>M_PI) a2=a2-2*M_PI;
    while (a2<-M_PI) a2=a2+2*M_PI;
    SwitchExtension->RightSwitch=a2<0; //lustrzany uk³ad OXY...
   }
  break;
 }
 parser->getTokens();
 *parser >> token;
 str=AnsiString(token.c_str());
 while (str!="endtrack")
 {
  if (str=="event0")
  {
   parser->getTokens();
   *parser >> token;
   asEvent0Name=AnsiString(token.c_str());
  }
  else if (str=="event1")
  {
   parser->getTokens();
   *parser >> token;
   asEvent1Name=AnsiString(token.c_str());
  }
  else if (str=="event2")
  {
   parser->getTokens();
   *parser >> token;
   asEvent2Name=AnsiString(token.c_str());
  }
  else if (str=="eventall0")
  {
   parser->getTokens();
   *parser >> token;
   asEventall0Name=AnsiString(token.c_str());
  }
  else if (str=="eventall1")
  {
   parser->getTokens();
   *parser >> token;
   asEventall1Name=AnsiString(token.c_str());
  }
  else if (str=="eventall2")
  {
   parser->getTokens();
   *parser >> token;
   asEventall2Name=AnsiString(token.c_str());
  }
  else if (str=="velocity")
  {
   parser->getTokens();
   *parser >> fVelocity; //*0.28; McZapkie-010602
  }
  else if (str=="isolated")
  {//obwód izolowany, do którego tor nale¿y
   parser->getTokens();
   *parser >> token;
   pIsolated=TIsolated::Find(AnsiString(token.c_str()));
  }
  else if (str=="angle1")
  {//k¹t œciêcia od strony 1
   parser->getTokens();
   *parser >> a1;
   Segment->AngleSet(0,a1);
  }
  else if (str=="angle2")
  {//k¹t œciêcia od strony 2
   parser->getTokens();
   *parser >> a2;
   Segment->AngleSet(1,a2);
  }
  else
   Error("Unknown track property: \""+str+"\"");
  parser->getTokens(); *parser >> token;
  str=AnsiString(token.c_str());
 }
 //alternatywny zapis nazwy odcinka izolowanego - po znaku "@" w nazwie toru
 if (!pIsolated)
  if ((i=name.Pos("@"))>0)
   if (i<name.Length()) //nie mo¿e byæ puste
    pIsolated=TIsolated::Find(name.SubString(i+1,name.Length()));
}

bool __fastcall TTrack::AssignEvents(TEvent *NewEvent0,TEvent *NewEvent1,TEvent *NewEvent2)
{
 bool bError=false;
 if (!Event0)
 {
  if (NewEvent0)
  {
   Event0=NewEvent0;
   asEvent0Name="";
   iEvents|=1; //sumaryczna informacja o eventach
  }
  else
  {
   if (!asEvent0Name.IsEmpty())
   {
    Error(AnsiString("Event0 \"")+asEvent0Name+AnsiString("\" does not exist"));
    bError=true;
   }
  }
 }
 else
 {
  Error(AnsiString("Event 0 cannot be assigned to track, track already has one"));
  bError=true;
 }
 if (!Event1)
 {
  if (NewEvent1)
  {
   Event1=NewEvent1;
   asEvent1Name="";
   iEvents|=2; //sumaryczna informacja o eventach
  }
  else
  {
   if (!asEvent0Name.IsEmpty())
   {//Ra: tylko w logu informacja
    WriteLog(AnsiString("Event1 \"")+asEvent1Name+AnsiString("\" does not exist").c_str());
    bError=true;
   }
  }
 }
 else
 {
  Error(AnsiString("Event 1 cannot be assigned to track, track already has one"));
  bError=true;
 }
 if (!Event2)
 {
  if (NewEvent2)
  {
   Event2=NewEvent2;
   asEvent2Name="";
   iEvents|=4; //sumaryczna informacja o eventach
  }
  else
  {
   if (!asEvent0Name.IsEmpty())
   {//Ra: tylko w logu informacja
    WriteLog(AnsiString("Event2 \"")+asEvent2Name+AnsiString("\" does not exist"));
    bError=true;
   }
  }
 }
 else
 {
  Error(AnsiString("Event 2 cannot be assigned to track, track already has one"));
  bError=true;
 }
 return !bError;
}

bool __fastcall TTrack::AssignallEvents(TEvent *NewEvent0,TEvent *NewEvent1,TEvent *NewEvent2)
{
 bool bError=false;
 if (!Eventall0)
 {
  if (NewEvent0)
  {
   Eventall0=NewEvent0;
   asEventall0Name="";
   iEvents|=8; //sumaryczna informacja o eventach
  }
  else
  {
   if (!asEvent0Name.IsEmpty())
   {
    Error(AnsiString("Eventall 0 \"")+asEventall0Name+AnsiString("\" does not exist"));
    bError=true;
   }
  }
 }
 else
 {
  Error(AnsiString("Eventall 0 cannot be assigned to track, track already has one"));
  bError=true;
 }
 if (!Eventall1)
 {
  if (NewEvent1)
  {
   Eventall1=NewEvent1;
   asEventall1Name="";
   iEvents|=16; //sumaryczna informacja o eventach
  }
  else
  {
   if (!asEvent0Name.IsEmpty())
   {//Ra: tylko w logu informacja
    WriteLog(AnsiString("Eventall 1 \"")+asEventall1Name+AnsiString("\" does not exist"));
    bError=true;
   }
  }
 }
 else
 {
  Error(AnsiString("Eventall 1 cannot be assigned to track, track already has one"));
  bError=true;
 }
 if (!Eventall2)
 {
  if (NewEvent2)
  {
   Eventall2=NewEvent2;
   asEventall2Name="";
   iEvents|=32; //sumaryczna informacja o eventach
  }
  else
  {
   if (!asEvent0Name.IsEmpty())
   {//Ra: tylko w logu informacja
    WriteLog(AnsiString("Eventall 2 \"")+asEventall2Name+AnsiString("\" does not exist"));
    bError=true;
   }
  }
 }
 else
 {
  Error(AnsiString("Eventall 2 cannot be assigned to track, track already has one"));
  bError=true;
 }
 return !bError;
}

bool __fastcall TTrack::AssignForcedEvents(TEvent *NewEventPlus, TEvent *NewEventMinus)
{//ustawienie eventów sygnalizacji rozprucia
 if (SwitchExtension)
 {if (NewEventPlus)
   SwitchExtension->EventPlus=NewEventPlus;
  if (NewEventMinus)
   SwitchExtension->EventMinus=NewEventMinus;
  return true;
 }
 return false;
};

AnsiString __fastcall TTrack::IsolatedName()
{//podaje nazwê odcinka izolowanego, jesli nie ma on jeszcze przypisanych zdarzeñ
 if (pIsolated)
  if (!pIsolated->eBusy&&!pIsolated->eFree)
   return pIsolated->asName;
 return "";
};

bool __fastcall TTrack::IsolatedEventsAssign(TEvent *busy, TEvent *free)
{//ustawia zdarzenia dla odcinka izolowanego
 if (pIsolated)
 {if (busy)
   pIsolated->eBusy=busy;
  if (free)
   pIsolated->eFree=free;
  return true;
 }
 return false;
};


//ABu: przeniesione z Track.h i poprawione!!!
bool __fastcall TTrack::AddDynamicObject(TDynamicObject *Dynamic)
{//dodanie pojazdu do trajektorii
 //Ra: tymczasowo wysy³anie informacji o zajêtoœci konkretnego toru
 //Ra: usun¹æ po upowszechnieniu siê odcinków izolowanych
 if (iCategoryFlag&0x100) //jeœli usuwaczek
 {Dynamic->MyTrack=NULL;
  return true;
 }
 if (Global::iMultiplayer) //jeœli multiplayer
  if (!iNumDynamics) //pierwszy zajmuj¹cy
   if (pMyNode->asName!="none")
    Global::pGround->WyslijString(pMyNode->asName,8); //przekazanie informacji o zajêtoœci toru
 if (iNumDynamics<iMaxNumDynamics)
 {//jeœli jest miejsce, dajemy na koniec
  Dynamics[iNumDynamics++]=Dynamic;
  Dynamic->MyTrack=this; //ABu: na ktorym torze jesteœmy
  return true;
 }
 else
 {
  Error("Too many dynamics on track "+pMyNode->asName);
  return false;
 }
};

void __fastcall TTrack::MoveMe(vector3 pPosition)
{
    if(SwitchExtension)
    {
        SwitchExtension->Segments[0]->MoveMe(1*pPosition);
        SwitchExtension->Segments[1]->MoveMe(1*pPosition);
        SwitchExtension->Segments[2]->MoveMe(3*pPosition); //Ra: 3 razy?
        SwitchExtension->Segments[3]->MoveMe(4*pPosition);
    }
    else
    {
       Segment->MoveMe(pPosition);
    };
    ResourceManager::Unregister(this);
};


const int numPts=4;
const int nnumPts=12;
const vector6 szyna[nnumPts]= //szyna - vextor3(x,y,mapowanie tekstury)
{vector6( 0.111,-0.180,0.00, 1.000, 0.000,0.000),
 vector6( 0.045,-0.155,0.15, 0.707, 0.707,0.000),
 vector6( 0.045,-0.070,0.25, 0.707,-0.707,0.000),
 vector6( 0.071,-0.040,0.35, 0.707,-0.707,0.000), //albo tu 0.073
 vector6( 0.072,-0.010,0.40, 0.707, 0.707,0.000),
 vector6( 0.052,-0.000,0.45, 0.000, 1.000,0.000),
 vector6( 0.020,-0.000,0.55, 0.000, 1.000,0.000),
 vector6( 0.000,-0.010,0.60,-0.707, 0.707,0.000),
 vector6( 0.001,-0.040,0.65,-0.707,-0.707,0.000), //albo tu -0.001
 vector6( 0.027,-0.070,0.75,-0.707,-0.707,0.000), //albo zostanie asymetryczna
 vector6( 0.027,-0.155,0.85,-0.707, 0.707,0.000),
 vector6(-0.039,-0.180,1.00,-1.000, 0.000,0.000)
};
const vector6 iglica[nnumPts]= //iglica - vextor3(x,y,mapowanie tekstury)
{vector6( 0.010,-0.180,0.00, 1.000, 0.000,0.000),
 vector6( 0.010,-0.155,0.15, 1.000, 0.000,0.000),
 vector6( 0.010,-0.070,0.25, 1.000, 0.000,0.000),
 vector6( 0.010,-0.040,0.35, 1.000, 0.000,0.000),
 vector6( 0.010,-0.010,0.40, 1.000, 0.000,0.000),
 vector6( 0.010,-0.000,0.45, 0.707, 0.707,0.000),
 vector6( 0.000,-0.000,0.55, 0.707, 0.707,0.000),
 vector6( 0.000,-0.010,0.60,-1.000, 0.000,0.000),
 vector6( 0.000,-0.040,0.65,-1.000, 0.000,0.000),
 vector6( 0.000,-0.070,0.75,-1.000, 0.000,0.000),
 vector6( 0.000,-0.155,0.85,-0.707, 0.707,0.000),
 vector6(-0.040,-0.180,1.00,-1.000, 0.000,0.000) //1mm wiêcej, ¿eby nie nachodzi³y tekstury?
};



void __fastcall TTrack::Compile(GLuint tex)
{//generowanie treœci dla Display Lists - model proceduralny

 GLuint  TextureID3;

 if (!tex)
 {//jeœli nie podana tekstura, to ka¿dy tor ma wlasne DL
  if (DisplayListID)
   Release(); //zwolnienie zasobów w celu ponownego utworzenia
  if (Global::bManageNodes)
  {
   DisplayListID=glGenLists(1); //otwarcie nowej listy
   glNewList(DisplayListID,GL_COMPILE);
  };
 }
 glColor3f(1.0f,1.0f,1.0f); //to tutaj potrzebne?
 //Ra: nie zmieniamy oœwietlenia przy kompilowaniu, poniewa¿ ono siê zmienia w czasie!
 //trochê podliczonych zmiennych, co siê potem przydadz¹
 double fHTW=0.5*fabs(fTrackWidth); //po³owa szerokoœci
 double side=fabs(fTexWidth); //szerokœæ podsypki na zewn¹trz szyny albo pobocza
 double slop=fabs(fTexSlope); //szerokoœæ pochylenia
 double rozp=fHTW+side+slop; //brzeg zewnêtrzny
 double hypot1=hypot(slop,fTexHeight); //rozmiar pochylenia do liczenia normalnych
 if (hypot1==0.0) hypot1=1.0;
 vector3 normal1=vector3(fTexSlope/hypot1,fTexHeight/hypot1,0.0); //wektor normalny
 double fHTW2,side2,slop2,rozp2,fTexHeight2,hypot2;
 vector3 normal2;
 if (iTrapezoid&2) //ten bit oznacza, ¿e istnieje odpowiednie pNext
 {//Ra: jest OK
  fHTW2=0.5*fabs(pNext->fTrackWidth); //po³owa rozstawu/nawierzchni
  side2=fabs(pNext->fTexWidth);
  slop2=fabs(pNext->fTexSlope);
  rozp2=fHTW2+side2+slop2; //szerokoœæ podstawy
  fTexHeight2=pNext->fTexHeight;
  hypot2=hypot(slop2,pNext->fTexHeight);
  if (hypot2==0.0) hypot2=1.0;
  normal2=vector3(pNext->fTexSlope/hypot2,fTexHeight2/hypot2,0.0);
 }
 else //gdy nie ma nastêpnego albo jest nieodpowiednim koñcem podpiêty
 {fHTW2=fHTW; side2=side; slop2=slop; rozp2=rozp; fTexHeight2=fTexHeight; normal2=normal1;}
 double roll1,roll2;
 switch (iCategoryFlag&15)
 {
  case 1: //tor
  {
   Segment->GetRolls(roll1,roll2);
   double sin1=sin(roll1),cos1=cos(roll1),sin2=sin(roll2),cos2=cos(roll2);
   // zwykla szyna: //Ra: czemu g³ówki s¹ asymetryczne na wysokoœci 0.140?
   vector6 rpts1[24],rpts2[24],rpts3[24],rpts4[24];
   vector6 bptsr[24];
   int i;
   for (i=0;i<12;++i)
   {rpts1[i]   =vector6(( fHTW+szyna[i].x)*cos1+szyna[i].y*sin1,-( fHTW+szyna[i].x)*sin1+szyna[i].y*cos1,szyna[i].z ,+szyna[i].n.x*cos1+szyna[i].n.y*sin1,-szyna[i].n.x*sin1+szyna[i].n.y*cos1,0.0);
    rpts2[11-i]=vector6((-fHTW-szyna[i].x)*cos1+szyna[i].y*sin1,-(-fHTW-szyna[i].x)*sin1+szyna[i].y*cos1,szyna[i].z ,-szyna[i].n.x*cos1+szyna[i].n.y*sin1,+szyna[i].n.x*sin1+szyna[i].n.y*cos1,0.0);
   }
   if (iTrapezoid) //trapez albo przechy³ki, to oddzielne punkty na koñcu
    for (i=0;i<12;++i)
    {rpts1[12+i]=vector6(( fHTW2+szyna[i].x)*cos2+szyna[i].y*sin2,-( fHTW2+szyna[i].x)*sin2+szyna[i].y*cos2,szyna[i].z ,+szyna[i].n.x*cos2+szyna[i].n.y*sin2,-szyna[i].n.x*sin2+szyna[i].n.y*cos2,0.0);
     rpts2[23-i]=vector6((-fHTW2-szyna[i].x)*cos2+szyna[i].y*sin2,-(-fHTW2-szyna[i].x)*sin2+szyna[i].y*cos2,szyna[i].z ,-szyna[i].n.x*cos2+szyna[i].n.y*sin2,+szyna[i].n.x*sin2+szyna[i].n.y*cos2,0.0);
    }
   switch (eType) //dalej zale¿nie od typu
   {
    case tt_Turn: //obrotnica jak zwyk³y tor, tylko animacja dochodzi
     if (InMovement()) //jeœli siê krêci
     {//wyznaczamy wspó³rzêdne koñców, przy za³o¿eniu sta³ego œródka i d³ugoœci
      double hlen=0.5*SwitchExtension->Segments[0]->GetLength(); //po³owa d³ugoœci
      //SwitchExtension->fOffset1=SwitchExtension->pAnim?SwitchExtension->pAnim->AngleGet():0.0; //pobranie k¹ta z modelu
      TAnimContainer *ac=SwitchExtension->pModel?SwitchExtension->pModel->GetContainer(NULL):NULL;
      if (ac)
       SwitchExtension->fOffset1=ac?180+ac->AngleGet():0.0; //pobranie k¹ta z modelu
      double sina=hlen*sin(DegToRad(SwitchExtension->fOffset1)),cosa=hlen*cos(DegToRad(SwitchExtension->fOffset1));
      vector3 middle=SwitchExtension->Segments[0]->FastGetPoint(0.5);
      Segment->Init(middle+vector3(sina,0.0,cosa),middle-vector3(sina,0.0,cosa),5.0);
     }
    case tt_Normal:
     if (TextureID2)
      if (tex?TextureID2==tex:true) //jeœli pasuje do grupy (tex)
      {//podsypka z podk³adami jest tylko dla zwyk³ego toru

       vector6 bpts1[8]; //punkty g³ównej p³aszczyzny nie przydaj¹ siê do robienia boków
       if (iTrapezoid) //trapez albo przechy³ki
       {//podsypka z podkladami trapezowata
        //ewentualnie poprawiæ mapowanie, ¿eby œrodek mapowa³ siê na 1.435/4.671 ((0.3464,0.6536)
        //bo siê tekstury podsypki rozje¿d¿aj¹ po zmianie proporcji profilu
        bpts1[0]=vector6(rozp,              -fTexHeight-0.18,        0.00,normal1.x,-normal1.y,0.0); //lewy brzeg
        bpts1[1]=vector6((fHTW+side)*cos1,  -(fHTW+side)*sin1-0.18,  0.33,0.0,1.0,0.0); //krawêdŸ za³amania
        bpts1[2]=vector6(-bpts1[1].x,       +(fHTW+side)*sin1-0.18,  0.67,-normal1.x,-normal1.y,0.0); //prawy brzeg pocz¹tku symetrycznie
        bpts1[3]=vector6(-rozp,             -fTexHeight-0.18,        1.00,-normal1.x,-normal1.y,0.0); //prawy skos
        //przekrój koñcowy
        bpts1[4]=vector6(rozp2,             -fTexHeight2-0.18,       0.00,normal2.x,-normal2.y,0.0); //lewy brzeg
        bpts1[5]=vector6((fHTW2+side2)*cos2,-(fHTW2+side2)*sin2-0.18,0.33,0.0,1.0,0.0); //krawêdŸ za³amania
        bpts1[6]=vector6(-bpts1[5].x,       +(fHTW2+side2)*sin2-0.18,0.67,0.0,1.0,0.0); //prawy brzeg pocz¹tku symetrycznie
        bpts1[7]=vector6(-rozp2,            -fTexHeight2-0.18,       1.00,-normal2.x,-normal2.y,0.0); //prawy skos
       }
       else
       {bpts1[0]=vector6(rozp,      -fTexHeight-0.18,0.0,+normal1.x,-normal1.y,0.0); //lewy brzeg
        bpts1[1]=vector6(fHTW+side, -0.18,0.33,          +normal1.x,-normal1.y,0.0); //krawêdŸ za³amania
        bpts1[2]=vector6(-fHTW-side,-0.18,0.67,          -normal1.x,-normal1.y,0.0); //druga
        bpts1[3]=vector6(-rozp,     -fTexHeight-0.18,1.0,-normal1.x,-normal1.y,0.0); //prawy skos
       }
       if (!tex) glBindTexture(GL_TEXTURE_2D,TextureID2);
       Segment->RenderLoft(bpts1,iTrapezoid?-4:4,fTexLength);
      }
     if (TextureID1)
      if (tex?TextureID1==tex:true) //jeœli pasuje do grupy (tex)
      {// szyny
       if (!tex) glBindTexture(GL_TEXTURE_2D,TextureID1);

       Segment->RenderLoft(rpts1,iTrapezoid?-nnumPts:nnumPts,fTexLength);
       Segment->RenderLoft(rpts2,iTrapezoid?-nnumPts:nnumPts,fTexLength);
      }
      break;

    // ROZJAZDY ****************************************************************
    case tt_Switch: //dla zwrotnicy dwa razy szyny
     if (TextureID1) //zwrotnice nie s¹ grupowane, aby proœciej by³o je animowaæ
     {//iglice liczone tylko dla zwrotnic

//      WriteLog(AnsiString(pNext->Length()));
      // GENEROWANIE PODSYPEK POD ROZJAZDAMI
      if (Global::bswitchpod)
      {
      double fHTW;
      double side;
      double slop;
      double rozp;
      double hypot1;
      int numPts = 4;
      vector6 bpts1[8];                                                         //punkty g³ównej p³aszczyzny nie przydaj¹ siê do robienia boków
      vector6 bpts2[8];

      if (pNext) TextureID3 = pNext->TextureID2;                                // POBIERANIE TEKSTURY DLA PODSYPKI ROZJAZDU
      if (pPrev) TextureID3 = pPrev->TextureID2;

      if (pNext) fHTW=0.5*fabs(pNext->fTrackWidth);                             //po³owa szerokoœci
      if (pNext) side=fabs(pNext->fTexWidth);                                   //szerokœæ podsypki na zewn¹trz szyny albo pobocza
      if (pNext) slop=fabs(pNext->fTexSlope);                                   //szerokoœæ pochylenia
      if (pNext) rozp=fHTW+side+slop;                                           //brzeg zewnêtrzny
      if (pNext) hypot1=hypot(slop,fTexHeight);                                 //rozmiar pochylenia do liczenia normalnych

      if (pNext) fHTW=0.5*fabs(pNext->fTrackWidth);                             //po³owa szerokoœci
      if (pNext) side=fabs(pNext->fTexWidth);                                   //szerokœæ podsypki na zewn¹trz szyny albo pobocza
      if (pNext) slop=fabs(pNext->fTexSlope);                                   //szerokoœæ pochylenia
      if (pPrev) rozp=fHTW+side+slop;                                           //brzeg zewnêtrzny
      if (pPrev) hypot1=hypot(slop,fTexHeight);                                 //rozmiar pochylenia do liczenia normalnych

      if (hypot1==0.0) hypot1=1.0;
      vector3 normal1=vector3(fTexSlope/hypot1,fTexHeight/hypot1,0.0);          //wektor normalny

      if ((pNext) || (pPrev))
       if ((SwitchExtension->Segments[0]->fLength > 9) || (SwitchExtension->Segments[1]->fLength > 9))
     //if (asTrackName.SubString(1,3) != "ang")
         {
         bpts1[0]=vector6(rozp,       -fTexHeight-0.185, 0.00, +normal1.x, -normal1.y, 0.0); //lewy brzeg
         bpts1[1]=vector6(fHTW+side,  -0.185,            0.33, +normal1.x, -normal1.y, 0.0); //krawêdŸ za³amania
         bpts1[2]=vector6(-fHTW-side, -0.185,            0.67, -normal1.x, -normal1.y, 0.0); //druga
         bpts1[3]=vector6(-rozp,      -fTexHeight-0.185, 1.00, -normal1.x, -normal1.y, 0.0); //prawy skos

         bpts2[0]=vector6(rozp,       -fTexHeight-0.180, 0.00, +normal1.x, -normal1.y, 0.0); //lewy brzeg
         bpts2[1]=vector6(fHTW+side,  -0.180,            0.33, +normal1.x, -normal1.y, 0.0); //krawêdŸ za³amania
         bpts2[2]=vector6(-fHTW-side, -0.180,            0.67, -normal1.x, -normal1.y, 0.0); //druga
         bpts2[3]=vector6(-rozp,      -fTexHeight-0.180, 1.00, -normal1.x, -normal1.y, 0.0); //prawy skos

         glBindTexture(GL_TEXTURE_2D,TextureID3);
         SwitchExtension->Segments[0]->RenderLoft(bpts1,numPts,fTexLength);        // ODCINEK PROSTY
         SwitchExtension->Segments[1]->RenderLoft(bpts2,numPts,fTexLength);
        }

      }
      //^KONIEC RYSOWANIA PODSYPEK


      vector6 rpts3[24],rpts4[24];
      for (i=0;i<12;++i)
      {rpts3[i]   =vector6(( fHTW+iglica[i].x)*cos1+iglica[i].y*sin1,-( fHTW+iglica[i].x)*sin1+iglica[i].y*cos1,iglica[i].z,+iglica[i].n.x*cos1+iglica[i].n.y*sin1,-iglica[i].n.x*sin1+iglica[i].n.y*cos1,0.0);
       rpts4[11-i]=vector6((-fHTW-iglica[i].x)*cos1+iglica[i].y*sin1,-(-fHTW-iglica[i].x)*sin1+iglica[i].y*cos1,iglica[i].z,-iglica[i].n.x*cos1+iglica[i].n.y*sin1,+iglica[i].n.x*sin1+iglica[i].n.y*cos1,0.0);
      }
      if (iTrapezoid) //trapez albo przechy³ki, to oddzielne punkty na koñcu
       for (i=0;i<12;++i)
       {rpts3[12+i]=vector6(( fHTW2+iglica[i].x)*cos2+iglica[i].y*sin2,-( fHTW2+iglica[i].x)*sin2+iglica[i].y*cos2,iglica[i].z,+iglica[i].n.x*cos2+iglica[i].n.y*sin2,-iglica[i].n.x*sin2+iglica[i].n.y*cos2,0.0);
        rpts4[23-i]=vector6((-fHTW2-iglica[i].x)*cos2+iglica[i].y*sin2,-(-fHTW2-iglica[i].x)*sin2+iglica[i].y*cos2,iglica[i].z,-iglica[i].n.x*cos2+iglica[i].n.y*sin2,+iglica[i].n.x*sin2+iglica[i].n.y*cos2,0.0);
       }
      if (InMovement())
      {//Ra: trochê bez sensu, ¿e tu jest animacja
       double v=SwitchExtension->fDesiredOffset1-SwitchExtension->fOffset1;
       SwitchExtension->fOffset1+=sign(v)*Timer::GetDeltaTime()*0.1;
       //Ra: trzeba daæ to do klasy...
       if (SwitchExtension->fOffset1<=0.00)
       {SwitchExtension->fOffset1; //1cm?
        SwitchExtension->bMovement=false; //koniec animacji
       }
       if (SwitchExtension->fOffset1>=fMaxOffset)
       {SwitchExtension->fOffset1=fMaxOffset; //maksimum-1cm?
        SwitchExtension->bMovement=false; //koniec animacji
       }
      }








      //McZapkie-130302 - poprawione rysowanie szyn
      if (SwitchExtension->RightSwitch)
      {//nowa wersja z SPKS, ale odwrotnie lewa/prawa
       glBindTexture(GL_TEXTURE_2D,TextureID1);
       SwitchExtension->Segments[0]->RenderLoft(rpts1,nnumPts,fTexLength,2);
       SwitchExtension->Segments[0]->RenderSwitchRail(rpts1,rpts3,nnumPts,fTexLength,2,SwitchExtension->fOffset1);
       SwitchExtension->Segments[0]->RenderLoft(rpts2,nnumPts,fTexLength);
       if (TextureID2!=TextureID1) //nie wiadomo, czy OpenGL to optymalizuje
        glBindTexture(GL_TEXTURE_2D,TextureID2);
       SwitchExtension->Segments[1]->RenderLoft(rpts1,nnumPts,fTexLength);
       SwitchExtension->Segments[1]->RenderLoft(rpts2,nnumPts,fTexLength,2);

       SwitchExtension->Segments[1]->RenderSwitchRail(rpts2,rpts4,nnumPts,fTexLength,2,-fMaxOffset+SwitchExtension->fOffset1);
       //WriteLog("Kompilacja prawej"); WriteLog(AnsiString(SwitchExtension->fOffset1).c_str());
      }
      else
      {//lewa dzia³a lepiej ni¿ prawa
       glBindTexture(GL_TEXTURE_2D,TextureID1);
       SwitchExtension->Segments[0]->RenderLoft(rpts1,nnumPts,fTexLength); //lewa szyna normalna ca³a
       SwitchExtension->Segments[0]->RenderLoft(rpts2,nnumPts,fTexLength,2); //prawa szyna za iglic¹
       SwitchExtension->Segments[0]->RenderSwitchRail(rpts2,rpts4,nnumPts,fTexLength,2,-SwitchExtension->fOffset1); //prawa iglica
       if (TextureID2!=TextureID1) //nie wiadomo, czy OpenGL to optymalizuje
        glBindTexture(GL_TEXTURE_2D,TextureID2);
       SwitchExtension->Segments[1]->RenderLoft(rpts1,nnumPts,fTexLength,2); //lewa szyna za iglic¹
       SwitchExtension->Segments[1]->RenderSwitchRail(rpts1,rpts3,nnumPts,fTexLength,2,fMaxOffset-SwitchExtension->fOffset1); //lewa iglica
       SwitchExtension->Segments[1]->RenderLoft(rpts2,nnumPts,fTexLength); //prawa szyna normalnie ca³a
       //WriteLog("Kompilacja lewej"); WriteLog(AnsiString(SwitchExtension->fOffset1).c_str());
      }
     }
     break;
   }
  } //koniec obs³ugi torów
  break;
  case 2:   //McZapkie-260302 - droga - rendering
  //McZapkie:240702-zmieniony zakres widzialnosci
   switch (eType) //dalej zale¿nie od typu
   {
    case tt_Normal:  //drogi proste, bo skrzy¿owania osobno
    {vector6 bpts1[4]; //punkty g³ównej p³aszczyzny przydaj¹ siê do robienia boków
     if (TextureID1||TextureID2) //punkty siê przydadz¹, nawet jeœli nawierzchni nie ma
     {//double max=2.0*(fHTW>fHTW2?fHTW:fHTW2); //z szerszej strony jest 100%
      double max=fTexRatio1*fTexLength; //test: szerokoœæ proporcjonalna do d³ugoœci
      double map1=max>0.0?fHTW/max:0.5; //obciêcie tekstury od strony 1
      double map2=max>0.0?fHTW2/max:0.5; //obciêcie tekstury od strony 2
      if (iTrapezoid) //trapez albo przechy³ki
      {//nawierzchnia trapezowata
       Segment->GetRolls(roll1,roll2);
       bpts1[0]=vector6(fHTW*cos(roll1),-fHTW*sin(roll1),0.5-map1,sin(roll1),cos(roll1),0.0); //lewy brzeg pocz¹tku
       bpts1[1]=vector6(-bpts1[0].x,-bpts1[0].y,0.5+map1,-sin(roll1),cos(roll1),0.0); //prawy brzeg pocz¹tku symetrycznie
       bpts1[2]=vector6(fHTW2*cos(roll2),-fHTW2*sin(roll2),0.5-map2,sin(roll2),cos(roll2),0.0); //lewy brzeg koñca
       bpts1[3]=vector6(-bpts1[2].x,-bpts1[2].y,0.5+map2,-sin(roll2),cos(roll2),0.0); //prawy brzeg pocz¹tku symetrycznie
      }
      else
      {bpts1[0]=vector6( fHTW,0.0,0.5-map1,0.0,1.0,0.0);
       bpts1[1]=vector6(-fHTW,0.0,0.5+map1,0.0,1.0,0.0);
      }
     }
     if (TextureID1) //jeœli podana by³a tekstura, generujemy trójk¹ty
      if (tex?TextureID1==tex:true) //jeœli pasuje do grupy (tex)
      {//tworzenie trójk¹tów nawierzchni szosy
       if (!tex) glBindTexture(GL_TEXTURE_2D,TextureID1);
       Segment->RenderLoft(bpts1,iTrapezoid?-2:2,fTexLength);
      }
     if (TextureID2)
      if (tex?TextureID2==tex:true) //jeœli pasuje do grupy (tex)
      {//pobocze drogi - poziome przy przechy³ce (a mo¿e krawê¿nik i chodnik zrobiæ jak w Midtown Madness 2?)
       if (!tex) glBindTexture(GL_TEXTURE_2D,TextureID2);
       vector6 rpts1[6],rpts2[6]; //wspó³rzêdne przekroju i mapowania dla prawej i lewej strony
       if (fTexHeight>=0.0)
       {//standardowo od zewn¹trz pochylenie, a od wewn¹trz poziomo
        rpts1[0]=vector6(rozp,-fTexHeight,0.0); //lewy brzeg podstawy
        rpts1[1]=vector6(bpts1[0].x+side,bpts1[0].y,0.5); //lewa krawêdŸ za³amania
        rpts1[2]=vector6(bpts1[0].x,bpts1[0].y,1.0); //lewy brzeg pobocza (mapowanie mo¿e byæ inne
        rpts2[0]=vector6(bpts1[1].x,bpts1[1].y,1.0); //prawy brzeg pobocza
        rpts2[1]=vector6(bpts1[1].x-side,bpts1[1].y,0.5); //prawa krawêdŸ za³amania
        rpts2[2]=vector6(-rozp,-fTexHeight,0.0); //prawy brzeg podstawy
        if (iTrapezoid) //trapez albo przechy³ki
        {//pobocza do trapezowatej nawierzchni - dodatkowe punkty z drugiej strony odcinka
         rpts1[3]=vector6(rozp2,-fTexHeight2,0.0); //lewy brzeg lewego pobocza
         rpts1[4]=vector6(bpts1[2].x+side2,bpts1[2].y,0.5); //krawêdŸ za³amania
         rpts1[5]=vector6(bpts1[2].x,bpts1[2].y,1.0); //brzeg pobocza
         rpts2[3]=vector6(bpts1[3].x,bpts1[3].y,1.0);
         rpts2[4]=vector6(bpts1[3].x-side2,bpts1[3].y,0.5);
         rpts2[5]=vector6(-rozp2,-fTexHeight2,0.0); //prawy brzeg prawego pobocza
        }
       }
       else
       {//wersja dla chodnika: skos 1:3.75, ka¿dy chodnik innej szerokoœci
        //mapowanie propocjonalne do szerokoœci chodnika
        //krawê¿nik jest mapowany od 31/64 do 32/64 lewy i od 32/64 do 33/64 prawy
        double d=-fTexHeight/3.75; //krawê¿nik o wysokoœci 150mm jest pochylony 40mm
        double max=fTexRatio2*fTexLength; //test: szerokoœæ proporcjonalna do d³ugoœci
        double map1l=max>0.0?side/max:0.484375; //obciêcie tekstury od lewej strony punktu 1
        double map1r=max>0.0?slop/max:0.484375; //obciêcie tekstury od prawej strony punktu 1
        rpts1[0]=vector6(bpts1[0].x+slop,bpts1[0].y-fTexHeight,0.515625+map1r ); //prawy brzeg prawego chodnika
        rpts1[1]=vector6(bpts1[0].x+d,   bpts1[0].y-fTexHeight,0.515625       ); //prawy krawê¿nik u góry
        rpts1[2]=vector6(bpts1[0].x,     bpts1[0].y,           0.515625-d/2.56); //prawy krawê¿nik u do³u
        rpts2[0]=vector6(bpts1[1].x,     bpts1[1].y,           0.484375+d/2.56); //lewy krawê¿nik u do³u
        rpts2[1]=vector6(bpts1[1].x-d,   bpts1[1].y-fTexHeight,0.484375       ); //lewy krawê¿nik u góry
        rpts2[2]=vector6(bpts1[1].x-side,bpts1[1].y-fTexHeight,0.484375-map1l ); //lewy brzeg lewego chodnika
        if (iTrapezoid) //trapez albo przechy³ki
        {//pobocza do trapezowatej nawierzchni - dodatkowe punkty z drugiej strony odcinka
         slop2=fabs((iTrapezoid&2)?slop2:slop); //szerokoœæ chodnika po prawej
         double map2l=max>0.0?slop2/max:0.484375; //obciêcie tekstury od lewej strony punktu 2
         double map2r=max>0.0?side2/max:0.484375; //obciêcie tekstury od prawej strony punktu 2
         rpts1[3]=vector6(bpts1[2].x+slop2,bpts1[2].y-fTexHeight2,0.515625+map2r ); //prawy brzeg prawego chodnika
         rpts1[4]=vector6(bpts1[2].x+d,    bpts1[2].y-fTexHeight2,0.515625       ); //prawy krawê¿nik u góry
         rpts1[5]=vector6(bpts1[2].x,      bpts1[2].y,            0.515625-d/2.56); //prawy krawê¿nik u do³u
         rpts2[3]=vector6(bpts1[3].x,      bpts1[3].y,            0.484375+d/2.56); //lewy krawê¿nik u do³u
         rpts2[4]=vector6(bpts1[3].x-d,    bpts1[3].y-fTexHeight2,0.484375       ); //lewy krawê¿nik u góry
         rpts2[5]=vector6(bpts1[3].x-side2,bpts1[3].y-fTexHeight2,0.484375-map2l ); //lewy brzeg lewego chodnika
        }
       }
      if (iTrapezoid) //trapez albo przechy³ki
      {//pobocza do trapezowatej nawierzchni - dodatkowe punkty z drugiej strony odcinka
       if ((fTexHeight>=0.0)?true:(slop!=0.0))
        Segment->RenderLoft(rpts1,-3,fTexLength);
       if ((fTexHeight>=0.0)?true:(side!=0.0))
        Segment->RenderLoft(rpts2,-3,fTexLength);
      }
      else
      {//pobocza zwyk³e, brak przechy³ki
       if ((fTexHeight>=0.0)?true:(slop!=0.0))
        Segment->RenderLoft(rpts1,3,fTexLength);
       if ((fTexHeight>=0.0)?true:(side!=0.0))
        Segment->RenderLoft(rpts2,3,fTexLength);
      }
     }
     break;
    }
    case tt_Cross: //skrzy¿owanie dróg rysujemy inaczej
    {//ustalenie wspó³rzêdnych œrodka - przeciêcie Point1-Point2 z CV4-Point4
     //na razie po³owa odleg³oœci pomiêdzy Point1 i Point2, potem siê dopracuje
     vector3 p[4]; //punkty siê przydadz¹ do obliczeñ
     double a[4]; //k¹ty osi ulic wchodz¹cych
     p[0]=SwitchExtension->Segments[0]->GetDirection1(); //Point1 - pobranie wektorów kontrolnych
     p[1]=SwitchExtension->Segments[1]->GetDirection2(); //Point4
     p[2]=SwitchExtension->Segments[0]->GetDirection2(); //Point2
     p[3]=SwitchExtension->Segments[1]->GetDirection1(); //Point3
     a[0]=atan2(-p[0].x,p[0].z); // k¹ty dróg
     a[1]=atan2(-p[1].x,p[1].z);
     a[2]=atan2(-p[2].x,p[2].z);
     a[3]=atan2(-p[3].x,p[3].z);
     p[0]=SwitchExtension->Segments[0]->FastGetPoint_0(); //Point1 - pobranie wspó³rzêdnych koñców
     p[1]=SwitchExtension->Segments[1]->FastGetPoint_1(); //Point4
     p[2]=SwitchExtension->Segments[0]->FastGetPoint_1(); //Point2
     p[3]=SwitchExtension->Segments[1]->FastGetPoint_0(); //Point3 - przy trzech drogach pokrywa siê z Point1
     int drogi=(p[2]==p[0])?3:4; //jeœli punkty siê nak³adaj¹, to mamy trzy drogi
     int points=2+3*drogi,i=0,j; //ile punktów (mo¿e byc ró¿na iloœæ punktów miêdzy drogami)
     vector3 *q=new vector3[points];
     double sina0=sin(a[0]),cosa0=cos(a[0]);
     q[0]=0.5*(p[0]+p[2]); //œrodek skrzy¿owania
     for (j=0;j<drogi;++j)
     {q[++i]=p[j]+vector3(+fHTW*cos(a[j]),0,+fHTW*sin(a[j])); //Point1
      q[++i]=p[j]+vector3(-fHTW*cos(a[j]),0,-fHTW*sin(a[j]));
      ++i; //punkt w œrodku obliczymy potem
     }
     q[++i]=q[1]; //domkniêcie obszaru
     i=0; //zaczynamy od zera, aby wype³niæ œrodki, mo¿e byc ró¿na iloœæ punktów
     for (j=0;j<drogi;++j)
     {i+=3;
      q[i]=0.5*(0.5*(q[i-1]+q[i+1])+q[0]); //tak na pocz¹tek
     }
     double u,v;
     if (TextureID1) //jeœli podana tekstura nawierzchni
     {if (!tex) glBindTexture(GL_TEXTURE_2D,TextureID1);
      glBegin(GL_TRIANGLE_FAN); //takie kó³eczko bêdzie
       for (i=0;i<points;++i)
       {glNormal3f(0,1,0);
        u=(q[i].x-q[0].x)/fTexLength; //mapowanie we wspó³rzêdnych scenerii
        v=(q[i].z-q[0].z)/(fTexRatio1*fTexLength);
        glTexCoord2f(cosa0*u+sina0*v+0.5,sina0*u+cosa0*v+0.5);
        glVertex3f(q[i].x,q[i].y,q[i].z);
       }
      glEnd();
      delete[] q;
     }
     break;
    }
   }
   break;
  case 4:   //McZapkie-260302 - rzeka- rendering
   //Ra: rzeki na razie bez zmian, przechy³ki na pewno nie maj¹
   //Ra: przemyœleæ wyrównanie u góry traw¹ do czworoboku
   vector6 bpts1[numPts]={vector6( fHTW,0.0,0.0), vector6( fHTW,0.2,0.33),
                          vector6(-fHTW,0.0,0.67),vector6(-fHTW,0.0,1.0 ) };
   //Ra: dziwnie ten kszta³t wygl¹da
   if (TextureID1)
    if (tex?TextureID1==tex:true) //jeœli pasuje do grupy (tex)
    {
     if (!tex) glBindTexture(GL_TEXTURE_2D,TextureID1);
     Segment->RenderLoft(bpts1,numPts,fTexLength);
    }
   if (TextureID2)
    if (tex?TextureID2==tex:true) //jeœli pasuje do grupy (tex)
    {//brzegi rzeki prawie jak pobocze derogi, tylko inny znak ma wysokoœæ
     //znak jest zmieniany przy wczytywaniu, wiêc tu musi byc minus fTexHeight
     vector6 rpts1[3]={ vector6(rozp,-fTexHeight,0.0),
                        vector6(fHTW+side,0.0,0.5),
                        vector6(fHTW,0.0,1.0) };
     vector6 rpts2[3]={ vector6(-fHTW,0.0,1.0),
                        vector6(-fHTW-side,0.0,0.5),
                        vector6(-rozp,-fTexHeight,0.1) }; //Ra: po kiego 0.1?
     if (!tex) glBindTexture(GL_TEXTURE_2D,TextureID2);      //brzeg rzeki
     Segment->RenderLoft(rpts1,3,fTexLength);
     Segment->RenderLoft(rpts2,3,fTexLength);
    }
   break;
 }
 if (!tex)
  if (Global::bManageNodes)
   glEndList();
};

void TTrack::Release()
{
 if (DisplayListID)
  glDeleteLists(DisplayListID,1);
 DisplayListID=0;
};

void __fastcall TTrack::Render()
{
 if (bVisible && SquareMagnitude(Global::pCameraPosition-Segment->FastGetPoint(0.5)) < 810000)
 {
  if (!DisplayListID)
  {
   Compile();
   if (Global::bManageNodes)
    ResourceManager::Register(this);
  };
  SetLastUsage(Timer::GetSimulationTime());
  glCallList(DisplayListID);
  if (InMovement()) Release(); //zwrotnica w trakcie animacji do odrysowania
 };
//#ifdef _DEBUG
#if 0
 if (DebugModeFlag && ScannedFlag) //McZapkie-230702
 //if (iNumDynamics) //bêdzie kreska na zajêtym torze
 {
  vector3 pos1,pos2,pos3;
  glDisable(GL_DEPTH_TEST);
      glDisable(GL_LIGHTING);
  glColor3ub(255,0,0);
  glBindTexture(GL_TEXTURE_2D,0);
  glBegin(GL_LINE_STRIP);
      pos1=Segment->FastGetPoint_0();
      pos2=Segment->FastGetPoint(0.5);
      pos3=Segment->FastGetPoint_1();
      glVertex3f(pos1.x,pos1.y,pos1.z);
      glVertex3f(pos2.x,pos2.y+10,pos2.z);
      glVertex3f(pos3.x,pos3.y,pos3.z);
  glEnd();
  glEnable(GL_LIGHTING);
  glEnable(GL_DEPTH_TEST);
  ScannedFlag=false;
 }
#endif
 glLightfv(GL_LIGHT0,GL_AMBIENT,Global::ambientDayLight);
 glLightfv(GL_LIGHT0,GL_DIFFUSE,Global::diffuseDayLight);
 glLightfv(GL_LIGHT0,GL_SPECULAR,Global::specularDayLight);
};

bool __fastcall TTrack::CheckDynamicObject(TDynamicObject *Dynamic)
{//sprawdzenie, czy pojazd jest przypisany do toru
 for (int i=0;i<iNumDynamics;i++)
  if (Dynamic==Dynamics[i])
   return true;
 return false;
};

bool __fastcall TTrack::RemoveDynamicObject(TDynamicObject *Dynamic)
{//usuniêcie pojazdu z listy przypisanych do toru
 for (int i=0;i<iNumDynamics;i++)
 {//sprawdzanie wszystkich po kolei
  if (Dynamic==Dynamics[i])
  {//znaleziony, przepisanie nastêpnych, ¿eby dziur nie by³o
   --iNumDynamics;
   for (i;i<iNumDynamics;i++)
    Dynamics[i]=Dynamics[i+1];
   return true;
  }
 }
 Error("Cannot remove dynamic from track");
 return false;
}

bool __fastcall TTrack::InMovement()
{//tory animowane (zwrotnica, obrotnica) maj¹ SwitchExtension
 if (SwitchExtension)
 {if (eType==tt_Switch)
   return SwitchExtension->bMovement; //ze zwrotnic¹ ³atwiej
  if (eType==tt_Turn)
   if (SwitchExtension->pModel)
   {if (!SwitchExtension->CurrentIndex) return false; //0=zablokowana siê nie animuje
    //trzeba ka¿dorazowo porównywaæ z k¹tem modelu
    TAnimContainer *ac=SwitchExtension->pModel?SwitchExtension->pModel->GetContainer(NULL):NULL;
    return ac?(ac->AngleGet()!=SwitchExtension->fOffset1):false;
    //return true; //jeœli jest taki obiekt
   }
 }
 return false;
};
void __fastcall TTrack::RaAssign(TGroundNode *gn,TAnimContainer *ac)
{//Ra: wi¹zanie toru z modelem obrotnicy
 //if (eType==tt_Turn) SwitchExtension->pAnim=p;
};
void __fastcall TTrack::RaAssign(TGroundNode *gn,TAnimModel *am)
{//Ra: wi¹zanie toru z modelem obrotnicy
 if (eType==tt_Turn)
 {SwitchExtension->pModel=am;
  SwitchExtension->pMyNode=gn;
 }
};

int __fastcall TTrack::RaArrayPrepare()
{//przygotowanie tablic do skopiowania do VBO (zliczanie wierzcho³ków)
 if (bVisible) //o ile w ogóle widaæ
  switch (iCategoryFlag&15)
  {
   case 1: //tor
    if (eType==tt_Switch) //dla zwrotnicy tylko szyny
     return 48*((TextureID1?SwitchExtension->Segments[0]->RaSegCount():0)+(TextureID2?SwitchExtension->Segments[1]->RaSegCount():0));
    else //dla toru podsypka plus szyny
     return (Segment->RaSegCount())*((TextureID1?48:0)+(TextureID2?8:0));
   case 2: //droga
    return (Segment->RaSegCount())*((TextureID1?4:0)+(TextureID2?12:0));
   case 4: //rzeki do przemyœlenia
    return (Segment->RaSegCount())*((TextureID1?4:0)+(TextureID2?12:0));
  }
 return 0;
};

void  __fastcall TTrack::RaArrayFill(CVertNormTex *Vert,const CVertNormTex *Start)
{//wype³nianie tablic VBO
 //Ra: trzeba rozdzieliæ szyny od podsypki, aby móc grupowaæ wg tekstur
 double fHTW=0.5*fabs(fTrackWidth);
 double side=fabs(fTexWidth); //szerokœæ podsypki na zewn¹trz szyny albo pobocza
 double slop=fabs(fTexSlope); //brzeg zewnêtrzny
 double rozp=fHTW+side+slop; //brzeg zewnêtrzny
 double fHTW2,side2,slop2,rozp2,fTexHeight2;
 if (iTrapezoid&2) //ten bit oznacza, ¿e istnieje odpowiednie pNext
 {//Ra: jest OK
  fHTW2=0.5*fabs(pNext->fTrackWidth); //po³owa rozstawu/nawierzchni
  side2=fabs(pNext->fTexWidth);
  slop2=fabs(pNext->fTexSlope); //nie jest u¿ywane póŸniej
  rozp2=fHTW2+side2+slop2;
  fTexHeight2=pNext->fTexHeight;
 }
 else //gdy nie ma nastêpnego albo jest nieodpowiednim koñcem podpiêty
 {fHTW2=fHTW; side2=side; /*slop2=slop;*/ rozp2=rozp; fTexHeight2=fTexHeight;}
 double roll1,roll2;
 switch (iCategoryFlag&15)
 {
  case 1: //tor
  {
   if (Segment)
    Segment->GetRolls(roll1,roll2);
   else
    roll1=roll2=0.0; //dla zwrotnic
   double sin1=sin(roll1),cos1=cos(roll1),sin2=sin(roll2),cos2=cos(roll2);
   // zwykla szyna: //Ra: czemu g³ówki s¹ asymetryczne na wysokoœci 0.140?
   vector6 rpts1[24],rpts2[24],rpts3[24],rpts4[24];
   int i;
   for (i=0;i<12;++i)
   {rpts1[i]   =vector6(( fHTW+szyna[i].x)*cos1+szyna[i].y*sin1,-( fHTW+szyna[i].x)*sin1+szyna[i].y*cos1,szyna[i].z ,+szyna[i].n.x*cos1+szyna[i].n.y*sin1,-szyna[i].n.x*sin1+szyna[i].n.y*cos1,0.0);
    rpts2[11-i]=vector6((-fHTW-szyna[i].x)*cos1+szyna[i].y*sin1,-(-fHTW-szyna[i].x)*sin1+szyna[i].y*cos1,szyna[i].z ,-szyna[i].n.x*cos1+szyna[i].n.y*sin1,+szyna[i].n.x*sin1+szyna[i].n.y*cos1,0.0);
   }
   if (iTrapezoid) //trapez albo przechy³ki, to oddzielne punkty na koñcu
    for (i=0;i<12;++i)
    {rpts1[12+i]=vector6(( fHTW2+szyna[i].x)*cos2+szyna[i].y*sin2,-( fHTW2+szyna[i].x)*sin2+szyna[i].y*cos2,szyna[i].z ,+szyna[i].n.x*cos2+szyna[i].n.y*sin2,-szyna[i].n.x*sin2+szyna[i].n.y*cos2,0.0);
     rpts2[23-i]=vector6((-fHTW2-szyna[i].x)*cos2+szyna[i].y*sin2,-(-fHTW2-szyna[i].x)*sin2+szyna[i].y*cos2,szyna[i].z ,-szyna[i].n.x*cos2+szyna[i].n.y*sin2,+szyna[i].n.x*sin2+szyna[i].n.y*cos2,0.0);
    }
   switch (eType) //dalej zale¿nie od typu
   {
    case tt_Turn: //obrotnica jak zwyk³y tor, tylko animacja dochodzi
     SwitchExtension->iLeftVBO=Vert-Start; //indeks toru obrotnicy
    case tt_Normal:
     if (TextureID2)
     {//podsypka z podk³adami jest tylko dla zwyk³ego toru
      vector6 bpts1[8]; //punkty g³ównej p³aszczyzny nie przydaj¹ siê do robienia boków
      if (iTrapezoid) //trapez albo przechy³ki
      {//podsypka z podkladami trapezowata
       //ewentualnie poprawiæ mapowanie, ¿eby œrodek mapowa³ siê na 1.435/4.671 ((0.3464,0.6536)
       //bo siê tekstury podsypki rozje¿d¿aj¹ po zmianie proporcji profilu
       bpts1[0]=vector6(rozp,              -fTexHeight-0.18,        0.00,-0.707,0.707,0.0); //lewy brzeg
       bpts1[1]=vector6((fHTW+side)*cos1,  -(fHTW+side)*sin1-0.18,  0.33,-0.707,0.707,0.0); //krawêdŸ za³amania
       bpts1[2]=vector6(-bpts1[1].x,       +(fHTW+side)*sin1-0.18,  0.67,0.707,0.707,0.0); //prawy brzeg pocz¹tku symetrycznie
       bpts1[3]=vector6(-rozp,             -fTexHeight-0.18,        1.00,0.707,0.707,0.0); //prawy skos
       //koñcowy przekrój
       bpts1[4]=vector6(rozp2,             -fTexHeight2-0.18,       0.00,-0.707,0.707,0.0); //lewy brzeg
       bpts1[5]=vector6((fHTW2+side2)*cos2,-(fHTW2+side2)*sin2-0.18,0.33,-0.707,0.707,0.0); //krawêdŸ za³amania
       bpts1[6]=vector6(-bpts1[5].x,       +(fHTW2+side2)*sin2-0.18,0.67,0.707,0.707,0.0); //prawy brzeg pocz¹tku symetrycznie
       bpts1[7]=vector6(-rozp2,            -fTexHeight2-0.18,       1.00,0.707,0.707,0.0); //prawy skos
      }
      else
      {bpts1[0]=vector6(rozp,      -fTexHeight-0.18,0.0,-0.707,0.707,0.0); //lewy brzeg
       bpts1[1]=vector6(fHTW+side, -0.18,0.33,-0.707,0.707,0.0); //krawêdŸ za³amania
       bpts1[2]=vector6(-fHTW-side,-0.18,0.67,0.707,0.707,0.0); //druga
       bpts1[3]=vector6(-rozp,     -fTexHeight-0.18,1.0,0.707,0.707,0.0); //prawy skos
      }
      Segment->RaRenderLoft(Vert,bpts1,iTrapezoid?-4:4,fTexLength);
     }
     if (TextureID1)
     {// szyny - generujemy dwie, najwy¿ej rysowaæ siê bêdzie jedn¹
      Segment->RaRenderLoft(Vert,rpts1,iTrapezoid?-nnumPts:nnumPts,fTexLength);
      Segment->RaRenderLoft(Vert,rpts2,iTrapezoid?-nnumPts:nnumPts,fTexLength);
     }
     break;
    case tt_Switch: //dla zwrotnicy dwa razy szyny
     if (TextureID1) //Ra: !!!! tu jest do poprawienia
     {//iglice liczone tylko dla zwrotnic
      vector6 rpts3[24],rpts4[24];
      for (i=0;i<12;++i)
      {rpts3[i]   =vector6(+(fHTW+iglica[i].x)*cos1+iglica[i].y*sin1,-(+fHTW+iglica[i].x)*sin1+iglica[i].y*cos1,iglica[i].z);
       rpts3[i+12]=vector6(+(fHTW2+szyna[i].x)*cos2+ szyna[i].y*sin2,-(+fHTW2+szyna[i].x)*sin2+iglica[i].y*cos2, szyna[i].z);
       rpts4[11-i]=vector6((-fHTW-iglica[i].x)*cos1+iglica[i].y*sin1,-(-fHTW-iglica[i].x)*sin1+iglica[i].y*cos1,iglica[i].z);
       rpts4[23-i]=vector6((-fHTW2-szyna[i].x)*cos2+ szyna[i].y*sin2,-(-fHTW2-szyna[i].x)*sin2+iglica[i].y*cos2, szyna[i].z);
      }
      if (SwitchExtension->RightSwitch)
      {//nowa wersja z SPKS, ale odwrotnie lewa/prawa
       SwitchExtension->iLeftVBO=Vert-Start; //indeks lewej iglicy
       SwitchExtension->Segments[0]->RaRenderLoft(Vert,rpts3,-nnumPts,fTexLength,0,2,SwitchExtension->fOffset1);
       SwitchExtension->Segments[0]->RaRenderLoft(Vert,rpts1,nnumPts,fTexLength,2);
       SwitchExtension->Segments[0]->RaRenderLoft(Vert,rpts2,nnumPts,fTexLength);
       SwitchExtension->Segments[1]->RaRenderLoft(Vert,rpts1,nnumPts,fTexLength);
       SwitchExtension->iRightVBO=Vert-Start; //indeks prawej iglicy
       SwitchExtension->Segments[1]->RaRenderLoft(Vert,rpts4,-nnumPts,fTexLength,0,2,-fMaxOffset+SwitchExtension->fOffset1);
       SwitchExtension->Segments[1]->RaRenderLoft(Vert,rpts2,nnumPts,fTexLength,2);
      }
      else
      {//lewa dzia³a lepiej ni¿ prawa
       SwitchExtension->Segments[0]->RaRenderLoft(Vert,rpts1,nnumPts,fTexLength); //lewa szyna normalna ca³a
       SwitchExtension->iLeftVBO=Vert-Start; //indeks lewej iglicy
       SwitchExtension->Segments[0]->RaRenderLoft(Vert,rpts4,-nnumPts,fTexLength,0,2,-SwitchExtension->fOffset1); //prawa iglica
       SwitchExtension->Segments[0]->RaRenderLoft(Vert,rpts2,nnumPts,fTexLength,2); //prawa szyna za iglic¹
       SwitchExtension->iRightVBO=Vert-Start; //indeks prawej iglicy
       SwitchExtension->Segments[1]->RaRenderLoft(Vert,rpts3,-nnumPts,fTexLength,0,2,fMaxOffset-SwitchExtension->fOffset1); //lewa iglica
       SwitchExtension->Segments[1]->RaRenderLoft(Vert,rpts1,nnumPts,fTexLength,2); //lewa szyna za iglic¹
       SwitchExtension->Segments[1]->RaRenderLoft(Vert,rpts2,nnumPts,fTexLength); //prawa szyna normalnie ca³a
      }
     }
     break;
   }
  } //koniec obs³ugi torów
  break;
  case 2: //McZapkie-260302 - droga - rendering
   switch (eType) //dalej zale¿nie od typu
   {
    case tt_Normal: //drogi proste, bo skrzy¿owania osobno
    {vector6 bpts1[4]; //punkty g³ównej p³aszczyzny przydaj¹ siê do robienia boków
     if (TextureID1||TextureID2) //punkty siê przydadz¹, nawet jeœli nawierzchni nie ma
     {//double max=2.0*(fHTW>fHTW2?fHTW:fHTW2); //z szerszej strony jest 100%
      double max=(iCategoryFlag&4)?0.0:fTexLength; //test: szerokoœæ dróg proporcjonalna do d³ugoœci
      double map1=max>0.0?fHTW/max:0.5; //obciêcie tekstury od strony 1
      double map2=max>0.0?fHTW2/max:0.5; //obciêcie tekstury od strony 2
      if (iTrapezoid) //trapez albo przechy³ki
      {//nawierzchnia trapezowata
       Segment->GetRolls(roll1,roll2);
       bpts1[0]=vector6(fHTW*cos(roll1),-fHTW*sin(roll1),0.5-map1); //lewy brzeg pocz¹tku
       bpts1[1]=vector6(-bpts1[0].x,-bpts1[0].y,0.5+map1); //prawy brzeg pocz¹tku symetrycznie
       bpts1[2]=vector6(fHTW2*cos(roll2),-fHTW2*sin(roll2),0.5-map2); //lewy brzeg koñca
       bpts1[3]=vector6(-bpts1[2].x,-bpts1[2].y,0.5+map2); //prawy brzeg pocz¹tku symetrycznie
      }
      else
      {bpts1[0]=vector6( fHTW,0.0,0.5-map1); //zawsze standardowe mapowanie
       bpts1[1]=vector6(-fHTW,0.0,0.5+map1);
      }
     }
     if (TextureID1) //jeœli podana by³a tekstura, generujemy trójk¹ty
     {//tworzenie trójk¹tów nawierzchni szosy
      Segment->RaRenderLoft(Vert,bpts1,iTrapezoid?-2:2,fTexLength);
     }
     if (TextureID2)
     {//pobocze drogi - poziome przy przechy³ce (a mo¿e krawê¿nik i chodnik zrobiæ jak w Midtown Madness 2?)
      //Ra: dorobiæ renderowanie chodnika
      vector6 rpts1[6],rpts2[6]; //wspó³rzêdne przekroju i mapowania dla prawej i lewej strony
      rpts1[0]=vector6(rozp,-fTexHeight,0.0); //lewy brzeg podstawy
      rpts1[1]=vector6(bpts1[0].x+side,bpts1[0].y,0.5), //lewa krawêdŸ za³amania
      rpts1[2]=vector6(bpts1[0].x,bpts1[0].y,1.0); //lewy brzeg pobocza (mapowanie mo¿e byæ inne
      rpts2[0]=vector6(bpts1[1].x,bpts1[1].y,1.0); //prawy brzeg pobocza
      rpts2[1]=vector6(bpts1[1].x-side,bpts1[1].y,0.5); //prawa krawêdŸ za³amania
      rpts2[2]=vector6(-rozp,-fTexHeight,0.0); //prawy brzeg podstawy
      if (iTrapezoid) //trapez albo przechy³ki
      {//pobocza do trapezowatej nawierzchni - dodatkowe punkty z drugiej strony odcinka
       rpts1[3]=vector6(rozp2,-fTexHeight2,0.0); //lewy brzeg lewego pobocza
       rpts1[4]=vector6(bpts1[2].x+side2,bpts1[2].y,0.5); //krawêdŸ za³amania
       rpts1[5]=vector6(bpts1[2].x,bpts1[2].y,1.0); //brzeg pobocza
       rpts2[3]=vector6(bpts1[3].x,bpts1[3].y,1.0);
       rpts2[4]=vector6(bpts1[3].x-side2,bpts1[3].y,0.5);
       rpts2[5]=vector6(-rozp2,-fTexHeight2,0.0); //prawy brzeg prawego pobocza
       Segment->RaRenderLoft(Vert,rpts1,-3,fTexLength);
       Segment->RaRenderLoft(Vert,rpts2,-3,fTexLength);
      }
      else
      {//pobocza zwyk³e, brak przechy³ki
       Segment->RaRenderLoft(Vert,rpts1,3,fTexLength);
       Segment->RaRenderLoft(Vert,rpts2,3,fTexLength);
      }
     }
    }
   }
  break;
  case 4: //Ra: rzeki na razie jak drogi, przechy³ki na pewno nie maj¹
   switch (eType) //dalej zale¿nie od typu
   {
    case tt_Normal: //drogi proste, bo skrzy¿owania osobno
    {vector6 bpts1[4]; //punkty g³ównej p³aszczyzny przydaj¹ siê do robienia boków
     if (TextureID1||TextureID2) //punkty siê przydadz¹, nawet jeœli nawierzchni nie ma
     {//double max=2.0*(fHTW>fHTW2?fHTW:fHTW2); //z szerszej strony jest 100%
      double max=(iCategoryFlag&4)?0.0:fTexLength; //test: szerokoœæ dróg proporcjonalna do d³ugoœci
      double map1=max>0.0?fHTW/max:0.5; //obciêcie tekstury od strony 1
      double map2=max>0.0?fHTW2/max:0.5; //obciêcie tekstury od strony 2
      if (iTrapezoid) //trapez albo przechy³ki
      {//nawierzchnia trapezowata
       Segment->GetRolls(roll1,roll2);
       bpts1[0]=vector6(fHTW*cos(roll1),-fHTW*sin(roll1),0.5-map1); //lewy brzeg pocz¹tku
       bpts1[1]=vector6(-bpts1[0].x,-bpts1[0].y,0.5+map1); //prawy brzeg pocz¹tku symetrycznie
       bpts1[2]=vector6(fHTW2*cos(roll2),-fHTW2*sin(roll2),0.5-map2); //lewy brzeg koñca
       bpts1[3]=vector6(-bpts1[2].x,-bpts1[2].y,0.5+map2); //prawy brzeg pocz¹tku symetrycznie
      }
      else
      {bpts1[0]=vector6( fHTW,0.0,0.5-map1); //zawsze standardowe mapowanie
       bpts1[1]=vector6(-fHTW,0.0,0.5+map1);
      }
     }
     if (TextureID1) //jeœli podana by³a tekstura, generujemy trójk¹ty
     {//tworzenie trójk¹tów nawierzchni szosy
      Segment->RaRenderLoft(Vert,bpts1,iTrapezoid?-2:2,fTexLength);
     }
     if (TextureID2)
     {//pobocze drogi - poziome przy przechy³ce (a mo¿e krawê¿nik i chodnik zrobiæ jak w Midtown Madness 2?)
      vector6 rpts1[6],rpts2[6]; //wspó³rzêdne przekroju i mapowania dla prawej i lewej strony
      rpts1[0]=vector6(rozp,-fTexHeight,0.0); //lewy brzeg podstawy
      rpts1[1]=vector6(bpts1[0].x+side,bpts1[0].y,0.5), //lewa krawêdŸ za³amania
      rpts1[2]=vector6(bpts1[0].x,bpts1[0].y,1.0); //lewy brzeg pobocza (mapowanie mo¿e byæ inne
      rpts2[0]=vector6(bpts1[1].x,bpts1[1].y,1.0); //prawy brzeg pobocza
      rpts2[1]=vector6(bpts1[1].x-side,bpts1[1].y,0.5); //prawa krawêdŸ za³amania
      rpts2[2]=vector6(-rozp,-fTexHeight,0.0); //prawy brzeg podstawy
      if (iTrapezoid) //trapez albo przechy³ki
      {//pobocza do trapezowatej nawierzchni - dodatkowe punkty z drugiej strony odcinka
       rpts1[3]=vector6(rozp2,-fTexHeight2,0.0); //lewy brzeg lewego pobocza
       rpts1[4]=vector6(bpts1[2].x+side2,bpts1[2].y,0.5); //krawêdŸ za³amania
       rpts1[5]=vector6(bpts1[2].x,bpts1[2].y,1.0); //brzeg pobocza
       rpts2[3]=vector6(bpts1[3].x,bpts1[3].y,1.0);
       rpts2[4]=vector6(bpts1[3].x-side2,bpts1[3].y,0.5);
       rpts2[5]=vector6(-rozp2,-fTexHeight2,0.0); //prawy brzeg prawego pobocza
       Segment->RaRenderLoft(Vert,rpts1,-3,fTexLength);
       Segment->RaRenderLoft(Vert,rpts2,-3,fTexLength);
      }
      else
      {//pobocza zwyk³e, brak przechy³ki
       Segment->RaRenderLoft(Vert,rpts1,3,fTexLength);
       Segment->RaRenderLoft(Vert,rpts2,3,fTexLength);
      }
     }
    }
   }
  break;
 }
};

void  __fastcall TTrack::RaRenderVBO(int iPtr)
{//renderowanie z u¿yciem VBO
 EnvironmentSet();
 int seg;
 int i;
 switch (iCategoryFlag&15)
 {
  case 1: //tor
   if (eType==tt_Switch) //dla zwrotnicy tylko szyny
   {if (TextureID1)
     if ((seg=SwitchExtension->Segments[0]->RaSegCount())>0)
     {glBindTexture(GL_TEXTURE_2D,TextureID1); //szyny +
      for (i=0;i<seg;++i)
       glDrawArrays(GL_TRIANGLE_STRIP,iPtr+24*i,24);
      iPtr+=24*seg; //pominiêcie lewej szyny
      for (i=0;i<seg;++i)
       glDrawArrays(GL_TRIANGLE_STRIP,iPtr+24*i,24);
      iPtr+=24*seg; //pominiêcie prawej szyny
     }
    if (TextureID2)
     if ((seg=SwitchExtension->Segments[1]->RaSegCount())>0)
     {glBindTexture(GL_TEXTURE_2D,TextureID2); //szyny -
      for (i=0;i<seg;++i)
       glDrawArrays(GL_TRIANGLE_STRIP,iPtr+24*i,24);
      iPtr+=24*seg; //pominiêcie lewej szyny
      for (i=0;i<seg;++i)
       glDrawArrays(GL_TRIANGLE_STRIP,iPtr+24*i,24);
     }
   }
   else //dla toru podsypka plus szyny
   {if ((seg=Segment->RaSegCount())>0)
    {if (TextureID2)
     {glBindTexture(GL_TEXTURE_2D,TextureID2); //podsypka
      for (i=0;i<seg;++i)
       glDrawArrays(GL_TRIANGLE_STRIP,iPtr+8*i,8);
      iPtr+=8*seg; //pominiêcie podsypki
     }
     if (TextureID1)
     {glBindTexture(GL_TEXTURE_2D,TextureID1); //szyny
      for (i=0;i<seg;++i)
       glDrawArrays(GL_TRIANGLE_STRIP,iPtr+24*i,24);
      iPtr+=24*seg; //pominiêcie lewej szyny
      for (i=0;i<seg;++i)
       glDrawArrays(GL_TRIANGLE_STRIP,iPtr+24*i,24);
     }
    }
   }
   break;
  case 2: //droga
  case 4: //rzeki - jeszcze do przemyœlenia
   if ((seg=Segment->RaSegCount())>0)
   {if (TextureID1)
    {glBindTexture(GL_TEXTURE_2D,TextureID1); //nawierzchnia
     for (i=0;i<seg;++i)
     {glDrawArrays(GL_TRIANGLE_STRIP,iPtr,4); iPtr+=4;}
    }
    if (TextureID2)
    {glBindTexture(GL_TEXTURE_2D,TextureID2); //pobocze
     for (i=0;i<seg;++i)
      glDrawArrays(GL_TRIANGLE_STRIP,iPtr+6*i,6);
     iPtr+=6*seg; //pominiêcie lewego pobocza
     for (i=0;i<seg;++i)
      glDrawArrays(GL_TRIANGLE_STRIP,iPtr+6*i,6);
    }
   }
   break;
 }
 EnvironmentReset();
};

void __fastcall TTrack::EnvironmentSet()
{//ustawienie zmienionego œwiat³a
 glColor3f(1.0f,1.0f,1.0f); //Ra: potrzebne to?
 if (eEnvironment)
 {//McZapkie-310702: zmiana oswietlenia w tunelu, wykopie
  GLfloat ambientLight[4]= {0.5f,0.5f,0.5f,1.0f};
  GLfloat diffuseLight[4]= {0.5f,0.5f,0.5f,1.0f};
  GLfloat specularLight[4]={0.5f,0.5f,0.5f,1.0f};
  switch (eEnvironment)
  {//modyfikacje oœwietlenia zale¿nie od œrodowiska
   case e_canyon:
    for (int li=0;li<3;li++)
    {
     //ambientLight[li]= Global::ambientDayLight[li]*0.8; //0.7
     diffuseLight[li]= Global::diffuseDayLight[li]*0.4;   //0.3
     specularLight[li]=Global::specularDayLight[li]*0.5;  //0.4
    }
    //glLightfv(GL_LIGHT0,GL_AMBIENT,ambientLight);
    glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuseLight);
    glLightfv(GL_LIGHT0,GL_SPECULAR,specularLight);
    break;
   case e_tunnel:
    for (int li=0;li<3;li++)
    {
     ambientLight[li]= Global::ambientDayLight[li]*0.2;
     diffuseLight[li]= Global::diffuseDayLight[li]*0.1;
     specularLight[li]=Global::specularDayLight[li]*0.2;
    }
    glLightfv(GL_LIGHT0,GL_AMBIENT,ambientLight);
    glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuseLight);
    glLightfv(GL_LIGHT0,GL_SPECULAR,specularLight);
    break;
  }
 }
};

void __fastcall TTrack::EnvironmentReset()
{//przywrócenie domyœlnego œwiat³a
 switch (eEnvironment)
 {//przywrócenie globalnych ustawieñ œwiat³a, o ile by³o zmienione
  case e_canyon: //wykop
  case e_tunnel: //tunel
   glLightfv(GL_LIGHT0,GL_AMBIENT,Global::ambientDayLight);
   glLightfv(GL_LIGHT0,GL_DIFFUSE,Global::diffuseDayLight);
   glLightfv(GL_LIGHT0,GL_SPECULAR,Global::specularDayLight);
 }
};

void __fastcall TTrack::RenderDyn()
{//renderowanie nieprzezroczystych pojazdów
 if (!iNumDynamics) return; //po co kombinowaæ, jeœli nie ma pojazdów?
 EnvironmentSet();
 for (int i=0;i<iNumDynamics;i++)
  Dynamics[i]->Render(); //sam sprawdza, czy VBO; zmienia kontekst VBO!
 EnvironmentReset();
};

void __fastcall TTrack::RenderDynAlpha()
{//renderowanie przezroczystych pojazdów
 if (!iNumDynamics) return; //po co kombinowaæ, jeœli nie ma pojazdów?
 EnvironmentSet();
 for (int i=0;i<iNumDynamics;i++)
  Dynamics[i]->RenderAlpha(); //sam sprawdza, czy VBO; zmienia kontekst VBO!
 EnvironmentReset();
};

//---------------------------------------------------------------------------
bool __fastcall TTrack::SetConnections(int i)
{//zapamiêtanie po³¹czen w segmencie
 if (SwitchExtension)
 {
  SwitchExtension->pNexts[NextMask[i]]=pNext;
  SwitchExtension->pPrevs[PrevMask[i]]=pPrev;
  SwitchExtension->iNextDirection[NextMask[i]]=iNextDirection;
  SwitchExtension->iPrevDirection[PrevMask[i]]=iPrevDirection;
  if (eType==tt_Switch)
  {
   SwitchExtension->pPrevs[PrevMask[i+2]]=pPrev;
   SwitchExtension->iPrevDirection[PrevMask[i+2]]=iPrevDirection;
  }
  if (i) Switch(0);
  return true;
 }
 Error("Cannot set connections");
 return false;
}

bool __fastcall TTrack::Switch(int i)
{//prze³¹czenie torów z uruchomieniem animacji
 if (SwitchExtension) //tory prze³¹czalne maj¹ doklejkê
  if (eType==tt_Switch)
  {//przek³adanie zwrotnicy jak zwykle
   i&=1; //ograniczenie b³êdów !!!!
   SwitchExtension->fDesiredOffset1=fMaxOffset*double(NextMask[i]); //od punktu 1
   //SwitchExtension->fDesiredOffset2=fMaxOffset*double(PrevMask[i]); //od punktu 2
   SwitchExtension->CurrentIndex=i;
   Segment=SwitchExtension->Segments[i]; //wybranie aktywnej drogi - potrzebne to?
   pNext=SwitchExtension->pNexts[NextMask[i]]; //prze³¹czenie koñców
   pPrev=SwitchExtension->pPrevs[PrevMask[i]];
   iNextDirection=SwitchExtension->iNextDirection[NextMask[i]];
   iPrevDirection=SwitchExtension->iPrevDirection[PrevMask[i]];
   fRadius=fRadiusTable[i]; //McZapkie: wybor promienia toru
   if (SwitchExtension->pOwner?SwitchExtension->pOwner->RaTrackAnimAdd(this):true) //jeœli nie dodane do animacji
    SwitchExtension->fOffset1=SwitchExtension->fDesiredOffset1; //nie ma siê co bawiæ
   return true;
  }
  else if (eType==tt_Turn)
  {//blokowanie (0, szukanie torów) lub odblokowanie (1, roz³¹czenie) obrotnicy
   if (i)
   {//0: roz³¹czenie s¹siednich torów od obrotnicy
    if (pPrev) //jeœli jest tor od Point1 obrotnicy
     if (iPrevDirection) //0:do³¹czony Point1, 1:do³¹czony Point2
      pPrev->pNext=NULL; //roz³¹czamy od Point2
     else
      pPrev->pPrev=NULL; //roz³¹czamy od Point1
    if (pNext) //jeœli jest tor od Point2 obrotnicy
     if (iPrevDirection) //0:do³¹czony Point1, 1:do³¹czony Point2
      pNext->pNext=NULL; //roz³¹czamy od Point2
     else
      pNext->pPrev=NULL; //roz³¹czamy od Point1
    pNext=pPrev=NULL; //na koñcu roz³¹czamy obrotnicê (wka¿niki do s¹siadów ju¿ niepotrzebne)
    fVelocity=0.0; //AI, nie ruszaj siê!
    if (SwitchExtension->pOwner)
     SwitchExtension->pOwner->RaTrackAnimAdd(this); //dodanie do listy animacyjnej
   }
   else
   {//1: ustalenie finalnego po³o¿enia (gdy nie by³o animacji)
    RaAnimate(); //ostatni etap animowania
    //zablokowanie pozycji i po³¹czenie do s¹siednich torów
    Global::pGround->TrackJoin(SwitchExtension->pMyNode);
    if (pNext||pPrev)
     fVelocity=6.0; //jazda dozwolona
   }
   SwitchExtension->CurrentIndex=i; //zapamiêtanie stanu zablokowania
   return true;
  }
  else if (eType==tt_Cross)
  {//to jest przydatne tylko do ³¹czenia odcinków
   i&=1;
   SwitchExtension->CurrentIndex=i;
   Segment=SwitchExtension->Segments[i]; //wybranie aktywnej drogi - potrzebne to?
   pNext=SwitchExtension->pNexts[NextMask[3*i]]; //prze³¹czenie koñców
   pPrev=SwitchExtension->pPrevs[PrevMask[3*i]];
   iNextDirection=SwitchExtension->iNextDirection[NextMask[3*i]];
   iPrevDirection=SwitchExtension->iPrevDirection[PrevMask[3*i]];
   return true;
  }
 Error("Cannot switch normal track");
 return false;
};

bool __fastcall TTrack::SwitchForced(int i,TDynamicObject *o)
{//rozprucie rozjazdu
 if (SwitchExtension)
  if (i!=SwitchExtension->CurrentIndex)
  {switch (i)
   {case 0:
     if (SwitchExtension->EventPlus)
      Global::pGround->AddToQuery(SwitchExtension->EventPlus,o); //dodanie do kolejki
     break;
    case 1:
     if (SwitchExtension->EventMinus)
      Global::pGround->AddToQuery(SwitchExtension->EventMinus,o); //dodanie do kolejki
     break;
   }
   Switch(i); //jeœli siê tu nie prze³¹czy, to ka¿dy pojazd powtórzy event rozrprucia
  }
 return true;
};

void __fastcall TTrack::RaAnimListAdd(TTrack *t)
{//dodanie toru do listy animacyjnej
 if (SwitchExtension)
 {if (t==this) return; //siebie nie dodajemy drugi raz do listy
  if (!t->SwitchExtension) return; //nie podlega animacji
  if (SwitchExtension->pNextAnim)
  {if (SwitchExtension->pNextAnim==t)
    return; //gdy ju¿ taki jest
   else
    SwitchExtension->pNextAnim->RaAnimListAdd(t);
  }
  else
  {SwitchExtension->pNextAnim=t;
   t->SwitchExtension->pNextAnim=NULL; //nowo dodawany nie mo¿e mieæ ogona
  }
 }
};

TTrack* __fastcall TTrack::RaAnimate()
{//wykonanie rekurencyjne animacji, wywo³ywane przed wyœwietleniem sektora
 //zwraca wskaŸnik toru wymagaj¹cego dalszej animacji
 if (SwitchExtension->pNextAnim)
  SwitchExtension->pNextAnim=SwitchExtension->pNextAnim->RaAnimate();
 bool m=true; //animacja trwa
 if (eType==tt_Switch) //dla zwrotnicy tylko szyny
 {double v=SwitchExtension->fDesiredOffset1-SwitchExtension->fOffset1; //prêdkoœæ
  SwitchExtension->fOffset1+=sign(v)*Timer::GetDeltaTime()*0.1;
  //Ra: trzeba daæ to do klasy...
  if (SwitchExtension->fOffset1<=0.00)
  {SwitchExtension->fOffset1; //1cm?
   m=false; //koniec animacji
  }
  if (SwitchExtension->fOffset1>=fMaxOffset)
  {SwitchExtension->fOffset1=fMaxOffset; //maksimum-1cm?
   m=false; //koniec animacji
  }
  if (Global::bUseVBO)
  {//dla OpenGL 1.4 odœwie¿y siê ca³y sektor, w póŸniejszych poprawiamy fragment
   if (Global::bOpenGL_1_5) //dla OpenGL 1.4 to siê nie wykona poprawnie
    if (TextureID1) //Ra: !!!! tu jest do poprawienia
    {//iglice liczone tylko dla zwrotnic
     vector6 rpts3[24],rpts4[24];
     double fHTW=0.5*fabs(fTrackWidth);
     double fHTW2=fHTW; //Ra: na razie niech tak bêdzie
     double cos1=1.0,sin1=0.0,cos2=1.0,sin2=0.0; //Ra: ...
     for (int i=0;i<12;++i)
     {rpts3[i]   =vector6((fHTW+iglica[i].x)*cos1+iglica[i].y*sin1,-(fHTW+iglica[i].x)*sin1+iglica[i].y*cos1,iglica[i].z);
      rpts3[i+12]=vector6((fHTW2+szyna[i].x)*cos2+szyna[i].y*sin2,-(fHTW2+szyna[i].x)*sin2+iglica[i].y*cos2,szyna[i].z);
      rpts4[11-i]=vector6((-fHTW-iglica[i].x)*cos1+iglica[i].y*sin1,-(-fHTW-iglica[i].x)*sin1+iglica[i].y*cos1,iglica[i].z);
      rpts4[23-i]=vector6((-fHTW2-szyna[i].x)*cos2+szyna[i].y*sin2,-(-fHTW2-szyna[i].x)*sin2+iglica[i].y*cos2,szyna[i].z);
     }
     CVertNormTex Vert[2*2*12]; //na razie 2 segmenty
     CVertNormTex *v=Vert; //bo RaAnimate() modyfikuje wskaŸnik
     glGetBufferSubData(GL_ARRAY_BUFFER,SwitchExtension->iLeftVBO*sizeof(CVertNormTex),2*2*12*sizeof(CVertNormTex),&Vert);//pobranie fragmentu bufora VBO
     if (SwitchExtension->RightSwitch)
     {//nowa wersja z SPKS, ale odwrotnie lewa/prawa
      SwitchExtension->Segments[0]->RaAnimate(v,rpts3,-nnumPts,fTexLength,0,2,SwitchExtension->fOffset1);
      glBufferSubData(GL_ARRAY_BUFFER,SwitchExtension->iLeftVBO*sizeof(CVertNormTex),2*2*12*sizeof(CVertNormTex),&Vert); //wys³anie fragmentu bufora VBO
      v=Vert;
      glGetBufferSubData(GL_ARRAY_BUFFER,SwitchExtension->iRightVBO*sizeof(CVertNormTex),2*2*12*sizeof(CVertNormTex),&Vert);//pobranie fragmentu bufora VBO
      SwitchExtension->Segments[1]->RaAnimate(v,rpts4,-nnumPts,fTexLength,0,2,-fMaxOffset+SwitchExtension->fOffset1);
     }
     else
     {//oryginalnie lewa dzia³a³a lepiej ni¿ prawa
      SwitchExtension->Segments[0]->RaAnimate(v,rpts4,-nnumPts,fTexLength,0,2,-SwitchExtension->fOffset1); //prawa iglica
      glBufferSubData(GL_ARRAY_BUFFER,SwitchExtension->iLeftVBO*sizeof(CVertNormTex),2*2*12*sizeof(CVertNormTex),&Vert);//wys³anie fragmentu bufora VBO
      v=Vert;
      glGetBufferSubData(GL_ARRAY_BUFFER,SwitchExtension->iRightVBO*sizeof(CVertNormTex),2*2*12*sizeof(CVertNormTex),&Vert); //pobranie fragmentu bufora VBO
      SwitchExtension->Segments[1]->RaAnimate(v,rpts3,-nnumPts,fTexLength,0,2,fMaxOffset-SwitchExtension->fOffset1); //lewa iglica
     }
     glBufferSubData(GL_ARRAY_BUFFER,SwitchExtension->iRightVBO*sizeof(CVertNormTex),2*2*12*sizeof(CVertNormTex),&Vert); //wys³anie fragmentu bufora VBO
    }
  }
  else //gdy Display List
   Release(); //niszczenie skompilowanej listy
 }
 else if (eType==tt_Turn) //dla obrotnicy - szyny i podsypka
 {
  if (SwitchExtension->pModel&&SwitchExtension->CurrentIndex) //0=zablokowana siê nie animuje
  {//trzeba ka¿dorazowo porównywaæ z k¹tem modelu
   //SwitchExtension->fOffset1=SwitchExtension->pAnim?SwitchExtension->pAnim->AngleGet():0.0; //pobranie k¹ta z modelu
   TAnimContainer *ac=SwitchExtension->pModel?SwitchExtension->pModel->GetContainer(NULL):NULL; //pobranie g³ównego submodelu
   if (ac?(ac->AngleGet()!=SwitchExtension->fOffset1):false) //czy przemieœci³o siê od ostatniego sprawdzania
   {double hlen=0.5*SwitchExtension->Segments[0]->GetLength(); //po³owa d³ugoœci
    SwitchExtension->fOffset1=180+ac->AngleGet(); //pobranie k¹ta z submodelu
    double sina=hlen*sin(DegToRad(SwitchExtension->fOffset1)),cosa=hlen*cos(DegToRad(SwitchExtension->fOffset1));
    vector3 middle=SwitchExtension->Segments[0]->FastGetPoint(0.5);
    Segment->Init(middle+vector3(sina,0.0,cosa),middle-vector3(sina,0.0,cosa),5.0); //nowy odcinek
    if (Global::bUseVBO)
    {//dla OpenGL 1.4 odœwie¿y siê ca³y sektor, w póŸniejszych poprawiamy fragment
     if (Global::bOpenGL_1_5) //dla OpenGL 1.4 to siê nie wykona poprawnie
     {int size=RaArrayPrepare();
      CVertNormTex *Vert=new CVertNormTex[size]; //bufor roboczy
      //CVertNormTex *v=Vert; //zmieniane przez
      RaArrayFill(Vert,Vert-SwitchExtension->iLeftVBO); //iLeftVBO powinno zostaæ niezmienione
      glBufferSubData(GL_ARRAY_BUFFER,SwitchExtension->iLeftVBO*sizeof(CVertNormTex),size*sizeof(CVertNormTex),Vert); //wys³anie fragmentu bufora VBO
     }
    }
    else //gdy Display List
     Release(); //niszczenie skompilowanej listy, aby siê wygenerowa³a nowa
   } //animacja trwa nadal
  } else m=false; //koniec animacji albo w ogóle nie po³¹czone z modelem
 }
 return m?this:SwitchExtension->pNextAnim; //zwraca obiekt do dalszej animacji
};
//---------------------------------------------------------------------------
void __fastcall TTrack::RadioStop()
{//przekazanie pojazdom rozkazu zatrzymania
 for (int i=0;i<iNumDynamics;i++)
  Dynamics[i]->RadioStop();
};

double __fastcall TTrack::WidthTotal()
{//szerokoœæ z poboczem
 if (iCategoryFlag&2) //jesli droga
  if (fTexHeight>=0.0) //i ma boki zagiête w dó³ (chodnik jest w górê)
   return 2.0*fabs(fTexWidth)+0.5*fabs(fTrackWidth+fTrackWidth2); //dodajemy pobocze
 return 0.5*fabs(fTrackWidth+fTrackWidth2); //a tak tylko zwyk³a œrednia szerokoœæ
};

bool __fastcall TTrack::IsGroupable()
{//czy wyœwietlanie toru mo¿e byæ zgrupwane z innymi
 if ((eType==tt_Switch)||(eType==tt_Turn)) return false; //tory ruchome nie s¹ grupowane
 if ((eEnvironment==e_canyon)||(eEnvironment==e_tunnel)) return false; //tory ze zmian¹ œwiat³a
 return true;
};

bool __fastcall Equal(vector3 v1, vector3 *v2)
{//sprawdzenie odleg³oœci punktów
 //Ra: powinno byæ do 10cm wzd³u¿ toru i ze 2cm w poprzek
 //Ra: z automatycznie dodawanym stukiem, jeœli dziura jest wiêksza ni¿ 2mm.
 if (fabs(v1.x-v2->x)>0.02) return false; //szeœcian zamiast kuli
 if (fabs(v1.z-v2->z)>0.02) return false;
 if (fabs(v1.y-v2->y)>0.02) return false;
 return true;
 //return (SquareMagnitude(v1-*v2)<0.00012); //0.011^2=0.00012
};

int __fastcall TTrack::TestPoint(vector3 *Point)
{//sprawdzanie, czy tory mo¿na po³¹czyæ
 switch (eType)
 {
  case tt_Normal :
   if (pPrev==NULL)
    if (Equal(Segment->FastGetPoint_0(),Point))
     return 0;
   if (pNext==NULL)
    if (Equal(Segment->FastGetPoint_1(),Point))
     return 1;
   break;
  case tt_Switch :
  {int state=GetSwitchState(); //po co?
   //Ra: TODO: jak siê zmieni na bezpoœrednie odwo³ania do segmentow zwrotnicy,
   //to siê wykoleja, poniewa¿ pNext zale¿y od prze³o¿enia
   Switch(0);
   if (pPrev==NULL)
    //if (Equal(SwitchExtension->Segments[0]->FastGetPoint_0(),Point))
    if (Equal(Segment->FastGetPoint_0(),Point))
    {
     Switch(state);
     return 2;
    }
   if (pNext==NULL)
    //if (Equal(SwitchExtension->Segments[0]->FastGetPoint_1(),Point))
    if (Equal(Segment->FastGetPoint_1(),Point))
    {
     Switch(state);
     return 3;
    }
   Switch(1); //mo¿na by siê pozbyæ tego prze³¹czania
   if (pPrev==NULL) //Ra: z tym chyba nie potrzeba ³¹czyæ
    //if (Equal(SwitchExtension->Segments[1]->FastGetPoint_0(),Point))
    if (Equal(Segment->FastGetPoint_0(),Point))
    {
     Switch(state);//Switch(0);
     return 4;
    }
   if (pNext==NULL) //TODO: to zale¿y od prze³o¿enia zwrotnicy
    //if (Equal(SwitchExtension->Segments[1]->FastGetPoint_1(),Point))
    if (Equal(Segment->FastGetPoint_1(),Point))
    {
     Switch(state);//Switch(0);
     return 5;
    }
   Switch(state);
  }
  break;
 }
 return -1;
};

