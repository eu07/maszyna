/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "Classes.h"
#include "dumb3d.h"
#include "Float3d.h"
#include "geometrybank.h"
#include "material.h"
#include "gl/query.h"

#define EU07_USE_GEOMETRYINDEXING

// Ra: specjalne typy submodeli, poza tym GL_TRIANGLES itp.
const int TP_ROTATOR = 256;
const int TP_FREESPOTLIGHT = 257;
const int TP_STARS = 258;
const int TP_TEXT = 259;

enum class TAnimType // rodzaj animacji
{
	at_None, // brak
	at_Rotate, // obrót względem wektora o kąt
	at_RotateXYZ, // obrót względem osi o kąty
	at_Translate, // przesunięcie
	at_SecondsJump, // sekundy z przeskokiem
	at_MinutesJump, // minuty z przeskokiem
	at_HoursJump, // godziny z przeskokiem 12h/360°
	at_Hours24Jump, // godziny z przeskokiem 24h/360°
	at_Seconds, // sekundy płynnie
	at_Minutes, // minuty płynnie
	at_Hours, // godziny płynnie 12h/360°
	at_Hours24, // godziny płynnie 24h/360°
	at_Billboard, // obrót w pionie do kamery
	at_Wind, // ruch pod wpływem wiatru
	at_Sky, // animacja nieba
	at_Digital, // dziesięciocyfrowy licznik mechaniczny (z cylindrami)
	at_DigiClk, // zegar cyfrowy jako licznik na dziesięciościanach
	at_Undefined // animacja chwilowo nieokreślona
};

namespace scene {
class shape_node;
}

class TModel3d;
using nameoffset_sequence = std::vector<std::pair<std::string, glm::vec3>>;

class TSubModel
{ // klasa submodelu - pojedyncza siatka, punkt świetlny albo grupa punktów
    //m7todo: zrobić normalną serializację

    friend opengl_renderer;
    friend opengl33_renderer;
    friend TModel3d; // temporary workaround. TODO: clean up class content/hierarchy
    friend TDynamicObject; // temporary etc
    friend scene::shape_node; // temporary etc

public:
    enum normalization {
        none = 0,
        rescale,
        normalize
    };
    struct geometry_data {
        gfx::geometry_handle handle;
        int vertex_offset;
        int vertex_count;
        int index_offset;
        int index_count;
    };

private:
    int iNext{ 0 };
    int iChild{ 0 };
    int eType{ TP_ROTATOR }; // Ra: modele binarne dają więcej możliwości niż mesh złożony z trójkątów
    int iName{ -1 }; // numer łańcucha z nazwą submodelu, albo -1 gdy anonimowy
public: // chwilowo
    TAnimType b_Anim{ TAnimType::at_None };

private:
    uint32_t iFlags{ 0x0200 }; // bit 9=1: submodel został utworzony a nie ustawiony na wczytany plik
                // flagi informacyjne:
				// bit  0: =1 faza rysowania zależy od wymiennej tekstury 0
				// bit  1: =1 faza rysowania zależy od wymiennej tekstury 1
				// bit  2: =1 faza rysowania zależy od wymiennej tekstury 2
				// bit  3: =1 faza rysowania zależy od wymiennej tekstury 3
				// bit  4: =1 rysowany w fazie nieprzezroczystych (stała tekstura albo brak)
				// bit  5: =1 rysowany w fazie przezroczystych (stała tekstura)
				// bit  7: =1 ta sama tekstura, co poprzedni albo nadrzędny
				// bit  8: =1 wierzchołki wyświetlane z indeksów
				// bit  9: =1 wczytano z pliku tekstowego (jest właścicielem tablic)
				// bit 13: =1 wystarczy przesunięcie zamiast mnożenia macierzy (trzy jedynki)
				// bit 14: =1 wymagane przechowanie macierzy (animacje)
				// bit 15: =1 wymagane przechowanie macierzy (transform niejedynkowy)
	union
	{ // transform, nie każdy submodel musi mieć
		float4x4 *fMatrix = nullptr; // pojedyncza precyzja wystarcza
		int iMatrix; // w pliku binarnym jest numer matrycy
	};
	float transformscalestack { 1.0f }; // tolerancescale used in calculate_indices for whole matrix chain
    int iTexture { 0 }; // numer nazwy tekstury, -1 wymienna, 0 brak
    float fLight { -1.0f }; // próg jasności światła do zadziałania selfillum
	glm::vec4
        f4Ambient { 1.0f,1.0f,1.0f,1.0f },
        f4Diffuse { 1.0f,1.0f,1.0f,1.0f },
        f4Specular { 0.0f,0.0f,0.0f,1.0f },
        f4Emision { 1.0f,1.0f,1.0f,1.0f };
    glm::vec3 DiffuseOverride { -1.f };
    normalization m_normalizenormals { normalization::none }; // indicates vectors need to be normalized due to scaling etc
    float fWireSize { 0.0f }; // nie używane, ale wczytywane
    float fSquareMaxDist { 10000.0f * 10000.0f };
    float fSquareMinDist { 0.0f };
	// McZapkie-050702: parametry dla swiatla:
    float fNearAttenStart { 40.0f };
    float fNearAttenEnd { 80.0f };
    bool bUseNearAtten { false }; // te 3 zmienne okreslaja rysowanie aureoli wokol zrodla swiatla
    int iFarAttenDecay { 0 }; // ta zmienna okresla typ zaniku natezenia swiatla (0:brak, 1,2: potega 1/R)
    float fFarDecayRadius { 100.0f }; // normalizacja j.w.
    float fCosFalloffAngle { 0.5f }; // cosinus kąta stożka pod którym widać światło
    float fCosHotspotAngle { 0.3f }; // cosinus kąta stożka pod którym widać aureolę i zwiększone natężenie światła
    float fCosViewAngle { 0.0f }; // cos kata pod jakim sie teraz patrzy

