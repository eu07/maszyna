//---------------------------------------------------------------------------
/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak, Maciej Czapkiewicz and others

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "system.hpp"
#include "classes.hpp"
#pragma hdrstop

#include "Driver.h"

/*

Modu� obs�uguj�cy sterowanie pojazdami (sk�adami poci�g�w, samochodami).
Dzia�a zar�wno jako AI oraz przy prowadzeniu przed cz�owieka. W tym drugim
przypadku jedynie informuje za pomoc� napis�w o tym, co by zrobi� w tym
pierwszym. Obejmuje zar�wno maszynist� jak i kierownika poci�gu (dawanie
sygna�u do odjazdu).

Trzeba przenie�� tutaj zawarto�� ai_driver.pas przerobione na C++ oraz
niekt�re funkcje dotycz�ce sk�ad�w z DynObj.cpp.

*/