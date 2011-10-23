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
#include "io.h"

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

    if (ext=="tga")
     texinfo=LoadTGA(realFileName,filter);
    else if (ext=="tex")
     texinfo=LoadTEX(realFileName);
    else if (ext=="bmp")
     texinfo=LoadBMP(realFileName);
    else if (ext=="dds")
     texinfo=LoadDDS(realFileName,filter);

    _alphas.insert(texinfo); //zapamiêtanie stanu przezroczystoœci tekstury - mo¿na by tylko przezroczyste
    std::pair<Names::iterator, bool> ret = _names.insert(std::make_pair(fileName, texinfo.first));

    if(!texinfo.first)
    {
        WriteLog("Failed");
        return _names.end();
    };

    _alphas.insert(texinfo);
    ret=_names.insert(std::make_pair(fileName,texinfo.first)); //dodanie tekstury do magazynu (spisu nazw)

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
{//ustalenie numeru tekstury, wczytanie jeœli nie jeszcze takiej nie by³o
 size_t pos=fileName.find(':'); //szukamy dwukropka
 if (pos!=std::string::npos) //po dwukropku mog¹ byæ podane dodatkowe informacje
  fileName=fileName.substr(0,pos); //niebêd¹ce nazw¹ tekstury
 std::transform(fileName.begin(),fileName.end(),fileName.begin(),ReplaceSlash());
 //jeœli bie¿aca œcie¿ka do tekstur nie zosta³a dodana to dodajemy domyœln¹
 if (fileName.find('\\')==std::string::npos)
  fileName.insert(0,szDefaultTexturePath);
 Names::iterator iter;
 if (fileName.find('.')==std::string::npos)
 {//Ra: wypróbowanie rozszerzeñ po kolei, zaczynaj¹c od domyœlnego
  fileName.append("."); //kropka bêdze na pewno, resztê trzeba próbowaæ
  std::string test; //zmienna robocza
  for (int i=0;i<4;++i)
  {//najpierw szukamy w magazynie
   test=fileName;
   test.append(Global::szDefaultExt[i]);
   iter=_names.find(fileName); //czy mamy ju¿ w magazynie?
   if (iter!=_names.end())
    return iter->second; //znalezione!
   test.insert(0,szDefaultTexturePath); //jeszcze próba z dodatkow¹ œcie¿k¹
   iter=_names.find(fileName); //czy mamy ju¿ w magazynie?
   if (iter!=_names.end())
    return iter->second; //znalezione!
  }
  for (int i=0;i<4;++i)
  {//w magazynie nie ma, to sprawdzamy na dysku
   test=fileName;
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
    break; //wyjœcie z pêtli na etapie danego rozszerzenia
   }
  }
 }
 iter=_names.find(fileName); //czy mamy ju¿ w magazynie
 if (iter==_names.end())
  iter=LoadFromFile(fileName,filter);
 return (iter!=_names.end()?iter->second:0);
}

bool TTexturesManager::GetAlpha(GLuint id)
{//atrybut przezroczystoœci dla tekstury o podanym numerze (id)
 Alphas::iterator iter=_alphas.find(id);
 return (iter!=_alphas.end()?iter->second:false);
}

