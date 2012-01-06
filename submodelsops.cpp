//---------------------------------------------------------------------------


#pragma hdrstop

#include "system.hpp"
#include "classes.hpp"
#include "submodelsops.h"
#include "model3d.h"
#include "Globals.h"
#include "Texture.h"
#include "timer.h"


#include <stdio.h>

GLuint bbscreen;
GLuint idig01, idig02, idig03, idig04, idig05, idig06, idig07, idig08, idig09, idig10;
AnsiString sdig01, sdig02, sdig03, sdig04, sdig05, sdig06, sdig07, sdig08, sdig09, sdig10;

AnsiString currprzebieg;


double bbtime = 0;
int r = 0;
double dt;
int x = 0;
int m = 0;


double hhdeg;
double mmdeg;
double ssdeg;


void __fastcall TSubModel::setreklam3in1(AnsiString nodename)
{
/*
 dt = Timer::GetDeltaTime();

if (smID > 500 && smID < 524)
    {
     if (rekrot_timepause == false) rekrot3_time += 0.1;                        // CZAS LECI ...

     if (rekrot_step == 1 && rekrot3_time >  30.0) {rekrot_timepause = true; rekrot_step = 2;}      // JEZELI CZAS = 9 TO PAUZA I KROK 2
     if (rekrot_step == 2) rekrot3_rot += 1.0;                                                    // KROK 2 - OBRACANIE
     if (rekrot_step == 2 && rekrot3_rot > 120.0) {rekrot_timepause = false; rekrot_step = 3;}        // JEZELI OSIAGNIE 60* WLACZ CZAS

     if (rekrot_step == 3 && rekrot3_time > 60.0) {rekrot_timepause = true; rekrot_step = 4;}      // JEZELI CZAS = 9 TO PAUZA I KROK 2
     if (rekrot_step == 4) rekrot3_rot += 1.0;
     if (rekrot_step == 4 && rekrot3_rot > 240.0) {rekrot_timepause = false; rekrot_step = 5;}

     if (rekrot_step == 5 && rekrot3_time > 90.0) {rekrot_timepause = true; rekrot_step = 6;}      // JEZELI CZAS = 9 TO PAUZA I KROK 2
     if (rekrot_step == 6) rekrot3_rot += 1.0;
     if (rekrot_step == 6 && rekrot3_rot > 360.0) {rekrot_timepause = false;}

     if (rekrot_step == 6 && rekrot3_rot > 360.0) {rekrot3_rot = 0.0; rekrot_step = 1; rekrot3_time = 0.0;}

     SetRotate(vector3(0,0,1), rekrot3_rot);
    }
 */
}

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// BILLBOARD ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void __fastcall TSubModel::setbillboardtex(AnsiString name, AnsiString nodename, int num)
{
/*
 dt = Timer::GetDeltaTime();
 bbtime += dt;
 TStringList *SL;

      if (name == "bbscreen")   // EKRANOWY BILLBOARD (PODPIAC GRABIENIE KLATEK Z AVI)
          {
           if (!mapsloaded)
           if (FileExists("data\\nodes\\" + nodename + ".txt"))
               {
                SL = new TStringList;
                SL->LoadFromFile("data\\nodes\\" + nodename + ".txt");
                mapsloaded = true;
               }
                       
           if (bbtime > 0000) r = 0;
           if (bbtime > 6000) r = 1;
           if (bbtime > 12000) bbtime = 0;

           if (mapsloaded) bbscreen = TTexturesManager::GetTextureID(SL->Strings[r].c_str());
           else bbscreen = TTexturesManager::GetTextureID("textures\\ip\\noise.bmp");
           
           glBindTexture(GL_TEXTURE_2D, bbscreen);

          }
 */
}

