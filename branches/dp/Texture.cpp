//---------------------------------------------------------------------------

/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak and others

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

#include <iostream>
#include <fstream>
#include "opengl/glew.h"
#include <ddraw>

#include "system.hpp"
#include "classes.hpp"
#include "stdio.h"
#pragma hdrstop

#include "Usefull.h"
#include "Texture.h"
#include "TextureDDS.h"

#include "logs.h"
#include "Globals.h"

TTexturesManager::Alphas TTexturesManager::_alphas;
TTexturesManager::Names TTexturesManager::_names;

void TTexturesManager::Init()
{
};

TTexturesManager::Names::iterator TTexturesManager::LoadFromFile(std::string fileName,int filter)
{

    std::string message("Loading - texture: ");
    message += fileName;

    std::string realFileName(fileName);
    std::ifstream file(fileName.c_str());
    if(!file.is_open())
        realFileName.insert(0, szDefaultTexturePath);
    else
        file.close();

    char* cFileName = const_cast<char*>(fileName.c_str());

    WriteLog(cFileName);

    size_t pos = fileName.rfind('.');
    std::string ext(fileName, pos + 1, std::string::npos);

    AlphaValue texinfo;

    if (ext == "tga")
        texinfo = LoadTGA(realFileName,filter);
    else if (ext == "tex")
        texinfo = LoadTEX(realFileName);
    else if (ext == "bmp")
        texinfo = LoadBMP(realFileName);
    else if (ext == "dds")
        texinfo = LoadDDS(realFileName);

    _alphas.insert(texinfo);
    std::pair<Names::iterator, bool> ret = _names.insert(std::make_pair(fileName, texinfo.first));

    if(!texinfo.first)
    {
        WriteLog("Failed");
        return _names.end();
    };

    _alphas.insert(texinfo);
    ret = _names.insert(std::make_pair(fileName, texinfo.first));

    WriteLog("OK");
    return ret.first;

};

struct ReplaceSlash
{
    const char operator()(const char input)
    {
        return input == '/' ? '\\' : input;
    }
};

GLuint TTexturesManager::GetTextureID(std::string fileName,int filter)
{

    std::transform(fileName.begin(), fileName.end(), fileName.begin(), ReplaceSlash());

    // jesli biezaca sciezka do tekstur nie zostala dodana to dodajemy defaultowa
    if (fileName.find('\\') == std::string::npos)
        fileName.insert(0, szDefaultTexturePath);

    if (fileName.find('.') == std::string::npos)
    {//Ra: wypr�bowanie rozszerze� po kolei, zaczynaj�c od szDefaultExt
     fileName.append(".");
     std::string test;
     for (int i=0;i<4;++i)
     {test=fileName;
      test.append(Global::szDefaultExt[i]);
      std::ifstream file(test.c_str());
      if (!file.is_open())
      {test.insert(0,szDefaultTexturePath);
       file.open(test.c_str());
      }
      if (file.is_open())
      {
       fileName.append(Global::szDefaultExt[i]); //dopisanie znalezionego
       file.close();
      }
     }
    };

    Names::iterator iter = _names.find(fileName);

    if (iter == _names.end())
        iter = LoadFromFile(fileName,filter);

    return (iter != _names.end() ? iter->second : 0);

}

bool TTexturesManager::GetAlpha(GLuint id)
{

    Alphas::iterator iter = _alphas.find(id);
    return (iter != _alphas.end() ? iter->second : false);

}

