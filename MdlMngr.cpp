/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak and others

*/

#include "system.hpp"
#include "classes.hpp"
#include "Texture.h"
#pragma hdrstop

#include "MdlMngr.h"
#include "Globals.h"

#define SeekFiles AnsiString("*.t3d")

TModel3d * TMdlContainer::LoadModel(char *newName, bool dynamic)
{ // wczytanie modelu do kontenerka
    SafeDeleteArray(Name);
    SafeDelete(Model);
    Name = new char[strlen(newName) + 1];
    strcpy(Name, newName);
    Model = new TModel3d();
    if (!Model->LoadFromFile(Name, dynamic)) // np. "models\\pkp/head1-y.t3d"
        SafeDelete(Model);
    return Model;
};

TMdlContainer *TModelsManager::Models;
int TModelsManager::Count;
const MAX_MODELS = 1000;

void TModelsManager::Init()
{
    Models = new TMdlContainer[MAX_MODELS];
    Count = 0;
}
/*
 TModelsManager::TModelsManager()
{
//    Models= NULL;
    Models= new TMdlContainer[MAX_MODELS];
    Count= 0;
};

 TModelsManager::~TModelsManager()
{
    Free();
};
  */
void TModelsManager::Free()
{
    SafeDeleteArray(Models);
}

TModel3d * TModelsManager::LoadModel(char *Name, bool dynamic)
{ // wczytanie modelu do tablicy
    TModel3d *mdl = NULL;
    if (Count >= MAX_MODELS)
        Error("FIXME: Too many models, program will now crash :)");
    else
    {
        mdl = Models[Count].LoadModel(Name, dynamic);
        if (mdl)
            Count++; // jeœli b³¹d wczytania modelu, to go nie wliczamy
    }
    return mdl;
}

