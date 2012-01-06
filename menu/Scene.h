//////////////////////////////////////////////////////////////////////
// Naprogramoval: Marek Mizanin, mizanin@szm.sk, mizanin@stonline.sk
//                www.mizanin.szm.sk, ICQ: 158283635
//
//
//////////////////////////////////////////////////////////////////////
//	Pouzivane klavesy:
//	
//	H - help
//
//////////////////////////////////////////////////////////////////////
//
// scene.h: interface for the scene class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _SCENE_H
#define _SCENE_H

#include "init.h"				// zakladne nastavenia a zakladne hlavickove subory
								// vazby na init.cpp

#include <stdlib.h>				// standardna kniznica
#include <stdarg.h>				// --
#include <stdio.h>				// --
#include <math.h>				// matematicka kniznica
#include "Timer.h"				// trieda Timer pre pracu z casovacom
#include "bitmap_Font.h"		// trieda pre vypisovanie textu pomocou bitmapoveho fontu
#include "camera.h"				// trieda pre transformacie


#include "Vector.h"
#include "Matrix.h"
//#include "3ds.h"
//#include "ase.h"
#include "texture.h"
#include "Menu.h"
#include "Water.h"
#include "Util.h"

struct z_face{	vec	a,b,c,normal;};		// for Collision Detection

class scene
{
	Font	*font0;
	Timer	timer0;
	camera	*c;
//	C3ds	*model;
//	Case	*base;
	Water	w;
	float	fps;
	uFPS_counter fps_counter;
	float	timeframe;
	float	clip_distance;				// orezavacia vzdialenost
	bool	kurzor;
	int		filterForTexture;
	texture refle,refra;
	texture refle_sp,refra_sp;
	texture skybox[6];
	vector<z_face> collision;
	Menu	*menu[2];
	int		smenu;						// show menu
	struct setting
	{
		int		help, cull_face, print_variable;
		int		separate_specular;
		float	time_delay;
		int		cubemapping;
		int		fresnel_vzorec;
		int		draw_sky_box;
		int		quality_of_refraction;
		int		language_switch;
		struct water_set
		{
			float amplitude;
			float length;
			float drops_per_sec;
			int one_drop;
			float utlm;
			float change_velocity;
			int	reset_water;
			float nr;
			int	division;
			float fresnel;
		}w;
		struct render_set
		{
			int type;
			int normals;
			int lighting;
		}r;
	}set;
private:
	char	text[100];					// text pre konverziu z float na text
	
	POINT MouseBod;
	int mouse_x,mouse_y;
	static int sirka,vyska;
public:
	int CheckCollisionGround( vec center, float radius, float angle, float mindist);
	vec CheckCollision( vec vp, vec vpold, float radius);
	void CubeMapEnd();
	void CubeMapBegin();
	void BuildDisplayLists();
	int GetVariable(char *temp, FILE *sub, char* varname, char* varvalue, int size_strings);
	int ReadInit(char* filename);
	int LoadTextureSkyBox( char *directory, char *extension, texture* tex, int repeat=0, int filter=3, int compression=0 );
	static void DrawQuad( vec p, vec n, vec s, vec t);
	void DrawSkyBox( texture *tex, float size=10);
	void SaveScreen_BMP(int x0=0, int y0=0, int scr_size_x=sirka, int scr_size_y=vyska);
	void Print_vec(int stlpec, int riadok, const vec &a, char* meno=NULL);
	void Print_mat4(int stlpec, int riadok, const mat4 &m, char* nadpis=NULL);
	void Print_int(int stlpec, int riadok, int a, char* meno=NULL);
	void Print_float(int stlpec, int riadok, float a, char* meno=NULL);
	void Paint_axes( float size=2, float r=1, float g=0, float b=0);
	void Paint_vector( const vec &v, float size=1, float r=1, float g=0, float b=0);
	void Paint_point( const vec &v, float size=1, float r=1, float g=1, float b=1);
	void SaveScreen_TGA();
	void ReSizeGLScene( int width=screen_x, int height=screen_y);
	void ReSizeGLScene_2D(int width, int height);	// funkcia pro zmenu velkosti okna
	void ReSizeGLScene_3D(int width, int height);	// funkcia pro zmenu velkosti okna
	void DrawGLScene(void);
	scene();
	~scene();
	void KillGL(void);
	void InitGL(void);
private:
	void Prepni_do_2D(void);
	void Prepni_do_3D(void);
	void Kurzor(void);			// zisti poziciu mysi a zobrazy kurzor
};

#define velkost_kurzoru 5

#endif
