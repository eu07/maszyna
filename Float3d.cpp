/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "float3d.h"

//---------------------------------------------------------------------------

void float4x4::Quaternion(float4 *q)
{ // konwersja kwaternionu obrotu na macierz obrotu
    float xx = q->x * q->x, yy = q->y * q->y, zz = q->z * q->z;
    float xy = q->x * q->y, xz = q->x * q->z, yz = q->y * q->z;
    float wx = q->w * q->x, wy = q->w * q->y, wz = q->w * q->z;
    e[0] = 1.0f - yy - yy - zz - zz;
    e[1] = xy + xy + wz + wz;
    e[2] = xz + xz - wy - wy;
    e[4] = xy + xy - wz - wz;
    e[5] = 1.0f - xx - xx - zz - zz;
    e[6] = yz + yz + wx + wx;
    e[8] = xz + xz + wy + wy;
    e[9] = yz + yz - wx - wx;
    e[10] = 1.0f - xx - xx - yy - yy;
    // czwartej kolumny i czwartego wiersza nie ruszamy
};
