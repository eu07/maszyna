//---------------------------------------------------------------------------

#include    "system.hpp"
#include    "classes.hpp"
#pragma hdrstop

#include "Train.h"
#include "eu07series.h"
#include "Timer.h"

__fastcall TEu07::TEu07():TTrain()
{

}

__fastcall TEu07::~TEu07()
{
}

bool __fastcall TEu07::Init(TDynamicObject *NewDynamicObject)
{
    TTrain::Init(NewDynamicObject);
}

void __fastcall TEu07::OnKeyPress(int cKey)
{
    TTrain::OnKeyPress(cKey);
}

bool __fastcall TEu07::Update(double dt)
{
    TTrain::Update(dt);
}

bool __fastcall TEu07::UpdateMechPosition()
{
    TTrain::UpdateMechPosition();
}

bool __fastcall TEu07::Render()
{
    TTrain::Render();
}
//---------------------------------------------------------------------------

#pragma package(smart_init)
