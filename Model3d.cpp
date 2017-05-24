/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/
/*
MaSzyna EU07 locomotive simulator
Copyright (C) 2001-2004  Marcin Wozniak, Maciej Czapkiewicz and others

*/

#include "stdafx.h"
#include "Model3d.h"

#include "Globals.h"
#include "logs.h"
#include "mczapkie/mctools.h"
#include "Usefull.h"
#include "renderer.h"
#include "Timer.h"
#include "mtable.h"
#include "sn_utils.h"
//---------------------------------------------------------------------------

using namespace Mtable;

double TSubModel::fSquareDist = 0;
size_t TSubModel::iInstance; // numer renderowanego egzemplarza obiektu
texture_manager::size_type const *TSubModel::ReplacableSkinId = NULL;
int TSubModel::iAlpha = 0x30300030; // maska do testowania flag tekstur wymiennych
TModel3d *TSubModel::pRoot; // Ra: tymczasowo wskaźnik na model widoczny z submodelu
std::string *TSubModel::pasText;
// przykłady dla TSubModel::iAlpha:
// 0x30300030 - wszystkie bez kanału alfa
// 0x31310031 - tekstura -1 używana w danym cyklu, pozostałe nie
// 0x32320032 - tekstura -2 używana w danym cyklu, pozostałe nie
// 0x34340034 - tekstura -3 używana w danym cyklu, pozostałe nie
// 0x38380038 - tekstura -4 używana w danym cyklu, pozostałe nie
// 0x3F3F003F - wszystkie wymienne tekstury używane w danym cyklu
// Ale w TModel3d okerśla przezroczystość tekstur wymiennych!

TSubModel::TSubModel()
{
	::SecureZeroMemory(this, sizeof(TSubModel)); // istotne przy zapisywaniu wersji binarnej
	FirstInit();
};

void TSubModel::FirstInit()
{
	eType = TP_ROTATOR;
	Vertices = NULL;
	uiDisplayList = 0;
	iNumVerts = -1; // do sprawdzenia
	iVboPtr = -1;
	fLight = -1.0; //świetcenie wyłączone
	v_RotateAxis = float3(0, 0, 0);
	v_TransVector = float3(0, 0, 0);
	f_Angle = 0;
	b_Anim = at_None;
	b_aAnim = at_None;
	fVisible = 0.0; // zawsze widoczne
	iVisible = 1;
	fMatrix = NULL; // to samo co iMatrix=0;
	Next = NULL;
	Child = NULL;
	TextureID = 0;
	// TexAlpha=false;
	iFlags = 0x0200; // bit 9=1: submodel został utworzony a nie ustawiony na
					 // wczytany plik
					 // TexHash=false;
					 // Hits=NULL;
					 // CollisionPts=NULL;
					 // CollisionPtsCount=0;
	Opacity = 0.0f; // przy wczytywaniu modeli było dzielone przez 100...
	bWire = false;
	fWireSize = 0;
	fNearAttenStart = 40;
	fNearAttenEnd = 80;
	bUseNearAtten = false;
	iFarAttenDecay = 0;
	fFarDecayRadius = 100.0f;
	fCosFalloffAngle = 0.5f; // 120°?
	fCosHotspotAngle = 0.3f; // 145°?
	fCosViewAngle = 0;
	fSquareMaxDist = 10000 * 10000; // 10km
	fSquareMinDist = 0;
	iName = -1; // brak nazwy
	iTexture = 0; // brak tekstury
				  // asName="";
				  // asTexture="";
	pName = "";
	pTexture = "";
	f4Ambient[0] = f4Ambient[1] = f4Ambient[2] = f4Ambient[3] = 1.0; //{1,1,1,1};
	f4Diffuse[0] = f4Diffuse[1] = f4Diffuse[2] = f4Diffuse[3] = 1.0; //{1,1,1,1};
	f4Specular[0] = f4Specular[1] = f4Specular[2] = 0.0;
	f4Specular[3] = 1.0; //{0,0,0,1};
	f4Emision[0] = f4Emision[1] = f4Emision[2] = f4Emision[3] = 1.0;
	smLetter = NULL; // używany tylko roboczo dla TP_TEXT, do przyspieszenia wyświetlania
};

TSubModel::~TSubModel()
{
	if (uiDisplayList)
		glDeleteLists(uiDisplayList, 1);
	if (iFlags & 0x0200)
	{ // wczytany z pliku tekstowego musi sam posprzątać
	  // SafeDeleteArray(Indices);
		SafeDelete(Next);
		SafeDelete(Child);
		delete fMatrix; // własny transform trzeba usunąć (zawsze jeden)
		delete[] Vertices;
	}
	/*
	else
	{//wczytano z pliku binarnego (nie jest właścicielem tablic)
	}
	*/
	delete[] smLetter; // używany tylko roboczo dla TP_TEXT, do przyspieszenia
					   // wyświetlania
};

void TSubModel::TextureNameSet(const char *n)
{ // ustawienie nazwy submodelu, o
  // ile nie jest wczytany z E3D
	if (iFlags & 0x0200)
	{ // tylko jeżeli submodel zosta utworzony przez new
		pTexture = std::string(n);
	}
};

void TSubModel::NameSet(const char *n)
{ // ustawienie nazwy submodelu, o ile
  // nie jest wczytany z E3D
	if (iFlags & 0x0200)
		pName = std::string(n);
};

// int TSubModel::SeekFaceNormal(DWORD *Masks, int f,DWORD dwMask,vector3
// *pt,GLVERTEX
// *Vertices)
int TSubModel::SeekFaceNormal(unsigned int *Masks, int f, unsigned int dwMask, float3 *pt,
	float8 *Vertices)
{ // szukanie punktu stycznego
  // do (pt), zwraca numer
  // wierzchołka, a nie trójkąta
	int iNumFaces = iNumVerts / 3; // bo maska powierzchni jest jedna na trójkąt
								   // GLVERTEX *p; //roboczy wskaźnik
	float8 *p; // roboczy wskaźnik
	for (int i = f; i < iNumFaces; ++i) // pętla po trójkątach, od trójkąta (f)
		if (Masks[i] & dwMask) // jeśli wspólna maska powierzchni
		{
			p = Vertices + 3 * i;
			if (p->Point == *pt)
				return 3 * i;
			if ((++p)->Point == *pt)
				return 3 * i + 1;
			if ((++p)->Point == *pt)
				return 3 * i + 2;
		}
	return -1; // nie znaleziono stycznego wierzchołka
}

float emm1[] = { 1, 1, 1, 0 };
float emm2[] = { 0, 0, 0, 1 };

inline double readIntAsDouble(cParser &parser, int base = 255)
{
	int value = parser.getToken<int>(false);
	return (static_cast<double>(value) / base);
};

template <typename ColorT> inline void readColor(cParser &parser, ColorT *color)
{
    double discard;
    parser.getTokens(4, false);
    parser >> discard >> color[0] >> color[1] >> color[2];
    color[ 0 ] /= 255.0;
    color[ 1 ] /= 255.0;
    color[ 2 ] /= 255.0;
};

inline void readColor(cParser &parser, int &color)
{
	int r, g, b, discard;
	parser.getTokens(4, false);
	parser >> discard >> r >> g >> b;
	color = r + (g << 8) + (b << 16);
};
/*
inline void readMatrix(cParser& parser,matrix4x4& matrix)
{//Ra: wczytanie transforma
for (int x=0;x<=3;x++) //wiersze
for (int y=0;y<=3;y++) //kolumny
parser.getToken(matrix(x)[y]);
};
*/
inline void readMatrix(cParser &parser, float4x4 &matrix)
{ // Ra: wczytanie transforma
	parser.getTokens(16, false);
	for (int x = 0; x <= 3; ++x) // wiersze
		for (int y = 0; y <= 3; ++y) // kolumny
			parser >> matrix(x)[y];
};

