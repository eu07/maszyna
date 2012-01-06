#ifndef _BITMAP_FONT_H
#define _BITMAP_FONT_H

#include "opengl/glew.h"
#include "system.hpp"
#include "classes.hpp"
#include "sysutils.hpp"
#include "init.h"
#include "texturex.h"

class	Font{
	GLuint	base;				// Base Display List For The Font
	GLuint	loop;				// Generic Loop Variable

	GLfloat	cnt1;				// 1st Counter Used To Move Text & For Coloring
	GLfloat	cnt2;				// 2nd Counter Used To Move Text & For Coloring
	
public:
	//texture textur;
	void Print_scale(GLint x, GLint y, char * string, int set,float scale_x,float scale_y);
        void Print_label(GLint x, GLint y, char * string, int set,float scale_x,float scale_y);
	void Print_xy_scale(GLint x, GLint y, char *string, int set,float scale_x,float scale_y);
	void Print_xy_rot(GLint x, GLint y, char *string, int set,float uhol,float scale);
	void Print_xy(GLint x, GLint y, char * string, int set);
	void loadf(char *filename);
	~Font(void);
        Font(void);
	GLvoid glPrint_xy(GLint x, GLint y, char *string, int set);		// pise na OpenGL suradnice x,y
	GLvoid glPrint(GLint x, GLint y, char *string, int set);		// pise na y riadok a x stlpec
	
	// funkcie Begin a End treba pouzivat v paroch !!!!
	void Begin(void);			// nastavi vsetko potrebne pre pisenie pomocou Print
	void Print(GLint x, GLint y, char *string, int set);	// pisanie
	void End(void);				// vypne vsetko co sa zaplo pre pisenie pomocou Print
};

#endif