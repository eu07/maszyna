/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/
#pragma once

#include "Model3d.h"
#include "usefull.h"

class TMdlContainer
{
    friend class TModelsManager;
    TMdlContainer()
    {
        Model = NULL;
    };
    ~TMdlContainer()
    {
        SafeDelete(Model);
    };
    TModel3d * LoadModel(std::string const &NewName, bool dynamic);
    TModel3d *Model;
    std::string Name;
};

class TModelsManager
{ // klasa statyczna, nie ma obiektu
  private:
    static TMdlContainer *Models;
    static int Count;
    static TModel3d * LoadModel(std::string const &Name, bool dynamic);

  public:
    //    TModelsManager();
    //    ~TModelsManager();
    static void Init();
    static void Free();
    // McZapkie: dodalem sciezke, notabene Path!=Patch :)
    static TModel3d * GetModel(std::string const &Name, bool dynamic = false);
};
//---------------------------------------------------------------------------
