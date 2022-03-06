#include "stdafx.h"

#include "Model3d.h"
#include "Globals.h"
#include "renderer.h"

void export_e3d_standalone(std::string in, std::string out, int flags, bool dynamic)
{
    Global.iConvertModels = flags;
    Global.iWriteLogEnabled = 2;
    Global.ParserLogIncludes = true;
    GfxRenderer = gfx_renderer_factory::get_instance()->create("null");
    TModel3d model;
    model.LoadFromTextFile(in, dynamic);
    model.Init();
    model.SaveToBinFile(out);
}