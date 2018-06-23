/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/
#include "stdafx.h"
#include "AnimModel.h"
#include "renderer.h"
#include "MdlMngr.h"
#include "simulation.h"
#include "Globals.h"
#include "Timer.h"
#include "Logs.h"
#include "renderer.h"

TAnimContainer* TAnimModel::acAnimList = nullptr;

TAnimAdvanced::TAnimAdvanced(){};

TAnimAdvanced::~TAnimAdvanced(){};

int TAnimAdvanced::sortByBone()
{ // sortowanie pliku animacji w celu optymalniejszego wykonania
	// rekordy zostają ułożone wg kolejnych ramek dla każdej kości
	// ułożenie kości alfabetycznie nie jest niezbędne, ale upraszcza sortowanie bąbelkowe
	TAnimVocaloidFrame buf; // bufor roboczy (przydało by się pascalowe Swap()
	int i, j, k, swaps = 0, last = iMovements - 1, e;
	for (i = 0; i < iMovements; ++i) {
		for (j = 0; j < 15; ++j) {
			if (pMovementData[i].cBone[j] == '\0') { // jeśli znacznik końca
				for (++j; j < 15; ++j) {
					pMovementData[i].cBone[j] = '\0'; // zerowanie bajtów za znacznikiem końca
				}
			}
		}
	}
	for (i = 0; i < last; ++i) { // do przedostatniego
		// dopóki nie posortowane
		j = i; // z którym porównywać
		k = i; // z którym zamienić (nie trzeba zamieniać, jeśli ==i)
		while (++j < iMovements) {
			e = strcmp(pMovementData[k].cBone, pMovementData[j].cBone); // numery trzeba porównywać inaczej
			if (e > 0) {
				k = j; // trzeba zamienić - ten pod j jest mniejszy
			} else if (!e) {
				if (pMovementData[k].iFrame > pMovementData[j].iFrame) {
					k = j; // numer klatki pod j jest mniejszy
				}
			}
		}
		if (k > i) { // jeśli trzeba przestawić
			memcpy(&buf, pMovementData + i, sizeof(TAnimVocaloidFrame));
			memcpy(pMovementData + i, pMovementData + k, sizeof(TAnimVocaloidFrame));
			memcpy(pMovementData + k, &buf, sizeof(TAnimVocaloidFrame));
			++swaps;
		}
	}
	return swaps;
};

TAnimContainer::TAnimContainer()
{
	pNext = nullptr;
	vRotateAngles = Math3D::vector3(0.0f, 0.0f, 0.0f); // aktualne kąty obrotu
	vDesiredAngles = Math3D::vector3(0.0f, 0.0f, 0.0f); // docelowe kąty obrotu
	fRotateSpeed = 0.0;
	vTranslation = Math3D::vector3(0.0f, 0.0f, 0.0f); // aktualne przesunięcie
	vTranslateTo = Math3D::vector3(0.0f, 0.0f, 0.0f); // docelowe przesunięcie
	fTranslateSpeed = 0.0;
	fAngleSpeed = 0.0;
	pSubModel = nullptr;
	iAnim = 0; // położenie początkowe
	pMovementData = nullptr; // nie ma zaawansowanej animacji
	mAnim = nullptr; // nie ma macierzy obrotu dla submodelu
	evDone = nullptr; // powiadamianie o zakończeniu animacji
	acAnimNext = nullptr; // na razie jest poza listą
}

TAnimContainer::~TAnimContainer()
{
	SafeDelete(pNext);
	delete mAnim; // AnimContainer jest właścicielem takich macierzy
}

bool TAnimContainer::init(TSubModel* pNewSubModel)
{
	fRotateSpeed = 0.0f;
	pSubModel = pNewSubModel;
	return (pSubModel != nullptr);
}

