//---------------------------------------------------------------------------

#include    "system.hpp"
#include    "classes.hpp"
#pragma hdrstop

#include "SplFoll.h"

__fastcall TSplineFollower::TSplineFollower()
{
    CurrentKnot= NULL;
    s=v=a=dist= 0.0f;
    fDirection= 1.0f;
    v= 0;
    oldt= 0;
    Accurate= false;
    Position= vector3(0,0,0);
    bChangeableDirection= true;
//    vDirection= vector3(0,0,0);
}

__fastcall TSplineFollower::~TSplineFollower()
{
}

bool __fastcall TSplineFollower::Init(TKnot *Knot, bool NAccurate)
{
    CurrentKnot= Knot;
    s=v=a=dist= 0.0f;
    v= 0;
    oldt= 0;
    Accurate= NAccurate;
    Position= CurrentKnot->Point;

    return true;
}

void __fastcall TSplineFollower::Move(float Distance)
{
    v= Distance;
    Refresh(1);
}

bool __fastcall TSplineFollower::Refresh(float DeltaTime)
{
    if (CurrentKnot==NULL || CurrentKnot->Next==NULL)
        return false;
    v+= a*DeltaTime;
    float ds= v*DeltaTime*fDirection;
    float t;
    vector3 pt1= CurrentKnot->Point;
    vector3 pt2= CurrentKnot->Next->Point;
    vector3 vDirection;
    do
    {
        if (ds>0 && CurrentKnot->bSwitchDirectionForward)
        {
            if (!bChangeableDirection)
                return false;
            fDirection= -fDirection;
            ds= -ds;
            s= CurrentKnot->Length-s;
            if (CurrentKnot->Next)
                CurrentKnot= CurrentKnot->Next;
            if (CurrentKnot->Next)
                CurrentKnot= CurrentKnot->Next;
            if (CurrentKnot->Prev)
                CurrentKnot= CurrentKnot->Prev;
            pt1= CurrentKnot->Point;
            pt2= CurrentKnot->Next->Point;
        }
        else
            if (ds<0 && CurrentKnot->bSwitchDirectionBackward)
            {
                if (!bChangeableDirection)
                    return false;
                fDirection= -fDirection;
                ds= -ds;
                s= CurrentKnot->Length-s;
                if (CurrentKnot->Prev)
                    CurrentKnot= CurrentKnot->Prev;
                pt1= CurrentKnot->Point;
                pt2= CurrentKnot->Next->Point;
            }
        if (CurrentKnot->IsCurve)
        {
            if (s+ds<0)
            {
                if (CurrentKnot->Prev==NULL)
                    return false;
                ds+= s;
                dist-= CurrentKnot->Length;

//                KnotIndex--;                        //BUGBUG
  //              if (KnotIndex<0)
                CurrentKnot= CurrentKnot->Prev;
                s= CurrentKnot->Length;//+ds;
                pt1= CurrentKnot->Point;
                pt2= CurrentKnot->Next->Point;
            }
            else
            if (CurrentKnot->Length>s+ds)
            {
                s+= ds;
                if (Accurate)
                {
                    t= CurrentKnot->GetTFromS(s);
                    if (t>1)
                        t= 1;
//                    vDirection= CurrentKnot->GetDirection(t);
                    Position= Interpolate(t,
                                          pt1,CurrentKnot->CPointOut,
                                          CurrentKnot->Next->CPointIn,pt2);
                }
                else
                {
                    t= s/CurrentKnot->Length;
  //                  vDirection= CurrentKnot->GetDirection(t);
                    Position= Interpolate(t,pt1,CurrentKnot->CPointOut,
                                            CurrentKnot->Next->CPointIn,pt2);
                }
                break;
            }
            else
            {
                CurrentKnot= CurrentKnot->Next;
//                KnotIndex++;                        //BUGBUG
  //              if (KnotIndex>=Spline->KnotsCount-1)
                if (CurrentKnot->Next==NULL)
                    return false;
                ds-= CurrentKnot->Prev->Length-s;
                dist+= CurrentKnot->Prev->Length;
                pt1= CurrentKnot->Point;
                pt2= CurrentKnot->Next->Point;
                s= 0;
                oldt= 0;
            }

        }
        else             // Is line
        {
            if (s+ds<0)
            {
                if (CurrentKnot->Prev==NULL)
                    return false;
                ds+= s;
                dist-= CurrentKnot->Length;
                CurrentKnot= CurrentKnot->Prev;
                s= CurrentKnot->Length;//+ds;
                pt1= CurrentKnot->Point;
                pt2= CurrentKnot->Next->Point;
            }
            else
            if (CurrentKnot->Length>s+ds)
            {
                s+= ds;
                vDirection= Normalize(pt2-pt1);
                Position= pt1+vDirection*s;
                break;
            }
            else
            {
                CurrentKnot= CurrentKnot->Next;
//                KnotIndex++;                        //BUGBUG
  //              if (KnotIndex>=Spline->KnotsCount-1)
                if (CurrentKnot->Next==NULL)
                    return false;
                ds-= CurrentKnot->Prev->Length-s;
                dist+= CurrentKnot->Prev->Length;
                pt1= CurrentKnot->Point;
                pt2= CurrentKnot->Next->Point;
                s= 0;
                oldt= 0;
            }
        }
    } while (true);
    return true;
}

/*
    if (Spline->Knots[KnotIndex].IsCurve)
    {
        vector3 v1= Spline->Knots[KnotIndex].CPointOut-Spline->Knots[KnotIndex].Point;
        vector3 v2= Spline->Knots[KnotIndex+1].Point-Spline->Knots[KnotIndex+1].CPointIn;    //BUGBUG
        float t= s/Spline->Knots[KnotIndex].Length;
        return v1*(1-t)+v2*(t);
    }
    else
//        return (Spline->Knots[KnotIndex].GetDirection());//Spline->Knots[KnotIndex].Point-Spline->Knots[KnotIndex].Point);
        return (Spline->Knots[KnotIndex+1].Point-Spline->Knots[KnotIndex].Point);//Spline->Knots[KnotIndex].Point-Spline->Knots[KnotIndex].Point);
        */

//---------------------------------------------------------------------------

#pragma package(smart_init)
