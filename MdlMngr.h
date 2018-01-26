/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/
#pragma once

#include "classes.h"

class TMdlContainer {
    friend class TModelsManager;
private:
    TModel3d *LoadModel( std::string const &Name, bool const Dynamic );
    std::shared_ptr<TModel3d> Model { nullptr };
    std::string m_name;
};

// klasa statyczna, nie ma obiektu
class TModelsManager {
// types:
    typedef std::deque<TMdlContainer> modelcontainer_sequence;
    typedef std::unordered_map<std::string, modelcontainer_sequence::size_type> stringmodelcontainerindex_map;
// members:
    static modelcontainer_sequence m_models;
    static stringmodelcontainerindex_map m_modelsmap;
// methods:
    static TModel3d *LoadModel( std::string const &Name, bool const Dynamic );
public:
    // McZapkie: dodalem sciezke, notabene Path!=Patch :)
    static TModel3d *GetModel( std::string const &Name, bool dynamic = false );
};

//---------------------------------------------------------------------------
