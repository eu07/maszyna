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

#include "Model3d.h"
#include "Globals.h"
#include "Logs.h"
#include "utilities.h"

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

TModelsManager::modelcontainer_sequence TModelsManager::m_models { 1, TMdlContainer{} };
TModelsManager::stringmodelcontainerindex_map TModelsManager::m_modelsmap;

// wczytanie modelu do tablicy
TModel3d *
TModelsManager::LoadModel(std::string const &Name, std::string const &virtualName, bool dynamic) {
    
    m_models.emplace_back();
    auto model = m_models.back().LoadModel( Name, dynamic );
    if( model != nullptr ) {
		m_modelsmap.emplace( virtualName, m_models.size() - 1 );
    }
    else {
        m_models.pop_back();
		m_modelsmap.emplace( virtualName, null_handle );
    }
    return model;
}

TModel3d *
TModelsManager::GetModel(std::string const &Name, bool const Dynamic, bool const Logerrors, int uid )
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
    std::string const buftp { Global.asCurrentTexturePath }; // zapamiętanie aktualnej ścieżki do tekstur,
    std::string filename { Name };
    if( ( false == Dynamic )
     && ( contains( Name, '/' ) ) ) {
        // pobieranie tekstur z katalogu, w którym jest model
        // when loading vehicles the path is set by the calling routine, so we can skip it here
        Global.asCurrentTexturePath += Name;
        Global.asCurrentTexturePath.erase( Global.asCurrentTexturePath.rfind( '/' ) + 1 );
    }
    erase_extension( filename );
    filename = ToLower( filename );
	std::string postfix;
	if (uid != 0)
		postfix = "^^" + std::to_string(uid);

	// see if we have it in the databank
	auto banklookup { find_in_databank( filename + postfix ) };
    TModel3d *model { banklookup.second };
    if( true == banklookup.first ) {
        Global.asCurrentTexturePath = buftp;
        return model;
    }

    // first load attempt, check if it's on disk
    std::string disklookup { find_on_disk( filename ) };

    if( false == disklookup.empty() ) {
		model = LoadModel( disklookup, disklookup + postfix, Dynamic ); // model nie znaleziony, to wczytać
    }
    else {
        // there's nothing matching in the databank nor on the disk, report failure...
        if( Logerrors ) {
            ErrorLog( "Bad file: failed to locate 3d model file \"" + filename + "\"", logtype::file );
        }
        // ...and link it with the error model slot
		m_modelsmap.emplace( filename + postfix, null_handle );
    }
    Global.asCurrentTexturePath = buftp; // odtworzenie ścieżki do tekstur
    return model; // NULL jeśli błąd
};

std::pair<bool, TModel3d *>
TModelsManager::find_in_databank( std::string const &Name ) {

    std::vector<std::string> filenames {
        Name,
        szModelPath + Name };

    for( auto const &filename : filenames ) {
        auto const lookup { m_modelsmap.find( filename ) };
        if( lookup != m_modelsmap.end() ) {
            return { true, m_models[ lookup->second ].Model.get() };
        }
    }

    return { false, nullptr };
}

// checks whether specified file exists. returns name of the located file, or empty string.
std::string
TModelsManager::find_on_disk( std::string const &Name ) {

    std::vector<std::string> extensions { { ".e3d" }, { ".t3d" } };
    for( auto const &extension : extensions ) {

        auto lookup = (
            FileExists( Name + extension ) ? Name :
            FileExists( szModelPath + Name + extension ) ? szModelPath + Name :
            "" );
        if( false == lookup.empty() ) {
            return lookup;
        }
    }

    return {};
}

//---------------------------------------------------------------------------
