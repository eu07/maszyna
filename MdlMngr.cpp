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

#include "stdafx.h"
#include "MdlMngr.h"

#include "Globals.h"
#include "Logs.h"
#include "McZapkie/mctools.h"

//#define SeekFiles std::string("*.t3d")

TModel3d * TMdlContainer::LoadModel(std::string const &NewName, bool dynamic)
{ // wczytanie modelu do kontenerka
    SafeDelete(Model);
	Name = NewName;
    Model = new TModel3d();
    if (!Model->LoadFromFile(Name, dynamic)) // np. "models\\pkp/head1-y.t3d"
        SafeDelete(Model);
    return Model;
};

TMdlContainer *TModelsManager::Models;
int TModelsManager::Count;
int const MAX_MODELS = 1000;

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
    delete[] Models;
    Models = nullptr;
}

TModel3d * TModelsManager::LoadModel(std::string const &Name, bool dynamic)
{ // wczytanie modelu do tablicy
    TModel3d *mdl = NULL;
    if (Count >= MAX_MODELS)
        Error("FIXME: Too many models, program will now crash :)");
    else
    {
        mdl = Models[Count].LoadModel(Name, dynamic);
        if (mdl)
            Count++; // jeśli błąd wczytania modelu, to go nie wliczamy
    }
    return mdl;
}

TModel3d * TModelsManager::GetModel(std::string const &Name, bool dynamic)
{ // model może być we wpisie "node...model" albo "node...dynamic", a także być dodatkowym w dynamic
    // (kabina, wnętrze, ładunek)
    // dla "node...dynamic" mamy podaną ścieżkę w "\dynamic\" i musi być co najmniej 1 poziom, zwkle
    // są 2
    // dla "node...model" może być typowy model statyczny ze ścieżką, domyślnie w "\scenery\" albo
    // "\models"
    // albo może być model z "\dynamic\", jeśli potrzebujemy wstawić auto czy wagon nieruchomo
    // - ze ścieżki z której jest wywołany, np. dir="scenery\bud\" albo dir="dynamic\pkp\st44_v1\"
    // plus name="model.t3d"
    // - z domyślnej ścieżki dla modeli, np. "scenery\" albo "models\" plus name="bud\dombale.t3d"
    // (dir="")
    // - konkretnie podanej ścieżki np. name="\scenery\bud\dombale.t3d" (dir="")
    // wywołania:
    // - konwersja wszystkiego do E3D, podawana dokładna ścieżka, tekstury tam, gdzie plik
    // - wczytanie kabiny, dokładna ścieżka, tekstury z katalogu modelu
    // - wczytanie ładunku, ścieżka dokładna, tekstury z katalogu modelu
    // - wczytanie modelu, ścieżka dokładna, tekstury z katalogu modelu
    // - wczytanie przedsionków, ścieżka dokładna, tekstury z katalogu modelu
    // - wczytanie uproszczonego wnętrza, ścieżka dokładna, tekstury z katalogu modelu
    // - niebo animowane, ścieżka brana ze wpisu, tekstury nieokreślone
    // - wczytanie modelu animowanego - Init() - sprawdzić
	std::string buf;
    std::string buftp = Global::asCurrentTexturePath; // zapamiętanie aktualnej ścieżki do tekstur,
    // bo będzie tyczmasowo zmieniana
    /*
    // Ra: niby tak jest lepiej, ale działa gorzej, więc przywrócone jest oryginalne
     //nawet jeśli model będzie pobrany z tablicy, to trzeba ustalić ścieżkę dla tekstur
     if (dynamic) //na razie tak, bo nie wiadomo, jaki może mieć wpływ na pozostałe modele
     {//dla pojazdów podana jest zawsze pełna ścieżka do modelu
      strcpy(buf,Name);
      if (strchr(Name,'/')!=NULL)
      {//pobieranie tekstur z katalogu, w którym jest model
       Global::asCurrentTexturePath=Global::asCurrentTexturePath+AnsiString(Name);
       Global::asCurrentTexturePath.Delete(Global::asCurrentTexturePath.Pos("/")+1,Global::asCurrentTexturePath.Length());
      }
     }
     else
     {//dla modeli scenerii trzeba ustalić ścieżkę
      if (strchr(Name,'\\')==NULL)
      {//jeśli nie ma lewego ukośnika w ścieżce, a jest prawy, to zmienić ścieżkę dla tekstur na tę
    z modelem
       strcpy(buf,"models\\"); //Ra: było by lepiej katalog dodać w parserze
       //strcpy(buf,"scenery\\"); //Ra: było by lepiej katalog dodać w parserze
       strcat(buf,Name);
       if (strchr(Name,'/')!=NULL)
       {//jeszcze musi być prawy ukośnik
        Global::asCurrentTexturePath=Global::asCurrentTexturePath+AnsiString(Name);
        Global::asCurrentTexturePath.Delete(Global::asCurrentTexturePath.Pos("/")+1,Global::asCurrentTexturePath.Length());
       }
      }
      else
      {//jeśli jest lewy ukośnik, to ścieżkę do tekstur zmienić tylko dla pojazdów
       strcpy(buf,Name);
      }
     }
     StrLower(buf);
     for (int i=0;i<Count;i++)
     {//bezsensowne przeszukanie tabeli na okoliczność wystąpienia modelu
      if (strcmp(buf,Models[i].Name)==0)
      {
       Global::asCurrentTexturePath=buftp; //odtworzenie ścieżki do tekstur
       return (Models[i].Model); //model znaleziony
      }
     };
    */
    if( Name.find('\\') == std::string::npos )
    {
        buf = "models\\"; // Ra: było by lepiej katalog dodać w parserze
		buf.append( Name );
        if( Name.find( '/') != std::string::npos)
        {
            Global::asCurrentTexturePath = Global::asCurrentTexturePath + Name;
            Global::asCurrentTexturePath.erase(Global::asCurrentTexturePath.find("/") + 1,
                                                Global::asCurrentTexturePath.length());
        }
    }
    else
    {
		buf = Name;
        if (dynamic) // na razie tak, bo nie wiadomo, jaki może mieć wpływ na pozostałe modele
            if (Name.find( '/') != std::string::npos)
            { // pobieranie tekstur z katalogu, w którym jest model
                Global::asCurrentTexturePath = Global::asCurrentTexturePath + Name;
                Global::asCurrentTexturePath.erase(Global::asCurrentTexturePath.find("/") + 1,
                                                    Global::asCurrentTexturePath.length() - 1);
            }
    }
	buf = ToLower( buf );
    for (int i = 0; i < Count; ++i)
    {
        if ( buf == Models[i].Name )
        {
            Global::asCurrentTexturePath = buftp;
            return (Models[i].Model);
        }
    };
    TModel3d *tmpModel = LoadModel(buf, dynamic); // model nie znaleziony, to wczytać
    Global::asCurrentTexturePath = buftp; // odtworzenie ścieżki do tekstur
    return (tmpModel); // NULL jeśli błąd
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