int TSubModel::Load(cParser &parser, TModel3d *Model, int Pos, bool dynamic)
{ // Ra: VBO tworzone na poziomie modelu, a nie submodeli
    iNumVerts = 0;
    iVboPtr = Pos; // pozycja w VBO
    // TMaterialColorf Ambient,Diffuse,Specular;
    // GLuint TextureID;
    // char *extName;
    if (!parser.expectToken("type:"))
        Error("Model type parse failure!");
    {
        std::string type = parser.getToken<std::string>();
        if (type == "mesh")
            eType = GL_TRIANGLES; // submodel - trójkaty
        else if (type == "point")
            eType = GL_POINTS; // co to niby jest?
        else if (type == "freespotlight")
            eType = TP_FREESPOTLIGHT; //światełko
        else if (type == "text")
            eType = TP_TEXT; // wyświetlacz tekstowy (generator napisów)
        else if (type == "stars")
            eType = TP_STARS; // wiele punktów świetlnych
    };
    parser.ignoreToken();
    std::string token;
    // parser.getToken(token1); //ze zmianą na małe!
    parser.getTokens(1, false); // nazwa submodelu bez zmieny na małe
    parser >> token;
    NameSet(token.c_str());
    if (dynamic)
    { // dla pojazdu, blokujemy załączone submodele, które mogą być
        // nieobsługiwane
        if ( (token.size() >= 3)
          && (token.find("_on") + 3 == token.length())) // jeśli nazwa kończy się na "_on"
            iVisible = 0; // to domyślnie wyłączyć, żeby się nie nakładało z obiektem "_off"
    }
    else // dla pozostałych modeli blokujemy zapalone światła, które mogą być
        // nieobsługiwane
        if (token.compare(0, 8, "Light_On") == 0) // jeśli nazwa zaczyna się od "Light_On"
        iVisible = 0; // to domyślnie wyłączyć, żeby się nie nakładało z obiektem
    // "Light_Off"

    if (parser.expectToken("anim:")) // Ra: ta informacja by się przydała!
    { // rodzaj animacji
        std::string type = parser.getToken<std::string>();
        if (type != "false")
        {
            iFlags |= 0x4000; // jak animacja, to trzeba przechowywać macierz zawsze
            if (type == "seconds_jump")
                b_Anim = b_aAnim = at_SecondsJump; // sekundy z przeskokiem
            else if (type == "minutes_jump")
                b_Anim = b_aAnim = at_MinutesJump; // minuty z przeskokiem
            else if (type == "hours_jump")
                b_Anim = b_aAnim = at_HoursJump; // godziny z przeskokiem
            else if (type == "hours24_jump")
                b_Anim = b_aAnim = at_Hours24Jump; // godziny z przeskokiem
            else if (type == "seconds")
                b_Anim = b_aAnim = at_Seconds; // minuty płynnie
            else if (type == "minutes")
                b_Anim = b_aAnim = at_Minutes; // minuty płynnie
            else if (type == "hours")
                b_Anim = b_aAnim = at_Hours; // godziny płynnie
            else if (type == "hours24")
                b_Anim = b_aAnim = at_Hours24; // godziny płynnie
            else if (type == "billboard")
                b_Anim = b_aAnim = at_Billboard; // obrót w pionie do kamery
            else if (type == "wind")
                b_Anim = b_aAnim = at_Wind; // ruch pod wpływem wiatru
            else if (type == "sky")
                b_Anim = b_aAnim = at_Sky; // aniamacja nieba
            else if (type == "ik")
                b_Anim = b_aAnim = at_IK; // IK: zadający
            else if (type == "ik11")
                b_Anim = b_aAnim = at_IK11; // IK: kierunkowany
            else if (type == "ik21")
                b_Anim = b_aAnim = at_IK21; // IK: kierunkowany
            else if (type == "ik22")
                b_Anim = b_aAnim = at_IK22; // IK: kierunkowany
            else if (type == "digital")
                b_Anim = b_aAnim = at_Digital; // licznik mechaniczny
            else if (type == "digiclk")
                b_Anim = b_aAnim = at_DigiClk; // zegar cyfrowy
            else
                b_Anim = b_aAnim = at_Undefined; // nieznana forma animacji
        }
    }
    if (eType < TP_ROTATOR)
        readColor(parser, f4Ambient); // ignoruje token przed
    readColor(parser, f4Diffuse);
    if (eType < TP_ROTATOR)
        readColor(parser, f4Specular);
    parser.ignoreTokens(1); // zignorowanie nazwy "SelfIllum:"
    {
        std::string light = parser.getToken<std::string>();
        if (light == "true")
            fLight = 2.0; // zawsze świeci
        else if (light == "false")
            fLight = -1.0; // zawsze ciemy
        else
            fLight = std::stod(light);
    };
    if (eType == TP_FREESPOTLIGHT)
    {
        if (!parser.expectToken("nearattenstart:"))
        {
            Error("Model light parse failure!");
        }
        std::string discard;
        parser.getTokens(13, false);
        parser
            >> fNearAttenStart
            >> discard >> fNearAttenEnd
            >> discard >> bUseNearAtten
            >> discard >> iFarAttenDecay
            >> discard >> fFarDecayRadius
            >> discard >> fCosFalloffAngle // kąt liczony dla średnicy, a nie promienia
            >> discard >> fCosHotspotAngle; // kąt liczony dla średnicy, a nie promienia
        // convert conve parameters if specified in degrees
        if( fCosFalloffAngle > 1.0 ) {
            fCosFalloffAngle = std::cos( DegToRad( 0.5f * fCosFalloffAngle ) );
        }
        if( fCosHotspotAngle > 1.0 ) {
            fCosHotspotAngle = std::cos( DegToRad( 0.5f * fCosHotspotAngle ) );
        }
        iNumVerts = 1;
/*
        iFlags |= 0x4010; // rysowane w cyklu nieprzezroczystych, macierz musi zostać bez zmiany
*/
        iFlags |= 0x4030; // drawn both in solid (light point) and transparent (light glare) phases
    }
    else if (eType < TP_ROTATOR)
    {
        std::string discard;
        parser.getTokens(5, false);
        parser >> discard >> bWire >> discard >> fWireSize >> discard;
        Opacity = readIntAsDouble(parser,
                                  100.0f); // wymagane jest 0 dla szyb, 100 idzie w nieprzezroczyste
        if (Opacity > 1.0f)
            Opacity *= 0.01f; // w 2013 był błąd i aby go obejść, trzeba było wpisać 10000.0
        if ((Global::iConvertModels & 1) == 0) // dla zgodności wstecz
            Opacity = 0.0; // wszystko idzie w przezroczyste albo zależnie od tekstury
        if (!parser.expectToken("map:"))
            Error("Model map parse failure!");
        std::string texture = parser.getToken<std::string>();
        if (texture == "none")
        { // rysowanie podanym kolorem
            TextureID = 0;
            iFlags |= 0x10; // rysowane w cyklu nieprzezroczystych
        }
        else if (texture.find("replacableskin") != texture.npos)
        { // McZapkie-060702: zmienialne skory modelu
            TextureID = -1;
            iFlags |= (Opacity < 1.0) ? 1 : 0x10; // zmienna tekstura 1
        }
        else if (texture == "-1")
        {
            TextureID = -1;
            iFlags |= (Opacity < 1.0) ? 1 : 0x10; // zmienna tekstura 1
        }
        else if (texture == "-2")
        {
            TextureID = -2;
            iFlags |= (Opacity < 1.0) ? 2 : 0x10; // zmienna tekstura 2
        }
        else if (texture == "-3")
        {
            TextureID = -3;
            iFlags |= (Opacity < 1.0) ? 4 : 0x10; // zmienna tekstura 3
        }
        else if (texture == "-4")
        {
            TextureID = -4;
            iFlags |= (Opacity < 1.0) ? 8 : 0x10; // zmienna tekstura 4
        }
        else
        { // jeśli tylko nazwa pliku, to dawać bieżącą ścieżkę do tekstur
            // asTexture=AnsiString(texture.c_str()); //zapamiętanie nazwy tekstury
            TextureNameSet(texture.c_str());
            if (texture.find_first_of("/\\") == texture.npos)
                texture.insert(0, Global::asCurrentTexturePath.c_str());
            TextureID = GfxRenderer.GetTextureId( texture, szTexturePath );
            // TexAlpha=TTexturesManager::GetAlpha(TextureID);
            // iFlags|=TexAlpha?0x20:0x10; //0x10-nieprzezroczysta, 0x20-przezroczysta
            iFlags |=
                ( GfxRenderer.Texture(TextureID).has_alpha ?
                    0x20 :
                    0x10 ); // 0x10-nieprzezroczysta, 0x20-przezroczysta
            // renderowanie w cyklu przezroczystych tylko jeśli:
            // 1. Opacity=0 (przejściowo <1, czy tam <100) oraz
            // 2. tekstura ma przezroczystość
        };
    }
    else
        iFlags |= 0x10;

    // visibility range
	std::string discard;
	parser.getTokens(5, false);
	parser >> discard >> fSquareMaxDist >> discard >> fSquareMinDist >> discard;

    if( fSquareMaxDist <= 0.0 ) {
        // 15km to więcej, niż się obecnie wyświetla
        fSquareMaxDist = 15000.0;
    }
	fSquareMaxDist *= fSquareMaxDist;
	fSquareMinDist *= fSquareMinDist;

    // transformation matrix
	fMatrix = new float4x4();
	readMatrix(parser, *fMatrix); // wczytanie transform
	if (!fMatrix->IdentityIs())
		iFlags |= 0x8000; // transform niejedynkowy - trzeba go przechować
	int iNumFaces; // ilość trójkątów
	unsigned int *sg; // maski przynależności trójkątów do powierzchni
	if (eType < TP_ROTATOR)
	{ // wczytywanie wierzchołków
		parser.getTokens(2, false);
		parser >> discard >> token;
		// Ra 15-01: to wczytać jako tekst - jeśli pierwszy znak zawiera "*", to
		// dalej będzie nazwa wcześniejszego submodelu, z którego należy wziąć
		// wierzchołki
		// zapewni to jakąś zgodność wstecz, bo zamiast liczby będzie ciąg, którego
		// wartość powinna być uznana jako zerowa
		// parser.getToken(iNumVerts);
		if (token[0] == '*')
		{ // jeśli pierwszy znak jest gwiazdką, poszukać
		  // submodelu o nazwie bez tej gwiazdki i wziąć z
		  // niego wierzchołki
			Error("Verticles reference not yet supported!");
		}
		else
		{ // normalna lista wierzchołków
			iNumVerts = atoi(token.c_str());
			if (iNumVerts % 3)
			{
				iNumVerts = 0;
				Error("Mesh error, (iNumVertices=" + std::to_string(iNumVerts) + ")%3<>0");
				return 0;
			}
			// Vertices=new GLVERTEX[iNumVerts];
			if (iNumVerts)
			{
				Vertices = new float8[iNumVerts];
				iNumFaces = iNumVerts / 3;
				sg = new unsigned int[iNumFaces]; // maski powierzchni: 0 oznacza brak
												  // użredniania wektorów normalnych
				int *wsp = new int[iNumVerts]; // z którego wierzchołka kopiować wektor
											   // normalny
				int maska = 0;
				for (int i = 0; i < iNumVerts; i++)
				{ // Ra: z konwersją na układ scenerii - będzie wydajniejsze
				  // wyświetlanie
					wsp[i] = -1; // wektory normalne nie są policzone dla tego wierzchołka
					if ((i % 3) == 0)
					{ // jeśli będzie maska -1, to dalej będą
					  // wierzchołki z wektorami normalnymi, podanymi
					  // jawnie
						maska = parser.getToken<int>(false); // maska powierzchni trójkąta
						sg[i / 3] = (maska == -1) ? 0 : maska; // dla maski -1 będzie 0,
															   // czyli nie ma wspólnych
															   // wektorów normalnych
					}
					parser.getTokens(3, false);
					parser >> Vertices[i].Point.x >> Vertices[i].Point.y >> Vertices[i].Point.z;
					if (maska == -1)
					{ // jeśli wektory normalne podane jawnie
						parser.getTokens(3, false);
						parser >> Vertices[i].Normal.x >> Vertices[i].Normal.y >>
							Vertices[i].Normal.z;
						wsp[i] = i; // wektory normalne "są już policzone"
					}
					parser.getTokens(2, false);
					parser >> Vertices[i].tu >> Vertices[i].tv;
					if (i % 3 == 2) // jeżeli wczytano 3 punkty
					{
						if (Vertices[i].Point == Vertices[i - 1].Point ||
							Vertices[i - 1].Point == Vertices[i - 2].Point ||
							Vertices[i - 2].Point == Vertices[i].Point)
						{ // jeżeli punkty się nakładają na siebie
							--iNumFaces; // o jeden trójkąt mniej
							iNumVerts -= 3; // czyli o 3 wierzchołki
							i -= 3; // wczytanie kolejnego w to miejsce
							WriteLog(std::string("Degenerated triangle ignored in: \"") + pName +
								"\", verticle " + std::to_string(i));
						}
						if (i > 0) // jeśli pierwszy trójkąt będzie zdegenerowany, to
								   // zostanie usunięty i nie ma co sprawdzać
							if (((Vertices[i].Point - Vertices[i - 1].Point).Length() > 1000.0) ||
								((Vertices[i - 1].Point - Vertices[i - 2].Point).Length() >
									1000.0) ||
									((Vertices[i - 2].Point - Vertices[i].Point).Length() > 1000.0))
							{ // jeżeli są dalej niż 2km od siebie //Ra 15-01:
							  // obiekt wstawiany nie powinien być większy niż
							  // 300m (trójkąty terenu w E3D mogą mieć 1.5km)
								--iNumFaces; // o jeden trójkąt mniej
								iNumVerts -= 3; // czyli o 3 wierzchołki
								i -= 3; // wczytanie kolejnego w to miejsce
								WriteLog(std::string("Too large triangle ignored in: \"") + pName +
									"\"");
							}
					}
				}
				int i; // indeks dla trójkątów
				float3 *n = new float3[iNumFaces]; // tablica wektorów normalnych dla trójkątów
				for (i = 0; i < iNumFaces; i++) // pętla po trójkątach - będzie
												// szybciej, jak wstępnie przeliczymy
												// normalne trójkątów
					n[i] = SafeNormalize(
						CrossProduct(Vertices[i * 3].Point - Vertices[i * 3 + 1].Point,
							Vertices[i * 3].Point - Vertices[i * 3 + 2].Point));
				int v; // indeks dla wierzchołków
				int f; // numer trójkąta stycznego
				float3 norm; // roboczy wektor normalny
				for (v = 0; v < iNumVerts; v++)
				{ // pętla po wierzchołkach trójkątów
					if (wsp[v] >= 0) // jeśli już był liczony wektor normalny z użyciem
									 // tego wierzchołka
						Vertices[v].Normal =
						Vertices[wsp[v]].Normal; // to wystarczy skopiować policzony wcześniej
					else
					{ // inaczej musimy dopiero policzyć
						i = v / 3; // numer trójkąta
						norm = float3(0, 0, 0); // liczenie zaczynamy od zera
						f = v; // zaczynamy dodawanie wektorów normalnych od własnego
						while (f >= 0)
						{ // sumowanie z wektorem normalnym sąsiada (włącznie
						  // ze sobą)
							wsp[f] = v; // informacja, że w tym wierzchołku jest już policzony
										// wektor normalny
							norm += n[f / 3];
							f = SeekFaceNormal(sg, f / 3 + 1, sg[i], &Vertices[v].Point,
								Vertices); // i szukanie od kolejnego trójkąta
						}
						// Ra 15-01: należało by jeszcze uwzględnić skalowanie wprowadzane
						// przez transformy, aby normalne po przeskalowaniu były jednostkowe
						Vertices[v].Normal =
							SafeNormalize(norm); // przepisanie do wierzchołka trójkąta
					}
				}
				delete[] wsp;
				delete[] n;
				delete[] sg;
			}
			else // gdy brak wierzchołków
			{
				eType = TP_ROTATOR; // submodel pomocniczy, ma tylko macierz przekształcenia
				iVboPtr = iNumVerts = 0; // dla formalności
			}
		} // obsługa submodelu z własną listą wierzchołków
	}
	else if (eType == TP_STARS)
	{ // punkty świecące dookólnie - składnia jak
	  // dla smt_Mesh
		std::string discard;
		parser.getTokens(2, false);
		parser >> discard >> iNumVerts;
		// Vertices=new GLVERTEX[iNumVerts];
		Vertices = new float8[iNumVerts];
		int i, j;
		for (i = 0; i < iNumVerts; i++)
		{
			if (i % 3 == 0)
			{
				parser.ignoreToken(); // maska powierzchni trójkąta
			}
			parser.getTokens(5, false);
			parser >> Vertices[i].Point.x >> Vertices[i].Point.y >> Vertices[i].Point.z >>
				j // zakodowany kolor
				>> discard;
			Vertices[i].Normal.x = ((j) & 0xFF) / 255.0; // R
			Vertices[i].Normal.y = ((j >> 8) & 0xFF) / 255.0; // G
			Vertices[i].Normal.z = ((j >> 16) & 0xFF) / 255.0; // B
		}
	}
	// Visible=true; //się potem wyłączy w razie potrzeby
	// iFlags|=0x0200; //wczytano z pliku tekstowego (jest właścicielem tablic)
	if (iNumVerts < 1)
		iFlags &= ~0x3F; // cykl renderowania uzależniony od potomnych
	return iNumVerts; // do określenia wielkości VBO
};