void TAnimContainer::setRotateAnim(Math3D::vector3 vNewRotateAngles, double fNewRotateSpeed)
{
	if (this) {
		vDesiredAngles = vNewRotateAngles;
		fRotateSpeed = fNewRotateSpeed;
		iAnim |= 1;
		/* //Ra 2014-07: jeśli model nie jest renderowany, to obliczyć czas animacji i dodać event
           wewnętrzny
           //można by też ustawić czas początku animacji zamiast pobierać czas ramki i liczyć różnicę
        */
		if (evDone) { // dołączyć model do listy aniomowania, żeby animacje były przeliczane również bez
			// wyświetlania
			if (iAnim >= 0) { // jeśli nie jest jeszcze na liście animacyjnej
				acAnimNext = TAnimModel::acAnimList; // pozostałe doklić sobie jako ogon
				TAnimModel::acAnimList = this; // a wstawić się na początek
				iAnim |= 0x80000000; // dodany do listy
			}
		}
	}
}

void TAnimContainer::setTranslateAnim(Math3D::vector3 vNewTranslate, double fNewSpeed)
{
	if (this) {
		vTranslateTo = vNewTranslate;
		fTranslateSpeed = fNewSpeed;
		iAnim |= 2;
		/* //Ra 2014-07: jeśli model nie jest renderowany, to obliczyć czas animacji i dodać event
           wewnętrzny
           //można by też ustawić czas początku animacji zamiast pobierać czas ramki i liczyć różnicę
        */
		if (evDone) { // dołączyć model do listy aniomowania, żeby animacje były przeliczane również bez
			// wyświetlania
			if (iAnim >= 0) { // jeśli nie jest jeszcze na liście animacyjnej
				acAnimNext = TAnimModel::acAnimList; // pozostałe doklić sobie jako ogon
				TAnimModel::acAnimList = this; // a wstawić się na początek
				iAnim |= 0x80000000; // dodany do listy
			}
		}
	}
}

void TAnimContainer::animSetVMD(double fNewSpeed)
{
	if (this) {
		// skala do ustalenia, "cal" japoński (sun) to nieco ponad 3cm
		// X-w lewo, Y-w górę, Z-do tyłu
		// minimalna wysokość to -7.66, a nadal musi być ponad podłogą
		vTranslateTo = Math3D::vector3(0.1 * pMovementData->f3Vector.x, 0.1 * pMovementData->f3Vector.z, 0.1 * pMovementData->f3Vector.y);
		if (LengthSquared3(vTranslateTo) > 0.0 ? true : LengthSquared3(vTranslation) > 0.0) { // jeśli ma być przesunięte albo jest przesunięcie
			iAnim |= 2; // wyłączy się samo
			if (fNewSpeed > 0.0) {
				fTranslateSpeed = fNewSpeed; // prędkość jest mnożnikiem, nie podlega skalowaniu
			} else { // za późno na animacje, trzeba przestawić
				vTranslation = vTranslateTo;
			}
		}
		// if ((qCurrent.w<1.0)||(pMovementData->qAngle.w<1.0))
		{ // jeśli jest jakiś obrót
			if (!mAnim) {
				mAnim = new float4x4(); // będzie potrzebna macierz animacji
				mAnim->Identity(); // jedynkowanie na początek
			}
			iAnim |= 4; // animacja kwaternionowa
			qStart = qCurrent; // potrzebna początkowa do interpolacji
			//---+ - też niby dobrze, ale nie tak trąca włosy na początku (macha w dół)
			//-+-+ - dłoń ma w górze zamiast na pasie w pozycji początkowej
			//+--+ - głowa do tyłu (broda w górę) w pozycji początkowej
			//--++ - pozycja początkowa dobra, trąca u góry, ale z rękami jakoś nie tak, kółko w
			// przeciwną stronę
			//++++ - kładzie się brzuchem do góry
			//-+++ - ręce w górze na początku, zamiast w dół, łokieć jakby w przeciwną stronę
			//+-++ - nie podnosi ręki do głowy
			//++-+ - dłoń ma w górze zamiast na pasie
			qDesired = Normalize(float4(-pMovementData->qAngle.x, -pMovementData->qAngle.z, -pMovementData->qAngle.y,
			    pMovementData->qAngle.w)); // tu trzeba będzie osie zamienić
			if (fNewSpeed > 0.0) {
				fAngleSpeed = fNewSpeed; // wtedy animować za pomocą interpolacji
				fAngleCurrent = 0.0; // początek interpolacji
			} else { // za późno na animację, można tylko przestawić w docelowe miejsce
				fAngleSpeed = 0.0;
				fAngleCurrent = 1.0; // interpolacja zakończona
				qCurrent = qDesired;
			}
		}
	}
}

