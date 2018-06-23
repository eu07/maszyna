/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/
#include "system.hpp"
#include "classes.hpp"
#pragma hdrstop

#include "Curve.h"

TCurve::TCurve()
{
	values = nullptr;
	iNumValues = 0;
	iNumCols = 0;
}

TCurve::~TCurve()
{
	for (int i = 0; i < iNumValues; i++) {
		safeDelete(values[i]);
	}
	safeDelete(values);
}

bool TCurve::init(int n, int c)
{
	for (int i = 0; i < iNumValues; i++) {
		safeDelete(values[i]);
	}
	safeDelete(values);

	iNumValues = n;
	iNumCols = c;
	values = new float*[iNumValues];
	for (int i = 0; i < iNumValues; i++) {
		values[i] = new float[iNumCols];
	}

	for (int i = 0; i < iNumValues; i++) {
		for (int j = 0; j < iNumCols; j++) {
			values[i][j] = 0;
		}
	}
}

float TCurve::getValue(int c, float p)
{
	int a = floor(p);
	int b = ceil(p);
	if (a < 0) {
		return values[0][c];
	}
	if (b >= iNumValues) {
		return values[iNumValues - 1][c];
	}
	p -= floor(p);
	return values[a][c] * (1.0f - p) + values[b][c] * (p);
}

bool TCurve::setValue(int c, float p, float v)
{
	int a = floor(p);
	int b = ceil(p);
	if (a < 0) {
		return false;
	}
	if (b >= iNumValues) {
		return false;
	}
	p -= floor(p);
	if (p < 0.5) {
		Values[a][c] = v;
	} else {
		Values[b][c] = v;
	}
	return true;
}

bool TCurve::load(TQueryParserComp* parser)
{
	decimalSeparator = '.';
	AnsiString token;

	int n = parser->GetNextSymbol().ToInt();
	int c = parser->GetNextSymbol().ToInt();
	init(n, c);

	n = 0;
	int i;
	while (!parser->EOF && n < iNumValues) {
		for (i = 0; i < iNumCols; i++) {
			values[n][i] = parser->GetNextSymbol().ToDouble();
		}
		n++;
	}
	decimalSeparator = ',';
}

bool TCurve::lLoadFromFile(AnsiString asName)
{
	decimalSeparator = '.';
	TFileStream* fs;
	fs = new TFileStream(asName, fmOpenRead | fmShareCompat);
	AnsiString str = "xxx";
	int size = fs->Size;
	str.SetLength(size);
	fs->Read(str.c_str(), size);
	str += "";
	delete fs;
	TQueryParserComp* parser;
	parser = new TQueryParserComp(nullptr);
	parser->TextToParse = str;
	parser->First();
	load(parser);

	delete parser;
	decimalSeparator = ',';
}

#include <stdio.h>

bool TCurve::saveToFile(AnsiString asName)
{
	decimalSeparator = '.';
	FILE* stream = nullptr;
	stream = fopen(asName.c_str(), "w");

	AnsiString str;
	str = AnsiString(iNumValues);
	fprintf(stream, str.c_str());
	fprintf(stream, "\n");
	for (int i = 0; i < iNumValues; ++i) {
		str = "";
		if (i % 10 == 0) {
			str += "\n";
		}
		for (int j = 0; j < iNumCols; ++j) {
			str += FloatToStrF(Values[i][j], ffFixed, 6, 2) + AnsiString(" ");
		}
		str += AnsiString("\n");
		fprintf(stream, str.c_str());
	}

	fclose(stream);
	decimalSeparator = ',';
}

#pragma package(smart_init)