int TSubModel::TriangleAdd(TModel3d *m, texture_manager::size_type tex, int tri)
{ // dodanie trójkątów do submodelu, używane
    // przy tworzeniu E3D terenu
    TSubModel *s = this;
    while (s ? (s->TextureID != tex) : false)
    { // szukanie submodelu o danej teksturze
        if (s == this)
            s = Child;
        else
            s = s->Next;
    }
    if (!s)
    {
        if (TextureID <= 0)
            s = this; // użycie głównego
        else
        { // dodanie nowego submodelu do listy potomnych
            s = new TSubModel();
            m->AddTo(this, s);
        }
        // s->asTexture=AnsiString(TTexturesManager::GetName(tex).c_str());
        s->TextureNameSet(GfxRenderer.Texture(tex).name.c_str());
        s->TextureID = tex;
        s->eType = GL_TRIANGLES;
        // iAnimOwner=0; //roboczy wskaźnik na wierzchołek
    }
    if (s->iNumVerts < 0)
        s->iNumVerts = tri; // bo na początku jest -1, czyli że nie wiadomo
    else
        s->iNumVerts += tri; // aktualizacja ilości wierzchołków
    return s->iNumVerts - tri; // zwraca pozycję tych trójkątów w submodelu
};

float8 *TSubModel::TrianglePtr(int tex, int pos, int *la, int *ld, int *ls)
{ // zwraca wskaźnik do wypełnienia tabeli wierzchołków, używane
  // przy tworzeniu E3D terenu
	TSubModel *s = this;
	while (s ? s->TextureID != tex : false)
	{ // szukanie submodelu o danej teksturze
		if (s == this)
			s = Child;
		else
			s = s->Next;
	}
	if (!s)
		return NULL; // coś nie tak poszło
	if (!s->Vertices)
	{ // utworznie tabeli trójkątów
		s->Vertices = new float8[s->iNumVerts];
		// iVboPtr=pos; //pozycja submodelu w tabeli wierzchołków
		// pos+=iNumVerts; //rezerwacja miejsca w tabeli
		s->iVboPtr = iInstance; // pozycja submodelu w tabeli wierzchołków
		iInstance += s->iNumVerts; // pozycja dla następnego
	}
	s->ColorsSet(la, ld, ls); // ustawienie kolorów świateł
	return s->Vertices + pos; // wskaźnik na wolne miejsce w tabeli wierzchołków
};

