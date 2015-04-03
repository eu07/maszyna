//---------------------------------------------------------------------------
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
    TModel3d *__fastcall LoadModel(char *newName, bool dynamic);
    TModel3d *Model;
    char *Name;
};

class TModelsManager
{ // klasa statyczna, nie ma obiektu
  private:
    static TMdlContainer *Models;
    static int Count;
    static TModel3d *__fastcall LoadModel(char *Name, bool dynamic);

  public:
    //    TModelsManager();
    //    ~TModelsManager();
    static void Init();
    static void Free();
    // McZapkie: dodalem sciezke, notabene Path!=Patch :)
    static TModel3d *__fastcall GetModel(const char *Name, bool dynamic = false);
};
//---------------------------------------------------------------------------
#endif
