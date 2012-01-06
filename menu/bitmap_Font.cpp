#include "bitmap_font.h"
#include "Globals.h"
#include "Texture.h"

int _screen_x_font;
int _screen_y_font;
//GLuint Global::bfonttex;

Font::~Font(GLvoid)										// Delete The Font From Memory
{
	glDeleteLists(base,256);							// Delete All 256 Display Lists
}


Font::Font(GLvoid)
{

}


GLvoid Font::glPrint_xy(GLint x, GLint y, char *string, int set)	// Where The Printing Happens
{
if (!Global::SCNLOADED) _screen_x_font = Global::iWindowWidth;
if (!Global::SCNLOADED) _screen_y_font = Global::iWindowHeight;

if (Global::SCNLOADED) _screen_x_font = 1280;
if (Global::SCNLOADED) _screen_y_font = 1024;

	if (set>1)set=1;
	glBlendFunc(GL_SRC_ALPHA			,	GL_ONE_MINUS_SRC_COLOR);	// nastavenie miešania farieb
//farba bodu = (pôvodná farba * ALPHA)	+	(kreslená farba - pôvodná farba)
	glEnable(GL_BLEND);									// povolenie miešania farieb

	glBindTexture(GL_TEXTURE_2D, Global::bfonttex);		// Select Our Font Texture
	glDisable(GL_DEPTH_TEST);							// Disables Depth Testing
	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glPushMatrix();										// Store The Projection Matrix
	glLoadIdentity();									// Reset The Projection Matrix
	glOrtho(0,_screen_x_font,0,_screen_y_font,-1,1);				// Set Up An Ortho Screen
	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glPushMatrix();										// Store The Modelview Matrix
	glLoadIdentity();									// Reset The Modelview Matrix
	glTranslated(x,y,0);								// Position The Text (0,0 - Bottom Left)
	glListBase(base-32+(128*set));						// Choose The Font Set (0 or 1)
	glCallLists(strlen(string),GL_BYTE,string);			// Write The Text To The Screen
	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glPopMatrix();										// Restore The Old Projection Matrix
	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glPopMatrix();										// Restore The Old Projection Matrix
	glEnable(GL_DEPTH_TEST);							// Enables Depth Testing

	glDisable(GL_BLEND);								//vypnutie miešania farieb

}

GLvoid Font::glPrint(GLint x, GLint y, char *string, int set)	// Where The Printing Happens
{
if (!Global::SCNLOADED) _screen_x_font = Global::iWindowWidth;
if (!Global::SCNLOADED) _screen_y_font = Global::iWindowHeight;

if (Global::SCNLOADED) _screen_x_font = 1280;
if (Global::SCNLOADED) _screen_y_font = 1024;

	x = x*11;											// vypocet x zo suradnice stlpca
	y = screen_y_font - (y+1)*16;							// vypocet y zo suradnice riadka

	if (set>1)set=1;
	glBlendFunc(GL_SRC_ALPHA			,	GL_ONE_MINUS_SRC_COLOR);	// nastavenie miešania farieb
//farba bodu = (pôvodná farba * ALPHA)	+	(kreslená farba - pôvodná farba)
	glEnable(GL_BLEND);									// povolenie miešania farieb

	glBindTexture(GL_TEXTURE_2D, Global::bfonttex);		// Select Our Font Texture
	glDisable(GL_DEPTH_TEST);							// Disables Depth Testing
	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glPushMatrix();										// Store The Projection Matrix
	glLoadIdentity();									// Reset The Projection Matrix
	glOrtho(0,_screen_x_font,0,_screen_y_font,-1,1);				// Set Up An Ortho Screen
	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glPushMatrix();										// Store The Modelview Matrix
	glLoadIdentity();									// Reset The Modelview Matrix
	glTranslatef((float)x,(float)y,0);					// Position The Text (0,0 - Bottom Left)
	glListBase(base-32+(128*set));						// Choose The Font Set (0 or 1)
	glCallLists(strlen(string),GL_BYTE,string);			// Write The Text To The Screen
	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glPopMatrix();										// Restore The Old Projection Matrix
	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glPopMatrix();										// Restore The Old Projection Matrix
	glEnable(GL_DEPTH_TEST);							// Enables Depth Testing

	glDisable(GL_BLEND);								//vypnutie miešania farieb

}


void Font::loadf(char *filename)				// Build Our Font Display List
{
	float	cx;											// Holds Our X Character Coord
	float	cy;											// Holds Our Y Character Coord
	base = 0;


        //Global::bfonttex = TTexturesManager::GetTextureID(filename);

	base=glGenLists(256);								// Creating 256 Display Lists
	glBindTexture(GL_TEXTURE_2D, Global::bfonttex);			// Select Our Font Texture
	for (loop=0; loop<256; loop++)						// Loop Through All 256 Lists
	{
		cx=float(loop%16)/16.0f;						// X Position Of Current Character
		cy=float(loop/16)/16.0f;						// Y Position Of Current Character

		glNewList(base+loop,GL_COMPILE);				// Start Building A List
			glBegin(GL_QUADS);							// Use A Quad For Each Character
				glTexCoord2f(cx,1.0f-cy-0.0625f);		// Texture Coord (Bottom Left)
				glVertex2i(0,0);						// Vertex Coord (Bottom Left)
				glTexCoord2f(cx+0.0625f,1.0f-cy-0.0625f);	// Texture Coord (Bottom Right)
				glVertex2i(16,0);						// Vertex Coord (Bottom Right)
				glTexCoord2f(cx+0.0625f,1.0f-cy);		// Texture Coord (Top Right)
				glVertex2i(16,16);						// Vertex Coord (Top Right)
				glTexCoord2f(cx,1.0f-cy);				// Texture Coord (Top Left)
				glVertex2i(0,16);						// Vertex Coord (Top Left)
			glEnd();									// Done Building Our Quad (Character)
			glTranslatef(11,0,0);	//10,0,0			// Move To The Right Of The Character
		glEndList();									// Done Building The Display List
	}													// Loop Until All 256 Are Built
}