// przeliczanie animacji wykonać tylko raz na model
void TAnimContainer::updateModel()
{
	if (pSubModel) { // pozbyć się tego - sprawdzać wcześniej
		if (fTranslateSpeed != 0.0) {
			auto dif = vTranslateTo - vTranslation; // wektor w kierunku docelowym
			double l = LengthSquared3(dif); // długość wektora potrzebnego przemieszczenia
			if (l >= 0.0001) { // jeśli do przemieszczenia jest ponad 1cm
				auto s = Math3D::SafeNormalize(dif); // jednostkowy wektor kierunku
				s = s * (fTranslateSpeed * Timer::GetDeltaTime()); // przemieszczenie w podanym czasie z daną prędkością
				if (LengthSquared3(s) < l) { //żeby nie jechało na drugą stronę
					vTranslation += s;
				} else {
					vTranslation = vTranslateTo; // koniec animacji, "koniec animowania" uruchomi się w następnej klatce
				}
			} else { // koniec animowania
				vTranslation = vTranslateTo;
				fTranslateSpeed = 0.0; // wyłączenie przeliczania wektora
				if (LengthSquared3(vTranslation) <= 0.0001) { // jeśli jest w punkcie początkowym
					iAnim &= ~2; // wyłączyć zmianę pozycji submodelu
				}
				if (evDone) {
					// wykonanie eventu informującego o zakończeniu
					simulation::Events.AddToQuery(evDone, nullptr);
				}
			}
		}
		if (fRotateSpeed != 0.0) {
			bool anim = false;
			auto dif = vDesiredAngles - vRotateAngles;
			double s;
			s = fRotateSpeed * sign(dif.x) * Timer::GetDeltaTime();
			if (fabs(s) >= fabs(dif.x)) {
				vRotateAngles.x = vDesiredAngles.x;
			} else {
				vRotateAngles.x += s;
				anim = true;
			}
			s = fRotateSpeed * sign(dif.y) * Timer::GetDeltaTime();
			if (fabs(s) >= fabs(dif.y)) {
				vRotateAngles.y = vDesiredAngles.y;
			} else {
				vRotateAngles.y += s;
				anim = true;
			}
			s = fRotateSpeed * sign(dif.z) * Timer::GetDeltaTime();
			if (fabs(s) >= fabs(dif.z)) {
				vRotateAngles.z = vDesiredAngles.z;
			} else {
				vRotateAngles.z += s;
				anim = true;
			}
			while (vRotateAngles.x >= 360) {
				vRotateAngles.x -= 360;
			}
			while (vRotateAngles.x <= -360) {
				vRotateAngles.x += 360;
			}
			while (vRotateAngles.y >= 360) {
				vRotateAngles.y -= 360;
			}
			while (vRotateAngles.y <= -360) {
				vRotateAngles.y += 360;
			}
			while (vRotateAngles.z >= 360) {
				vRotateAngles.z -= 360;
			}
			while (vRotateAngles.z <= -360) {
				vRotateAngles.z += 360;
			}
			if (vRotateAngles.x == 0.0) {
				if (vRotateAngles.y == 0.0) {
					if (vRotateAngles.z == 0.0) {
						iAnim &= ~1; // kąty są zerowe
					}
				}
			}
			if (!anim) { // nie potrzeba przeliczać już
				fRotateSpeed = 0.0;
				if (evDone) {
					// wykonanie eventu informującego o zakończeniu
					simulation::Events.AddToQuery(evDone, nullptr);
				}
			}
		}
		if (fAngleSpeed != 0.f) {
			// NOTE: this is angle- not quaternion-based rotation TBD, TODO: switch to quaternion rotations?
			fAngleCurrent += fAngleSpeed * Timer::GetDeltaTime(); // aktualny parametr interpolacji
		}
	}
};

