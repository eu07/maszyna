//---------------------------------------------------------------------------

#include "system.hpp"
#include "classes.hpp"
#pragma hdrstop

#include "Machajka.h"
#include "Timer.h"
#include "Globals.h"

__fastcall TMachajka::TMachajka() : TTrain() { TTrain::TTrain(); }

__fastcall TMachajka::~TMachajka() {}

bool TMachajka::Init(TDynamicObject *NewDynamicObject)
{
    TTrain::Init(NewDynamicObject);
    return true;
}

void TMachajka::OnKeyPress(int cKey)
{
    if (!GetAsyncKeyState(VK_SHIFT) < 0) // bez shifta
    {
        if (cKey == Global::Keys[k_IncMainCtrl])
            (DynamicObject->MoverParameters->AddPulseForce(1));
        else if (cKey == Global::Keys[k_DecMainCtrl])
            (DynamicObject->MoverParameters->AddPulseForce(-1));
        else if (cKey == Global::Keys[k_IncLocalBrakeLevel])
            (DynamicObject->MoverParameters->IncLocalBrakeLevel(1));
        else if (cKey == Global::Keys[k_DecLocalBrakeLevel])
            (DynamicObject->MoverParameters->DecLocalBrakeLevel(1));
        else if (cKey == Global::Keys[k_MechLeft])
            vMechMovement.x += fMechCroach;
        else if (cKey == Global::Keys[k_MechRight])
            vMechMovement.x -= fMechCroach;
        else if (cKey == Global::Keys[k_MechBackward])
            vMechMovement.z -= fMechCroach;
        else if (cKey == Global::Keys[k_MechForward])
            vMechMovement.z += fMechCroach;
        else if (cKey == Global::Keys[k_MechUp])
            pMechOffset.y += 0.3; // McZapkie-120302 - wstawanie
        else if (cKey == Global::Keys[k_MechDown])
            pMechOffset.y -= 0.3; // McZapkie-120302 - siadanie, kucanie itp
    }
    else // z shiftem
    {
        if (cKey == Global::Keys[k_IncMainCtrlFAST])
            (DynamicObject->MoverParameters->AddPulseForce(2));
        else if (cKey == Global::Keys[k_DecMainCtrlFAST])
            (DynamicObject->MoverParameters->AddPulseForce(-2));
        else if (cKey == Global::Keys[k_IncLocalBrakeLevelFAST])
            (DynamicObject->MoverParameters->IncLocalBrakeLevel(2));
        else if (cKey == Global::Keys[k_DecLocalBrakeLevelFAST])
            (DynamicObject->MoverParameters->DecLocalBrakeLevel(2));
    }
}

bool TMachajka::Update(double dt) { TTrain::Update(dt); }

bool TMachajka::UpdateMechPosition() { TTrain::UpdateMechPosition(); }

bool TMachajka::Render() { TTrain::Render(); }
//---------------------------------------------------------------------------

#pragma package(smart_init)
