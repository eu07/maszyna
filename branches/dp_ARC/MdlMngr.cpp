//---------------------------------------------------------------------------

/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak and others

*/

#include    "system.hpp"
#include    "classes.hpp"
#include "Texture.h"
#pragma hdrstop

#include "MdlMngr.h"
#include "Globals.h"

#define SeekFiles AnsiString("*.t3d")

TModel3d* __fastcall TMdlContainer::LoadModel(char *newName,bool dynamic)
{//wczytanie modelu do kontenerka
 SafeDeleteArray(Name);
 SafeDelete(Model);
 Name=new char[strlen(newName)+1];
 strcpy(Name,newName);
 Model=new TModel3d();
 if (!Model->LoadFromFile(Name,dynamic)) //np. "models\\pkp/head1-y.t3d"
  SafeDelete(Model);
 return Model;
};

TMdlContainer *TModelsManager::Models;
int TModelsManager::Count;
const MAX_MODELS=1000;

void __fastcall TModelsManager::Init()
{
 Models=new TMdlContainer[MAX_MODELS];
 Count=0;
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


TModel3d*  __fastcall TModelsManager::LoadModel(char *Name,bool dynamic)
{//wczytanie modelu do tablicy
 TModel3d *mdl=NULL;
 if (Count>=MAX_MODELS)
  Error("FIXME: Too many models, program will now crash :)");
 else
 {
  mdl=Models[Count].LoadModel(Name,dynamic);
  if (mdl) Count++; //je�li b��d wczytania modelu, to go nie wliczamy
 }
 return mdl;
}

TModel3d* __fastcall TModelsManager::GetModel(const char *Name,bool dynamic)
{
 char buf[255];
 AnsiString buftp=Global::asCurrentTexturePath;
 TModel3d* tmpModel; //tymczasowe zmienne
 if (strchr(Name,'\\')==NULL)
 {
  strcpy(buf,"models\\"); //Ra: by�o by lepiej katalog doda� w parserze
  strcat(buf,Name);
  if (strchr(Name,'/')!=NULL)
  {
   Global::asCurrentTexturePath=Global::asCurrentTexturePath+AnsiString(Name);
   Global::asCurrentTexturePath.Delete(Global::asCurrentTexturePath.Pos("/")+1,Global::asCurrentTexturePath.Length());
  }
 }
 else
 {strcpy(buf,Name);
  if (dynamic) //na razie tak, bo nie wiadomo, jaki mo�e mie� wp�yw na pozosta�e modele
   if (strchr(Name,'/')!=NULL)
   {//pobieranie tekstur z katalogu, w kt�rym jest model
    Global::asCurrentTexturePath=Global::asCurrentTexturePath+AnsiString(Name);
    Global::asCurrentTexturePath.Delete(Global::asCurrentTexturePath.Pos("/")+1,Global::asCurrentTexturePath.Length());
   }
 }
 StrLower(buf);
 for (int i=0;i<Count;i++)
 {
  if (strcmp(buf,Models[i].Name)==0)
  {
   Global::asCurrentTexturePath=buftp;
   return (Models[i].Model);
  }
 };
 tmpModel=LoadModel(buf,dynamic);
 Global::asCurrentTexturePath=buftp;
 return (tmpModel); //NULL je�li b��d
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