TTexturesManager::AlphaValue TTexturesManager::LoadBMP(std::string fileName)
{

    AlphaValue fail(0, false);
    std::ifstream file(fileName.c_str(), std::ios::binary);

    if(file.eof())
    {
        file.close();
        return fail;
    };

    BITMAPFILEHEADER header;
    size_t bytes;

    file.read((char*) &header, sizeof(BITMAPFILEHEADER));
    if(file.eof())
    {
        file.close();
        return fail;
    }

    // Read in bitmap information structure
    BITMAPINFO info;
    long infoSize = header.bfOffBits - sizeof(BITMAPFILEHEADER);
    file.read((char*) &info, infoSize);

    if(file.eof())
    {
        file.close();
        return fail;
    };

    GLuint width = info.bmiHeader.biWidth;
    GLuint height = info.bmiHeader.biHeight;

    unsigned long bitSize = info.bmiHeader.biSizeImage;
    if(!bitSize)
        bitSize = (width * info.bmiHeader.biBitCount + 7) / 8 * height;

    GLubyte* data = new GLubyte[bitSize];
    file.read((char*) data, bitSize);

    if(file.eof())
    {
        delete[] data;
        file.close();
        return fail;
    };

    file.close();

    GLuint id;

    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // This is specific to the binary format of the data read in.
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);

    glTexImage2D(GL_TEXTURE_2D, 0, 3, width, height, 0,
                 GL_BGR_EXT, GL_UNSIGNED_BYTE, data);

    delete[] data;
    return std::make_pair(id, false);

};