void Font::Begin()
{
if (!Global::SCNLOADED) _screen_x_font = Global::iWindowWidth;
if (!Global::SCNLOADED) _screen_y_font = Global::iWindowHeight;

if (Global::SCNLOADED) _screen_x_font = 1280;
if (Global::SCNLOADED) _screen_y_font = 1024;

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);	// nastavenie miešania farieb
//farba bodu = (pôvodná farba * ALPHA)	+	(kreslená farba - pôvodná farba)
	glEnable(GL_BLEND);
	glBindTexture(GL_TEXTURE_2D, Global::bfonttex);		// Select Our Font Texture
	glDisable(GL_DEPTH_TEST);							// Disables Depth Testing
	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glPushMatrix();										// Store The Projection Matrix
	glLoadIdentity();									// Reset The Projection Matrix
	glOrtho(0,_screen_x_font,0,_screen_y_font,-1,1);				// Set Up An Ortho Screen
        //glOrtho(0, Global::iWindowWidth, Global::iWindowHeight, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glPushMatrix();										// Store The Modelview Matrix
}

void Font::End()
{
	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glPopMatrix();										// Restore The Old Projection Matrix
	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glPopMatrix();										// Restore The Old Projection Matrix
	glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
       	glDisable(GL_BLEND);								// vypnutie miešania farieb
}

void Font::Print(GLint x, GLint y, char * string, int set)
{
	x = x*12;											// vypocet x zo suradnice stlpca
	y = screen_y_font - (y+1)*16;							// vypocet y zo suradnice riadka

	if (set>1)set=1;
	glLoadIdentity();									// Reset The Modelview Matrix
	glTranslatef((float)x,(float)y,0);					// Position The Text (0,0 - Bottom Left)
	glListBase(base-32+(128*set));						// Choose The Font Set (0 or 1)
	glCallLists(strlen(string),GL_BYTE,string);			// Write The Text To The Screen
}

void Font::Print_scale(GLint x, GLint y, char * string, int set,float scale_x,float scale_y)
{
	x = x*11;											// vypocet x zo suradnice stlpca
	y = screen_y_font - (y+1)*16;							// vypocet y zo suradnice riadka

	if (set>1)set=1;
	glLoadIdentity();									// Reset The Modelview Matrix
	glTranslatef((float)x,(float)y,0);					// Position The Text (0,0 - Bottom Left)
	glScalef(scale_x,scale_y,1.0f);
	glListBase(base-32+(128*set));						// Choose The Font Set (0 or 1)
	glCallLists(strlen(string),GL_BYTE,string);			// Write The Text To The Screen
}

void Font::Print_label(GLint x, GLint y, char * string, int set,float scale_x,float scale_y)
{

	if (set>1)set=1;
	//glLoadIdentity();									// Reset The Modelview Matrix
	glTranslatef((float)x,(float)y,0);					// Position The Text (0,0 - Bottom Left)
	glScalef(scale_x,scale_y,1.0f);
	glListBase(base-32+(128*set));						// Choose The Font Set (0 or 1)
	glCallLists(strlen(string),GL_BYTE,string);			// Write The Text To The Screen
}


void Font::Print_xy(GLint x, GLint y, char *string, int set)
{
	if (set>1)set=1;
	glLoadIdentity();									// Reset The Modelview Matrix
	glTranslatef(x,y,0);					// Position The Text (0,0 - Bottom Left)
	glListBase(base-32+(128*set));						// Choose The Font Set (0 or 1)
	glCallLists(strlen(string),GL_BYTE,string);			// Write The Text To The Screen
}

void Font::Print_xy_scale(GLint x, GLint y, char *string, int set,float scale_x,float scale_y)
{
	if (set>1)set=1;
	glLoadIdentity();									// Reset The Modelview Matrix
	glTranslatef((float)x,(float)y,0);					// Position The Text (0,0 - Bottom Left)
	glScalef(scale_x,scale_y,1.0f);
	glListBase(base-32+(128*set));						// Choose The Font Set (0 or 1)
	glCallLists(strlen(string),GL_BYTE,string);			// Write The Text To The Screen
}

void Font::Print_xy_rot(GLint x, GLint y, char *string, int set,float uhol,float scale)
{
	if (set>1)set=1;
	glLoadIdentity();									// Reset The Modelview Matrix
	glTranslatef((float)x,(float)y,0);					// Position The Text (0,0 - Bottom Left)
	glRotatef(uhol,0,0,1);
	glScalef(scale,1.0f,1.0f);
	glListBase(base-32+(128*set));						// Choose The Font Set (0 or 1)
	glCallLists(strlen(string),GL_BYTE,string);			// Write The Text To The Screen
}