TModel3d * TModelsManager::GetModel(const char *Name, bool dynamic)
{ // model mo¿e byæ we wpisie "node...model" albo "node...dynamic", a tak¿e byæ dodatkowym w dynamic
    // (kabina, wnêtrze, ³adunek)
    // dla "node...dynamic" mamy podan¹ œcie¿kê w "\dynamic\" i musi byæ co najmniej 1 poziom, zwkle
    // s¹ 2
    // dla "node...model" mo¿e byæ typowy model statyczny ze œcie¿k¹, domyœlnie w "\scenery\" albo
    // "\models"
    // albo mo¿e byæ model z "\dynamic\", jeœli potrzebujemy wstawiæ auto czy wagon nieruchomo
    // - ze œcie¿ki z której jest wywo³any, np. dir="scenery\bud\" albo dir="dynamic\pkp\st44_v1\"
    // plus name="model.t3d"
    // - z domyœlnej œcie¿ki dla modeli, np. "scenery\" albo "models\" plus name="bud\dombale.t3d"
    // (dir="")
    // - konkretnie podanej œcie¿ki np. name="\scenery\bud\dombale.t3d" (dir="")
    // wywo³ania:
    // - konwersja wszystkiego do E3D, podawana dok³adna œcie¿ka, tekstury tam, gdzie plik
    // - wczytanie kabiny, dok³adna œcie¿ka, tekstury z katalogu modelu
    // - wczytanie ³adunku, œcie¿ka dok³adna, tekstury z katalogu modelu
    // - wczytanie modelu, œcie¿ka dok³adna, tekstury z katalogu modelu
    // - wczytanie przedsionków, œcie¿ka dok³adna, tekstury z katalogu modelu
    // - wczytanie uproszczonego wnêtrza, œcie¿ka dok³adna, tekstury z katalogu modelu
    // - niebo animowane, œcie¿ka brana ze wpisu, tekstury nieokreœlone
    // - wczytanie modelu animowanego - Init() - sprawdziæ
    char buf[255];
    AnsiString buftp = Global::asCurrentTexturePath; // zapamiêtanie aktualnej œcie¿ki do tekstur,
    // bo bêdzie tyczmasowo zmieniana
    /*
    // Ra: niby tak jest lepiej, ale dzia³a gorzej, wiêc przywrócone jest oryginalne
     //nawet jeœli model bêdzie pobrany z tablicy, to trzeba ustaliæ œcie¿kê dla tekstur
     if (dynamic) //na razie tak, bo nie wiadomo, jaki mo¿e mieæ wp³yw na pozosta³e modele
     {//dla pojazdów podana jest zawsze pe³na œcie¿ka do modelu
      strcpy(buf,Name);
      if (strchr(Name,'/')!=NULL)
      {//pobieranie tekstur z katalogu, w którym jest model
       Global::asCurrentTexturePath=Global::asCurrentTexturePath+AnsiString(Name);
       Global::asCurrentTexturePath.Delete(Global::asCurrentTexturePath.Pos("/")+1,Global::asCurrentTexturePath.Length());
      }
     }
     else
     {//dla modeli scenerii trzeba ustaliæ œcie¿kê
      if (strchr(Name,'\\')==NULL)
      {//jeœli nie ma lewego ukoœnika w œcie¿ce, a jest prawy, to zmieniæ œcie¿kê dla tekstur na tê
    z modelem
       strcpy(buf,"models\\"); //Ra: by³o by lepiej katalog dodaæ w parserze
       //strcpy(buf,"scenery\\"); //Ra: by³o by lepiej katalog dodaæ w parserze
       strcat(buf,Name);
       if (strchr(Name,'/')!=NULL)
       {//jeszcze musi byæ prawy ukoœnik
        Global::asCurrentTexturePath=Global::asCurrentTexturePath+AnsiString(Name);
        Global::asCurrentTexturePath.Delete(Global::asCurrentTexturePath.Pos("/")+1,Global::asCurrentTexturePath.Length());
       }
      }
      else
      {//jeœli jest lewy ukoœnik, to œcie¿kê do tekstur zmieniæ tylko dla pojazdów
       strcpy(buf,Name);
      }
     }
     StrLower(buf);
     for (int i=0;i<Count;i++)
     {//bezsensowne przeszukanie tabeli na okolicznoœæ wyst¹pienia modelu
      if (strcmp(buf,Models[i].Name)==0)
      {
       Global::asCurrentTexturePath=buftp; //odtworzenie œcie¿ki do tekstur
       return (Models[i].Model); //model znaleziony
      }
     };
    */
    if (strchr(Name, '\\') == NULL)
    {
        strcpy(buf, "models\\"); // Ra: by³o by lepiej katalog dodaæ w parserze
        strcat(buf, Name);
        if (strchr(Name, '/') != NULL)
        {
            Global::asCurrentTexturePath = Global::asCurrentTexturePath + AnsiString(Name);
            Global::asCurrentTexturePath.Delete(Global::asCurrentTexturePath.Pos("/") + 1,
                                                Global::asCurrentTexturePath.Length());
        }
    }
    else
    {
        strcpy(buf, Name);
        if (dynamic) // na razie tak, bo nie wiadomo, jaki mo¿e mieæ wp³yw na pozosta³e modele
            if (strchr(Name, '/') != NULL)
            { // pobieranie tekstur z katalogu, w którym jest model
                Global::asCurrentTexturePath = Global::asCurrentTexturePath + AnsiString(Name);
                Global::asCurrentTexturePath.Delete(Global::asCurrentTexturePath.Pos("/") + 1,
                                                    Global::asCurrentTexturePath.Length());
            }
    }
    StrLower(buf);
    for (int i = 0; i < Count; i++)
    {
        if (strcmp(buf, Models[i].Name) == 0)
        {
            Global::asCurrentTexturePath = buftp;
            return (Models[i].Model);
        }
    };
    TModel3d *tmpModel = LoadModel(buf, dynamic); // model nie znaleziony, to wczytaæ
    Global::asCurrentTexturePath = buftp; // odtworzenie œcie¿ki do tekstur
    return (tmpModel); // NULL jeœli b³¹d
};

/*
TModel3d TModelsManager::GetModel(char *Name, AnsiString asReplacableTexture)
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
