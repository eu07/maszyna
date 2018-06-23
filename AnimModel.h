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
#include "Model3d.h"
#include "DynObj.h"
#include "scenenode.h"

const int iMaxNumLights = 8;
const float defaultDarkThresholdLevel = 0.325f;

// typy stanu świateł
enum TLightState {
	LS_OFF = 0, // zgaszone
	LS_ON = 1, // zapalone
	LS_BLINK = 2, // migające
	LS_DARK = 3 // Ra: zapalajce się automatycznie, gdy zrobi się ciemno
};

class TAnimVocaloidFrame
{ // ramka animacji typu Vocaloid Motion Data z programu MikuMikuDance
public:
	char cBone[15]; // nazwa kości, może być po japońsku
	int iFrame; // numer ramki
	float3 f3Vector; // przemieszczenie
	float4 qAngle; // kwaternion obrotu
	char cBezier[64]; // krzywe Béziera do interpolacji dla x,y,z i obrotu
};

class TEvent;

class TAnimContainer
{ // opakowanie submodelu, określające animację egzemplarza - obsługiwane jako lista
	friend class TAnimModel;

public:
	TAnimContainer();
	~TAnimContainer();
	bool init(TSubModel* pNewSubModel);
	inline std::string nameGet()
	{
		return (pSubModel ? pSubModel->pName : "");
	};
	void setRotateAnim(Math3D::vector3 vNewRotateAngles, double fNewRotateSpeed);
	void setTranslateAnim(Math3D::vector3 vNewTranslate, double fNewSpeed);
	void animSetVMD(double fNewSpeed);
	void prepareModel();
	void updateModel();
	void updateModelIK();
	bool inMovement(); // czy w trakcie animacji?
	inline double angleGet()
	{
		return vRotateAngles.z;
	}; // jednak ostatnia, T3D ma inny układ
	inline Math3D::vector3 transGet()
	{
		return Math3D::vector3(-vTranslation.x, vTranslation.z, vTranslation.y);
	}; // zmiana, bo T3D ma inny układ
	inline void willBeAnimated()
	{
		if (pSubModel) {
			pSubModel->WillBeAnimated();
		}
	};
	void eventAssign(TEvent* ev);
	inline TEvent* event()
	{
		return evDone;
	};

	TAnimContainer* pNext;
	TAnimContainer* acAnimNext; // lista animacji z eventem, które muszą być przeliczane również bez wyświetlania

private:
	Math3D::vector3 vRotateAngles; // dla obrotów Eulera
	Math3D::vector3 vDesiredAngles;
	double fRotateSpeed;
	Math3D::vector3 vTranslation;
	Math3D::vector3 vTranslateTo;
	double fTranslateSpeed; // może tu dać wektor?
	float4 qCurrent; // aktualny interpolowany
	float4 qStart; // pozycja początkowa (0 dla interpolacji)
	float4 qDesired; // pozycja końcowa (1 dla interpolacji)
	float fAngleCurrent; // parametr interpolacyjny: 0=start, 1=docelowy
	float fAngleSpeed; // zmiana parametru interpolacji w sekundach
	TSubModel* pSubModel;
	float4x4* mAnim; // macierz do animacji kwaternionowych
	// dla kinematyki odwróconej używane są kwaterniony
	float fLength; // długość kości dla IK
	int iAnim; // animacja: +1-obrót Eulera, +2-przesuw, +4-obrót kwaternionem, +8-IK
	//+0x80000000: animacja z eventem, wykonywana poza wyświetlaniem
	//+0x100: pierwszy stopień IK - obrócić w stronę pierwszego potomnego (dziecka)
	//+0x200: drugi stopień IK - dostosować do pozycji potomnego potomnego (wnuka)
	union { // mogą być animacje klatkowe różnego typu, wskaźniki używa AnimModel
		TAnimVocaloidFrame* pMovementData; // wskaźnik do klatki
	};
	TEvent* evDone; // ewent wykonywany po zakończeniu animacji, np. zapór, obrotnicy
};

class TAnimAdvanced
{ // obiekt zaawansowanej animacji submodelu
public:
	TAnimAdvanced();
	~TAnimAdvanced();
	int sortByBone();

	TAnimVocaloidFrame* pMovementData;
	unsigned char* pVocaloidMotionData; // plik animacyjny dla egzemplarza (z eventu)
	double fFrequency; // przeliczenie czasu rzeczywistego na klatki animacji
	double fCurrent; // klatka animacji wyświetlona w poprzedniej klatce renderingu
	double fLast; // klatka kończąca animację
	int iMovements;
};

// opakowanie modelu, określające stan egzemplarza
class TAnimModel : public scene::basic_node
{
	friend class opengl_renderer;

public:
	explicit TAnimModel(const scene::node_data& Nodedata);
	~TAnimModel();
	static void animUpdate(double dt);
	bool init(const std::string& asName, const std::string& asReplacableTexture);
	bool load(cParser* parser, bool ter = false);
	TAnimContainer* addContainer(const std::string& name);
	TAnimContainer* getContainer(const std::string& name = "");
	inline void raAnglesSet(glm::vec3 angles)
	{
		vAngle = { angles };
	};
	void lightSet(const int n, const float v);
	void animationVND(void* pData, double a, double b, double c, double d);
	bool terrainLoaded();
	int terrainCount();
	TSubModel* terrainSquare(int n);
	int flags();
	inline const material_data* material() const
	{
		return &materialData;
	}
	inline TModel3d* model()
	{
		return pModel;
	}

	static TAnimContainer* acAnimList; // lista animacji z eventem, które muszą być przeliczane również bez wyświetlania

private:
	void raPrepare(); // ustawienie animacji egzemplarza na wzorcu
	void raAnimate(const unsigned int framestamp); // przeliczenie animacji egzemplarza
	void advanced();
	// radius() subclass details, calculates node's bounding radius
	float radius();
	// serialize() subclass details, sends content of the subclass to provided stream
	void serialize_(std::ostream& output) const override;
	// deserialize() subclass details, restores content of the subclass from provided stream
	void deserialize_(std::istream& input) override;
	// export() subclass details, sends basic content of the class in legacy (text) format to provided stream
	void export_as_text_(std::ostream& output) const override;

	TAnimContainer* pRoot{ nullptr }; // pojemniki sterujące, tylko dla aniomowanych submodeli
	TModel3d* pModel{ nullptr };
	double fBlinkTimer{ 0.0 };
	int iNumLights{ 0 };
	TSubModel* lightsOn[iMaxNumLights]; // Ra: te wskaźniki powinny być w ramach TModel3d
	TSubModel* lightsOff[iMaxNumLights];
	Math3D::vector3 vAngle; // bazowe obroty egzemplarza względem osi
	material_data materialData;
	std::string asText; // tekst dla wyświetlacza znakowego
	TAnimAdvanced* pAdvanced{ nullptr };
	float lsLights[iMaxNumLights];
	// float fDark { DefaultDarkThresholdLevel }; // poziom zapalanie światła (powinno być chyba powiązane z danym światłem?)
	float fOnTime{ 0.66f };
	float fOffTime{ 0.66f + 0.66f }; // były stałymi, teraz mogą być zmienne dla każdego egzemplarza
	unsigned int framestamp{ 0 }; // id of last rendered gfx frame
};

class InstanceTable : public basic_table<TAnimModel>
{
};