void TSubModel::DisplayLists()
{ // utworznie po jednej skompilowanej liście dla
  // każdego submodelu
	if (Global::bUseVBO)
		return; // Ra: przy VBO to się nie przyda
				// iFlags|=0x4000; //wyłączenie przeliczania wierzchołków, bo nie są zachowane
	if (eType < TP_ROTATOR)
	{
		if (iNumVerts > 0)
		{
			uiDisplayList = glGenLists(1);
			glNewList(uiDisplayList, GL_COMPILE);
#ifdef USE_VERTEX_ARRAYS
								   // ShaXbee-121209: przekazywanie wierzcholkow hurtem
			glVertexPointer(3, GL_DOUBLE, sizeof(GLVERTEX), &Vertices[0].Point.x);
			glNormalPointer(GL_DOUBLE, sizeof(GLVERTEX), &Vertices[0].Normal.x);
			glTexCoordPointer(2, GL_FLOAT, sizeof(GLVERTEX), &Vertices[0].tu);
			glDrawArrays(eType, 0, iNumVerts);
#else
			glBegin(eType);
			for (int i = 0; i < iNumVerts; i++)
			{
				/*
				glNormal3dv(&Vertices[i].Normal.x);
				glTexCoord2f(Vertices[i].tu,Vertices[i].tv);
				glVertex3dv(&Vertices[i].Point.x);
				*/
				glNormal3fv(&Vertices[i].Normal.x);
				glTexCoord2f(Vertices[i].tu, Vertices[i].tv);
				glVertex3fv(&Vertices[i].Point.x);
			};
			glEnd();
#endif
            glEndList();
        }
    }
    else if (eType == TP_FREESPOTLIGHT)
    {
        uiDisplayList = glGenLists(1);
        glNewList(uiDisplayList, GL_COMPILE);
        glBegin(GL_POINTS);
        glVertex3f( 0.0f, 0.0f, -0.05f ); // shift point towards the viewer, to avoid z-fighting with the light polygons
        glEnd();
        glEndList();
    }
    else if (eType == TP_STARS)
    { // punkty świecące dookólnie
        uiDisplayList = glGenLists(1);
        glNewList(uiDisplayList, GL_COMPILE);
        glBegin(GL_POINTS);
        for (int i = 0; i < iNumVerts; i++)
        {
            glColor3f(Vertices[i].Normal.x, Vertices[i].Normal.y, Vertices[i].Normal.z);
            glVertex3fv(&Vertices[i].Point.x);
        };
        glEnd();
        glEndList();
    }
    // SafeDeleteArray(Vertices); //przy VBO muszą zostać do załadowania całego
    // modelu
    if (Child)
        Child->DisplayLists();
    if (Next)
        Next->DisplayLists();
};

void TSubModel::InitialRotate(bool doit)
{ // konwersja układu współrzędnych na zgodny ze scenerią
    if (iFlags & 0xC000) // jeśli jest animacja albo niejednostkowy transform
    { // niejednostkowy transform jest mnożony i wystarczy zabawy
        if (doit)
        { // obrót lewostronny
            if (!fMatrix) // macierzy może nie być w dodanym "bananie"
            {
                fMatrix = new float4x4(); // tworzy macierz o przypadkowej zawartości
                fMatrix->Identity(); // a zaczynamy obracanie od jednostkowej
            }
            iFlags |= 0x8000; // po obróceniu będzie raczej niejedynkowy matrix
            fMatrix->InitialRotate(); // zmiana znaku X oraz zamiana Y i Z
            if (fMatrix->IdentityIs())
                iFlags &= ~0x8000; // jednak jednostkowa po obróceniu
        }
        if (Child)
            Child->InitialRotate(false); // potomnych nie obracamy już, tylko
        // ewentualnie optymalizujemy
        else if (Global::iConvertModels & 2) // optymalizacja jest opcjonalna
            if ((iFlags & 0xC000) == 0x8000) // o ile nie ma animacji
            { // jak nie ma potomnych, można wymnożyć przez transform i wyjedynkować
                // go
                float4x4 *mat = GetMatrix(); // transform submodelu
                if (Vertices)
                {
                    for (int i = 0; i < iNumVerts; ++i)
                        Vertices[i].Point = (*mat) * Vertices[i].Point;
                    (*mat)(3)[0] = (*mat)(3)[1] = (*mat)(3)[2] =
                        0.0; // zerujemy przesunięcie przed obracaniem normalnych
                    if (eType != TP_STARS) // gwiazdki mają kolory zamiast normalnych, to
                        // ich wtedy nie ruszamy
                        for (int i = 0; i < iNumVerts; ++i)
                            Vertices[i].Normal = SafeNormalize((*mat) * Vertices[i].Normal);
                }
                mat->Identity(); // jedynkowanie transformu po przeliczeniu wierzchołków
                iFlags &= ~0x8000; // transform jedynkowy
            }
    }
    else // jak jest jednostkowy i nie ma animacji
        if (doit)
    { // jeśli jest jednostkowy transform, to przeliczamy
        // wierzchołki, a mnożenie podajemy dalej
        double t;
        if (Vertices)
            for (int i = 0; i < iNumVerts; ++i)
            {
                Vertices[i].Point.x = -Vertices[i].Point.x; // zmiana znaku X
                t = Vertices[i].Point.y; // zamiana Y i Z
                Vertices[i].Point.y = Vertices[i].Point.z;
                Vertices[i].Point.z = t;
                // wektory normalne również trzeba przekształcić, bo się źle oświetlają
                if( eType != TP_STARS ) {
                    // gwiazdki mają kolory zamiast normalnych, to // ich wtedy nie ruszamy
                    Vertices[ i ].Normal.x = -Vertices[ i ].Normal.x; // zmiana znaku X
                    t = Vertices[ i ].Normal.y; // zamiana Y i Z
                    Vertices[ i ].Normal.y = Vertices[ i ].Normal.z;
                    Vertices[ i ].Normal.z = t;
                }
            }
        if (Child)
            Child->InitialRotate(doit); // potomne ewentualnie obrócimy
    }
    if (Next)
        Next->InitialRotate(doit);
};

void TSubModel::ChildAdd(TSubModel *SubModel)
{ // dodanie submodelu potemnego (uzależnionego)
  // Ra: zmiana kolejności, żeby kolejne móc renderować po aktualnym (było
  // przed)
	if (SubModel)
		SubModel->NextAdd(Child); // Ra: zmiana kolejności renderowania
	Child = SubModel;
};

void TSubModel::NextAdd(TSubModel *SubModel)
{ // dodanie submodelu kolejnego (wspólny przodek)
	if (Next)
		Next->NextAdd(SubModel);
	else
		Next = SubModel;
};

int TSubModel::FlagsCheck()
{ // analiza koniecznych zmian pomiędzy submodelami
  // samo pomijanie glBindTexture() nie poprawi wydajności
  // ale można sprawdzić, czy można w ogóle pominąć kod do tekstur (sprawdzanie
  // replaceskin)
	int i = 0;
	if (Child)
	{ // Child jest renderowany po danym submodelu
		if (Child->TextureID) // o ile ma teksturę
			if (Child->TextureID != TextureID) // i jest ona inna niż rodzica
				Child->iFlags |= 0x80; // to trzeba sprawdzać, jak z teksturami jest
		i = Child->FlagsCheck();
		iFlags |= 0x00FF0000 & ((i << 16) | (i) | (i >> 8)); // potomny, rodzeństwo i dzieci
		if (eType == TP_TEXT)
		{ // wyłączenie renderowania Next dla znaków
		  // wyświetlacza tekstowego
			TSubModel *p = Child;
			while (p)
			{
				p->iFlags &= 0xC0FFFFFF;
				p = p->Next;
			}
		}
	}
	if (Next)
	{ // Next jest renderowany po danym submodelu (kolejność odwrócona
	  // po wczytaniu T3D)
		if (TextureID) // o ile dany ma teksturę
			if ((TextureID != Next->TextureID) ||
				(i & 0x00800000)) // a ma inną albo dzieci zmieniają
				iFlags |= 0x80; // to dany submodel musi sobie ją ustawiać
		i = Next->FlagsCheck();
		iFlags |= 0xFF000000 & ((i << 24) | (i << 8) | (i)); // następny, kolejne i ich dzieci
															 // tekstury nie ustawiamy tylko wtedy, gdy jest taka sama jak Next i jego
															 // dzieci nie zmieniają
	}
	return iFlags;
};

void TSubModel::SetRotate(float3 vNewRotateAxis, float fNewAngle)
{ // obrócenie submodelu wg podanej
  // osi (np. wskazówki w kabinie)
	v_RotateAxis = vNewRotateAxis;
	f_Angle = fNewAngle;
	if (fNewAngle != 0.0)
	{
		b_Anim = at_Rotate;
		b_aAnim = at_Rotate;
	}
	iAnimOwner = iInstance; // zapamiętanie czyja jest animacja
}

void TSubModel::SetRotateXYZ(float3 vNewAngles)
{ // obrócenie submodelu o
  // podane kąty wokół osi
  // lokalnego układu
	v_Angles = vNewAngles;
	b_Anim = at_RotateXYZ;
	b_aAnim = at_RotateXYZ;
	iAnimOwner = iInstance; // zapamiętanie czyja jest animacja
}

void TSubModel::SetRotateXYZ(vector3 vNewAngles)
{ // obrócenie submodelu o
  // podane kąty wokół osi
  // lokalnego układu
	v_Angles.x = vNewAngles.x;
	v_Angles.y = vNewAngles.y;
	v_Angles.z = vNewAngles.z;
	b_Anim = at_RotateXYZ;
	b_aAnim = at_RotateXYZ;
	iAnimOwner = iInstance; // zapamiętanie czyja jest animacja
}

void TSubModel::SetTranslate(float3 vNewTransVector)
{ // przesunięcie submodelu (np. w kabinie)
	v_TransVector = vNewTransVector;
	b_Anim = at_Translate;
	b_aAnim = at_Translate;
	iAnimOwner = iInstance; // zapamiętanie czyja jest animacja
}

void TSubModel::SetTranslate(vector3 vNewTransVector)
{ // przesunięcie submodelu (np. w kabinie)
	v_TransVector.x = vNewTransVector.x;
	v_TransVector.y = vNewTransVector.y;
	v_TransVector.z = vNewTransVector.z;
	b_Anim = at_Translate;
	b_aAnim = at_Translate;
	iAnimOwner = iInstance; // zapamiętanie czyja jest animacja
}

void TSubModel::SetRotateIK1(float3 vNewAngles)
{ // obrócenie submodelu o
  // podane kąty wokół osi
  // lokalnego układu
	v_Angles = vNewAngles;
	iAnimOwner = iInstance; // zapamiętanie czyja jest animacja
}