void __fastcall TSubModel::setsecucamtex(AnsiString nodename)
{
/*
if (Global::SECUCAMS_ACTIVE)
    {
      if ( smID == 603 || smID == 604 || smID == 605 || smID == 606)
            {
             if (smID == 603)  glBindTexture(GL_TEXTURE_2D, Global::secucamciew[0]);    // SECUCAM 1
             if (smID == 604)  glBindTexture(GL_TEXTURE_2D, Global::secucamciew[1]);    // SECUCAM 2
             if (smID == 605)  glBindTexture(GL_TEXTURE_2D, Global::secucamciew[2]);    // SECUCAM 3
             if (smID == 606)  glBindTexture(GL_TEXTURE_2D, Global::secucamciew[3]);    // SECUCAM 4
            }
   }
 */      
}

void __fastcall TSubModel::setmirrorstex(AnsiString nodename)
{

//if ((Global::MIRROR_R_ACTIVE == FALSE) && (name == "mirror_r")) glBindTexture(GL_TEXTURE_2D, Global::mirrorO);   // GDY LUSTERKO PRAWE WYLACZONE
//if ((Global::MIRROR_L_ACTIVE == FALSE) && (name == "mirror_l")) glBindTexture(GL_TEXTURE_2D, Global::mirrorO);   // GDY LUSTERKO LEWE WYLACZONE

/*
   if (Global::MIRROR_R_ACTIVE || Global::MIRROR_L_ACTIVE)
     {

      if ( smID == 601 || smID == 602 || smID == 607 || smID == 608 ||
           smID == 603 || smID == 604 || smID == 605 || smID == 606)
            {
             if ((Global::MIRROR_R_ACTIVE) && (smID == 601)) glBindTexture(GL_TEXTURE_2D, Global::mirrorR);
             if ((Global::MIRROR_L_ACTIVE) && (smID == 602)) glBindTexture(GL_TEXTURE_2D, Global::mirrorL);

             //if (smID == 607) glBindTexture(GL_TEXTURE_2D, Global::mirrorA);
             //if (smID == 608)  glBindTexture(GL_TEXTURE_2D, Global::mirrorA);     // COMP SCREEN

            }
     }
*/     
}

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// NUMER ROZJAZDU NA ZWROTNIKU ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void __fastcall TSubModel::setswitchnumber(AnsiString nodename)
{
      if (smID == 404)
          {
           sdig01 = AnsiString("data\\zwr\\" + nodename.SubString(9,2) + ".bmp");
           idig01 = TTexturesManager::GetTextureID(sdig01.c_str());
           glBindTexture(GL_TEXTURE_2D, idig01);
          }

}


// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// USTAWIANIE PRZEBYTEGO DYSTANSU NA HASLERZE ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void __fastcall TSubModel::sethaslerdistance()
{
//      if (smID == 901) glBindTexture(GL_TEXTURE_2D, Global::ddigit01);
//      if (smID == 902) glBindTexture(GL_TEXTURE_2D, Global::ddigit02);
//      if (smID == 903) glBindTexture(GL_TEXTURE_2D, Global::ddigit03);
//      if (smID == 904) glBindTexture(GL_TEXTURE_2D, Global::ddigit04);
//      if (smID == 905) glBindTexture(GL_TEXTURE_2D, Global::ddigit05);
//      if (smID == 906) glBindTexture(GL_TEXTURE_2D, Global::ddigit06);
}


