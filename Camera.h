/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#ifndef CameraH
#define CameraH

#include "dumb3d.h"
using namespace Math3D;

//---------------------------------------------------------------------------
enum TCameraType
{ // tryby pracy kamery
    tp_Follow, // jazda z pojazdem
    tp_Free, // stoi na scenerii
    tp_Satelite // widok z góry (nie u¿ywany)
};

class TCamera
{
  private:
    vector3 pOffset; // nie u¿ywane (zerowe)
  public: // McZapkie: potrzebuje do kiwania na boki
    double Pitch;
    double Yaw; // w œrodku: 0=do przodu; na zewn¹trz: 0=na po³udnie
    double Roll;
    TCameraType Type;
    vector3 Pos; // wspó³rzêdne obserwatora
    vector3 LookAt; // wspó³rzêdne punktu, na który ma patrzeæ
    vector3 vUp;
    vector3 Velocity;
    vector3 OldVelocity; // lepiej usredniac zeby nie bylo rozbiezne przy malym FPS
    vector3 CrossPos;
    double CrossDist;
    void Init(vector3 NPos, vector3 NAngle);
    void Reset()
    {
        Pitch = Yaw = Roll = 0;
    };
    void OnCursorMove(double x, double y);
    void Update();
    vector3 GetDirection();
    // vector3 inline GetCrossPos() { return Pos+GetDirection()*CrossDist+CrossPos; };

    bool SetMatrix();
    void SetCabMatrix(vector3 &p);
    void RaLook();
    void Stop();
    // bool GetMatrix(matrix4x4 &Matrix);
    vector3 PtNext, PtPrev;
};
#endif
