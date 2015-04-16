/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#ifndef TextureH
#define TextureH

#include <map>
#include <string>

#include "opengl/glew.h"
#include "usefull.h"

/*
//Ra: miejsce umieszczenia tego jest deczko bezsensowne
void glDebug()
{//logowanie b³êdów OpenGL
 GLenum err;
 if (Global::iErorrCounter==326) //tu wpisz o 1 mniej niz wartoœæ, przy której siê wy³o¿y³o
  Global::iErorrCounter=Global::iErorrCounter+0; //do zastawiania pu³apki przed b³êdnym kodem
 while ((err=glGetError())!=GL_NO_ERROR) //dalej jest pu³apka po wykonaniu b³êdnego kodu
  WriteLog("OpenGL error found: "+AnsiString(err)+", step:"+AnsiString(Global::iErorrCounter));
 ++Global::iErorrCounter;
};
*/

class TTexturesManager
{
  public:
    static void Init();
    static void Free();

    static GLuint GetTextureID(char *dir, char *where, std::string name, int filter = -1);
    static bool GetAlpha(GLuint ID); // McZapkie-141203: czy tekstura ma polprzeroczystosc
    static std::string GetName(GLuint id);

  private:
    typedef std::pair<GLuint, bool> AlphaValue;

    typedef std::map<std::string, GLuint> Names;
    typedef std::map<GLuint, bool> Alphas;

    static Names::iterator LoadFromFile(std::string name, int filter = -1);

    static AlphaValue LoadBMP(std::string fileName);
    static AlphaValue LoadTEX(std::string fileName);
    static AlphaValue LoadTGA(std::string fileName, int filter = -1);
    static AlphaValue LoadDDS(std::string fileName, int filter = -1);

    static void SetFiltering(int filter);
    static void SetFiltering(bool alpha, bool hash);
    static GLuint CreateTexture(char *buff, GLint bpp, int width, int height, bool bHasAlpha,
                                bool bHash, bool bDollar = true, int filter = -1);

    static Names _names;
    static Alphas _alphas;
    //    std::list<TTexture> Textures;
};
//---------------------------------------------------------------------------
#endif
