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
#include "McZapkie/mctools.h"

// wczytanie modelu do kontenerka
TModel3d *
TMdlContainer::LoadModel(std::string const &Name, bool const Dynamic) { 

    Model = std::make_shared<TModel3d>();
    if( true == Model->LoadFromFile( Name, Dynamic ) ) {
        m_name = Name;
        return Model.get();
    }
    else {
        m_name.clear();
        Model = nullptr;
        return nullptr;
    }
};

TModelsManager::modelcontainer_sequence TModelsManager::m_models;
TModelsManager::stringmodelcontainerindex_map TModelsManager::m_modelsmap;

// wczytanie modelu do tablicy
TModel3d *
TModelsManager::LoadModel(std::string const &Name, bool dynamic) {
    
    m_models.emplace_back();
    auto model = m_models.back().LoadModel( Name, dynamic );
    if( model != nullptr ) {
        m_modelsmap.emplace( Name, m_models.size() - 1 );
        return model;
    }
    else {
        m_models.pop_back();
        return nullptr;
    }
}

TModel3d *
TModelsManager::GetModel(std::string const &Name, bool const Dynamic)
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
    std::string const buftp = Global::asCurrentTexturePath; // zapamiętanie aktualnej ścieżki do tekstur,
    if( Name.find('\\') == std::string::npos )
    {
        buf = "models\\" + Name; // Ra: było by lepiej katalog dodać w parserze
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
        if( Dynamic ) {
            // na razie tak, bo nie wiadomo, jaki może mieć wpływ na pozostałe modele
            if( Name.find( '/' ) != std::string::npos ) { // pobieranie tekstur z katalogu, w którym jest model
                Global::asCurrentTexturePath = Global::asCurrentTexturePath + Name;
                Global::asCurrentTexturePath.erase(
                    Global::asCurrentTexturePath.find( "/" ) + 1,
                    Global::asCurrentTexturePath.length() - 1 );
            }
        }
    }
	buf = ToLower( buf );

    auto const lookup = m_modelsmap.find( buf );
    if( lookup != m_modelsmap.end() ) {
        Global::asCurrentTexturePath = buftp;
        return ( m_models[ lookup->second ].Model.get() );
    }

    auto model = LoadModel(buf, Dynamic); // model nie znaleziony, to wczytać
    Global::asCurrentTexturePath = buftp; // odtworzenie ścieżki do tekstur
    return model; // NULL jeśli błąd
};

//---------------------------------------------------------------------------