	bool m_rotation_init_done = false;

    TSubModel *Next { nullptr };
    TSubModel *Child { nullptr };
private:
    material_handle m_material { null_handle }; // numer tekstury, -1 wymienna, 0 brak
    bool bWire { false }; // nie używane, ale wczytywane
    float Opacity { 1.0f };
    float f_Angle { 0.0f };
    float3 v_RotateAxis { 0.0f, 0.0f, 0.0f };
	float3 v_Angles { 0.0f, 0.0f, 0.0f };

public: // chwilowo
    float3 v_TransVector { 0.0f, 0.0f, 0.0f };
    geometry_data m_geometry { /*this,*/ { 0, 0 }, 0, 0, 0, 0 };
    gfx::vertex_array Vertices;
    gfx::index_array Indices;
    float m_boundingradius { 0 };
    std::uintptr_t iAnimOwner{ 0 }; // roboczy numer egzemplarza, który ustawił animację
    TAnimType b_aAnim{ TAnimType::at_None }; // kody animacji oddzielnie, bo zerowane
	std::shared_ptr<float4x4> mAnimMatrix; // macierz do animacji kwaternionowych
    TSubModel **smLetter{ nullptr }; // wskaźnik na tablicę submdeli do generoania tekstu (docelowo zapisać do E3D)
    TSubModel *Parent{ nullptr }; // nadrzędny, np. do wymnażania macierzy
    int iVisible { 1 }; // roboczy stan widoczności
    float fVisible { 1.f }; // visibility level
	std::string m_materialname; // robocza nazwa tekstury do zapisania w pliku binarnym
	std::string pName; // robocza nazwa
private:
	int SeekFaceNormal( std::vector<unsigned int> const &Masks, int const Startface, unsigned int const Mask, glm::vec3 const &Position, gfx::vertex_array const &Vertices );
	void RaAnimation(TAnimType a);
	void RaAnimation(glm::mat4 &m, TAnimType a);
    // returns true if the submodel is a smoke emitter attachment point, false otherwise
    bool is_emitter() const;

public:
	static size_t iInstance; // identyfikator egzemplarza, który aktualnie renderuje model
	static material_handle const *ReplacableSkinId;
	static int iAlpha; // maska bitowa dla danego przebiegu
	static float fSquareDist;
	static TModel3d *pRoot;
	static std::string *pasText; // tekst dla wyświetlacza (!!!! do przemyślenia)
    TSubModel() = default;
	~TSubModel();
	std::pair<int, int> Load(cParser &Parser, /*TModel3d *Model, int Pos,*/ bool dynamic);
	void ChildAdd(TSubModel *SubModel);
	void NextAdd(TSubModel *SubModel);
	TSubModel * NextGet() { return Next; };
	TSubModel * ChildGet() { return Child; };
    int count_siblings();
    int count_children();
    // locates submodel mapped with replacable -4
    std::tuple<TSubModel *, bool> find_replacable4();
    // locates particle emitter submodels and adds them to provided list
    void find_smoke_sources( nameoffset_sequence &Sourcelist ) const;
#ifndef EU07_USE_GEOMETRYINDEXING
	int TriangleAdd(TModel3d *m, material_handle tex, int tri);
#endif
	void SetRotate(float3 vNewRotateAxis, float fNewAngle);
	void SetRotateXYZ( Math3D::vector3 vNewAngles);
	void SetRotateXYZ(float3 vNewAngles);
	void SetTranslate( Math3D::vector3 vNewTransVector);
	void SetTranslate(float3 vNewTransVector);
	void SetRotateIK1(float3 vNewAngles);
	TSubModel * GetFromName( std::string const &search, bool i = true );
	inline float4x4 * GetMatrix() { return fMatrix; };
    inline float4x4 const * GetMatrix() const { return fMatrix; };
    // returns offset vector from root
    glm::vec3 offset( float const Geometrytestoffsetthreshold = 0.f ) const;
    inline void Hide() { iVisible = 0; };