void __fastcall TSubModel::setclockanalog()
{
/*
      if (smID == 401)  // HH ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
          {
          if (clk_showhh)
              {
               hhdeg = -30.0 * GlobalTime->hh;
               SetRotate(vector3(0,0,1),hhdeg);
              }
           }

      if (smID == 402) // MM ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
          {
          if (clk_showmm)
              {
               mmdeg = -6.00 * GlobalTime->mm;
               SetRotate(vector3(0,0,1),mmdeg);
              }
            }


      if (smID == 403) // SS ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
          {
          if (clk_showss)
              {
               ssdeg = -6.00 * GlobalTime->ss;
               SetRotate(vector3(0,0,1),ssdeg);
              }
            }

*/
}

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// ZEGAR DIODOWY (SEGMENTOWY) ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void __fastcall TSubModel::setclockdigital()
{

      if (smID == 801) { glBindTexture(GL_TEXTURE_2D, Global::cdigit01); }
      if (smID == 802) { glBindTexture(GL_TEXTURE_2D, Global::cdigit02); }
      if (smID == 803) { glBindTexture(GL_TEXTURE_2D, Global::cdigit03); }
      if (smID == 804) { glBindTexture(GL_TEXTURE_2D, Global::cdigit04); }
      if (smID == 805) { glBindTexture(GL_TEXTURE_2D, Global::cdigit05); }
      if (smID == 806) { glBindTexture(GL_TEXTURE_2D, Global::cdigit06); }
      if (smID == 807) { glBindTexture(GL_TEXTURE_2D, Global::cdigit07); }
      if (smID == 808) { glBindTexture(GL_TEXTURE_2D, Global::cdigit08); }
}

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// TABLICA KIERUNKOWA 1 2x13 ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void __fastcall TSubModel::setdirecttable1()
{
      if (smID == 1001) { glBindTexture(GL_TEXTURE_2D, Global::dichar01); }
      if (smID == 1002) { glBindTexture(GL_TEXTURE_2D, Global::dichar02); }
      if (smID == 1003) { glBindTexture(GL_TEXTURE_2D, Global::dichar03); }
      if (smID == 1004) { glBindTexture(GL_TEXTURE_2D, Global::dichar04); }
      if (smID == 1005) { glBindTexture(GL_TEXTURE_2D, Global::dichar05); }
      if (smID == 1006) { glBindTexture(GL_TEXTURE_2D, Global::dichar06); }
      if (smID == 1007) { glBindTexture(GL_TEXTURE_2D, Global::dichar07); }
      if (smID == 1008) { glBindTexture(GL_TEXTURE_2D, Global::dichar08); }
      if (smID == 1009) { glBindTexture(GL_TEXTURE_2D, Global::dichar09); }
      if (smID == 1010) { glBindTexture(GL_TEXTURE_2D, Global::dichar10); }
      if (smID == 1011) { glBindTexture(GL_TEXTURE_2D, Global::dichar11); }
      if (smID == 1012) { glBindTexture(GL_TEXTURE_2D, Global::dichar12); }
      if (smID == 1013) { glBindTexture(GL_TEXTURE_2D, Global::dichar13); }
      if (smID == 1014) { glBindTexture(GL_TEXTURE_2D, Global::dichar14); }
      if (smID == 1015) { glBindTexture(GL_TEXTURE_2D, Global::dichar15); }
      if (smID == 1016) { glBindTexture(GL_TEXTURE_2D, Global::dichar16); }
      if (smID == 1017) { glBindTexture(GL_TEXTURE_2D, Global::dichar17); }
      if (smID == 1018) { glBindTexture(GL_TEXTURE_2D, Global::dichar18); }
      if (smID == 1019) { glBindTexture(GL_TEXTURE_2D, Global::dichar19); }
      if (smID == 1020) { glBindTexture(GL_TEXTURE_2D, Global::dichar20); }
      if (smID == 1021) { glBindTexture(GL_TEXTURE_2D, Global::dichar21); }
      if (smID == 1022) { glBindTexture(GL_TEXTURE_2D, Global::dichar22); }
      if (smID == 1023) { glBindTexture(GL_TEXTURE_2D, Global::dichar23); }
      if (smID == 1024) { glBindTexture(GL_TEXTURE_2D, Global::dichar24); }
      if (smID == 1025) { glBindTexture(GL_TEXTURE_2D, Global::dichar25); }
      if (smID == 1026) { glBindTexture(GL_TEXTURE_2D, Global::dichar26); }
      if (smID == 1027) { glBindTexture(GL_TEXTURE_2D, Global::dichar27); }
      if (smID == 1028) { glBindTexture(GL_TEXTURE_2D, Global::dichar28); }
      if (smID == 1029) { glBindTexture(GL_TEXTURE_2D, Global::dichar29); }
      if (smID == 1030) { glBindTexture(GL_TEXTURE_2D, Global::dichar30); }
      if (smID == 1031) { glBindTexture(GL_TEXTURE_2D, Global::dichar31); }
      if (smID == 1032) { glBindTexture(GL_TEXTURE_2D, Global::dichar32); }
}


