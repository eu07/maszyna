//---------------------------------------------------------------------------
#ifndef CameraH
#define CameraH


#include "dumb3d.h"
using namespace Math3D;

//---------------------------------------------------------------------------
enum TCameraType
{//tryby pracy kamery
 tp_Follow,  //jazda z pojazdem
 tp_Free,    //stoi na scenerii
 tp_Satelite //widok z góry (nie u¿ywany)
};

class TCamera
{
private:
    vector3 pOffset; //nie u¿ywane (zerowe)
public:
    double Pitch,Yaw,Roll;  //McZapkie: potrzebuje do kiwania na boki
    TCameraType Type;
    vector3 Pos; //wspó³rzêdne obserwatora
    vector3 LookAt; //wspó³rzêdne punktu, na który ma patrzeæ
    vector3 vUp;
    vector3 Velocity;
    vector3 OldVelocity; //lepiej usredniac zeby nie bylo rozbiezne przy malym FPS
    vector3 CrossPos;
    double CrossDist;
    void __fastcall Init(vector3 NPos, vector3 NAngle);
    void __fastcall Reset() { Pitch=Yaw=Roll= 0; };
    void __fastcall OnCursorMove(double x, double y);
    void __fastcall Update();
    void __fastcall Render();
    vector3 __fastcall GetDirection();
    vector3 inline __fastcall GetCrossPos() { return Pos+GetDirection()*CrossDist+CrossPos; };

 bool __fastcall SetMatrix();
 void __fastcall SetCabMatrix(vector3 &p);
 void __fastcall RaLook();
 void __fastcall Stop();
 //bool __fastcall GetMatrix(matrix4x4 &Matrix);
 vector3 PtNext, PtPrev;
};
#endif
