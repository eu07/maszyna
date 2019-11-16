/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/
#pragma once

#include "Classes.h"

class TMdlContainer {
    friend class TModelsManager;
private:
    TModel3d *LoadModel( std::string const &Name, bool const Dynamic );
    std::shared_ptr<TModel3d> Model { nullptr };
    std::string m_name;
};

// klasa statyczna, nie ma obiektu
class TModelsManager {
public:
    // McZapkie: dodalem sciezke, notabene Path!=Patch :)
	static TModel3d *GetModel(std::string const &Name, bool const dynamic = false, bool const Logerrors = true , int uid = 0);

private:
// types:
    typedef std::deque<TMdlContainer> modelcontainer_sequence;
    typedef std::unordered_map<std::string, modelcontainer_sequence::size_type> stringmodelcontainerindex_map;
// members:
    static modelcontainer_sequence m_models;
    static stringmodelcontainerindex_map m_modelsmap;
// methods:
	static TModel3d *LoadModel(std::string const &Name, const std::string &virtualName, bool const Dynamic );
    static std::pair<bool, TModel3d *> find_in_databank( std::string const &Name );
    // checks whether specified file exists. returns name of the located file, or empty string.
    static std::string find_on_disk( std::string const &Name );

};

//---------------------------------------------------------------------------