void TAnimContainer::prepareModel()
{ // tutaj zostawić tylko ustawienie submodelu, przeliczanie ma być w UpdateModel()
	if (pSubModel) { // pozbyć się tego - sprawdzać wcześniej
		// nanoszenie animacji na wzorzec
		if (iAnim & 1) { // zmieniona pozycja względem początkowej
			pSubModel->SetRotateXYZ(vRotateAngles);
		} // ustawia typ animacji
		if (iAnim & 2) { // zmieniona pozycja względem początkowej
			pSubModel->SetTranslate(vTranslation);
		}
		if (iAnim & 4) { // zmieniona pozycja względem początkowej
			if (fAngleSpeed > 0.0f) {
				if (fAngleCurrent >= 1.0f) { // interpolacja zakończona, ustawienie na pozycję końcową
					qCurrent = qDesired;
					fAngleSpeed = 0.0; // wyłączenie przeliczania wektora
					if (evDone) {
						// wykonanie eventu informującego o zakończeniu
						simulation::Events.AddToQuery(evDone, nullptr);
					}
				} else { // obliczanie pozycji pośredniej
					// normalizacja jest wymagana do interpolacji w następnej animacji
					qCurrent = Normalize(Slerp(qStart, qDesired, fAngleCurrent)); // interpolacja sferyczna kąta
					// qCurrent=Slerp(qStart,qDesired,fAngleCurrent); //interpolacja sferyczna kąta
					if (qCurrent.w == 1.0) { // rozpoznać brak obrotu i wyłączyć w iAnim w takim przypadku
						iAnim &= ~4; // kąty są zerowe
					}
				}
			}
			mAnim->Quaternion(&qCurrent); // wypełnienie macierzy (wymaga normalizacji?)
			pSubModel->mAnimMatrix = mAnim; // użyczenie do submodelu (na czas renderowania!)
		}
	}
}

void TAnimContainer::updateModelIK()
{ // odwrotna kinematyka wyliczana dopiero po ustawieniu macierzy w submodelach
	if (pSubModel) { // pozbyć się tego - sprawdzać wcześniej
		if (pSubModel->b_Anim & at_IK) { // odwrotna kinematyka
			float3 d, k;
			TSubModel* ch = pSubModel->ChildGet();
			switch (pSubModel->b_Anim) {
				case at_IK11: // stopa: ustawić w kierunku czubka (pierwszy potomny)
					d = ch->Translation1Get(); // wektor względem aktualnego układu (nie uwzględnia
					// obrotu)
					k = float3(RadToDeg(atan2(d.z, hypot(d.x, d.y))), 0.0, -RadToDeg(atan2(d.y, d.x))); // proste skierowanie na punkt
					pSubModel->SetRotateIK1(k);
					// if (!strcmp(pSubModel->pName,"?Z?“?^?[")) //jak główna kość
					// WriteLog("--> "+AnsiString(k.x)+" "+AnsiString(k.y)+" "+AnsiString(k.z));
					// Ra: to już jest dobrze, może być inna ćwiartka i znak
					break;
				case at_IK22: // udo: ustawić w kierunku pierwszej potomnej pierwszej potomnej (kostki)
					// pozycję kostki należy określić względem kości centralnej (+biodro może być
					// pochylone)
					// potem wyliczyć ewentualne odchylenie w tej i następnej
					// w sumie to proste, jak wyznaczenie kątów w trójkącie o znanej długości boków...
					d = ch->Translation2Get(); // wektor względem aktualnego układu (nie uwzględnia
					// obrotu)
					// if ()
					{ // kość IK jest dalej niż pozycja spoczynkowa
						k = float3(RadToDeg(atan2(d.z, hypot(d.x, d.y))), 0.0, -RadToDeg(atan2(d.y, d.x))); // proste skierowanie na punkt
						pSubModel->SetRotateIK1(k);
					}
					break;
			}
		}
	}
}

bool TAnimContainer::inMovement()
{ // czy trwa animacja - informacja dla obrotnicy
	return (fRotateSpeed != 0.0) || (fTranslateSpeed != 0.0);
}

