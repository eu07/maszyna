/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/
#ifndef MdlMngrH
#define MdlMngrH

#include "Model3d.h"
#include "usefull.h"

class TMdlContainer
{
    friend class TModelsManager;
    TMdlContainer()
    {
        Name = NULL;
        Model = NULL;
    };
    ~TMdlContainer()
    {
        SafeDeleteArray(Name);
        SafeDelete(Model);
    };
    TModel3d * LoadModel(char *newName, bool dynamic);
    TModel3d *Model;
    char *Name;
};

class TModelsManager
{ // klasa statyczna, nie ma obiektu
  private:
    static TMdlContainer *Models;
    static int Count;
    static TModel3d * LoadModel(char *Name, bool dynamic);

  public:
    //    TModelsManager();
    //    ~TModelsManager();
    static void Init();
    static void Free();
    // McZapkie: dodalem sciezke, notabene Path!=Patch :)
    static TModel3d * GetModel(const char *Name, bool dynamic = false);
};
//---------------------------------------------------------------------------
#endif