struct ToLower
{
	char operator()(char input)
	{
		return tolower(input);
	}
};

TSubModel *TSubModel::GetFromName(std::string const &search, bool i)
{
	return GetFromName(search.c_str(), i);
};

TSubModel *TSubModel::GetFromName(char const *search, bool i)
{
	TSubModel *result;
	// std::transform(search.begin(),search.end(),search.begin(),ToLower());
	// search=search.LowerCase();
	// AnsiString name=AnsiString();
	std::string search_lc = std::string(search);
	if (i)
		std::transform(search_lc.begin(), search_lc.end(), search_lc.begin(), ::tolower);
	std::string pName_lc = pName;
	if (i)
		std::transform(pName_lc.begin(), pName_lc.end(), pName_lc.begin(), ::tolower);
	if (pName.size() && search)
		if (pName_lc == search_lc)
			return this;
	if (Next)
	{
		result = Next->GetFromName(search);
		if (result)
			return result;
	}
	if (Child)
	{
		result = Child->GetFromName(search);
		if (result)
			return result;
	}
	return NULL;
};

// WORD hbIndices[18]={3,0,1,5,4,2,1,0,4,1,5,3,2,3,5,2,4,0};

void TSubModel::RaAnimation(TAnimType a)
{ // wykonanie animacji niezależnie od renderowania
	switch (a)
	{ // korekcja położenia, jeśli submodel jest animowany
	case at_Translate: // Ra: było "true"
		if (iAnimOwner != iInstance)
			break; // cudza animacja
		glTranslatef(v_TransVector.x, v_TransVector.y, v_TransVector.z);
		break;
	case at_Rotate: // Ra: było "true"
		if (iAnimOwner != iInstance)
			break; // cudza animacja
		glRotatef(f_Angle, v_RotateAxis.x, v_RotateAxis.y, v_RotateAxis.z);
		break;
	case at_RotateXYZ:
		if (iAnimOwner != iInstance)
			break; // cudza animacja
		glTranslatef(v_TransVector.x, v_TransVector.y, v_TransVector.z);
		glRotatef(v_Angles.x, 1.0f, 0.0f, 0.0f);
		glRotatef(v_Angles.y, 0.0f, 1.0f, 0.0f);
		glRotatef(v_Angles.z, 0.0f, 0.0f, 1.0f);
		break;
	case at_SecondsJump: // sekundy z przeskokiem
		glRotatef(simulation::Time.data().wSecond * 6.0, 0.0, 1.0, 0.0);
		break;
	case at_MinutesJump: // minuty z przeskokiem
		glRotatef(simulation::Time.data().wMinute * 6.0, 0.0, 1.0, 0.0);
		break;
	case at_HoursJump: // godziny skokowo 12h/360°
		glRotatef(simulation::Time.data().wHour * 30.0 * 0.5, 0.0, 1.0, 0.0);
		break;
	case at_Hours24Jump: // godziny skokowo 24h/360°
		glRotatef(simulation::Time.data().wHour * 15.0 * 0.25, 0.0, 1.0, 0.0);
		break;
	case at_Seconds: // sekundy płynnie
		glRotatef(simulation::Time.second() * 6.0, 0.0, 1.0, 0.0);
		break;
	case at_Minutes: // minuty płynnie
		glRotatef(simulation::Time.data().wMinute * 6.0 + simulation::Time.second() * 0.1, 0.0, 1.0, 0.0);
		break;
	case at_Hours: // godziny płynnie 12h/360°
		glRotatef(2.0 * Global::fTimeAngleDeg, 0.0, 1.0, 0.0);
		break;
	case at_Hours24: // godziny płynnie 24h/360°
		glRotatef(Global::fTimeAngleDeg, 0.0, 1.0, 0.0);
		break;
	case at_Billboard: // obrót w pionie do kamery
	{
/*
		matrix4x4 mat; // potrzebujemy współrzędne przesunięcia środka układu
					   // współrzędnych submodelu
		glGetDoublev(GL_MODELVIEW_MATRIX, mat.getArray()); // pobranie aktualnej matrycy
*/
        matrix4x4 mat; mat.OpenGL_Matrix( OpenGLMatrices.data_array( GL_MODELVIEW ) );
		float3 gdzie = float3(mat[3][0], mat[3][1], mat[3][2]); // początek układu współrzędnych submodelu względem kamery
		glLoadIdentity(); // macierz jedynkowa
		glTranslatef(gdzie.x, gdzie.y, gdzie.z); // początek układu zostaje bez
												 // zmian
		glRotated(atan2(gdzie.x, gdzie.z) * 180.0 / M_PI, 0.0, 1.0,
			0.0); // jedynie obracamy w pionie o kąt
	}
	break;
	case at_Wind: // ruch pod wpływem wiatru (wiatr będziemy liczyć potem...)
		glRotated(1.5 * std::sin(M_PI * simulation::Time.second() / 6.0), 0.0, 1.0, 0.0);
		break;
	case at_Sky: // animacja nieba
		glRotated(Global::fLatitudeDeg, 1.0, 0.0, 0.0); // ustawienie osi OY na północ
														// glRotatef(Global::fTimeAngleDeg,0.0,1.0,0.0); //obrót dobowy osi OX
		glRotated(-fmod(Global::fTimeAngleDeg, 360.0), 0.0, 1.0, 0.0); // obrót dobowy osi OX
		break;
	case at_IK11: // ostatni element animacji szkieletowej (podudzie, stopa)
		glRotatef(v_Angles.z, 0.0f, 1.0f, 0.0f); // obrót względem osi pionowej (azymut)
		glRotatef(v_Angles.x, 1.0f, 0.0f, 0.0f); // obrót względem poziomu (deklinacja)
		break;
	case at_DigiClk: // animacja zegara cyfrowego
	{ // ustawienie animacji w submodelach potomnych
		TSubModel *sm = ChildGet();
		do
		{ // pętla po submodelach potomnych i obracanie ich o kąt zależy od czasu
			if (sm->pName.size())
			{ // musi mieć niepustą nazwę
				if ((sm->pName[0]) >= '0')
					if ((sm->pName[0]) <= '5') // zegarek ma 6 cyfr maksymalnie
						sm->SetRotate(float3(0, 1, 0),
							-Global::fClockAngleDeg[(sm->pName[0]) - '0']);
			}
			sm = sm->NextGet();
		} while (sm);
	}
	break;
	}
	if (mAnimMatrix) // można by to dać np. do at_Translate
	{
		glMultMatrixf(mAnimMatrix->readArray());
		mAnimMatrix = NULL; // jak animator będzie potrzebował, to ustawi ponownie
	}
};

   //---------------------------------------------------------------------------

void TSubModel::RaArrayFill(CVertNormTex *Vert)
{ // wypełnianie tablic VBO
	if (Child)
		Child->RaArrayFill(Vert);
	if ((eType < TP_ROTATOR) || (eType == TP_STARS))
		for (int i = 0; i < iNumVerts; ++i)
		{
			Vert[iVboPtr + i].x = Vertices[i].Point.x;
			Vert[iVboPtr + i].y = Vertices[i].Point.y;
			Vert[iVboPtr + i].z = Vertices[i].Point.z;
			Vert[iVboPtr + i].nx = Vertices[i].Normal.x;
			Vert[iVboPtr + i].ny = Vertices[i].Normal.y;
			Vert[iVboPtr + i].nz = Vertices[i].Normal.z;
			Vert[iVboPtr + i].u = Vertices[i].tu;
			Vert[iVboPtr + i].v = Vertices[i].tv;
		}
	else if (eType == TP_FREESPOTLIGHT)
		Vert[iVboPtr].x = Vert[iVboPtr].y = Vert[iVboPtr].z = 0.0;
	if (Next)
		Next->RaArrayFill(Vert);
};

// NOTE: leftover from static distance factor adjustment.
// TODO: get rid of it, once we have the dynamic adjustment code in place
void TSubModel::AdjustDist()
{ // aktualizacja odległości faz LoD, zależna od
  // rozdzielczości pionowej oraz multisamplingu
	if (fSquareMaxDist > 0.0)
		fSquareMaxDist *= Global::fDistanceFactor;
	if (fSquareMinDist > 0.0)
		fSquareMinDist /= Global::fDistanceFactor;
	// if (fNearAttenStart>0.0) fNearAttenStart*=Global::fDistanceFactor;
	// if (fNearAttenEnd>0.0) fNearAttenEnd*=Global::fDistanceFactor;
	if (Child)
		Child->AdjustDist();
	if (Next)
		Next->AdjustDist();
};

