/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/
#pragma once

#include "QueryParserComp.hpp"
#include "usefull.h"

class TCurve
{
public:
	TCurve();
	~TCurve();
	bool init(int n, int c);
	float getValue(int c, float p);
	bool setValue(int c, float p, float v);
	bool load(TQueryParserComp* parser);
	bool loadFromFile(AnsiString asName);
	bool saveToFile(AnsiString asName);

	int iNumValues;
	int iNumCols;

private:
	float** values;
};
