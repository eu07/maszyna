//---------------------------------------------------------------------------

#include "system.hpp"
#include "classes.hpp"
#include <gl/gl.h>
#include <gl/glu.h>
#include "opengl/glut.h"
#pragma hdrstop

#include "Texture.h"
#include "usefull.h"
#include "Globals.h"
#include "Geom.h"

__fastcall TGeometry::TGeometry() {}

__fastcall TGeometry::~TGeometry() {}

bool TGeometry::Init() {}

vector3 TGeometry::Load(TQueryParserComp *Parser)
{
    str = Parser->GetNextSymbol().LowerCase();
    tmp->TextureID = TTexturesManager::GetTextureID(str.c_str());

    i = 0;
    do
    {
        tf = Parser->GetNextSymbol().ToDouble();
        TempVerts[i].Point.x = tf;
        tf = Parser->GetNextSymbol().ToDouble();
        TempVerts[i].Point.y = tf;
        tf = Parser->GetNextSymbol().ToDouble();
        TempVerts[i].Point.z = tf;
        tf = Parser->GetNextSymbol().ToDouble();
        TempVerts[i].Normal.x = tf;
        tf = Parser->GetNextSymbol().ToDouble();
        TempVerts[i].Normal.y = tf;
        tf = Parser->GetNextSymbol().ToDouble();
        TempVerts[i].Normal.z = tf;

        str = Parser->GetNextSymbol().LowerCase();
        if (str == "x")
            TempVerts[i].tu = (TempVerts[i].Point.x + Parser->GetNextSymbol().ToDouble()) /
                              Parser->GetNextSymbol().ToDouble();
        else if (str == "y")
            TempVerts[i].tu = (TempVerts[i].Point.y + Parser->GetNextSymbol().ToDouble()) /
                              Parser->GetNextSymbol().ToDouble();
        else if (str == "z")
            TempVerts[i].tu = (TempVerts[i].Point.z + Parser->GetNextSymbol().ToDouble()) /
                              Parser->GetNextSymbol().ToDouble();
        else
            TempVerts[i].tu = str.ToDouble();
        ;

        str = Parser->GetNextSymbol().LowerCase();
        if (str == "x")
            TempVerts[i].tv = (TempVerts[i].Point.x + Parser->GetNextSymbol().ToDouble()) /
                              Parser->GetNextSymbol().ToDouble();
        else if (str == "y")
            TempVerts[i].tv = (TempVerts[i].Point.y + Parser->GetNextSymbol().ToDouble()) /
                              Parser->GetNextSymbol().ToDouble();
        else if (str == "z")
            TempVerts[i].tv = (TempVerts[i].Point.z + Parser->GetNextSymbol().ToDouble()) /
                              Parser->GetNextSymbol().ToDouble();
        else
            TempVerts[i].tv = str.ToDouble();
        ;

        //            tf= Parser->GetNextSymbol().ToDouble();
        //          TempVerts[i].tu= tf;
        //        tf= Parser->GetNextSymbol().ToDouble();
        //      TempVerts[i].tv= tf;

        TempVerts[i].Point.RotateZ(aRotate.z / 180 * M_PI);
        TempVerts[i].Point.RotateX(aRotate.x / 180 * M_PI);
        TempVerts[i].Point.RotateY(aRotate.y / 180 * M_PI);
        TempVerts[i].Normal.RotateZ(aRotate.z / 180 * M_PI);
        TempVerts[i].Normal.RotateX(aRotate.x / 180 * M_PI);
        TempVerts[i].Normal.RotateY(aRotate.y / 180 * M_PI);

        TempVerts[i].Point += pOrigin;
        tmp->pCenter += TempVerts[i].Point;

        i++;

        //        }
    } while (Parser->GetNextSymbol().LowerCase() != "endtri");

    nv = i;
    tmp->Init(nv);
    tmp->pCenter /= (nv > 0 ? nv : 1);

    //        memcpy(tmp->Vertices,TempVerts,nv*sizeof(TGroundVertex));

    r = 0;
    for (int i = 0; i < nv; i++)
    {
        tmp->Vertices[i] = TempVerts[i];
        tf = SquareMagnitude(tmp->Vertices[i].Point - tmp->pCenter);
        if (tf > r)
            r = tf;
    }

    //        tmp->fSquareRadius= 2000*2000+r;
    tmp->fSquareRadius += r;
}

bool TGeometry::Render() {}
//---------------------------------------------------------------------------

#pragma package(smart_init)