void TSubModel::ColorsSet(int *a, int *d, int *s)
{ // ustawienie kolorów dla modelu terenu
	int i;
	if (a)
		for (i = 0; i < 4; ++i)
			f4Ambient[i] = a[i] / 255.0;
	if (d)
		for (i = 0; i < 4; ++i)
			f4Diffuse[i] = d[i] / 255.0;
	if (s)
		for (i = 0; i < 4; ++i)
			f4Specular[i] = s[i] / 255.0;
};
void TSubModel::ParentMatrix(float4x4 *m)
{ // pobranie transformacji względem wstawienia modelu
  // jeśli nie zostało wykonane Init() (tzn. zaraz po wczytaniu T3D), to
  // dodatkowy obrót
  // obrót T3D jest wymagany np. do policzenia wysokości pantografów
	*m = float4x4(*fMatrix); // skopiowanie, bo będziemy mnożyć
							 // m(3)[1]=m[3][1]+0.054; //w górę o wysokość ślizgu (na razie tak)
	TSubModel *sm = this;
	while (sm->Parent)
	{ // przenieść tę funkcję do modelu
		if (sm->Parent->GetMatrix())
			*m = *sm->Parent->GetMatrix() * *m;
		sm = sm->Parent;
	}
	// dla ostatniego może być potrzebny dodatkowy obrót, jeśli wczytano z T3D, a
	// nie obrócono jeszcze
};
float TSubModel::MaxY(const float4x4 &m)
{ // obliczenie maksymalnej wysokości,
  // na początek ślizgu w pantografie
	if (eType != 4)
		return 0; // tylko dla trójkątów liczymy
	if (iNumVerts < 1)
		return 0;
	if (!Vertices)
		return 0;
	float y,
		my = m[0][1] * Vertices[0].Point.x + m[1][1] * Vertices[0].Point.y +
		m[2][1] * Vertices[0].Point.z + m[3][1];
	for (int i = 1; i < iNumVerts; ++i)
	{
		y = m[0][1] * Vertices[i].Point.x + m[1][1] * Vertices[i].Point.y +
			m[2][1] * Vertices[i].Point.z + m[3][1];
		if (my < y)
			my = y;
	}
	return my;
};
//---------------------------------------------------------------------------

TModel3d::TModel3d()
{
	// Materials=NULL;
	// MaterialsCount=0;
	Root = NULL;
	iFlags = 0;
	iSubModelsCount = 0;
	iModel = NULL; // tylko jak wczytany model binarny
	iNumVerts = 0; // nie ma jeszcze wierzchołków
};
/*
TModel3d::TModel3d(char *FileName)
{
//    Root=NULL;
//    Materials=NULL;
//    MaterialsCount=0;
Root=NULL;
SubModelsCount=0;
iFlags=0;
LoadFromFile(FileName);
};
*/
TModel3d::~TModel3d()
{
	// SafeDeleteArray(Materials);
	if (iFlags & 0x0200)
	{ // wczytany z pliku tekstowego, submodele sprzątają
	  // same
		SafeDelete(Root); // submodele się usuną rekurencyjnie
	}
	else
	{ // wczytano z pliku binarnego (jest właścicielem tablic)
#ifdef EU07_USE_OLD_VERTEXBUFFER
		m_pVNT = nullptr; // nie usuwać tego, bo wskazuje na iModel
#else
        m_pVNT.clear();
#endif
		Root = nullptr;
		delete[] iModel; // usuwamy cały wczytany plik i to wystarczy
	}
	// później się jeszcze usuwa obiekt z którego dziedziczymy tabelę VBO
};

TSubModel *TModel3d::AddToNamed(const char *Name, TSubModel *SubModel)
{
	TSubModel *sm = Name ? GetFromName(Name) : NULL;
	AddTo(sm, SubModel); // szukanie nadrzędnego
	return sm; // zwracamy wskaźnik do nadrzędnego submodelu
};

void TModel3d::AddTo(TSubModel *tmp, TSubModel *SubModel)
{ // jedyny poprawny sposób dodawania
  // submodeli, inaczej mogą zginąć
  // przy zapisie E3D
	if (tmp)
	{ // jeśli znaleziony, podłączamy mu jako potomny
		tmp->ChildAdd(SubModel);
	}
	else
	{ // jeśli nie znaleziony, podczepiamy do łańcucha głównego
		SubModel->NextAdd(Root); // Ra: zmiana kolejności renderowania wymusza zmianę tu
		Root = SubModel;
	}
	++iSubModelsCount; // teraz jest o 1 submodel więcej
	iFlags |= 0x0200; // submodele są oddzielne
};

TSubModel *TModel3d::GetFromName(const char *sName)
{ // wyszukanie submodelu po nazwie
	if (!sName)
		return Root; // potrzebne do terenu z E3D
	if (iFlags & 0x0200) // wczytany z pliku tekstowego, wyszukiwanie rekurencyjne
		return Root ? Root->GetFromName(sName) : NULL;
	else // wczytano z pliku binarnego, można wyszukać iteracyjnie
	{
		// for (int i=0;i<iSubModelsCount;++i)
		return Root ? Root->GetFromName(sName) : NULL;
	}
};

/*
TMaterial* TModel3d::GetMaterialFromName(char *sName)
{
AnsiString tmp=AnsiString(sName).Trim();
for (int i=0; i<MaterialsCount; i++)
if (strcmp(sName,Materials[i].Name.c_str())==0)
//        if (Trim()==Materials[i].Name.tmp)
return Materials+i;
return Materials;
}
*/

bool TModel3d::LoadFromFile(std::string const &FileName, bool dynamic)
{
    // wczytanie modelu z pliku
    std::string name = ToLower(FileName);
    // trim extension if needed
    if( name.rfind( '.' ) != std::string::npos )
    {
        name.erase(name.rfind('.'));
    }

	asBinary = name + ".e3d";
	if (FileExists(asBinary))
	{
		LoadFromBinFile(asBinary, dynamic);
		asBinary = ""; // wyłączenie zapisu
		Init();
        // cache the file name, in case someone wants it later
        m_filename = name + ".e3d";
    }
	else
	{
		if (FileExists(name + ".t3d"))
		{
			LoadFromTextFile(FileName, dynamic); // wczytanie tekstowego
            if( !dynamic ) {
                // pojazdy dopiero po ustawieniu animacji
                Init(); // generowanie siatek i zapis E3D
            }
            // cache the file name, in case someone wants it later
            m_filename = name + ".t3d";
        }
	}
	bool const result =
		Root ? (iSubModelsCount > 0) : false; // brak pliku albo problem z wczytaniem
	if (false == result)
	{
		ErrorLog("Failed to load 3d model \"" + FileName + "\"");
	}
	return result;
};

// E3D serialization
// http://rainsted.com/pl/Format_binarny_modeli_-_E3D


//m7todo: wymyślić lepszą nazwę
template <typename L, typename T>
size_t get_container_pos(L &list, T o)
{
	auto i = std::find(list.begin(), list.end(), o);
	if (i == list.end())
	{
		list.push_back(o);
		return list.size() - 1;
	}
	else
	{
		return std::distance(list.begin(), i);
	}
}

//m7todo: za dużo argumentów, może przenieść do osobnej
//klasy serializera mającej własny stan, albo zrobić
//strukturę TModel3d::SerializerContext?
void TSubModel::serialize(std::ostream &s,
	std::vector<TSubModel*> &models,
	std::vector<std::string> &names,
	std::vector<std::string> &textures,
	std::vector<float4x4> &transforms)
{
	size_t end = (size_t)s.tellp() + 256;

	if (!Next)
		sn_utils::ls_int32(s, -1);
	else
		sn_utils::ls_int32(s, (int32_t)get_container_pos(models, Next));
	if (!Child)
		sn_utils::ls_int32(s, -1);
	else
		sn_utils::ls_int32(s, (int32_t)get_container_pos(models, Child));

	sn_utils::ls_int32(s, eType);
	if (pName.size() == 0)
		sn_utils::ls_int32(s, -1);
	else
		sn_utils::ls_int32(s, (int32_t)get_container_pos(names, pName));
	sn_utils::ls_int32(s, (int)b_Anim);

	sn_utils::ls_int32(s, iFlags);
	sn_utils::ls_int32(s, (int32_t)get_container_pos(transforms, *fMatrix));

	sn_utils::ls_int32(s, iNumVerts);
	sn_utils::ls_int32(s, (int)iVboPtr);
	if (TextureID <= 0)
		sn_utils::ls_int32(s, TextureID);
	else
		sn_utils::ls_int32(s, (int32_t)get_container_pos(textures, pTexture));

	sn_utils::ls_float32(s, fVisible);
	sn_utils::ls_float32(s, fLight);

	for (size_t i = 0; i < 4; i++)
		sn_utils::ls_float32(s, f4Ambient[i]);
	for (size_t i = 0; i < 4; i++)
		sn_utils::ls_float32(s, f4Diffuse[i]);
	for (size_t i = 0; i < 4; i++)
		sn_utils::ls_float32(s, f4Specular[i]);
	for (size_t i = 0; i < 4; i++)
		sn_utils::ls_float32(s, f4Emision[i]);

	sn_utils::ls_float32(s, fWireSize);
	sn_utils::ls_float32(s, fSquareMaxDist);
	sn_utils::ls_float32(s, fSquareMinDist);

	sn_utils::ls_float32(s, fNearAttenStart);
	sn_utils::ls_float32(s, fNearAttenEnd);
	sn_utils::ls_uint32(s, bUseNearAtten ? 1 : 0);

	sn_utils::ls_int32(s, iFarAttenDecay);
	sn_utils::ls_float32(s, fFarDecayRadius);
	sn_utils::ls_float32(s, fCosFalloffAngle);
	sn_utils::ls_float32(s, fCosHotspotAngle);
	sn_utils::ls_float32(s, fCosViewAngle);

	size_t fill = end - s.tellp();
	for (size_t i = 0; i < fill; i++)
		s.put(0);
}