    void create_geometry( std::size_t &Indexoffset, std::size_t &Vertexoffset, gfx::geometrybank_handle const &Bank );
	uint32_t FlagsCheck();
	void WillBeAnimated() {
        iFlags |= 0x4000; };
	void InitialRotate(bool doit);
	void BinInit(TSubModel *s, float4x4 *m, std::vector<std::string> *t, std::vector<std::string> *n, bool dynamic);
	static void ReplacableSet(material_handle const *r, int a) {
		ReplacableSkinId = r;
		iAlpha = a; };
	void Name_Material( std::string const &Name );
	void Name( std::string const &Name );
	// Ra: funkcje do budowania terenu z E3D
	uint32_t Flags() const { return iFlags; };
	void UnFlagNext() { iFlags &= 0x00FFFFFF; };
	void ColorsSet( glm::vec3 const &Ambient, glm::vec3 const &Diffuse, glm::vec3 const &Specular );
    // sets rgb components of diffuse color override to specified value
    void SetDiffuseOverride( glm::vec3 const &Color, bool const Includechildren = false, bool const Includesiblings = false );
    // gets rgb components of any freespot diffuse color (searches also in children)
    std::optional<glm::vec3> GetDiffuse( float Includesiblings = false );
    // sets visibility level (alpha component) to specified value
    void SetVisibilityLevel( float const Level, bool const Includechildren = false, bool const Includesiblings = false );
    // sets light level (alpha component of illumination color) to specified value
    void SetLightLevel( glm::vec4 const &Level, bool const Includechildren = false, bool const Includesiblings = false );
    // sets activation threshold of self-illumination to specitied value
    void SetSelfIllum( float const Threshold, bool const Includechildren = false, bool const Includesiblings = false );
	inline float3 Translation1Get() {
		return fMatrix ? *(fMatrix->TranslationGet()) + v_TransVector : v_TransVector; }
	inline float3 Translation2Get() {
		return *(fMatrix->TranslationGet()) + Child->Translation1Get(); }
    material_handle GetMaterial() const {
		return m_material; }
	void ParentMatrix(float4x4 *m) const;
    void ReplaceMatrix(const glm::mat4 &mat);
    void ReplaceMaterial(const std::string &name);
	float MaxY( float4x4 const &m );
    std::shared_ptr<std::vector<glm::vec2>> screen_touch_list; // for python screen touching
	std::optional<gl::query> occlusion_query;
    glm::mat4 future_transform;

	void deserialize(std::istream&);
	void serialize(std::ostream&,
		std::vector<TSubModel*>&,
		std::vector<std::string>&,
		std::vector<std::string>&,
		std::vector<float4x4>&);
    void serialize_geometry( std::ostream &Output, bool const Packed, bool const Indexed ) const;
    int index_size() const;
    void serialize_indices( std::ostream &Output, int const Size ) const;
    // places contained geometry in provided ground node
};

class TModel3d
{
    friend opengl_renderer;
    friend opengl33_renderer;

private:
    TSubModel *Root { nullptr }; // drzewo submodeli
    uint32_t iFlags { 0 }; // Ra: czy submodele mają przezroczyste tekstury
public: // Ra: tymczasowo
    gfx::geometrybank_handle m_geometrybank;
    int m_indexcount { 0 };
    int m_vertexcount { 0 }; // ilość wierzchołków
private:
	std::vector<std::string> Textures; // nazwy tekstur
	std::vector<std::string> Names; // nazwy submodeli
    std::vector<float4x4> Matrices; // submodel matrices
    int iSubModelsCount { 0 }; // Ra: używane do tworzenia binarnych
	std::string asBinary; // nazwa pod którą zapisać model binarny
    std::string m_filename;
    nameoffset_sequence m_smokesources; // list of particle sources defined in the model

public:
    TModel3d() = default;
    ~TModel3d();
    float bounding_radius() const {
        return (
            Root ?
                Root->m_boundingradius :
                0.f ); }
	inline TSubModel * GetSMRoot() { return (Root); };
	TSubModel * GetFromName(std::string const &Name) const;
	TSubModel * AddToNamed(const char *Name, TSubModel *SubModel);
    nameoffset_sequence const & find_smoke_sources();
	void AddTo(TSubModel *tmp, TSubModel *SubModel);
	void LoadFromTextFile(std::string const &FileName, bool dynamic);
	void LoadFromBinFile(std::string const &FileName, bool dynamic);
    bool LoadFromFile(std::string const &FileName, bool dynamic);
    TSubModel *AppendChildFromGeometry(const std::string &name, const std::string &parent, const gfx::vertex_array &vertices, const gfx::index_array &indices);
	void SaveToBinFile(std::string const &FileName);
	uint32_t Flags() const { return iFlags; };
	void Init();
	std::string NameGet() const { return m_filename; };
    nameoffset_sequence const & smoke_sources() const {
        return m_smokesources; }
	int TerrainCount() const;
	TSubModel * TerrainSquare(int n);
	void deserialize(std::istream &s, size_t size, bool dynamic);
};

//---------------------------------------------------------------------------
