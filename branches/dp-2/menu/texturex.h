#ifndef _TEXTURE_H
#define _TEXTURE_H

#include "init.h"
#include "jpeglibm.h"
#include "Vector.h"
/*
struct IMAGE
{
	int planes;					// pocet rovin
	int sizeX;					// sirka obrazka
	int sizeY;					// vyska obrazka
	unsigned char *data;		// smernik na data
	char isBGR;					// nastavene na 1, ak nacitane data su bgr
	IMAGE(){ planes=0; sizeX=0; sizeY=0; data=NULL; isBGR=0; }
};*/

class texture  
{
public:
	IMAGE im;
	int load( char* filename, int repeat=0, int filter=3, int compression=0 );
	int loadCube( char* directory, char* extension, int filter=3, int compression=0 );
	int MakeDOT3( char* filename, int repeat=0, int filter=3, int compression=0, float strong=1.f, char *savename=NULL, int only_make=0);
	int MakeSphereMap(char *directory, char *extension, int filter=3, int compression=0);
	int loadDUDV( char *filename, int repeat=0, int filter=1, int compression=0);
	
	void glBindTexture2D(){ glBindTexture( GL_TEXTURE_2D, id); }
	void glBindTextureCube(){ glBindTexture( GL_TEXTURE_CUBE_MAP_ARB, id); }
	
	void setForCopyToTexture( int x, int y, int repeat=0, int filter=3);
	void setCubeForCopyToTexture(int x, int y, int repeat=0, int filter=1);

	int reload( int repeat, int filter){ return load( NULL, repeat, filter); }
	int reloadCube( int filter){ return loadCube( NULL, NULL, filter); }
	int reloadDOT3( int repeat, int filter, float strong){ return MakeDOT3( NULL, repeat, filter, 0, strong); }
	static void setAnisotropicFiltering(int level);

	int imageLoad( char* filename);
	int imageResizeOgl();
	int imageLoadToOgl( int repeat=0, int filter=3, int compression=0);

	static IMAGE LoadBMP( char* filename );
	static void SaveBMP( IMAGE &imag, char* filename);
	static void BGRtoRGB( IMAGE &i);
	static IMAGE LoadBMP1bit(FILE *f, IMAGE &i, BITMAPFILEHEADER &fileheader, BITMAPINFOHEADER &infoheader);
	static IMAGE LoadBMP4bit(FILE *f, IMAGE &i, BITMAPFILEHEADER &fileheader, BITMAPINFOHEADER &infoheader);
	static IMAGE LoadBMP8bit( FILE* f, IMAGE &i, BITMAPFILEHEADER &fileheader, BITMAPINFOHEADER &infoheader);
	static IMAGE LoadJPG( char* filename );
	static IMAGE LoadTGA( char* filename );

	static IMAGE shift(const IMAGE &in);
	static IMAGE invShift(const IMAGE &in);
	static IMAGE resize( const IMAGE &in, int newx, int newy);
	static IMAGE resizeToHalfX(const IMAGE &in);
	static IMAGE resizeToHalfY( const IMAGE &in);
	static IMAGE resizeToHalfXY(const IMAGE &in);
	static void Flip( IMAGE &i);
	static void FlipMirror( IMAGE &i);
      //	static char isPow2(int x){	for( int i=0; x; x>>=1) if(x&1)i++; return (i==1);}

	unsigned int id;
	char *name;
	char *extens;		// iba pre cube map

	texture();
	virtual ~texture();
};

#endif // _TEXTURE_H