void TModel3d::SaveToBinFile(char const *FileName)
{
	WriteLog("saving e3d model..");

	//m7todo: można by zoptymalizować robiąc unordered_map
	//na wyszukiwanie numerów już dodanych stringów i osobno
	//vector na wskaźniki do stringów w kolejności numeracji
	//tylko czy potrzeba?
	std::vector<TSubModel*> models;
	models.push_back(Root);
	std::vector<std::string> names;
	std::vector<std::string> textures;
	textures.push_back("");
	std::vector<float4x4> transforms;

	std::ofstream s(FileName, std::ios::binary);

	sn_utils::ls_uint32(s, MAKE_ID4('E', '3', 'D', '0'));
	size_t e3d_spos = s.tellp();
	sn_utils::ls_uint32(s, 0);

	{
		sn_utils::ls_uint32(s, MAKE_ID4('S', 'U', 'B', '0'));
		size_t sub_spos = s.tellp();
		sn_utils::ls_uint32(s, 0);
		for (size_t i = 0; i < models.size(); i++)
			models[i]->serialize(s, models, names, textures, transforms);
		size_t pos = s.tellp();
		s.seekp(sub_spos);
		sn_utils::ls_uint32(s, (uint32_t)(4 + pos - sub_spos));
		s.seekp(pos);
	}

	sn_utils::ls_uint32(s, MAKE_ID4('T', 'R', 'A', '0'));
	sn_utils::ls_uint32(s, 8 + (uint32_t)transforms.size() * 64);
	for (size_t i = 0; i < transforms.size(); i++)
		transforms[i].serialize_float32(s);

	MakeArray(iNumVerts);
#ifdef EU07_USE_OLD_VERTEXBUFFER
    Root->RaArrayFill(m_pVNT);
#else
    Root->RaArrayFill( m_pVNT.data() );
#endif
	sn_utils::ls_uint32(s, MAKE_ID4('V', 'N', 'T', '0'));
	sn_utils::ls_uint32(s, 8 + iNumVerts * 32);
	for (size_t i = 0; i < (size_t)iNumVerts; i++)
		m_pVNT[i].serialize(s);

	if (textures.size())
	{
		sn_utils::ls_uint32(s, MAKE_ID4('T', 'E', 'X', '0'));
		size_t tex_spos = s.tellp();
		sn_utils::ls_uint32(s, 0);
		for (size_t i = 0; i < textures.size(); i++)
			sn_utils::s_str(s, textures[i]);
		size_t pos = s.tellp();
		s.seekp(tex_spos);
		sn_utils::ls_uint32(s, (uint32_t)(4 + pos - tex_spos));
		s.seekp(pos);
	}

	if (names.size())
	{
		sn_utils::ls_uint32(s, MAKE_ID4('N', 'A', 'M', '0'));
		size_t nam_spos = s.tellp();
		sn_utils::ls_uint32(s, 0);
		for (size_t i = 0; i < names.size(); i++)
			sn_utils::s_str(s, names[i]);
		size_t pos = s.tellp();
		s.seekp(nam_spos);
		sn_utils::ls_uint32(s, (uint32_t)(4 + pos - nam_spos));
		s.seekp(pos);
	}

	size_t end = s.tellp();
	s.seekp(e3d_spos);
	sn_utils::ls_uint32(s, (uint32_t)(4 + end - e3d_spos));
	s.close();

	WriteLog("..done.");
}

void TSubModel::deserialize(std::istream &s)
{
	iNext = sn_utils::ld_int32(s);
	iChild = sn_utils::ld_int32(s);

	eType = sn_utils::ld_int32(s);
	iName = sn_utils::ld_int32(s);

	b_Anim = (TAnimType)sn_utils::ld_int32(s);

	iFlags = sn_utils::ld_int32(s);
	iMatrix = sn_utils::ld_int32(s);

	iNumVerts = sn_utils::ld_int32(s);
	tVboPtr = sn_utils::ld_int32(s);
	iTexture = sn_utils::ld_int32(s);

	fVisible = sn_utils::ld_float32(s);
	fLight = sn_utils::ld_float32(s);

	for (size_t i = 0; i < 4; i++)
		f4Ambient[i] = sn_utils::ld_float32(s);
	for (size_t i = 0; i < 4; i++)
		f4Diffuse[i] = sn_utils::ld_float32(s);
	for (size_t i = 0; i < 4; i++)
		f4Specular[i] = sn_utils::ld_float32(s);
	for (size_t i = 0; i < 4; i++)
		f4Emision[i] = sn_utils::ld_float32(s);

	fWireSize = sn_utils::ld_float32(s);
	fSquareMaxDist = sn_utils::ld_float32(s);
	fSquareMinDist = sn_utils::ld_float32(s);

	fNearAttenStart = sn_utils::ld_float32(s);
	fNearAttenEnd = sn_utils::ld_float32(s);
	bUseNearAtten = sn_utils::ld_uint32(s) != 0;
	iFarAttenDecay = sn_utils::ld_int32(s);
	fFarDecayRadius = sn_utils::ld_float32(s);
	fCosFalloffAngle = sn_utils::ld_float32(s);
	fCosHotspotAngle = sn_utils::ld_float32(s);
	fCosViewAngle = sn_utils::ld_float32(s);
}

void TModel3d::deserialize(std::istream &s, size_t size, bool dynamic)
{
#ifdef EU07_USE_OLD_VERTEXBUFFER
    m_pVNT = nullptr;
#else
    m_pVNT.clear();
#endif
	Root = nullptr;
	float4x4 *tm = nullptr;

	std::streampos end = s.tellg() + (std::streampos)size;

	while (s.tellg() < end)
	{
		uint32_t type = sn_utils::ld_uint32(s);
		uint32_t size = sn_utils::ld_uint32(s) - 8;
		std::streampos end = s.tellg() + (std::streampos)size;

		if (type == MAKE_ID4('V', 'N', 'T', '0'))
		{
#ifdef EU07_USE_OLD_VERTEXBUFFER
            if (m_pVNT != nullptr)
#else
            if( false == m_pVNT.empty() )
#endif
				throw std::runtime_error("e3d: duplicated VNT chunk");

			size_t vt_cnt = size / 32;
			iNumVerts = (int)vt_cnt;
			m_nVertexCount = (int)vt_cnt;
#ifdef EU07_USE_OLD_VERTEXBUFFER
            assert( m_pVNT == nullptr );
            m_pVNT = new CVertNormTex[vt_cnt];
#else
            m_pVNT.resize( vt_cnt );
#endif
			for (size_t i = 0; i < vt_cnt; i++)
				m_pVNT[i].deserialize(s);
		}
		else if ((type & 0x00FFFFFF) == MAKE_ID4('S', 'U', 'B', 0))
		{
			if (Root != nullptr)
				throw std::runtime_error("e3d: duplicated SUB chunk");

			size_t sm_size = 256 + 64 * (((type & 0xFF000000) >> 24) - '0');
			size_t sm_cnt = size / sm_size;
			iSubModelsCount = (int)sm_cnt;
			Root = new TSubModel[sm_cnt];
			size_t pos = s.tellg();
			for (size_t i = 0; i < sm_cnt; i++)
			{
				s.seekg(pos + sm_size * i);
				Root[i].deserialize(s);
			}
		}
		else if (type == MAKE_ID4('T', 'R', 'A', '0'))
		{
			if (tm != nullptr)
				throw std::runtime_error("e3d: duplicated TRA chunk");
			size_t t_cnt = size / 64;

			tm = new float4x4[t_cnt];
			for (size_t i = 0; i < t_cnt; i++)
				tm[i].deserialize_float32(s);
		}
		else if (type == MAKE_ID4('T', 'R', 'A', '1'))
		{
			if (tm != nullptr)
				throw std::runtime_error("e3d: duplicated TRA chunk");
			size_t t_cnt = size / 128;

			tm = new float4x4[t_cnt];
			for (size_t i = 0; i < t_cnt; i++)
				tm[i].deserialize_float64(s);
		}
		else if (type == MAKE_ID4('T', 'E', 'X', '0'))
		{
			if (Textures.size())
				throw std::runtime_error("e3d: duplicated TEX chunk");
			while (s.tellg() < end)
				Textures.push_back(sn_utils::d_str(s));
		}
		else if (type == MAKE_ID4('N', 'A', 'M', '0'))
		{
			if (Names.size())
				throw std::runtime_error("e3d: duplicated NAM chunk");
			while (s.tellg() < end)
				Names.push_back(sn_utils::d_str(s));
		}

		s.seekg(end);
	}

	if (!Root)
		throw std::runtime_error("e3d: no submodels");

#ifdef EU07_USE_OLD_VERTEXBUFFER
    if (!m_pVNT)
#else
    if(m_pVNT.empty() )
#endif
		throw std::runtime_error("e3d: no vertices");

	for (size_t i = 0; (int)i < iSubModelsCount; i++)
	{
#ifdef EU07_USE_OLD_VERTEXBUFFER
        Root[i].BinInit(Root, tm, (float8*)m_pVNT, &Textures, &Names, dynamic);
#else
        Root[ i ].BinInit( Root, tm, (float8*)m_pVNT.data(), &Textures, &Names, dynamic );
#endif

		if (Root[i].ChildGet())
			Root[i].ChildGet()->Parent = &Root[i];
		if (Root[i].NextGet())
			Root[i].NextGet()->Parent = Root[i].Parent;
	}
}