void TAnimContainer::eventAssign(TEvent* ev)
{ // przypisanie eventu wykonywanego po zakończeniu animacji
	evDone = ev;
};

TAnimModel::TAnimModel(const scene::node_data& nodeData) : basic_node(nodeData)
{
	// TODO: wrap these in a tuple and move to underlying model
	for (int index = 0; index < iMaxNumLights; ++index) {
		lightsOn[index] = lightsOff[index] = nullptr; // normalnie nie ma
		lsLights[index] = LS_OFF; // a jeśli są, to wyłączone
	}
}

TAnimModel::~TAnimModel()
{
	delete pAdvanced; // nie ma zaawansowanej animacji
	SafeDelete(pRoot);
}

bool TAnimModel::init(const std::string& asName, const std::string& asReplacableTexture)
{
	if (asReplacableTexture.substr(0, 1) == "*") {
		// od gwiazdki zaczynają się teksty na wyświetlaczach
		asText = asReplacableTexture.substr(1, asReplacableTexture.length() - 1); // zapamiętanie tekstu
	} else if (asReplacableTexture != "none") {
		materialData.replacable_skins[1] = GfxRenderer.Fetch_Material(asReplacableTexture);
	}
	if ((materialData.replacable_skins[1] != null_handle) && (GfxRenderer.Material(materialData.replacable_skins[1]).has_alpha)) {
		// tekstura z kanałem alfa - nie renderować w cyklu nieprzezroczystych
		materialData.textures_alpha = 0x31310031;
	} else {
		// tekstura nieprzezroczysta - nie renderować w cyklu przezroczystych
		materialData.textures_alpha = 0x30300030;
	}

	fBlinkTimer = double(Random(1000 * fOffTime)) / (1000 * fOffTime);

	pModel = TModelsManager::GetModel(asName);
	return (pModel != nullptr);
}

bool TAnimModel::load(cParser* parser, bool ter)
{ // rozpoznanie wpisu modelu i ustawienie świateł
	std::string name = parser->getToken<std::string>();
	std::string texture = parser->getToken<std::string>(); // tekstura (zmienia na małe)
	replace_slashes(name);
	if (!init(name, texture)) {
		if (name != "notload") { // gdy brak modelu
			if (ter) { // jeśli teren
				if (name.substr(name.rfind('.')) == ".t3d") {
					name[name.length() - 3] = 'e';
				}
#ifdef EU07_USE_OLD_TERRAINCODE
				Global.asTerrainModel = name;
				WriteLog("Terrain model \"" + name + "\" will be created.");
#endif
			} else
				ErrorLog("Missed file: " + name);
		}
	} else { // wiązanie świateł, o ile model wczytany
		lightsOn[0] = pModel->GetFromName("Light_On00");
		lightsOn[1] = pModel->GetFromName("Light_On01");
		lightsOn[2] = pModel->GetFromName("Light_On02");
		lightsOn[3] = pModel->GetFromName("Light_On03");
		lightsOn[4] = pModel->GetFromName("Light_On04");
		lightsOn[5] = pModel->GetFromName("Light_On05");
		lightsOn[6] = pModel->GetFromName("Light_On06");
		lightsOn[7] = pModel->GetFromName("Light_On07");
		lightsOff[0] = pModel->GetFromName("Light_Off00");
		lightsOff[1] = pModel->GetFromName("Light_Off01");
		lightsOff[2] = pModel->GetFromName("Light_Off02");
		lightsOff[3] = pModel->GetFromName("Light_Off03");
		lightsOff[4] = pModel->GetFromName("Light_Off04");
		lightsOff[5] = pModel->GetFromName("Light_Off05");
		lightsOff[6] = pModel->GetFromName("Light_Off06");
		lightsOff[7] = pModel->GetFromName("Light_Off07");
	}
	for (int i = 0; i < iMaxNumLights; ++i) {
		if (lightsOn[i] || lightsOff[i]) { // Ra: zlikwidowałem wymóg istnienia obu
			iNumLights = i + 1;
		}
	}

	if (parser->getToken<std::string>() == "lights") {
		int i = 0;
		std::string token = parser->getToken<std::string>();
		while (!token.empty() && (token != "endmodel")) {
			lightSet(i, std::stof(token)); // stan światła jest liczbą z ułamkiem
			++i;

			token = "";
			parser->getTokens();
			*parser >> token;
		}
	}
	return true;
}