TTexturesManager::AlphaValue TTexturesManager::LoadTGA(std::string fileName,int filter)
{
 AlphaValue fail(0,false);
 //GLubyte TGAheader[]={0,0,2,0,0,0,0,0,0,0,0,0};	// Uncompressed TGA Header
 GLubyte TGACompheader[]={0,0,10,0,0,0,0,0,0,0,0,0}; // Uncompressed TGA Header
 GLubyte TGAcompare[12]; // Used To Compare TGA Header
 GLubyte header[6]; // First 6 Useful Bytes From The Header
 std::ifstream file(fileName.c_str(),std::ios::binary);
 file.read((char*)TGAcompare,sizeof(TGAcompare));
 file.read((char*)header,sizeof(header));
 std::cout << file.tellg() << std::endl;
 if (file.eof())
 {
  file.close();
  return fail;
 };
 bool compressed=(memcmp(TGACompheader,TGAcompare,sizeof(TGACompheader))==0);
 GLint width =header[1]*256+header[0]; // Determine The TGA width (highbyte*256+lowbyte)
 GLint height=header[3]*256+header[2]; // Determine The TGA height (highbyte*256+lowbyte)
 // check if width, height and bpp is correct
 if ( !width || !height || (header[4]!=24 && header[4]!=32))
 {
  file.close();
  return fail;
 };
 GLuint bpp=header[4];	// Grab The TGA's Bits Per Pixel (24 or 32)
 GLuint bytesPerPixel=bpp/8; // Divide By 8 To Get The Bytes Per Pixel
 GLuint imageSize=width*height*bytesPerPixel; // Calculate The Memory Required For The TGA Data
 GLubyte *imageData=new GLubyte[imageSize]; // Reserve Memory To Hold The TGA Data
 if (!compressed)
 {
  file.read(imageData,imageSize);
  if (file.eof())
  {
   delete[] imageData;
   file.close();
   return fail;
  };
/* Ra: nie potrzeba tego robi�, mo�na zamieni� przy tworzeniu tekstury
  // Swap R and B components
  GLuint temp;
  for (GLuint i=0;i<imageSize;i+=bytesPerPixel)
  {
   temp          =imageData[i];
   imageData[i]  =imageData[i+2];
   imageData[i+2]=temp;
  };
*/
 }
 else
 {//compressed TGA
  GLuint pixelcount=height*width; // Nuber of pixels in the image
  GLuint currentpixel=0; // Current pixel being read
  GLuint currentbyte=0; // Current byte
  GLubyte *colorbuffer=new GLubyte[bytesPerPixel]; // Storage for 1 pixel
  int chunkheader=0; //storage for "chunk" header //Ra: b�dziemy wczytywa� najm�odszy bajt
  while (currentpixel<pixelcount)
  {
   file.read((char*)&chunkheader,1); //jeden bajt, pozosta�e zawsze zerowe
   if (file.eof())
   {
    MessageBox(NULL,"Could not read RLE header","ERROR",MB_OK); // Display Error
    delete[] imageData;
    file.close();
    return fail;
   };
   if (chunkheader<128)
   {// If the header is < 128, it means the that is the number of RAW color packets minus 1
    chunkheader++; // add 1 to get number of following color values
    /* Ra: nie potrzeba zamienia�, mo�na da� informacj� przy tworzeniu tekstury
    for (int counter=0;counter<chunkheader;counter++) // Read RAW color values
    {
     file.read(colorbuffer,bytesPerPixel);
     // Flip R and B vcolor values around in the process
     imageData[currentbyte]  =colorbuffer[2];
     imageData[currentbyte+1]=colorbuffer[1];
     imageData[currentbyte+2]=colorbuffer[0];
     if (bytesPerPixel==4)	// if its a 32 bpp image
      imageData[currentbyte+3]=colorbuffer[3];// copy the 4th byte
     currentbyte+=bytesPerPixel;
     currentpixel++;
    }
    */
    file.read(imageData+currentbyte,chunkheader*bytesPerPixel);
    currentbyte+=chunkheader*bytesPerPixel;
   }
   else
   {// chunkheader > 128 RLE data, next color reapeated chunkheader - 127 times
    chunkheader-=127;
    file.read(colorbuffer,bytesPerPixel);
    // copy the color into the image data as many times as dictated
    if (bytesPerPixel==4)
    {//przy czterech bajtach powinno by� szybsze u�ywanie int
     __int32 *ptr=(__int32*)(imageData+currentbyte),bgra=*((__int32*)colorbuffer);
     for (int counter=0;counter<chunkheader;counter++)
      *ptr++=bgra;
     currentbyte+=chunkheader*bytesPerPixel;
    }
    else
     for (int counter=0;counter<chunkheader;counter++)
     {																			// by the header
      memcpy(imageData+currentbyte,colorbuffer,bytesPerPixel);
      /*
      imageData[currentbyte  ]=colorbuffer[0];
      imageData[currentbyte+1]=colorbuffer[1];
      imageData[currentbyte+2]=colorbuffer[2];
      if (bytesPerPixel==4)												// If TGA images is 32 bpp
       imageData[currentbyte+3]=colorbuffer[3];// Copy 4th byte
      */
      currentbyte+=bytesPerPixel;
     }
   }
   currentpixel+=chunkheader;
  };
 };
 file.close();
 bool alpha = (bpp == 32);
 bool hash = (fileName.find('#') != std::string::npos); //true gdy w nazwie jest "#"
 bool dollar = (fileName.find('$') == std::string::npos); //true gdy w nazwie nie ma "$"
 size_t pos=fileName.rfind('%'); //ostatni % w nazwie
 if (pos!=std::string::npos)
  if (pos<fileName.size())
  {filter=(int)fileName[pos+1]-'0'; //zamiana cyfry za % na liczb�
   if ((filter<0)||(filter>10)) filter=-1; //je�li nie jest cyfr�
  }
 if (!alpha&&!hash&&dollar&&(filter<0))
  filter=Global::iDefaultFiltering; //dotyczy tekstur TGA bez kana�u alfa
 //ewentualne przeskalowanie tekstury do dopuszczalnego rozumiaru
 GLint w=width,h=height;
 if (width>Global::iMaxTextureSize) width=Global::iMaxTextureSize; //ogranizczenie wielko�ci
 if (height>Global::iMaxTextureSize) height=Global::iMaxTextureSize;
 if ((w!=width)||(h!=height))
 {//przeskalowanie tekstury, �eby si� nie wy�wietla�a jako bia�a
  GLubyte* imgData=new GLubyte[width*height*bytesPerPixel]; //nowy rozmiar
  gluScaleImage(bytesPerPixel==3?GL_RGB:GL_RGBA,w,h,GL_UNSIGNED_BYTE,imageData,width,height,GL_UNSIGNED_BYTE,imgData);
  delete imageData; //usuni�cie starego
  imageData=imgData;
 }
 GLuint id=CreateTexture(imageData,(alpha?GL_BGRA:GL_BGR),width,height,alpha,hash,dollar,filter);
 delete[] imageData;
 return std::make_pair(id,alpha);
};

