//---------------------------------------------------------------------------
#ifndef CameraH
#define CameraH


#include "dumb3d.h"
using namespace Math3D;

//---------------------------------------------------------------------------
enum TCameraType { tp_Follow, tp_Free, tp_Satelite };

class TCamera
{
//private:

public:
    double Pitch,Yaw,Roll;  //McZapkie: potrzebuje do kiwania na boki
    TCameraType Type;
    vector3 Pos;
    vector3 LookAt;
    vector3 vUp;
    vector3 Velocity;
    vector3 OldVelocity; //lepiej usredniac zeby nie bylo rozbiezne przy malym FPS    
    vector3 CrossPos;
    vector3 pOffset;
    double CrossDist;
    void __fastcall Init(vector3 NPos, vector3 NAngle);
    void __fastcall Reset() { Pitch=Yaw=Roll= 0; };
    void __fastcall OnCursorMove(double x, double y);
    void __fastcall Update();
    void __fastcall Render();
    vector3 __fastcall GetDirection();
    vector3 inline __fastcall GetCrossPos() { return Pos+GetDirection()*CrossDist+CrossPos; };

    bool __fastcall SetMatrix();
//    bool __fastcall GetMatrix(matrix4x4 &Matrix);

    vector3 PtNext, PtPrev;
};
#endif
