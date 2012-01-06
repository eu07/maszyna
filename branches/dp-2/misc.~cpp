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


//#include    "system.hpp"
//#include    "classes.hpp"
#pragma hdrstop

#include "ground.h"
#include "misc.h"
#include "Timer.h"
#include "Usefull.h"
#include "Globals.h"
#include "Texture.h"
#include "camera.h"


#include <gl/gl.h>
#include <gl/glu.h>
#include <GL/glaux.h>
#include "opengl/glext.h"
#include "opengl/glut.h"


#define MAX 256000     // SNOW
#define MAX2 200      // FLASHES

TczastkiS obiekt[MAX];
TczastkiS napis[MAX2];

int clspeed=700;      //szybkosc spadania if FPS>35 clspeed=200 else clspeed=600
float r;

bool camreset = false;
bool swp=1;
long t,t2,t3;
float Z=6;
double cabx, caby, cabz;


double SRX = 0.0;  // SUN ROTATE X
double SRY = 0.0;  // SUN ROTATE Y
double SRZ = 0.0;  // SUN ROTATE Z
double MRX = 0.0;  // MOON ROTATE X
double MRY = 0.0;  // MOON ROTATE Y

double sunrottime = 0;








void TWorld::LosujPozycje(int i, vector3 pos)
{
        obiekt[i].rot = Camera.Yaw*180.0f/M_PI;
	obiekt[i].xp=pos.x + (float)(150-(rand()%200));
	obiekt[i].yp=pos.y + (float)(150-(rand()%200)); // 60-(float)(rand()%460);    //%160 /4
	if (obiekt[i].xp==0 && obiekt[i].yp==0) LosujPozycje(i, pos);
	obiekt[i].zp=pos.z + (float)(175-(rand()%150));                        //gd
	obiekt[i].speed=(float)(rand()%clspeed+1)/100;
	obiekt[i].rgba[0]=(float)(rand()%60)/100+0.3; //(sin(r/100)+1)/2;
	obiekt[i].rgba[1]=(cos(r/150)+1)/2;
	obiekt[i].rgba[2]=(cos(r/100)+1)/2;
	obiekt[i].rgba[3]=(float)(rand()%60)/100+0.3;
}

void LosujPozycje2(int i)
{
		napis[i].xp=sin(i)+(float)(rand()%2)/12;
		napis[i].yp=cos(i)-(float)(rand()%2)/12;
		napis[i].zp=(float)(rand()%40);
		napis[i].speed=(float)(rand()%3+1);
		napis[i].rgba[0]=1;
		napis[i].rgba[1]=(sin(r/100)+1)/2;
		napis[i].rgba[2]=(cos(r/100)+1)/2;
		napis[i].rgba[3]=0.0;
}


void TWorld::UstawPoczatek(vector3 pos)
{
if (Global::iSnowFlakesNum > 256000); Global::iSnowFlakesNum = 256000;

//Global::iSnowFlakesNum  = 13000;
	int i;
	float j;
	srand( (unsigned)time( NULL ) );
	for (i=0; i<Global::iSnowFlakesNum; i++) LosujPozycje(i, pos);
//	for (i=0; i<MAX2; i++) LosujPozycje2(i);
}




void TWorld::RuchObiektow(vector3 pos)
{
	int i;
	for (i=0; i<Global::iSnowFlakesNum; i++)
	{
        obiekt[i].rot += 1.5;
        if (obiekt[i].rot > 360) obiekt[i].rot = 0;
		obiekt[i].yp=obiekt[i].yp-obiekt[i].speed/24;
                //if (obiekt[i].yp<0.3)  obiekt[i].yp = 0.3;
	   	if (obiekt[i].yp<0.3 || obiekt[i].yp>50) LosujPozycje(i, pos);
	}
//	for (i=0; i<MAX2; i++)
//	{
//			napis[i].zp=napis[i].zp+napis[i].speed/4;
//			napis[i].rgba[3]=napis[i].rgba[3]+0.001;
//			if (napis[i].zp>100 || napis[i].zp<1) LosujPozycje2(i);
//	}
}




__fastcall TWorld::RysujBlyski(int tid, int type)
{
	int i;

	glBindTexture(GL_TEXTURE_2D, Global::texturetab[3]);    // SNOW
        glTexEnvf(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);
        glPointSize(0.5);
        glEnable(GL_FOG);

	for (i=0; i<Global::iSnowFlakesNum; i++)
	{
              //glRotatef(obiekt[i].rot,1,1,1);
	   	glColor4f(obiekt[i].rgba[0],obiekt[i].rgba[1],obiekt[i].rgba[2],obiekt[i].rgba[3]);

                
        if (type == 1) // PARTICLES
            {
             glBegin(GL_POINTS);
             glVertex3f(obiekt[i].xp-0.03f, obiekt[i].yp-0.03f, obiekt[i].zp);
             glEnd();
            }

        if (type == 0)  // QUADS
            {


	   	glBegin(GL_POLYGON);
                glTexCoord2f(0,0); glVertex3f(obiekt[i].xp-0.03f,obiekt[i].yp-0.03f,obiekt[i].zp);
                glTexCoord2f(0,1); glVertex3f(obiekt[i].xp-0.03f,obiekt[i].yp+0.03f,obiekt[i].zp);
                glTexCoord2f(1,1); glVertex3f(obiekt[i].xp+0.03f,obiekt[i].yp+0.03f,obiekt[i].zp);
                glTexCoord2f(1,0); glVertex3f(obiekt[i].xp+0.03f,obiekt[i].yp-0.03f,obiekt[i].zp);


	   	glEnd();

            }

	}

        
//	glBindTexture(GL_TEXTURE_2D, texture[8]);   // FLASHES
//	glRotatef(r,0,0,1);
//	for (i=0; i<MAX2; i++)
//	{
//		glColor4f(napis[i].rgba[0],napis[i].rgba[1],napis[i].rgba[2],napis[i].rgba[3]);
//		glBegin(GL_POLYGON);
//			glTexCoord2f(0,0); glVertex3f(napis[i].xp-0.4f,napis[i].yp-0.4f,napis[i].zp);
//			glTexCoord2f(0,1); glVertex3f(napis[i].xp-0.4f,napis[i].yp+0.4f,napis[i].zp);
//			glTexCoord2f(1,1); glVertex3f(napis[i].xp+0.4f,napis[i].yp+0.4f,napis[i].zp);
//			glTexCoord2f(1,0); glVertex3f(napis[i].xp+0.4f,napis[i].yp-0.4f,napis[i].zp);
//		glEnd();
//	}

}




// SNOW ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

__fastcall TWorld::draw_snow(int tid, int type, vector3 pos)
 {
        glBlendFunc(GL_SRC_ALPHA,GL_ONE); glEnable(GL_BLEND);
        glDisable(GL_FOG);
        glTranslatef(0,0,0);

        glPushMatrix();
        RuchObiektow(pos);
        RysujBlyski(0, type);
	glPopMatrix();

        glDisable(GL_BLEND);
        glEnable(GL_FOG);

}



//---------------------------------------------------------------------------

#pragma package(smart_init)

