//---------------------------------------------------------------------------

#include    "system.hpp"
#include    "classes.hpp"
#include <gl/gl.h>
#include <gl/glu.h>
#include "opengl/glut.h"
#pragma hdrstop

#include "Timer.h"
#include "Texture.h"
#include "SplFoll.h"
#include "Scenery.h"
#include "Globals.h"

//---------------------------------------------------------------------------

__fastcall TScenerySquare::TScenerySquare()
{
}

__fastcall TScenerySquare::~TScenerySquare()
{
    Free();
}

bool __fastcall TScenerySquare::Free()
{
}



bool __fastcall TScenerySquare::Init(AnsiString asFile)
{

    return true;

}

bool __fastcall TScenerySquare::Render(vector3 pPosition)
{

    return true;

}

//---------------------------------------------------------------------------
/*
__fastcall TScenery::TScenery()
{
}

__fastcall TScenery::~TScenery()
{
    Free();
}

bool __fastcall TScenery::Free()
{
}



bool __fastcall TScenery::Init(AnsiString asFile)
{

    return true;

}

bool __fastcall TScenery::Render(vector3 pPosition)
{

    return true;

}
*/
