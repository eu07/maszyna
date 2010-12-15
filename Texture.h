//---------------------------------------------------------------------------

#ifndef TextureH
#define TextureH

#include <map>
#include <string>

#include "opengl/glew.h"
#include "usefull.h"

class TTexturesManager
{
public:
    static void Init();
    static void Free();

    static GLuint GetTextureID(std::string name,int filter=-1);
    static bool GetAlpha( GLuint ID ); //McZapkie-141203: czy tekstura ma polprzeroczystosc

private:
    typedef std::pair<GLuint, bool> AlphaValue;

    typedef std::map<std::string, GLuint> Names;
    typedef std::map<GLuint, bool> Alphas;

    static Names::iterator LoadFromFile(std::string name,int filter=-1);

    static AlphaValue LoadBMP(std::string fileName);
    static AlphaValue LoadTEX(std::string fileName);
    static AlphaValue LoadTGA(std::string fileName,int filter=-1);
    static AlphaValue LoadDDS(std::string fileName);

    static void SetFiltering(int filter);
    static void SetFiltering(bool alpha, bool hash);
    static GLuint CreateTexture(char *buff, int bpp, int width, int Height, bool bHasAlpha, bool bHash, bool bDollar=true,int filter=-1);

    static Names _names;
    static Alphas _alphas;

//    std::list<TTexture> Textures;

};
//---------------------------------------------------------------------------
#endif
