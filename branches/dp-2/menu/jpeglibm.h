#ifndef _JPEGLIBM_H
#define _JPEGLIBM_H

struct IMAGE
{
	int planes;					// pocet rovin
	int sizeX;					// sirka obrazka
	int sizeY;					// vyska obrazka
	unsigned char *data;		// smernik na data
	char isBGR;					// nastavene na 1, ak nacitane data su bgr
	IMAGE(){ planes=0; sizeX=0; sizeY=0; data=NULL; isBGR=0; }
};

IMAGE LoadJPG(const char *filename);

#endif // _JPEGLIBM_H