TTexturesManager::AlphaValue TTexturesManager::LoadBMP(std::string fileName)
{

    AlphaValue fail(0, false);
    std::ifstream file(fileName.c_str(), std::ios::binary);

    if (!file.is_open())
    {
        //file.close();
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
 int writeback=-1; //-1 plik jest OK, >=0 - od którego bajtu zapisaæ poprawiony plik
 GLubyte TGACompheader[]={0,0,10,0,0,0,0,0,0,0,0,0}; //uncompressed TGA header
 GLubyte TGAcompare[12]; //used to compare TGA header
 GLubyte header[6]; //first 6 useful bytes from the header
 std::fstream file(fileName.c_str(),std::ios::binary|std::ios::in);
 file.read((char*)TGAcompare,sizeof(TGAcompare));
 file.read((char*)header,sizeof(header));
 //std::cout << file.tellg() << std::endl;
 if (file.eof())
 {
  file.close();
  return fail;
 };
 bool compressed=(memcmp(TGACompheader,TGAcompare,sizeof(TGACompheader))==0);
 GLint width =header[1]*256+header[0]; //determine the TGA width (highbyte*256+lowbyte)
 GLint height=header[3]*256+header[2]; //determine the TGA height (highbyte*256+lowbyte)
 // check if width, height and bpp is correct
 if ( !width || !height || (header[4]!=24 && header[4]!=32))
 {
  file.close();
  return fail;
 };
 GLuint bpp=header[4];	//grab the TGA's bits per pixel (24 or 32)
 GLuint bytesPerPixel=bpp/8; // divide by 8 to get the bytes per pixel
 GLuint imageSize=width*height*bytesPerPixel; //calculate the memory required for the TGA data
 GLubyte *imageData=new GLubyte[imageSize]; //reserve memory to hold the TGA data
 if (!compressed)
 {//WriteLog("Not compressed.");
  file.read(imageData,imageSize);
  if (file.eof())
  {
   delete[] imageData;
   file.close();
   return fail;
  };
 }
 else
 {//skompresowany plik TGA
  GLuint filesize; //current byte
  GLuint colorbuffer[1]; // Storage for 1 pixel
  file.seekg(0,ios::end); //na koniec
  filesize=(int)file.tellg()-18; //rozmiar bez nag³ówka
  file.seekg(18,ios::beg); //ponownie za nag³ówkiem
  GLubyte *copyto=imageData; //gdzie wstawiaæ w buforze
  GLubyte *copyend=imageData+imageSize; //za ostatnim bajtem bufora
  GLubyte *copyfrom=imageData+imageSize-filesize; //gdzie jest pocz¹tek
  int chunkheader=0; //Ra: bêdziemy wczytywaæ najm³odszy bajt
  if (filesize<imageSize) //jeœli po kompresji jest mniejszy ni¿ przed
  {//Ra: nowe wczytywanie skompresowanych: czytamy ca³e od razu, dekompresja w pamiêci
   GLuint copybytes;
   file.read(copyfrom,filesize); //wczytanie reszty po nag³ówku
   //najpierw trzeba ustaliæ, ile skopiowanych pikseli jest na samym koñcu
   copyto=copyfrom; //roboczo przelatujemy wczytane dane
   copybytes=0; //licznik bajtów obrazka
   while (copybytes<imageSize)
   {
    chunkheader=(unsigned char)*copyto; //jeden bajt, pozosta³e zawsze zerowe
    copyto+=1+bytesPerPixel; //bajt licznika oraz jeden piksel jest zawsze
    copybytes+=(1+(chunkheader&127))*bytesPerPixel; //iloœæ pikseli
    if (chunkheader<128) //jeœli kopiowanie, pikseli jest wiêcej
     copyto+=(chunkheader)*bytesPerPixel; //rozmiar kopiowanego obszaru (bez jednego piksela)
   }
   if (copybytes>imageSize)
   {//nie ma prawa byæ wiêksze
    WriteLog("Compression error");
    delete[] imageData;
    file.close();
    return fail;
   }
   //na koñcu mog¹ byæ œmieci
   int extraend=copyend-copyto; //d³ugoœæ œmieci na koñcu
   if (extraend>0)
   {//przesuwamy bufor do koñca obszaru dekompresji
    WriteLog("Extra bytes: "+AnsiString(extraend));
    memmove(copyfrom+extraend,copyfrom,filesize-extraend);
    copyfrom+=extraend;
    file.close();
    filesize-=extraend; //to chyba nie ma znaczenia
    if (Global::iModifyTGA&2) //flaga obcinania œmieci
    {//najlepiej by by³o obci¹æ plik, ale fstream tego nie potrafi
     int handle;
     for (unsigned int i=0;i<fileName.length();++i)
      if (fileName[i]=='/')
       fileName[i]=='\\'; //bo to Windows
     WriteLog("Truncating extra bytes");
     handle=open(fileName.c_str(),O_RDWR|O_BINARY);
     chsize(handle,18+filesize); //obciêcie œmieci
     close(handle);
     extraend=0; //skoro obciêty, to siê ju¿ nie liczy
    }
    file.open(fileName.c_str(),std::ios::binary|std::ios::in);
   }
   if (chunkheader<128) //jeœli ostatnie piksele s¹ kopiowane
    copyend-=(1+chunkheader)*bytesPerPixel; //bajty kopiowane na koñcu nie podlegaj¹ce dekompresji
   else
    copyend-=bytesPerPixel; //ostatni piksel i tak siê nie zmieni
   copyto=imageData; //teraz bêdzie wype³nianie od pocz¹tku obszaru
   while (copyto<copyend)
   {
    if (copyto>=copyfrom)
    {WriteLog("Decompression problem at pixel "+AnsiString((copyto-imageData)/bytesPerPixel)+"+"+AnsiString((copyend-copyto)/bytesPerPixel));
     writeback=imageData+imageSize+extraend-copyfrom; //ile bajtów zosta³o do koñca
     //tê pozycjê w pliku trzeba by zapamietaæ i po wczytaniu reszty pikseli star¹ metod¹
     //zapisaæ od niej dane od (copyfrom), poprzedzone bajtem o wartoœci ((copyend-copyto)/bytesPerPixel-1)
     file.seekg(-writeback,ios::end); //odleg³oœæ od koñca (ujemna)
     //if (writeback<128) //mo¿liwe tylko jeœli iloœæ problematycznych pikseli nie przekaracza 128
     writeback=-1; //na razie wy³¹czone
     break; //bufor siê zatka³, dalej w ten sposób siê nie da
    }
    chunkheader=(unsigned char)*copyfrom; //jeden bajt, pozosta³e zawsze zerowe
    if (chunkheader<128)
    {//if the header is < 128, it means the that is the number of RAW color packets minus 1
     copybytes=(++chunkheader)*bytesPerPixel; //rozmiar kopiowanego obszaru
     memcpy(copyto,++copyfrom,copybytes); //skopiowanie tylu bajtów
     copyto+=copybytes;
     copyfrom+=copybytes;
    }
    else
    {//chunkheader > 128 RLE data, next color reapeated chunkheader - 127 times
     chunkheader-=127;
     //copy the color into the image data as many times as dictated
     if (bytesPerPixel==4)
     {//przy czterech bajtach powinno byæ szybsze u¿ywanie int
      __int32 *ptr=(__int32*)(copyto); //wskaŸnik na int
      __int32 bgra=*((__int32*)++copyfrom); //kolor wype³niaj¹cy (4 bajty)
      for (int counter=0;counter<chunkheader;counter++)
       *ptr++=bgra;
      copyto=(char*)(ptr); //rzutowanie, ¿eby nie dodawaæ
      copyfrom+=4;
     }
     else
     {colorbuffer[0]=*((int*)(++copyfrom)); //pobranie koloru (3 bajty)
      for (int counter=0;counter<chunkheader;counter++)
      {																			// by the header
       memcpy(copyto,colorbuffer,3);
       copyto+=3;
      }
      copyfrom+=3;
     }
    }
   } //while (copyto<copyend)
  }
  else
  {WriteLog("Compressed file is larger than uncompressed!");
   if (Global::iModifyTGA&1)
    writeback=0; //no zapisaæ ten krótszy zaczynajac od pocz¹tku...
  }
  if (copyto<copyend) WriteLog("Slow loader...");
  while (copyto<copyend)
  {//Ra: stare wczytywanie skompresowanych, z nadu¿ywaniem file.read()
   //równie¿ wykonywane, jeœli dekompresja w buforze przekroczy jego rozmiar
   file.read((char*)&chunkheader,1); //jeden bajt, pozosta³e zawsze zerowe
   if (file.eof())
   {
    MessageBox(NULL,"Could not read RLE header","ERROR",MB_OK); // Display Error
    delete[] imageData;
    file.close();
    return fail;
   };
   if (chunkheader<128)
   {//if the header is < 128, it means the that is the number of RAW color packets minus 1
    chunkheader++; // add 1 to get number of following color values
    file.read(copyto,chunkheader*bytesPerPixel);
    copyto+=chunkheader*bytesPerPixel;
   }
   else
   {//chunkheader>128 RLE data, next color reapeated (chunkheader-127) times
    chunkheader-=127;
    file.read((char*)colorbuffer,bytesPerPixel);
    // copy the color into the image data as many times as dictated
    if (bytesPerPixel==4)
    {//przy czterech bajtach powinno byæ szybsze u¿ywanie int
     __int32 *ptr=(__int32*)(copyto),bgra=*((__int32*)colorbuffer);
     for (int counter=0;counter<chunkheader;counter++)
      *ptr++=bgra;
     copyto=(char*)(ptr); //rzutowanie, ¿eby nie dodawaæ
     //filesize+=chunkheader*bytesPerPixel;
    }
    else
     for (int counter=0;counter<chunkheader;counter++)
     {																			// by the header
      memcpy(copyto,colorbuffer,bytesPerPixel);
      copyto+=bytesPerPixel;
     }
   }
  } //while (copyto<copyend)
  if (writeback>=0)
  {//zapisanie pliku
   WriteLog("Writing uncompressed file...");
   TGAcompare[2]=2; //bez kompresji
   file.close(); //tamten zamykamy i robimy nowy
   if (writeback)
   {//zapisanie samej koñcówki pliku, która utrudnia dekompresjê w buforze
    //file.open(fileName.c_str(),std::ios::binary|std::ios::out|std::ios::in);
    file.seekg(-writeback,ios::end); //odleg³oœæ od koñca (ujemna)
    file.write(copyfrom,imageSize);
   }
   else
   {//zapisywanie ca³oœci pliku, bêdzie krótszy, wiêc trzeba usun¹æ go w ca³oœci
    file.open(fileName.c_str(),std::ios::binary|std::ios::out|std::ios::trunc);
    file.write((char*)TGAcompare,sizeof(TGAcompare));
    file.write((char*)header,sizeof(header));
    file.write(imageData,imageSize);
   }
  }
 };
 file.close(); //plik zamykamy dopiero na samym koñcu
 bool alpha = (bpp == 32);
 bool hash = (fileName.find('#') != std::string::npos); //true gdy w nazwie jest "#"
 bool dollar = (fileName.find('$') == std::string::npos); //true gdy w nazwie nie ma "$"
 size_t pos=fileName.rfind('%'); //ostatni % w nazwie
 if (pos!=std::string::npos)
  if (pos<fileName.size())
  {filter=(int)fileName[pos+1]-'0'; //zamiana cyfry za % na liczbê
   if ((filter<0)||(filter>10)) filter=-1; //jeœli nie jest cyfr¹
  }
 if (!alpha&&!hash&&dollar&&(filter<0))
  filter=Global::iDefaultFiltering; //dotyczy tekstur TGA bez kana³u alfa
 //ewentualne przeskalowanie tekstury do dopuszczalnego rozumiaru
 GLint w=width,h=height;
 if (width>Global::iMaxTextureSize) width=Global::iMaxTextureSize; //ogranizczenie wielkoœci
 if (height>Global::iMaxTextureSize) height=Global::iMaxTextureSize;
 if ((w!=width)||(h!=height))
 {//przeskalowanie tekstury, ¿eby siê nie wyœwietla³a jako bia³a
  GLubyte* imgData=new GLubyte[width*height*bytesPerPixel]; //nowy rozmiar
  gluScaleImage(bytesPerPixel==3?GL_RGB:GL_RGBA,w,h,GL_UNSIGNED_BYTE,imageData,width,height,GL_UNSIGNED_BYTE,imgData);
  delete imageData; //usuniêcie starego
  imageData=imgData;
 }
 GLuint id=CreateTexture(imageData,(alpha?GL_BGRA:GL_BGR),width,height,alpha,hash,dollar,filter);
 delete[] imageData;
 ++Global::iTextures;
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
    if (filter>=0)
     SetFiltering(filter); //cyfra po % w nazwie
    else
     //SetFiltering(bHasAlpha&&bDollar,bHash); //znaki #, $ i kana³ alfa w nazwie
     SetFiltering(data.components==4,fileName.find('#')!=std::string::npos);

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
 if (filter<4) //rozmycie przy powiêkszeniu
 {//brak rozmycia z bliska - tych jest 4: 0..3, aby nie by³o przeskoku
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  filter+=4;
 }
 else
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
 switch (filter) //rozmycie przy oddaleniu
 {case 4: //najbli¿szy z tekstury
   glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST); break;
  case 5: //œrednia z tekstury
   glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); break;
  case 6: //najbli¿szy z mipmapy
   glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST_MIPMAP_NEAREST); break;
  case 7: //œrednia z mipmapy
   glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST); break;
  case 8: //najbli¿szy z dwóch mipmap
   glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST_MIPMAP_LINEAR); break;
  case 9: //œrednia z dwóch mipmap
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
    else // $: filtruj wszystko - brzydko siê zlewa
     {
       glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, GL_LINEAR);
       glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
     }

};

///////////////////////////////////////////////////////////////////////////////
GLuint TTexturesManager::CreateTexture(char* buff,GLint bpp,int width,int height,bool bHasAlpha,bool bHash,bool bDollar,int filter)
{//Ra: u¿ywane tylko dla TGA i TEX
 //Ra: dodana obs³uga GL_BGR oraz GL_BGRA dla TGA - szybciej siê wczytuje
 GLuint ID;
 glGenTextures(1,&ID);
 glBindTexture(GL_TEXTURE_2D,ID);
 glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
 glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
 if (filter>=0)
  SetFiltering(filter); //cyfra po % w nazwie
 else
  SetFiltering(bHasAlpha&&bDollar,bHash); //znaki #, $ i kana³ alfa w nazwie
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