TAnimContainer* TAnimModel::addContainer(const std::string& name)
{ // dodanie sterowania submodelem dla egzemplarza
	if (!pModel) {
		return nullptr;
	}
	TSubModel* tsb = pModel->GetFromName(name);
	if (tsb) {
		TAnimContainer* tmp = new TAnimContainer();
		tmp->init(tsb);
		tmp->pNext = pRoot;
		pRoot = tmp;
		return tmp;
	}
	return nullptr;
}

TAnimContainer* TAnimModel::getContainer(const std::string& name)
{ // szukanie/dodanie sterowania submodelem dla egzemplarza
	if (name.empty()) {
		return pRoot; // pobranie pierwszego (dla obrotnicy)
	}
	TAnimContainer* pCurrent;
	for (pCurrent = pRoot; pCurrent != nullptr; pCurrent = pCurrent->pNext) {
		if (name == pCurrent->nameGet()) {
			return pCurrent;
		}
	}
	return addContainer(name);
}

// przeliczenie animacji - jednorazowo na klatkę
void TAnimModel::raAnimate(const unsigned int framestamp)
{
	if (framestamp == this->framestamp) {
		return;
	}

	fBlinkTimer -= Timer::GetDeltaTime();
	if (fBlinkTimer <= 0) {
		fBlinkTimer += fOffTime;
	}

	// Ra 2F1I: to by można pomijać dla modeli bez animacji, których jest większość
	TAnimContainer* pCurrent;
	for (pCurrent = pRoot; pCurrent != nullptr; pCurrent = pCurrent->pNext) {
		if (!pCurrent->evDone) { // jeśli jest bez eventu
			pCurrent->updateModel(); // przeliczenie animacji każdego submodelu
		}
	}
	// if () //tylko dla modeli z IK !!!!
	for (pCurrent = pRoot; pCurrent != nullptr; pCurrent = pCurrent->pNext) { // albo osobny łańcuch
		pCurrent->updateModelIK(); // przeliczenie odwrotnej kinematyki
	}

	this->framestamp = framestamp;
};

void TAnimModel::raPrepare()
{ // ustawia światła i animacje we wzorcu modelu przed renderowaniem egzemplarza
	bool state; // stan światła
	for (int i = 0; i < iNumLights; ++i) {
		auto const lightmode{ static_cast<int>(lsLights[i]) };
		switch (lightmode) {
			case LS_BLINK: // migotanie
				state = (fBlinkTimer < fOnTime);
				break;
			case LS_DARK: // zapalone, gdy ciemno
				state = (Global.fLuminance <= (lsLights[i] == 3.f ? defaultDarkThresholdLevel : (lsLights[i] - 3.f)));
				break;
			default: // zapalony albo zgaszony
				state = (lightmode == LS_ON);
		}
		if (lightsOn[i]) {
			lightsOn[i]->iVisible = state;
		}
		if (lightsOff[i]) {
			lightsOff[i]->iVisible = !state;
		}
	}
	TSubModel::iInstance = (size_t)this; //żeby nie robić cudzych animacji
	TSubModel::pasText = &asText; // przekazanie tekstu do wyświetlacza (!!!! do przemyślenia)
	if (pAdvanced) { // jeśli jest zaawansowana animacja
		advanced(); // wykonać co tam trzeba
	}
	TAnimContainer* pCurrent;
	for (pCurrent = pRoot; pCurrent != nullptr; pCurrent = pCurrent->pNext) {
		pCurrent->prepareModel(); // ustawienie animacji egzemplarza dla każdego submodelu
	}
}