void TSubModel::BinInit(TSubModel *s, float4x4 *m, float8 *v,
	std::vector<std::string> *t, std::vector<std::string> *n, bool dynamic)
{ // ustawienie wskaźników w submodelu
	//m7todo: brzydko
	iVisible = 1; // tymczasowo używane
	Child = (iChild > 0) ? s + iChild : nullptr; // zerowy nie może być potomnym
	Next = (iNext > 0) ? s + iNext : nullptr; // zerowy nie może być następnym
	fMatrix = ((iMatrix >= 0) && m) ? m + iMatrix : nullptr;
	if (n->size() && (iName >= 0))
	{
		pName = n->at(iName);
		if (!pName.empty())
		{ // jeśli dany submodel jest zgaszonym światłem, to
		  // domyślnie go ukrywamy
			if ((pName.size() >= 8) && (pName.substr(0, 8) == "Light_On"))
			{ // jeśli jest światłem numerowanym
				iVisible = 0; // to domyślnie wyłączyć, żeby się nie nakładało z
			}
			// obiektem "Light_Off"
			else if (dynamic)
			{ // inaczej wyłączało smugę w latarniach
				if ((pName.size() >= 3) && (pName.substr(pName.size() - 3, 3) == "_on"))
				{ // jeśli jest kontrolką w stanie zapalonym
					iVisible = 0; // to domyślnie wyłączyć, żeby się nie nakładało z
				}
			}
			// obiektem "_off"
		}
	}
	else
		pName = "";
	if (iTexture > 0)
	{ // obsługa stałej tekstury
		pTexture = t->at(iTexture);
		if (pTexture.find_last_of("/\\") == std::string::npos)
			pTexture.insert(0, Global::asCurrentTexturePath);
		TextureID = GfxRenderer.GetTextureId(pTexture, szTexturePath);
        iFlags |=
            ( GfxRenderer.Texture( TextureID ).has_alpha ?
                0x20 :
                0x10 ); // 0x10-nieprzezroczysta, 0x20-przezroczysta
    }
	else
		TextureID = iTexture;

	b_aAnim = b_Anim; // skopiowanie animacji do drugiego cyklu

    if( (eType == TP_FREESPOTLIGHT) && (iFlags & 0x10)) {
        // we've added light glare which needs to be rendered during transparent phase,
        // but models converted to e3d before addition won't have the render flag set correctly for this
        // so as a workaround we're doing it here manually
        iFlags |= 0x20;
    }
    // intercept and fix hotspot values if specified in degrees and not directly
    if( fCosFalloffAngle > 1.0 ) {
        fCosFalloffAngle = std::cos( DegToRad( 0.5f * fCosFalloffAngle ) );
    }
    if( fCosHotspotAngle > 1.0 ) {
        fCosHotspotAngle = std::cos( DegToRad( 0.5f * fCosHotspotAngle ) );
    }

	iFlags &= ~0x0200; // wczytano z pliku binarnego (nie jest właścicielem tablic)

	iVboPtr = tVboPtr;
	Vertices = v + iVboPtr;
	// if (!iNumVerts) eType=-1; //tymczasowo zmiana typu, żeby się nie
	// renderowało na siłę
};

void TModel3d::LoadFromBinFile(std::string const &FileName, bool dynamic)
{ // wczytanie modelu z pliku binarnego
	WriteLog("Loading binary format 3d model data from \"" + FileName + "\"...");
	
	std::ifstream file(FileName, std::ios::binary);

	uint32_t type = sn_utils::ld_uint32(file);
	uint32_t size = sn_utils::ld_uint32(file) - 8;

	if (type != MAKE_ID4('E', '3', 'D', '0'))
		throw std::runtime_error("e3d: unknown main chunk");

	deserialize(file, size, dynamic);
	file.close();

	WriteLog("Finished loading 3d model data from \"" + FileName + "\"");
};

void TModel3d::LoadFromTextFile(std::string const &FileName, bool dynamic)
{ // wczytanie submodelu z pliku tekstowego
	WriteLog("Loading text format 3d model data from \"" + FileName + "\"...");
	iFlags |= 0x0200; // wczytano z pliku tekstowego (właścicielami tablic są submodle)
	cParser parser(FileName, cParser::buffer_FILE); // Ra: tu powinno być "models\\"...
	TSubModel *SubModel;
	std::string token = parser.getToken<std::string>();
	iNumVerts = 0; // w konstruktorze to jest
	while (token != "" || parser.eof())
	{
		std::string parent;
		// parser.getToken(parent);
		parser.getTokens(1, false); // nazwa submodelu nadrzędnego bez zmieny na małe
		parser >> parent;
		if (parent == "")
			break;
		SubModel = new TSubModel();
		iNumVerts += SubModel->Load(parser, this, iNumVerts, dynamic);
		SubModel->Parent = AddToNamed(
			parent.c_str(), SubModel); // będzie potrzebne do wyliczenia pozycji, np. pantografu
									   // iSubModelsCount++;
		parser.getTokens();
		parser >> token;
	}
	// Ra: od wersji 334 przechylany jest cały model, a nie tylko pierwszy
	// submodel
	// ale bujanie kabiny nadal używa bananów :( od 393 przywrócone, ale z
	// dodatkowym warunkiem
	if (Global::iConvertModels & 4)
	{ // automatyczne banany czasem psuły przechylanie kabin...
		if (dynamic && Root)
		{
			if (Root->NextGet()) // jeśli ma jakiekolwiek kolejne
			{ // dynamic musi mieć "banana", bo tylko pierwszy obiekt jest animowany,
			  // a następne nie
				SubModel = new TSubModel(); // utworzenie pustego
				SubModel->ChildAdd(Root);
				Root = SubModel;
				++iSubModelsCount;
			}
			Root->WillBeAnimated(); // bo z tym jest dużo problemów
		}
	}
}

void TModel3d::Init()
{ // obrócenie początkowe układu współrzędnych, dla
  // pojazdów wykonywane po analizie animacji
	if (iFlags & 0x8000)
		return; // operacje zostały już wykonane
	if (Root)
	{
		if (iFlags & 0x0200) // jeśli wczytano z pliku tekstowego
		{ // jest jakiś dziwny błąd, że obkręcany ma być tylko ostatni submodel
		  // głównego łańcucha
		  // TSubModel *p=Root;
		  // do
		  //{p->InitialRotate(true); //ostatniemu należy się konwersja układu
		  // współrzędnych
		  // p=p->NextGet();
		  //}
		  // while (p->NextGet())
		  // Root->InitialRotate(false); //a poprzednim tylko optymalizacja
			Root->InitialRotate(true); // argumet określa, czy wykonać pierwotny obrót
		}
		iFlags |= Root->FlagsCheck() | 0x8000; // flagi całego modelu
		if (false == asBinary.empty()) // jeśli jest podana nazwa
		{
			if (Global::iConvertModels) // i włączony zapis
				SaveToBinFile(asBinary.c_str()); // utworzy tablicę (m_pVNT)
			asBinary = ""; // zablokowanie powtórnego zapisu
		}
		if (iNumVerts)
		{
/* // NOTE: we will be applying distance factor dynamically during render,
   // so we're leaving the defined ranges intact
			if (Global::fDistanceFactor !=
				1.0) // trochę zaoszczędzi czasu na modelach z wieloma submocelami
				Root->AdjustDist(); // aktualizacja odległości faz LoD, zależnie od
									// rozdzielczości pionowej oraz multisamplingu
*/
			if (Global::bUseVBO)
			{
#ifdef EU07_USE_OLD_VERTEXBUFFER
                if (!m_pVNT) // jeśli nie ma jeszcze tablicy (wczytano z pliku tekstowego)
#else
                if( m_pVNT.empty() )
#endif
				{ // tworzenie tymczasowej tablicy z wierzchołkami całego modelu
					MakeArray(iNumVerts); // tworzenie tablic dla VBO
#ifdef EU07_USE_OLD_VERTEXBUFFER
                    Root->RaArrayFill(m_pVNT); // wypełnianie tablicy
#else
                    Root->RaArrayFill( m_pVNT.data() ); // wypełnianie tablicy
#endif
					BuildVBOs(); // tworzenie VBO i usuwanie tablicy z pamięci
				}
				else
					BuildVBOs(false); // tworzenie VBO bez usuwania tablicy z pamięci
			}
			else
			{ // przygotowanie skompilowanych siatek dla DisplayLists
				Root->DisplayLists(); // tworzenie skompilowanej listy dla submodelu
			}
			// if (Root->TextureID) //o ile ma teksturę
			// Root->iFlags|=0x80; //konieczność ustawienia tekstury
		}
	}
};

void TModel3d::BreakHierarhy()
{
	Error("Not implemented yet :(");
};

//-----------------------------------------------------------------------------
// 2012-02 funkcje do tworzenia terenu z E3D
//-----------------------------------------------------------------------------

int TModel3d::TerrainCount()
{ // zliczanie kwadratów kilometrowych (główna
  // linia po Next) do tworznia tablicy
	int i = 0;
	TSubModel *r = Root;
	while (r)
	{
		r = r->NextGet();
		++i;
	}
	return i;
};
TSubModel *TModel3d::TerrainSquare(int n)
{ // pobieranie wskaźnika do submodelu (n)
	int i = 0;
	TSubModel *r = Root;
	while (i < n)
	{
		r = r->NextGet();
		++i;
	}
	r->UnFlagNext(); // blokowanie wyświetlania po Next głównej listy
	return r;
};
void TModel3d::TerrainRenderVBO(int n)
{ // renderowanie terenu z VBO
	glPushMatrix();
	// glTranslated(vPosition->x,vPosition->y,vPosition->z);
	// if (vAngle->y!=0.0) glRotated(vAngle->y,0.0,1.0,0.0);
	// if (vAngle->x!=0.0) glRotated(vAngle->x,1.0,0.0,0.0);
	// if (vAngle->z!=0.0) glRotated(vAngle->z,0.0,0.0,1.0);
	// TSubModel::fSquareDist=SquareMagnitude(*vPosition-Global::GetCameraPosition());
	// //zmienna globalna!
	if (StartVBO())
	{ // odwrócenie flag, aby wyłapać nieprzezroczyste
	  // Root->ReplacableSet(ReplacableSkinId,iAlpha^0x0F0F000F);
		TSubModel *r = Root;
		while (r)
		{
			if (r->iVisible == n) // tylko jeśli ma być widoczny w danej ramce (problem dla 0==false)
				GfxRenderer.Render(r); // sub kolejne (Next) się nie wyrenderują
			r = r->NextGet();
		}
		EndVBO();
	}
	glPopMatrix();
};
