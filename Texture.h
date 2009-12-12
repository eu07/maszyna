//---------------------------------------------------------------------------

#ifndef TextureH
#define TextureH

#include <gl/glu.h>
#include "usefull.h"

class TTexture
{
public:
    __fastcall TTexture() { ID=0; Name= NULL; };
//    __fastcall TTexture(TCHAR* szFileName, TTexture *NNext ) {  ID=0; LoadBMP(szFileName); Next= NNext; };
    __fastcall TTexture(char* szFileName, TTexture *NNext );
    __fastcall ~TTexture() { glDeleteTextures(1,&ID); SafeDeleteArray(Name); };
    bool inline __fastcall Allocated() { return(ID>0); };
    bool __fastcall LoadTargaFile( char* strPathname );
    bool __fastcall LoadBMP(char* szFileName);
    bool __fastcall LoadTEX(char* szFileName);
    GLuint ID;
    char *Name;
    TTexture *Next;
    bool HasAlpha;
    bool HasHash;
private:
//    int Width,Height,BPP;
};

class TTexturesManager
{
public:
    __fastcall TTexturesManager() { Init(); };
    __fastcall ~TTexturesManager();
    static __fastcall Free();
//    __fastcall Init( AnsiString NTexDir ) { TexDir= NTexDir; };

    static void __fastcall Init() { First= NULL; };
    static GLuint __fastcall GetTextureID( char* Name );
    static bool __fastcall GetAlpha( GLuint ID ); //McZapkie-141203: czy tekstura ma polprzeroczystosc 
//    static bool __fastcall GetHash( GLuint ID );
private:
    static GLuint __fastcall LoadFromFile( char* FileName );
    static TTexture *First;
//    std::list<TTexture> Textures;

};
//---------------------------------------------------------------------------
#endif
