//---------------------------------------------------------------------------

#ifndef SceneryH
#define SceneryH

#include    "system.hpp"
#include    "classes.hpp"


#include "dumb3d.h"
//#include "Entity.h"
#include "Geometry.h"
#include "QueryParserComp.hpp"
#include "Spline.h"
//#include "Semaphore.h"
#include "Event.h"
#include "DynObj.h"
#include "Train.h"
#include "Sound.h"
#include "Track.h"

class TScenerySquare
{
public:
    __fastcall TScenerySquare();
    __fastcall ~TScenerySquare();
    bool __fastcall Free();
    bool __fastcall Init(AnsiString asFile);
    bool __fastcall Render(vector3 pPosition);
private:

};
/*
class TScenery
{
public:
    __fastcall TScenery();
    __fastcall ~TScenery();
    bool __fastcall Free();
    bool __fastcall Init(AnsiString asFile);
    bool __fastcall Render(vector3 pPosition);
private:

};*/
//---------------------------------------------------------------------------
#endif
