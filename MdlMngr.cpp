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

#include    "system.hpp"
#include    "classes.hpp"
#include "Texture.h"
#pragma hdrstop

#include "MdlMngr.h"
#include "Globals.h"

//#define SeekFiles AnsiString("*.o3d")
#define SeekFiles AnsiString("*.t3d")
//#define SeekTextFiles AnsiString("*.t3d")

TModel3d* __fastcall TMdlContainer::LoadModel(char *newName,bool dynamic)
{
 SafeDeleteArray(Name);
 SafeDelete(Model);
 Name=new char[strlen(newName)+1];
 strcpy(Name,newName);
 Model=new TModel3d();
 Model->LoadFromTextFile(Name,dynamic); //np. "models\\pkp/head1-y.t3d"
 return Model;
};

TMdlContainer *TModelsManager::Models;
int TModelsManager::Count;
const MAX_MODELS= 600;

void __fastcall TModelsManager::Init()
{
    Models= new TMdlContainer[MAX_MODELS];
    Count= 0;
}
/*
__fastcall TModelsManager::TModelsManager()
{
//    Models= NULL;
    Models= new TMdlContainer[MAX_MODELS];
    Count= 0;
};

__fastcall TModelsManager::~TModelsManager()
{
    Free();
};
  */
void __fastcall TModelsManager::Free()
{
    SafeDeleteArray(Models);
}


int __fastcall TModelsManager::LoadModels(char *asModelsPath)
{
    Error("LoadModels is obsolete");
    return -1;
/*
    WIN32_FIND_DATA FindFileData;
    HANDLE handle= FindFirstFile(AnsiString(asModelsPath+SeekFiles).c_str(), &FindFileData);
    if (handle==INVALID_HANDLE_VALUE) return(0);

    for (Count= 1; FindNextFile(handle, &FindFileData); Count++);

    FindClose(handle);

    Models= new TMdlContainer[Count];

    handle= FindFirstFile(AnsiString(asModelsPath+SeekFiles).c_str(), &FindFileData);
    for (int i=0; i<Count; i++)
    {

        Models[i].Model= new TModel3d();
        Models[i].Model->LoadFromTextFile(AnsiString(asModelsPath+AnsiString(FindFileData.cFileName)).c_str());
        strcpy(Models[i].Name,AnsiString(FindFileData.cFileName).LowerCase().c_str());
//        *strchr(Models[i].Name,'.')= 0;
//        AnsiString(Models[i].Name).LowerCase();
        FindNextFile(handle, &FindFileData);
    };
    FindClose(handle);
    return(Count);*/
};

double Radius;


TModel3d*  __fastcall TModelsManager::LoadModel(char *Name,bool dynamic)
{
 TModel3d *mdl=NULL;;
 WIN32_FIND_DATA FindFileData;
 HANDLE handle=FindFirstFile(Name,&FindFileData);
 if (handle==INVALID_HANDLE_VALUE)
 {
  //WriteLog("Missed model "+AnsiString(Name));
  return NULL; //zg³oszenie b³êdu wy¿ej
 }
 else
 {
  if (Count==MAX_MODELS)
   Error("FIXME: Too many models, program will now crash :)");
  else
  {
   mdl=Models[Count].LoadModel(Name,dynamic);
   Count++;
  }
 }
 FindClose(handle);
 return mdl;
}

TModel3d* __fastcall TModelsManager::GetModel(char *Name,bool dynamic)
{
 char buf[255];
 AnsiString buftp=Global::asCurrentTexturePath;
 TModel3d* tmpModel; //tymczasowe zmienne
 if (strchr(Name,'\\')==NULL)
 {
  strcpy(buf,"models\\"); //Ra: by³o by lepiej katalog dodaæ w parserze
  strcat(buf,Name);
  if (strchr(Name,'/')!=NULL)
  {
   Global::asCurrentTexturePath= Global::asCurrentTexturePath+AnsiString(Name);
   Global::asCurrentTexturePath.Delete(Global::asCurrentTexturePath.Pos("/")+1,Global::asCurrentTexturePath.Length());
  }
 }
 else
  strcpy(buf,Name);
 StrLower(buf);
 for (int i=0; i<Count; i++)
 {
  if (strcmp(buf,Models[i].Name)==0)
  {
   Global::asCurrentTexturePath= buftp;
   return (Models[i].Model);
  }
 };
 tmpModel=LoadModel(buf,dynamic);
 Global::asCurrentTexturePath=buftp;
 return (tmpModel); //NULL jeœli b³¹d
};

/*
TModel3d __fastcall TModelsManager::GetModel(char *Name, AnsiString asReplacableTexture)
{
    GLuint ReplacableTextureID= 0;
    TModel3d NewModel;

    NewModel= *GetNextModel(Name);

    if (asReplacableTexture!=AnsiString("none"))
      ReplacableTextureID= TTexturesManager::GetTextureID(asReplacableTexture.c_str());

    NewModel.ReplacableSkinID=ReplacableTextureID;

    return NewModel;
};
*/

//---------------------------------------------------------------------------
#pragma package(smart_init)