int TAnimModel::flags()
{ // informacja dla TGround, czy ma być w Render, RenderAlpha, czy RenderMixed
	int i = pModel ? pModel->Flags() : 0; // pobranie flag całego modelu
	if (materialData.replacable_skins[1] > 0) { // jeśli ma wymienną teksturę 0
		i |= (i & 0x01010001) * ((materialData.textures_alpha & 1) ? 0x20 : 0x10);
	}
	return i;
};

bool TAnimModel::terrainLoaded()
{ // zliczanie kwadratów kilometrowych (główna linia po Next) do tworznia tablicy
	return (this ? pModel != nullptr : false);
};
int TAnimModel::terrainCount()
{ // zliczanie kwadratów kilometrowych (główna linia po Next) do tworznia tablicy
	return pModel ? pModel->TerrainCount() : 0;
};
TSubModel* TAnimModel::terrainSquare(int n)
{ // pobieranie wskaźników do pierwszego submodelu
	return pModel ? pModel->TerrainSquare(n) : 0;
};

void TAnimModel::advanced()
{ // wykonanie zaawansowanych animacji na submodelach
	pAdvanced->fCurrent += pAdvanced->fFrequency * Timer::GetDeltaTime(); // aktualna ramka zmiennoprzecinkowo
	int frame = floor(pAdvanced->fCurrent); // numer klatki jako int
	TAnimContainer* pCurrent;
	if (pAdvanced->fCurrent >= pAdvanced->fLast) { // animacja została zakończona
		delete pAdvanced;
		pAdvanced = nullptr; // dalej już nic
		for (pCurrent = pRoot; pCurrent != nullptr; pCurrent = pCurrent->pNext) {
			if (pCurrent->pMovementData) { // jeśli obsługiwany tabelką animacji
				pCurrent->pMovementData = nullptr; // usuwanie wskaźników
			}
		}
	} else { // coś trzeba poanimować - wszystkie animowane submodele są w tym łańcuchu
		for (pCurrent = pRoot; pCurrent != nullptr; pCurrent = pCurrent->pNext) {
			if (pCurrent->pMovementData) { // jeśli obsługiwany tabelką animacji
				if (frame >= pCurrent->pMovementData->iFrame) { // koniec czekania
					if (!strcmp(pCurrent->pMovementData->cBone, (pCurrent->pMovementData + 1)->cBone)) { // jak kolejna ramka dotyczy tego samego submodelu, ustawić animację do kolejnej ramki
						++pCurrent->pMovementData; // kolejna klatka
						pCurrent->animSetVMD(pAdvanced->fFrequency / (double(pCurrent->pMovementData->iFrame) - pAdvanced->fCurrent));
					} else {
						pCurrent->pMovementData = nullptr; // inna nazwa, animowanie zakończone w aktualnym położeniu
					}
				}
			}
		}
	}
};

void TAnimModel::animationVND(void* pData, double a, double b, double c, double d)
{ // rozpoczęcie wykonywania animacji z podanego pliku
	// tabela w pliku musi być posortowana wg klatek dla kolejnych kości!
	// skrócone nagranie ma 3:42 = 222 sekundy, animacja kończy się na klatce 6518
	// daje to 29.36 (~=30) klatek na sekundę
	// w opisach jest podawane 24 albo 36 jako standard => powiedzmy, parametr (d) to FPS animacji
	delete pAdvanced; // usunięcie ewentualnego poprzedniego
	pAdvanced = nullptr; // gdyby się nie udało rozpoznać pliku
	if (std::string(static_cast<char*>(pData)) == "Vocaloid Motion Data 0002") {
		pAdvanced = new TAnimAdvanced();
		pAdvanced->pVocaloidMotionData = static_cast<unsigned char*>(pData); // podczepienie pliku danych
		pAdvanced->iMovements = *((int*)(((char*)pData) + 50)); // numer ostatniej klatki
		pAdvanced->pMovementData = (TAnimVocaloidFrame*)(((char*)pData) + 54); // rekordy animacji
		// WriteLog(sizeof(TAnimVocaloidFrame));
		pAdvanced->fFrequency = d;
		pAdvanced->fCurrent = 0.0; // aktualna ramka
		pAdvanced->fLast = 0.0; // ostatnia ramka

		std::string name;
		TAnimContainer* pSub;
		for (int i = 0; i < pAdvanced->iMovements; ++i) {
			if (strcmp(pAdvanced->pMovementData[i].cBone, name.c_str())) { // jeśli pozycja w tabelce nie była wyszukiwana w submodelach
				pSub = getContainer(pAdvanced->pMovementData[i].cBone); // szukanie
				if (pSub) { // znaleziony
					pSub->pMovementData = pAdvanced->pMovementData + i; // gotów do animowania
					pSub->animSetVMD(0.0); // usuawienie pozycji początkowej (powinna być zerowa,
					// inaczej będzie skok)
				}
				name = std::string(pAdvanced->pMovementData[i].cBone); // nowa nazwa do pomijania
			}
			if (pAdvanced->fLast < pAdvanced->pMovementData[i].iFrame) {
				pAdvanced->fLast = pAdvanced->pMovementData[i].iFrame;
			}
		}
	}
};

