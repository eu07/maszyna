//---------------------------------------------------------------------------

#include "system.hpp"
#include "classes.hpp"
#pragma hdrstop

#include "Curve.h"

__fastcall TCurve::TCurve()
{
    Values = NULL;
    iNumValues = 0;
    iNumCols = 0;
}

__fastcall TCurve::~TCurve()
{
    for (int i = 0; i < iNumValues; i++)
        SafeDelete(Values[i]);
    SafeDelete(Values);
}

bool __fastcall TCurve::Init(int n, int c)
{
    for (int i = 0; i < iNumValues; i++)
        SafeDelete(Values[i]);
    SafeDelete(Values);

    iNumValues = n;
    iNumCols = c;
    Values = new float *[iNumValues];
    for (int i = 0; i < iNumValues; i++)
        Values[i] = new float[iNumCols];

    for (int i = 0; i < iNumValues; i++)
        for (int j = 0; j < iNumCols; j++)
            Values[i][j] = 0;
}

float __fastcall TCurve::GetValue(int c, float p)
{
    int a = floor(p);
    int b = ceil(p);
    if (a < 0)
        return Values[0][c];
    if (b >= iNumValues)
        return Values[iNumValues - 1][c];
    p -= floor(p);
    return Values[a][c] * (1.0f - p) + Values[b][c] * (p);
}

bool __fastcall TCurve::SetValue(int c, float p, float v)
{
    int a = floor(p);
    int b = ceil(p);
    if (a < 0)
        return false;
    if (b >= iNumValues)
        return false;
    p -= floor(p);
    if (p < 0.5)
        Values[a][c] = v;
    else
        Values[b][c] = v;
    return true;
}

bool __fastcall TCurve::Load(TQueryParserComp *Parser)
{
    DecimalSeparator = '.';
    AnsiString Token;

    int n = Parser->GetNextSymbol().ToInt();
    int c = Parser->GetNextSymbol().ToInt();
    Init(n, c);

    n = 0;
    int i;
    while (!Parser->EOF && n < iNumValues)
    {
        for (i = 0; i < iNumCols; i++)
            Values[n][i] = Parser->GetNextSymbol().ToDouble();
        n++;
    }
    DecimalSeparator = ',';
}

bool __fastcall TCurve::LoadFromFile(AnsiString asName)
{
    DecimalSeparator = '.';
    TFileStream *fs;
    fs = new TFileStream(asName, fmOpenRead | fmShareCompat);
    AnsiString str = "xxx";
    int size = fs->Size;
    str.SetLength(size);
    fs->Read(str.c_str(), size);
    str += "";
    delete fs;
    TQueryParserComp *Parser;
    Parser = new TQueryParserComp(NULL);
    Parser->TextToParse = str;
    Parser->First();
    Load(Parser);

    delete Parser;
    DecimalSeparator = ',';
}

#include <stdio.h>

bool __fastcall TCurve::SaveToFile(AnsiString asName)
{

    DecimalSeparator = '.';
    FILE *stream = NULL;
    stream = fopen(asName.c_str(), "w");

    AnsiString str;
    str = AnsiString(iNumValues);
    fprintf(stream, str.c_str());
    fprintf(stream, "\n");
    for (int i = 0; i < iNumValues; i++)
    {
        str = "";
        if (i % 10 == 0)
            str += "\n";
        for (int j = 0; j < iNumCols; j++)
            str += FloatToStrF(Values[i][j], ffFixed, 6, 2) + AnsiString(" ");
        str += AnsiString("\n");
        fprintf(stream, str.c_str());
    }

    fclose(stream);
    DecimalSeparator = ',';
}

//---------------------------------------------------------------------------

#pragma package(smart_init)