TTexturesManager::AlphaValue TTexturesManager::LoadTEX(std::string fileName)
{

    AlphaValue fail(0, false);

    std::ifstream file(fileName.c_str(), ios::binary);

    char head[5];
    file.read(head, 4);
    head[4] = 0;

    bool alpha;

    if(std::string("RGB ") == head)
    {
        alpha = false;
    }
    else if(std::string("RGBA") == head)
    {
        alpha = true;
    }
    else
    {
        std::string message("Unrecognized texture format: ");
        message += head;
        Error(message.c_str());
        return fail;
    };

    GLuint width;
    GLuint height;

    file.read((char *) &width, sizeof(int));
    file.read((char *) &height, sizeof(int));

    GLuint bpp = alpha ? 4 : 3;
    GLuint size = width * height * bpp;

    GLubyte* data = new GLubyte[size];
    file.read(data, size);

    bool hash = (fileName.find('#') != std::string::npos);

    GLuint id = CreateTexture(data,(alpha?GL_RGBA:GL_RGB),width,height,alpha,hash);
    delete[] data;

    return std::make_pair(id, alpha);

};

TTexturesManager::AlphaValue TTexturesManager::LoadDDS(std::string fileName,int filter)
{

    AlphaValue fail(0, false);

    std::ifstream file(fileName.c_str(), ios::binary);

    char filecode[5];
    file.read(filecode, 4);
    filecode[4] = 0;

    if(std::string("DDS ") != filecode)
    {
        file.close();
        return fail;
    };

    DDSURFACEDESC2 ddsd;
    file.read((char*) &ddsd, sizeof(ddsd));

    DDS_IMAGE_DATA data;

    //
    // This .dds loader supports the loading of compressed formats DXT1, DXT3
    // and DXT5.
    //

    GLuint factor;

    switch( ddsd.ddpfPixelFormat.dwFourCC )
    {
        case FOURCC_DXT1:
            // DXT1's compression ratio is 8:1
            data.format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            factor = 2;
            break;

        case FOURCC_DXT3:
            // DXT3's compression ratio is 4:1
            data.format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            factor = 4;
            break;

        case FOURCC_DXT5:
            // DXT5's compression ratio is 4:1
            data.format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            factor = 4;
            break;

        default:
            file.close();
            return fail;
    }

    GLuint bufferSize = (ddsd.dwMipMapCount > 1 ? ddsd.dwLinearSize * factor : ddsd.dwLinearSize);

    data.pixels = new GLubyte[bufferSize];
    file.read((char*)data.pixels,bufferSize);

    file.close();

    data.width      = ddsd.dwWidth;
    data.height     = ddsd.dwHeight;
    data.numMipMaps = ddsd.dwMipMapCount;

    if (ddsd.ddpfPixelFormat.dwFourCC == FOURCC_DXT1)
        data.components = 3;
    else
        data.components = 4;

    data.blockSize = (data.format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ? 8 : 16);

    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    SetFiltering(true, fileName.find('#') != std::string::npos);

    GLuint offset = 0;
    int firstMipMap = 0;
    
    while(data.width > Global::iMaxTextureSize || data.height > Global::iMaxTextureSize)
    {
        offset += ((data.width + 3) / 4) * ((data.height+3)/4) * data.blockSize;
        data.width /= 2;
        data.height /= 2;
        firstMipMap++;
    };

    // Load the mip-map levels
    for (int i=0; i < data.numMipMaps - firstMipMap; i++)
    {
        if (!data.width) data.width = 1;
        if (!data.height) data.height = 1;

        GLuint size = ((data.width + 3) / 4) * ((data.height+3)/4) * data.blockSize;

        if ((Global::bDecompressDDS)&&(i==1))  //should be i==0 but then problem with "glBindTexture()"
        {
            GLuint decomp_size = data.width * data.height * 4;
            GLubyte* output = new GLubyte[decomp_size];
            DecompressDXT(data, data.pixels + offset, output);

            glTexImage2D(GL_TEXTURE_2D, i, GL_RGBA, data.width, data.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, output);

            delete[] output;
        }
        else
        {
            glCompressedTexImage2D(
                GL_TEXTURE_2D,
                i,
                data.format,
                data.width,
                data.height,
                0,
                size,
                data.pixels + offset
            );

        }

        offset += size;

        // Half the image size for the next mip-map level...
        data.width /= 2;
        data.height /= 2;
    };

    delete[] data.pixels;
    return std::make_pair(id, data.components == 4);

};