void TAnimModel::lightSet(int const n, float const v)
{ // ustawienie światła (n) na wartość (v)
	if (n >= iMaxNumLights) {
		return; // przekroczony zakres
	}
	lsLights[n] = v;
	switch (static_cast<int>(lsLights[n])) {
		// interpretacja ułamka zależnie od typu
		case LS_OFF: {
			// ustalenie czasu migotania, t<1s (f>1Hz), np. 0.1 => t=0.1 (f=10Hz)
			break;
		}
		case LS_ON: {
			// ustalenie wypełnienia ułamkiem, np. 1.25 => zapalony przez 1/4 okresu
			break;
		}
		case LS_BLINK: {
			// ustalenie częstotliwości migotania, f<1Hz (t>1s), np. 2.2 => f=0.2Hz (t=5s)
			break;
		}
		case LS_DARK: {
			// zapalenie świateł zależne od oświetlenia scenerii
			break;
		}
	}
};

void TAnimModel::animUpdate(double dt)
{ // wykonanie zakolejkowanych animacji, nawet gdy modele nie są aktualnie wyświetlane
	TAnimContainer* p = TAnimModel::acAnimList;
	while (p) {
		p->updateModel();
		p = p->acAnimNext; // na razie bez usuwania z listy, bo głównie obrotnica na nią wchodzi
	}
};

// radius() subclass details, calculates node's bounding radius
float TAnimModel::radius()
{
	return (pModel ? pModel->bounding_radius() : 0.f);
}

// serialize() subclass details, sends content of the subclass to provided stream
void TAnimModel::serialize_(std::ostream& output) const
{
	// TODO: implement
}
// deserialize() subclass details, restores content of the subclass from provided stream
void TAnimModel::deserialize_(std::istream& input)
{
	// TODO: implement
}

// export() subclass details, sends basic content of the class in legacy (text) format to provided stream
void TAnimModel::export_as_text_(std::ostream& output) const
{
	// header
	output << "model ";
	// location and rotation
	output << location().x << ' ' << location().y << ' ' << location().z << ' ' << vAngle.y << ' ';
	// 3d shape
	auto modelfile{ (pModel ? pModel->NameGet() + ".t3d" : "none") }; // rainsted requires model file names to include an extension
	if (modelfile.find(szModelPath) == 0) {
		// don't include 'models/' in the path
		modelfile.erase(0, std::string{ szModelPath }.size());
	}
	output << modelfile << ' ';
	// texture
	auto texturefile{ (materialData.replacable_skins[1] != null_handle ? GfxRenderer.Material(materialData.replacable_skins[1]).name : "none") };
	if (texturefile.find(szTexturePath) == 0) {
		// don't include 'textures/' in the path
		texturefile.erase(0, std::string{ szTexturePath }.size());
	}
	output << texturefile << ' ';
	// light submodels activation configuration
	if (iNumLights > 0) {
		output << "lights ";
		for (int lightidx = 0; lightidx < iNumLights; ++lightidx) {
			output << lsLights[lightidx] << ' ';
		}
	}
	// footer
	output << "endmodel\n";
}