void __fastcall TSubModel::setvechcomputer(AnsiString name, AnsiString nodename, int num)
{
//      if (smID == 301) { glBindTexture(GL_TEXTURE_2D, Global::tacho1_digit01); }
//      if (smID == 302) { glBindTexture(GL_TEXTURE_2D, Global::tacho1_digit02); }
//      if (smID == 303) { glBindTexture(GL_TEXTURE_2D, Global::tacho1_digit03); }
//      if (smID == 304) { glBindTexture(GL_TEXTURE_2D, Global::tacho1_digit04); }
//      if (smID == 305) { glBindTexture(GL_TEXTURE_2D, Global::tacho1_digit05); }
//      if (smID == 306) { glBindTexture(GL_TEXTURE_2D, Global::tacho1_digit06); }
//      if (smID == 307) { glBindTexture(GL_TEXTURE_2D, Global::tacho1_digit07); }

//      if (smID == 311) { glBindTexture(GL_TEXTURE_2D, Global::volto1_digit01); }
//      if (smID == 312) { glBindTexture(GL_TEXTURE_2D, Global::volto1_digit02); }
//      if (smID == 313) { glBindTexture(GL_TEXTURE_2D, Global::volto1_digit03); }
//      if (smID == 314) { glBindTexture(GL_TEXTURE_2D, Global::volto1_digit04); }
//      if (smID == 315) { glBindTexture(GL_TEXTURE_2D, Global::volto1_digit05); }

//      if (smID == 321) { glBindTexture(GL_TEXTURE_2D, Global::vcurr1_digit01); }
//      if (smID == 322) { glBindTexture(GL_TEXTURE_2D, Global::vcurr1_digit02); }
//      if (smID == 323) { glBindTexture(GL_TEXTURE_2D, Global::vcurr1_digit03); }
//      if (smID == 324) { glBindTexture(GL_TEXTURE_2D, Global::vcurr1_digit04); }
//      if (smID == 325) { glBindTexture(GL_TEXTURE_2D, Global::vcurr1_digit05); }

//      if (smID == 331) { glBindTexture(GL_TEXTURE_2D, Global::brakep_digit01); }
//      if (smID == 332) { glBindTexture(GL_TEXTURE_2D, Global::brakep_digit02); }
//      if (smID == 333) { glBindTexture(GL_TEXTURE_2D, Global::brakep_digit03); }
//      if (smID == 334) { glBindTexture(GL_TEXTURE_2D, Global::brakep_digit04); }
//      if (smID == 335) { glBindTexture(GL_TEXTURE_2D, Global::brakep_digit05); }
/*
      if (name == "pp1") { glBindTexture(GL_TEXTURE_2D, Global::bpipep_digit01); }
      if (name == "pp2") { glBindTexture(GL_TEXTURE_2D, Global::bpipep_digit02); }
      if (name == "pp3") { glBindTexture(GL_TEXTURE_2D, Global::bpipep_digit03); }
      if (name == "pp4") { glBindTexture(GL_TEXTURE_2D, Global::bpipep_digit04); }
      if (name == "pp5") { glBindTexture(GL_TEXTURE_2D, Global::bpipep_digit05); }

      if (name == "cp1") { glBindTexture(GL_TEXTURE_2D, Global::comprp_digit01); }
      if (name == "cp2") { glBindTexture(GL_TEXTURE_2D, Global::comprp_digit02); }
      if (name == "cp3") { glBindTexture(GL_TEXTURE_2D, Global::comprp_digit03); }
      if (name == "cp4") { glBindTexture(GL_TEXTURE_2D, Global::comprp_digit04); }
      if (name == "cp5") { glBindTexture(GL_TEXTURE_2D, Global::comprp_digit05); }
*/
}


// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// WAGA ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void __fastcall TSubModel::getvechicleweight(AnsiString name, AnsiString nodename, int num)
{
AnsiString tmp;
int x;



}
//---------------------------------------------------------------------------

#pragma package(smart_init)