void TTexturesManager::SetFiltering(int filter)
{
 if (filter<4) //rozmycie przy powi�kszeniu
 {//brak rozmycia z bliska - tych jest 4: 0..3, aby nie by�o przeskoku
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  filter+=4;
 }
 else
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
 switch (filter) //rozmycie przy oddaleniu
 {case 4: //najbli�szy z tekstury
   glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST); break;
  case 5: //�rednia z tekstury
   glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); break;
  case 6: //najbli�szy z mipmapy
   glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST_MIPMAP_NEAREST); break;
  case 7: //�rednia z mipmapy
   glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST); break;
  case 8: //najbli�szy z dw�ch mipmap
   glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST_MIPMAP_LINEAR); break;
  case 9: //�rednia z dw�ch mipmap
   glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR); break;
 }
};

void TTexturesManager::SetFiltering(bool alpha, bool hash)
{

    if ( alpha || hash )
    {
      if (alpha) // przezroczystosc: nie wlaczac mipmapingu
       {
         if (hash) // #: calkowity brak filtracji - pikseloza
          {
           glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, GL_NEAREST);
           glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, GL_NEAREST);
          }
         else
          {
           glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, GL_LINEAR);
           glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, GL_LINEAR);
          }
       }
      else  // filtruj ale bez dalekich mipmap - robi artefakty
       {
         glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, GL_LINEAR);
         glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, GL_LINEAR);
       }
     }
    else // $: filtruj wszystko - brzydko si� zlewa
     {
       glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, GL_LINEAR);
       glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
     }

};

///////////////////////////////////////////////////////////////////////////////
GLuint TTexturesManager::CreateTexture(char* buff,GLint bpp,int width,int height,bool bHasAlpha,bool bHash,bool bDollar,int filter)
{//Ra: u�ywane tylko dla TGA i TEX
 //Ra: doda� obs�ug� GL_BGR oraz GL_BGRA dla TGA - b�dzie si� szybciej wczytywa�
 GLuint ID;
 glGenTextures(1,&ID);
 glBindTexture(GL_TEXTURE_2D,ID);
 glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
 glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
 if (filter>=0)
  SetFiltering(filter); //cyfra po % w nazwie
 else
  SetFiltering(bHasAlpha&&bDollar,bHash); //znaki #, $ i kana� alfa w nazwie
 glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
 glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
 glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
 glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
 if (bHasAlpha || bHash || (filter==0))
  glTexImage2D(GL_TEXTURE_2D,0,(bHasAlpha?GL_RGBA:GL_RGB),width,height,0,bpp,GL_UNSIGNED_BYTE,buff);
 else
  gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGB,width,height,bpp,GL_UNSIGNED_BYTE,buff);
 return ID;
}

void TTexturesManager::Free()
{

    for(Names::iterator iter = _names.begin(); iter != _names.end(); iter++)
        glDeleteTextures(1, &(iter->second));

}

#pragma package(smart_init)

