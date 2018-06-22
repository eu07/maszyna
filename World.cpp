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
#include "World.h"

#include "Globals.h"
#include "simulation.h"
#include "Logs.h"
#include "MdlMngr.h"
#include "renderer.h"
#include "Timer.h"
#include "mtable.h"
#include "sound.h"
#include "Camera.h"
#include "ResourceManager.h"
#include "Event.h"
#include "Train.h"
#include "Driver.h"
#include "Console.h"
#include "color.h"
#include "uilayer.h"

//---------------------------------------------------------------------------

namespace simulation
{
	simulation_time Time;

	basic_station Station;
}

#ifdef _WIN32
extern "C" {
GLFWAPI HWND glfwGetWin32Window(GLFWwindow* window);
}
#endif

void simulation_time::init()
{
	char monthdaycounts[2][13] = { { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }, { 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 } };
	::memcpy(m_monthdaycounts, monthdaycounts, sizeof(monthdaycounts));

	// potentially adjust scenario clock
	auto const requestedtime{ clamp_circular<int>(m_time.wHour * 60 + m_time.wMinute + Global.ScenarioTimeOffset * 60, 24 * 60) };
	auto const requestedhour{ (requestedtime / 60) % 24 };
	auto const requestedminute{ requestedtime % 60 };
	// cache requested elements, if any

#ifdef __linux__
	timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	tm* tms = localtime(&ts.tv_sec);
	m_time.wYear = tms->tm_year;
	m_time.wMonth = tms->tm_mon;
	m_time.wDayOfWeek = tms->tm_wday;
	m_time.wDay = tms->tm_mday;
	m_time.wHour = tms->tm_hour;
	m_time.wMinute = tms->tm_min;
	m_time.wSecond = tms->tm_sec;
	m_time.wMilliseconds = ts.tv_nsec / 1000000;
#elif _WIN32
	::GetLocalTime(&m_time);
#endif

	if (Global.fMoveLight > 0.0) {
		// day and month of the year can be overriden by scenario setup
		daymonth(m_time.wDay, m_time.wMonth, m_time.wYear, static_cast<WORD>(Global.fMoveLight));
	}

	if (requestedhour != -1) {
		m_time.wHour = static_cast<WORD>(clamp(requestedhour, 0, 23));
	}
	if (requestedminute != -1) {
		m_time.wMinute = static_cast<WORD>(clamp(requestedminute, 0, 59));
	}
	// if the time is taken from the local clock leave the seconds intact, otherwise set them to zero
	if ((requestedhour != -1) || (requestedminute != 1)) {
		m_time.wSecond = 0;
	}

	m_yearday = year_day(m_time.wDay, m_time.wMonth, m_time.wYear);
}

void simulation_time::update(double const Deltatime)
{
	m_milliseconds += (1000.0 * Deltatime);
	while (m_milliseconds >= 1000.0) {
		++m_time.wSecond;
		m_milliseconds -= 1000.0;
	}
	m_time.wMilliseconds = std::floor(m_milliseconds);
	while (m_time.wSecond >= 60) {
		++m_time.wMinute;
		m_time.wSecond -= 60;
	}
	while (m_time.wMinute >= 60) {
		++m_time.wHour;
		m_time.wMinute -= 60;
	}
	while (m_time.wHour >= 24) {
		++m_time.wDay;
		++m_time.wDayOfWeek;
		if (m_time.wDayOfWeek >= 7) {
			m_time.wDayOfWeek -= 7;
		}
		m_time.wHour -= 24;
	}
	int leap = (m_time.wYear % 4 == 0) && (m_time.wYear % 100 != 0) || (m_time.wYear % 400 == 0);
	while (m_time.wDay > m_monthdaycounts[leap][m_time.wMonth]) {
		m_time.wDay -= m_monthdaycounts[leap][m_time.wMonth];
		++m_time.wMonth;
		// unlikely but we might've entered a new year
		if (m_time.wMonth > 12) {
			++m_time.wYear;
			leap = (m_time.wYear % 4 == 0) && (m_time.wYear % 100 != 0) || (m_time.wYear % 400 == 0);
			m_time.wMonth -= 12;
		}
	}
}

int simulation_time::year_day(int Day, const int Month, const int Year) const
{
	char const daytab[2][13] = { { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }, { 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 } };

	int leap{ (Year % 4 == 0) && (Year % 100 != 0) || (Year % 400 == 0) };
	for (int i = 1; i < Month; ++i)
		Day += daytab[leap][i];

	return Day;
}

void simulation_time::daymonth(WORD& Day, WORD& Month, WORD const Year, WORD const Yearday)
{
	WORD daytab[2][13] = { { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 }, { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 } };

	int leap = (Year % 4 == 0) && (Year % 100 != 0) || (Year % 400 == 0);
	WORD idx = 1;
	while ((idx < 13) && (Yearday >= daytab[leap][idx])) {
		++idx;
	}
	Month = idx;
	Day = Yearday - daytab[leap][idx - 1];
}

int simulation_time::julian_day() const
{
	int yy = (m_time.wYear < 0 ? m_time.wYear + 1 : m_time.wYear) - std::floor((12 - m_time.wMonth) / 10.f);
	int mm = m_time.wMonth + 9;
	if (mm >= 12) {
		mm -= 12;
	}

	int K1 = std::floor(365.25 * (yy + 4712));
	int K2 = std::floor(30.6 * mm + 0.5);

	// for dates in Julian calendar
	int JD = K1 + K2 + m_time.wDay + 59;
	// for dates in Gregorian calendar; 2299160 is October 15th, 1582
	const int gregorianswitchday = 2299160;
	if (JD > gregorianswitchday) {
		int K3 = std::floor(std::floor((yy * 0.01) + 49) * 0.75) - 38;
		JD -= K3;
	}

	return JD;
}

TWorld::TWorld()
{
	Train = NULL;
	for (int i = 0; i < 10; ++i)
		KeyEvents[i] = NULL; // eventy wyzwalane klawiszami cyfrowymi
	Global.iSlowMotion = 0;
	fTimeBuffer = 0.0; // bufor czasu aktualizacji dla stałego kroku fizyki
	fMaxDt = 0.01; //[s] początkowy krok czasowy fizyki
	fTime50Hz = 0.0; // bufor czasu dla komunikacji z PoKeys
}

TWorld::~TWorld()
{
	TrainDelete();
}

void TWorld::TrainDelete(TDynamicObject* d)
{ // usunięcie pojazdu prowadzonego przez użytkownika
	if (d)
		if (Train)
			if (Train->Dynamic() != d)
				return; // nie tego usuwać
#ifdef EU07_SCENERY_EDITOR
	if ((Train->DynamicObject) && (Train->DynamicObject->Mechanik)) {
		// likwidacja kabiny wymaga przejęcia przez AI
		Train->DynamicObject->Mechanik->TakeControl(true);
	}
#endif
	delete Train; // i nie ma czym sterować
	Train = NULL;
	Controlled = NULL; // tego też już nie ma
	mvControlled = NULL;
};

bool TWorld::Init(GLFWwindow* Window)
{
	auto timestart = std::chrono::system_clock::now();

	window = Window;
	Global.window = Window; // do WM_COPYDATA
	Global.pCamera = &Camera; // Ra: wskaźnik potrzebny do likwidacji drgań

	WriteLog("\nStarting MaSzyna rail vehicle simulator (release: " + Global.asVersion + ")");
	WriteLog("For online documentation and additional files refer to: http://eu07.pl");

	UILayer.set_background("logo");
	glfwSetWindowTitle(window, (Global.AppName + " (" + Global.SceneryFile + ")").c_str()); // nazwa scenerii
	UILayer.set_progress(0.01);
	UILayer.set_progress("Loading scenery / Wczytywanie scenerii");

	GfxRenderer.Render();

	WriteLog("World setup...");
	if (false == simulation::State.deserialize(Global.SceneryFile)) {
		return false;
	}

	simulation::Time.init();

	Environment.init();
	Camera.init(Global.FreeCameraInit[0], Global.FreeCameraInitAngle[0]);

	UILayer.set_progress("Preparing train / Przygotowanie kabiny");
	WriteLog("Player train init: " + Global.asHumanCtrlVehicle);

	TDynamicObject* nPlayerTrain;
	if (Global.asHumanCtrlVehicle != "ghostview")
		nPlayerTrain = simulation::Vehicles.find(Global.asHumanCtrlVehicle);
	if (nPlayerTrain) {
		Train = new TTrain();
		if (Train->Init(nPlayerTrain)) {
			Controlled = Train->Dynamic();
			mvControlled = Controlled->ControlledFind()->MoverParameters;
			WriteLog("Player train init OK");

			glfwSetWindowTitle(window, (Global.AppName + " (" + Controlled->MoverParameters->Name + " @ " + Global.SceneryFile + ")").c_str());

			FollowView();
		} else {
			Error("Player train init failed!");
			FreeFlyModeFlag = true; // Ra: automatycznie włączone latanie
			Controlled = NULL;
			mvControlled = NULL;
			SafeDelete(Train);
			Camera.type = TP_FREE;
		}
	} else {
		if (Global.asHumanCtrlVehicle != "ghostview") {
			Error("Player train doesn't exist!");
		}
		FreeFlyModeFlag = true; // Ra: automatycznie włączone latanie
		glfwSwapBuffers(window);
		Controlled = NULL;
		mvControlled = NULL;
		Camera.type = TP_FREE;
		DebugCamera = Camera;
		Global.DebugCameraPosition = DebugCamera.pos;
	}

	// if (!Global.bMultiplayer) //na razie włączone
	{ // eventy aktywowane z klawiatury tylko dla jednego użytkownika
		KeyEvents[0] = simulation::Events.FindEvent("keyctrl00");
		KeyEvents[1] = simulation::Events.FindEvent("keyctrl01");
		KeyEvents[2] = simulation::Events.FindEvent("keyctrl02");
		KeyEvents[3] = simulation::Events.FindEvent("keyctrl03");
		KeyEvents[4] = simulation::Events.FindEvent("keyctrl04");
		KeyEvents[5] = simulation::Events.FindEvent("keyctrl05");
		KeyEvents[6] = simulation::Events.FindEvent("keyctrl06");
		KeyEvents[7] = simulation::Events.FindEvent("keyctrl07");
		KeyEvents[8] = simulation::Events.FindEvent("keyctrl08");
		KeyEvents[9] = simulation::Events.FindEvent("keyctrl09");
	}

	WriteLog("Load time: " + std::to_string(std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() - timestart)).count()) + " seconds");

	UILayer.set_progress();
	UILayer.set_progress("");
	UILayer.set_background("");
	ui_log->enabled = false;

	Timer::ResetTimers();

	return true;
};

void TWorld::OnKeyDown(int cKey)
{
	// dump keypress info in the log
	// podczas pauzy klawisze nie działają
	std::string keyinfo;
	auto keyname = glfwGetKeyName(cKey, 0);
	if (keyname != nullptr) {
		keyinfo += std::string(keyname);
	} else {
		switch (cKey) {
			case GLFW_KEY_SPACE: {
				keyinfo += "Space";
				break;
			}
			case GLFW_KEY_ENTER: {
				keyinfo += "Enter";
				break;
			}
			case GLFW_KEY_ESCAPE: {
				keyinfo += "Esc";
				break;
			}
			case GLFW_KEY_TAB: {
				keyinfo += "Tab";
				break;
			}
			case GLFW_KEY_INSERT: {
				keyinfo += "Insert";
				break;
			}
			case GLFW_KEY_DELETE: {
				keyinfo += "Delete";
				break;
			}
			case GLFW_KEY_HOME: {
				keyinfo += "Home";
				break;
			}
			case GLFW_KEY_END: {
				keyinfo += "End";
				break;
			}
			case GLFW_KEY_F1: {
				keyinfo += "F1";
				break;
			}
			case GLFW_KEY_F2: {
				keyinfo += "F2";
				break;
			}
			case GLFW_KEY_F3: {
				keyinfo += "F3";
				break;
			}
			case GLFW_KEY_F4: {
				keyinfo += "F4";
				break;
			}
			case GLFW_KEY_F5: {
				keyinfo += "F5";
				break;
			}
			case GLFW_KEY_F6: {
				keyinfo += "F6";
				break;
			}
			case GLFW_KEY_F7: {
				keyinfo += "F7";
				break;
			}
			case GLFW_KEY_F8: {
				keyinfo += "F8";
				break;
			}
			case GLFW_KEY_F9: {
				keyinfo += "F9";
				break;
			}
			case GLFW_KEY_F10: {
				keyinfo += "F10";
				break;
			}
			case GLFW_KEY_F11: {
				keyinfo += "F11";
				break;
			}
			case GLFW_KEY_F12: {
				keyinfo += "F12";
				break;
			}
			case GLFW_KEY_PAUSE: {
				keyinfo += "Pause";
				break;
			}
		}
	}
	if (keyinfo.empty() == false) {
		std::string keymodifiers;
		if (Global.shiftState)
			keymodifiers += "[Shift]+";
		if (Global.ctrlState)
			keymodifiers += "[Ctrl]+";

		WriteLog("Key pressed: " + keymodifiers + "[" + keyinfo + "]");
	}

	// actual key processing
	// TODO: redo the input system
	if ((cKey >= GLFW_KEY_0) && (cKey <= GLFW_KEY_9)) // klawisze cyfrowe
	{
		int i = cKey - GLFW_KEY_0; // numer klawisza
		if (Global.shiftState) {
			// z [Shift] uruchomienie eventu
			if ((false == Global.iPause) // podczas pauzy klawisze nie działają
			    && (KeyEvents[i] != nullptr)) {
				simulation::Events.AddToQuery(KeyEvents[i], NULL);
			}
		} else // zapamiętywanie kamery może działać podczas pauzy
		if (FreeFlyModeFlag) // w trybie latania można przeskakiwać do ustawionych kamer
			if ((Global.iTextMode != GLFW_KEY_F12) && (Global.iTextMode != GLFW_KEY_F3)) // ograniczamy użycie kamer
			{
				if ((!Global.FreeCameraInit[i].x) && (!Global.FreeCameraInit[i].y) && (!Global.FreeCameraInit[i].z)) { // jeśli kamera jest w punkcie zerowym, zapamiętanie współrzędnych i kątów
					Global.FreeCameraInit[i] = Camera.pos;
					Global.FreeCameraInitAngle[i].x = Camera.pitch;
					Global.FreeCameraInitAngle[i].y = Camera.yaw;
					Global.FreeCameraInitAngle[i].z = Camera.roll;
					// logowanie, żeby można było do scenerii przepisać
					WriteLog("camera " + std::to_string(Global.FreeCameraInit[i].x) + " " + std::to_string(Global.FreeCameraInit[i].y) + " " + std::to_string(Global.FreeCameraInit[i].z) + " " +
					         std::to_string(RadToDeg(Global.FreeCameraInitAngle[i].x)) + " " + std::to_string(RadToDeg(Global.FreeCameraInitAngle[i].y)) + " " +
					         std::to_string(RadToDeg(Global.FreeCameraInitAngle[i].z)) + " " + std::to_string(i) + " endcamera");
				} else // również przeskakiwanie
				{ // Ra: to z tą kamerą (Camera.Pos i Global.pCameraPosition) jest trochę bez sensu
					Global.pCameraPosition = Global.FreeCameraInit[i]; // nowa pozycja dla generowania obiektów
					Camera.init(Global.FreeCameraInit[i],
					Global.FreeCameraInitAngle[i]); // przestawienie
				}
			}
		// będzie jeszcze załączanie sprzęgów z [Ctrl]
	} else if ((cKey >= GLFW_KEY_F1) && (cKey <= GLFW_KEY_F12)) {
		switch (cKey) {
			case GLFW_KEY_F1: {
				if (DebugModeFlag) {
					// additional simulation clock jump keys in debug mode
					if (Global.ctrlState) {
						// ctrl-f1
						simulation::Time.update(20.0 * 60.0);
					} else if (Global.shiftState) {
						// shift-f1
						simulation::Time.update(5.0 * 60.0);
					}
				}
				break;
			}

			case GLFW_KEY_F4: {
				InOutKey(!Global.shiftState); // distant view with Shift, short distance step out otherwise
				break;
			}
			case GLFW_KEY_F5: {
				// przesiadka do innego pojazdu
				if (false == FreeFlyModeFlag) {
					// only available in free fly mode
					break;
				}

				TDynamicObject* tmp = std::get<TDynamicObject*>(simulation::Region->find_vehicle(Global.pCameraPosition, 50, true, false));

				if ((tmp != nullptr) && (tmp != Controlled)) {
					if (Controlled) // jeśli mielismy pojazd
						if (Controlled->Mechanik) // na skutek jakiegoś błędu może czasem zniknąć
							Controlled->Mechanik->TakeControl(true); // oddajemy dotychczasowy AI

					if (DebugModeFlag || (tmp->MoverParameters->Vel <= 5.0)) {
						// works always in debug mode, or for stopped/slow moving vehicles otherwise
						Controlled = tmp; // przejmujemy nowy
						mvControlled = Controlled->ControlledFind()->MoverParameters;
						if (Train == nullptr)
							Train = new TTrain(); // jeśli niczym jeszcze nie jeździlismy
						if (Train->Init(Controlled)) { // przejmujemy sterowanie
							if (!DebugModeFlag) {
								// w DebugMode nadal prowadzi AI
								Controlled->Mechanik->TakeControl(false);
							}
						} else {
							SafeDelete(Train); // i nie ma czym sterować
						}
						if (Train) {
							InOutKey(); // do kabiny
						}
					}
				}
				break;
			}
			case GLFW_KEY_F6: {
				// przyspieszenie symulacji do testowania scenerii... uwaga na FPS!
				if (DebugModeFlag) {
					if (Global.ctrlState) {
						Global.fTimeSpeed = (Global.shiftState ? 60.0 : 20.0);
					} else {
						Global.fTimeSpeed = (Global.shiftState ? 5.0 : 1.0);
					}
				}
				break;
			}
			case GLFW_KEY_F7: {
				// debug mode functions
				if (DebugModeFlag) {
					if (Global.ctrlState && Global.shiftState) {
						// shift + ctrl + f7 toggles between debug and regular camera
						DebugCameraFlag = !DebugCameraFlag;
					} else if (Global.ctrlState) {
						// ctrl + f7 toggles static daylight
						ToggleDaylight();
						break;
					} else if (Global.shiftState) {
						// shift + f7 is currently unused
					} else {
						// f7: wireframe toggle
						// TODO: pass this to renderer instead of making direct calls
						Global.bWireFrame = !Global.bWireFrame;
						if (true == Global.bWireFrame) {
							glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
						} else {
							glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
						}
					}
				}
				break;
			}
			case GLFW_KEY_F11: {
				// scenery export
				if (Global.ctrlState && Global.shiftState) {
					simulation::State.export_as_text(Global.SceneryFile);
				}
				break;
			}
			case GLFW_KEY_F12: {
				// quick debug mode toggle
				if (Global.ctrlState && Global.shiftState) {
					DebugModeFlag = !DebugModeFlag;
				}
				break;
			}
			default: {
				break;
			}
		}
		// if (cKey!=VK_F4)
		return; // nie są przekazywane do pojazdu wcale
	}

	if ((Global.iTextMode == GLFW_KEY_F12) ? (cKey >= '0') && (cKey <= '9') : false) { // tryb konfiguracji debugmode (przestawianie kamery już wyłączone
		if (!Global.shiftState) // bez [Shift]
		{
			if (cKey == GLFW_KEY_1)
				Global.iWriteLogEnabled ^= 1; // włącz/wyłącz logowanie do pliku
#ifdef _WIN32
			else if (cKey == GLFW_KEY_2) { // włącz/wyłącz okno konsoli
				Global.iWriteLogEnabled ^= 2;
				if ((Global.iWriteLogEnabled & 2) == 0) // nie było okienka
				{ // otwarcie okna
					AllocConsole(); // jeśli konsola już jest, to zwróci błąd; uwalniać nie ma po
					// co, bo się odłączy
					SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN);
				}
			}
#endif
			// else if (cKey=='3') Global::iWriteLogEnabled^=4; //wypisywanie nazw torów
		}
	} else if (cKey == GLFW_KEY_ESCAPE) {
		// toggle pause
		if (Global.iPause & 1) // jeśli pauza startowa
			Global.iPause &= ~1; // odpauzowanie, gdy po wczytaniu miało nie startować
		else if (!(Global.iMultiplayer & 2)) // w multiplayerze pauza nie ma sensu
			Global.iPause ^= 2; // zmiana stanu zapauzowania
		if (Global.iPause) { // jak pauza
			Global.iTextMode = GLFW_KEY_F1; // to wyświetlić zegar i informację
		}
	} else {
		if ((true == DebugModeFlag) && (false == Global.shiftState) && (true == Global.ctrlState) && (Controlled != nullptr) && (Controlled->Controller == Humandriver)) {
			if (DebugModeFlag) {
				// przesuwanie składu o 100m
				TDynamicObject* d = Controlled;
				if (cKey == GLFW_KEY_LEFT_BRACKET) {
					while (d) {
						d->Move(100.0 * d->DirectionGet());
						d = d->Next(); // pozostałe też
					}
					d = Controlled->Prev();
					while (d) {
						d->Move(100.0 * d->DirectionGet());
						d = d->Prev(); // w drugą stronę też
					}
				} else if (cKey == GLFW_KEY_RIGHT_BRACKET) {
					while (d) {
						d->Move(-100.0 * d->DirectionGet());
						d = d->Next(); // pozostałe też
					}
					d = Controlled->Prev();
					while (d) {
						d->Move(-100.0 * d->DirectionGet());
						d = d->Prev(); // w drugą stronę też
					}
				} else if (cKey == GLFW_KEY_TAB) {
					while (d) {
						d->MoverParameters->V += d->DirectionGet() * 2.78;
						d = d->Next(); // pozostałe też
					}
					d = Controlled->Prev();
					while (d) {
						d->MoverParameters->V += d->DirectionGet() * 2.78;
						d = d->Prev(); // w drugą stronę też
					}
				}
			}
		}
	}
}

void TWorld::InOutKey(bool const Near)
{ // przełączenie widoku z kabiny na zewnętrzny i odwrotnie
	FreeFlyModeFlag = !FreeFlyModeFlag; // zmiana widoku
	if (FreeFlyModeFlag) {
		// jeżeli poza kabiną, przestawiamy w jej okolicę - OK
		if (Train) {
			// cache current cab position so there's no need to set it all over again after each out-in switch
			Train->pMechSittingPosition = Train->pMechOffset;

			Train->Dynamic()->bDisplayCab = false;
			DistantView(Near);
		}
		DebugCamera = Camera;
		Global.DebugCameraPosition = DebugCamera.pos;
	} else { // jazda w kabinie
		if (Train) {
			Train->Dynamic()->bDisplayCab = true;
			// zerowanie przesunięcia przed powrotem?
			Train->Dynamic()->ABuSetModelShake({ 0, 0, 0 });
			Train->MechStop();
			FollowView(); // na pozycję mecha
		} else
			FreeFlyModeFlag = true; // nadal poza kabiną
	}
	// update window title to reflect the situation
	glfwSetWindowTitle(window, (Global.AppName + " (" + (Controlled != nullptr ? Controlled->MoverParameters->Name : "") + " @ " + Global.SceneryFile + ")").c_str());
};

// places camera outside the controlled vehicle, or nearest if nothing is under control
// depending on provided switch the view is placed right outside, or at medium distance
void TWorld::DistantView(bool const Near)
{ // ustawienie widoku pojazdu z zewnątrz

	TDynamicObject const* vehicle{ nullptr };
	if (nullptr != Controlled) {
		vehicle = Controlled;
	} else if (nullptr != pDynamicNearest) {
		vehicle = pDynamicNearest;
	} else {
		return;
	}

	auto const cab = (vehicle->MoverParameters->ActiveCab == 0 ? 1 : vehicle->MoverParameters->ActiveCab);
	auto const left = vehicle->VectorLeft() * cab;

	if (true == Near) {
		Camera.pos = Math3D::vector3(Camera.pos.x, vehicle->GetPosition().y, Camera.pos.z) + left * vehicle->GetWidth() + Math3D::vector3(1.25 * left.x, 1.6, 1.25 * left.z);
	} else {
		Camera.pos = vehicle->GetPosition() + vehicle->VectorFront() * vehicle->MoverParameters->ActiveCab * 50.0 + Math3D::vector3(-10.0 * left.x, 1.6, -10.0 * left.z);
	}

	Camera.lookAt = vehicle->GetPosition();
	Camera.raLook(); // jednorazowe przestawienie kamery
};

// ustawienie śledzenia pojazdu
void TWorld::FollowView(bool wycisz)
{
	Camera.reset(); // likwidacja obrotów - patrzy horyzontalnie na południe

	if (Controlled) // jest pojazd do prowadzenia?
	{
		if (FreeFlyModeFlag) { // jeżeli poza kabiną, przestawiamy w jej okolicę - OK
			if (Train) {
				// wyłączenie trzęsienia na siłę?
				Train->Dynamic()->ABuSetModelShake({});
			}

			DistantView(); // przestawienie kamery
			//żeby nie bylo numerów z 'fruwajacym' lokiem - konsekwencja bujania pudła
			// tu ustawić nową, bo od niej liczą się odległości
			Global.pCameraPosition = Camera.pos;
		} else if (Train) {
			Camera.pos = Train->pMechPosition;
			// potentially restore cached camera angles
			Camera.pitch = Train->pMechViewAngle.x;
			Camera.yaw = Train->pMechViewAngle.y;

			Camera.roll = std::atan(Train->pMechShake.x * Train->fMechRoll); // hustanie kamery na boki
			Camera.pitch -= 0.5 * std::atan(Train->vMechVelocity.z * Train->fMechPitch); // hustanie kamery przod tyl

			if (Train->Dynamic()->MoverParameters->ActiveCab == 0) {
				Camera.lookAt = Train->pMechPosition + Train->GetDirection() * 5.0;
			} else {
				// patrz w strone wlasciwej kabiny
				Camera.lookAt = Train->pMechPosition + Train->GetDirection() * 5.0 * Train->Dynamic()->MoverParameters->ActiveCab;
			}
			Train->pMechOffset = Train->pMechSittingPosition;
		}
	} else
		DistantView();
};

bool TWorld::Update()
{
	Timer::UpdateTimers(Global.iPause != 0);
	Timer::subsystem.sim_total.start();

	if ((Global.iPause == 0) || (m_init == false)) {
		// jak pauza, to nie ma po co tego przeliczać
		simulation::Time.update(Timer::GetDeltaTime());
		// Ra 2014-07: przeliczenie kąta czasu (do animacji zależnych od czasu)
		auto const& time = simulation::Time.data();
		Global.fTimeAngleDeg = time.wHour * 15.0 + time.wMinute * 0.25 + ((time.wSecond + 0.001 * time.wMilliseconds) / 240.0);
		Global.fClockAngleDeg[0] = 36.0 * (time.wSecond % 10); // jednostki sekund
		Global.fClockAngleDeg[1] = 36.0 * (time.wSecond / 10); // dziesiątki sekund
		Global.fClockAngleDeg[2] = 36.0 * (time.wMinute % 10); // jednostki minut
		Global.fClockAngleDeg[3] = 36.0 * (time.wMinute / 10); // dziesiątki minut
		Global.fClockAngleDeg[4] = 36.0 * (time.wHour % 10); // jednostki godzin
		Global.fClockAngleDeg[5] = 36.0 * (time.wHour / 10); // dziesiątki godzin

		Update_Environment();
	} // koniec działań niewykonywanych podczas pauzy

	// fixed step, simulation time based updates

	double dt = Timer::GetDeltaTime(); // 0.0 gdy pauza
	/*
    fTimeBuffer += dt; //[s] dodanie czasu od poprzedniej ramki
*/
	//  m_primaryupdateaccumulator += dt; // unused for the time being
	m_secondaryupdateaccumulator += dt;
	/*
    if (fTimeBuffer >= fMaxDt) // jest co najmniej jeden krok; normalnie 0.01s
    { // Ra: czas dla fizyki jest skwantowany - fizykę lepiej przeliczać stałym krokiem
        // tak można np. moc silników itp., ale ruch musi być przeliczany w każdej klatce, bo
        // inaczej skacze
        Global.tranTexts.Update(); // obiekt obsługujący stenogramy dźwięków na ekranie
#ifdef _WIN32
        Console::Update(); // obsługa cykli PoKeys (np. aktualizacja wyjść analogowych)
#endif
        double iter =
            ceil(fTimeBuffer / fMaxDt); // ile kroków się zmieściło od ostatniego sprawdzania?
        int n = int(iter); // ile kroków jako int
        fTimeBuffer -= iter * fMaxDt; // reszta czasu na potem (do bufora)
        if (n > 20)
            n = 20; // Ra: jeżeli FPS jest zatrważająco niski, to fizyka nie może zająć całkowicie procesora
    }
*/
	/*
    // NOTE: until we have no physics state interpolation during render, we need to rely on the old code,
    // as doing fixed step calculations but flexible step render results in ugly mini jitter
    // core routines (physics)
    int updatecount = 0;
    while( ( m_primaryupdateaccumulator >= m_primaryupdaterate )
         &&( updatecount < 20 ) ) {
        // no more than 20 updates per single pass, to keep physics from hogging up all run time
        Ground.Update( m_primaryupdaterate, 1 );
        ++updatecount;
        m_primaryupdateaccumulator -= m_primaryupdaterate;
    }
*/
	int updatecount = 1;
	if (dt > m_primaryupdaterate) // normalnie 0.01s
	{
		/*
        // NOTE: experimentally disabled physics update cap
        auto const iterations = std::ceil(dt / m_primaryupdaterate);
        updatecount = std::min( 20, static_cast<int>( iterations ) );
*/
		updatecount = std::ceil(dt / m_primaryupdaterate);
		/*
        // NOTE: changing dt wrecks things further down the code. re-acquire proper value later or cleanup here
        dt = dt / iterations; // Ra: fizykę lepiej by było przeliczać ze stałym krokiem
*/
	}
	auto const stepdeltatime{ dt / updatecount };
	// NOTE: updates are limited to 20, but dt is distributed over potentially many more iterations
	// this means at count > 20 simulation and render are going to desync. is that right?
	// NOTE: experimentally changing this to prevent the desync.
	// TODO: test what happens if we hit more than 20 * 0.01 sec slices, i.e. less than 5 fps
	Timer::subsystem.sim_dynamics.start();
	if (true == Global.FullPhysics) {
		// mixed calculation mode, steps calculated in ~0.05s chunks
		while (updatecount >= 5) {
			simulation::State.update(stepdeltatime, 5);
			updatecount -= 5;
		}
		if (updatecount) {
			simulation::State.update(stepdeltatime, updatecount);
		}
	} else {
		// simplified calculation mode; faster but can lead to errors
		simulation::State.update(stepdeltatime, updatecount);
	}
	Timer::subsystem.sim_dynamics.stop();

	// secondary fixed step simulation time routines
	while (m_secondaryupdateaccumulator >= m_secondaryupdaterate) {
		ui::Transcripts.Update(); // obiekt obsługujący stenogramy dźwięków na ekranie

		// awaria PoKeys mogła włączyć pauzę - przekazać informację
		if (Global.iMultiplayer) // dajemy znać do serwera o wykonaniu
			if (iPause != Global.iPause) { // przesłanie informacji o pauzie do programu nadzorującego
				multiplayer::WyslijParam(5, 3); // ramka 5 z czasem i stanem zapauzowania
				iPause = Global.iPause;
			}

		// fixed step part of the camera update
		if ((Train != nullptr) && (Camera.type == TP_FOLLOW) && (false == DebugCameraFlag)) {
			// jeśli jazda w kabinie, przeliczyć trzeba parametry kamery
			Train->UpdateMechPosition(m_secondaryupdaterate);
		}

		m_secondaryupdateaccumulator -= m_secondaryupdaterate; // these should be inexpensive enough we have no cap
	}

	// variable step simulation time routines

	if (Global.changeDynObj) {
		// ABu zmiana pojazdu - przejście do innego
		ChangeDynamic();
	}

	if (Train != nullptr) {
		TSubModel::iInstance = reinterpret_cast<size_t>(Train->Dynamic());
		Train->Update(dt);
	} else {
		TSubModel::iInstance = 0;
	}

	simulation::Events.update();
	simulation::Region->update_events();
	simulation::Lights.update();

	// render time routines follow:

	dt = Timer::GetDeltaRenderTime(); // nie uwzględnia pauzowania ani mnożenia czasu

	// fixed step render time routines

	fTime50Hz += dt; // w pauzie też trzeba zliczać czas, bo przy dużym FPS będzie problem z odczytem ramek
	while (fTime50Hz >= 1.0 / 50.0) {
#ifdef _WIN32
		Console::Update(); // to i tak trzeba wywoływać
#endif
		UILayer.update();
		// decelerate camera
		Camera.velocity *= 0.65;
		if (std::abs(Camera.velocity.x) < 0.01) {
			Camera.velocity.x = 0.0;
		}
		if (std::abs(Camera.velocity.y) < 0.01) {
			Camera.velocity.y = 0.0;
		}
		if (std::abs(Camera.velocity.z) < 0.01) {
			Camera.velocity.z = 0.0;
		}
		// decelerate debug camera too
		DebugCamera.velocity *= 0.65;
		if (std::abs(DebugCamera.velocity.x) < 0.01) {
			DebugCamera.velocity.x = 0.0;
		}
		if (std::abs(DebugCamera.velocity.y) < 0.01) {
			DebugCamera.velocity.y = 0.0;
		}
		if (std::abs(DebugCamera.velocity.z) < 0.01) {
			DebugCamera.velocity.z = 0.0;
		}

		fTime50Hz -= 1.0 / 50.0;
	}

	// variable step render time routines

	Update_Camera(dt);

	Timer::subsystem.sim_total.stop();

	simulation::Region->update_sounds();
	audio::renderer.update(Global.iPause ? 0.0 : dt);

	GfxRenderer.Update(dt);
	ResourceSweep();

	m_init = true;

	return true;
};

void TWorld::Update_Camera(double const Deltatime)
{
	if (false == Global.ControlPicking) {
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
			Camera.reset(); // likwidacja obrotów - patrzy horyzontalnie na południe
			if (Controlled && LengthSquared3(Controlled->GetPosition() - Camera.pos) < (1500 * 1500)) {
				// gdy bliżej niż 1.5km
				Camera.lookAt = Controlled->GetPosition();
			} else {
				TDynamicObject* d = std::get<TDynamicObject*>(simulation::Region->find_vehicle(Global.pCameraPosition, 300, false, false));
				if (!d)
					d = std::get<TDynamicObject*>(simulation::Region->find_vehicle(Global.pCameraPosition, 1000, false, false)); // dalej szukanie, jesli bliżej nie ma

				if (d && pDynamicNearest) {
					// jeśli jakiś jest znaleziony wcześniej
					if (100.0 * LengthSquared3(d->GetPosition() - Camera.pos) > LengthSquared3(pDynamicNearest->GetPosition() - Camera.pos)) {
						d = pDynamicNearest; // jeśli najbliższy nie jest 10 razy bliżej niż
					}
				}
				// poprzedni najbliższy, zostaje poprzedni
				if (d)
					pDynamicNearest = d; // zmiana na nowy, jeśli coś znaleziony niepusty
				if (pDynamicNearest)
					Camera.lookAt = pDynamicNearest->GetPosition();
			}
			if (FreeFlyModeFlag)
				Camera.raLook(); // jednorazowe przestawienie kamery
		} else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
			FollowView(false); // bez wyciszania dźwięków
		} else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) {
			// middle mouse button controls zoom.
			Global.ZoomFactor = std::min(4.5f, Global.ZoomFactor + 15.0f * static_cast<float>(Deltatime));
		} else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) != GLFW_PRESS) {
			// reset zoom level if the button is no longer held down.
			// NOTE: yes, this is terrible way to go about it. it'll do for now.
			Global.ZoomFactor = std::max(1.0f, Global.ZoomFactor - 15.0f * static_cast<float>(Deltatime));
		}
	}

	if (DebugCameraFlag) {
		DebugCamera.update();
	} else {
		Camera.update();
	} // uwzględnienie ruchu wywołanego klawiszami

	if ((false == FreeFlyModeFlag) && (false == Global.CabWindowOpen) && (Train != nullptr)) {
		// cache cab camera view angles in case of view type switch
		Train->pMechViewAngle = { Camera.pitch, Camera.yaw };
	}

	// reset window state, it'll be set again if applicable in a check below
	Global.CabWindowOpen = false;

	if ((Train != nullptr) && (Camera.type == TP_FOLLOW) && (false == DebugCameraFlag)) {
		// jeśli jazda w kabinie, przeliczyć trzeba parametry kamery
		auto tempangle = Controlled->VectorFront() * (Controlled->MoverParameters->ActiveCab == -1 ? -1 : 1);
		//        double modelrotate = atan2( -tempangle.x, tempangle.z );

		if ((true == Global.ctrlState) && ((glfwGetKey(Global.window, GLFW_KEY_LEFT) == GLFW_TRUE) || (glfwGetKey(Global.window, GLFW_KEY_RIGHT) == GLFW_TRUE))) {
			// jeśli lusterko lewe albo prawe (bez rzucania na razie)
			Global.CabWindowOpen = true;

			auto const lr{ glfwGetKey(Global.window, GLFW_KEY_LEFT) == GLFW_TRUE };
			// Camera.Yaw powinno być wyzerowane, aby po powrocie patrzeć do przodu
			Camera.pos = Controlled->GetPosition() + Train->MirrorPosition(lr); // pozycja lusterka
			Camera.yaw = 0; // odchylenie na bok od Camera.LookAt
			if (Train->Dynamic()->MoverParameters->ActiveCab == 0)
				Camera.lookAt = Camera.pos - Train->GetDirection(); // gdy w korytarzu
			else if (Global.shiftState) {
				// patrzenie w bok przez szybę
				Camera.lookAt = Camera.pos - (lr ? -1 : 1) * Train->Dynamic()->VectorLeft() * Train->Dynamic()->MoverParameters->ActiveCab;
			} else { // patrzenie w kierunku osi pojazdu, z uwzględnieniem kabiny - jakby z lusterka,
				// ale bez odbicia
				Camera.lookAt = Camera.pos - Train->GetDirection() * Train->Dynamic()->MoverParameters->ActiveCab; //-1 albo 1
			}
			Camera.roll = std::atan(Train->pMechShake.x * Train->fMechRoll); // hustanie kamery na boki
			Camera.pitch = 0.5 * std::atan(Train->vMechVelocity.z * Train->fMechPitch); // hustanie kamery przod tyl
			Camera.vUp = Controlled->VectorUp();
		} else {
			// patrzenie standardowe
			// potentially restore view angle after returning from external view
			// TODO: mirror view toggle as separate method
			Camera.pitch = Train->pMechViewAngle.x;
			Camera.yaw = Train->pMechViewAngle.y;

			Camera.pos = Train->GetWorldMechPosition(); // Train.GetPosition1();
			if (!Global.iPause) {
				// podczas pauzy nie przeliczać kątów przypadkowymi wartościami
				// hustanie kamery na boki
				Camera.roll = atan(Train->vMechVelocity.x * Train->fMechRoll);
				// hustanie kamery przod tyl
				// Ra: tu jest uciekanie kamery w górę!!!
				Camera.pitch -= 0.5 * atan(Train->vMechVelocity.z * Train->fMechPitch);
			}
			if (Train->Dynamic()->MoverParameters->ActiveCab == 0)
				Camera.lookAt = Train->GetWorldMechPosition() + Train->GetDirection() * 5.0; // gdy w korytarzu
			else // patrzenie w kierunku osi pojazdu, z uwzględnieniem kabiny
				Camera.lookAt = Train->GetWorldMechPosition() + Train->GetDirection() * 5.0 * Train->Dynamic()->MoverParameters->ActiveCab; //-1 albo 1
			Camera.vUp = Train->GetUp();
		}
	} else {
		// kamera nieruchoma
	}
	// all done, update camera position to the new value
	Global.pCameraPosition = Camera.pos;
	Global.DebugCameraPosition = DebugCamera.pos;
}

void TWorld::Update_Environment()
{
	Environment.update();
}

void TWorld::ResourceSweep(){};

//---------------------------------------------------------------------------
void TWorld::OnCommandGet(multiplayer::DaneRozkaz* pRozkaz)
{ // odebranie komunikatu z serwera
	if (pRozkaz->iSygn == MAKE_ID4('E', 'U', '0', '7'))
		switch (pRozkaz->iComm) {
			case 0: // odesłanie identyfikatora wersji
				CommLog(Now() + " " + std::to_string(pRozkaz->iComm) + " version" + " rcvd");
				multiplayer::WyslijString(Global.asVersion, 0); // przedsatwienie się
				break;
			case 1: // odesłanie identyfikatora wersji
				CommLog(Now() + " " + std::to_string(pRozkaz->iComm) + " scenery" + " rcvd");
				multiplayer::WyslijString(Global.SceneryFile, 1); // nazwa scenerii
				break;
			case 2: {
				// event
				CommLog(Now() + " " + std::to_string(pRozkaz->iComm) + " " + std::string(pRozkaz->cString + 1, (unsigned)(pRozkaz->cString[0])) + " rcvd");

				if (Global.iMultiplayer) {
					auto* event = simulation::Events.FindEvent(std::string(pRozkaz->cString + 1, (unsigned)(pRozkaz->cString[0])));
					if (event != nullptr) {
						if ((event->Type == tp_Multiple) || (event->Type == tp_Lights) || (event->evJoined != 0)) {
							// tylko jawne albo niejawne Multiple
							simulation::Events.AddToQuery(event, nullptr); // drugi parametr to dynamic wywołujący - tu brak
						}
					}
				}
				break;
			}
			case 3: // rozkaz dla AI
				if (Global.iMultiplayer) {
					int i = int(pRozkaz->cString[8]); // długość pierwszego łańcucha (z przodu dwa floaty)
					CommLog(Now() + " " + to_string(pRozkaz->iComm) + " " + std::string(pRozkaz->cString + 11 + i, (unsigned)(pRozkaz->cString[10 + i])) + " rcvd");
					// nazwa pojazdu jest druga
					auto* vehicle = simulation::Vehicles.find({ pRozkaz->cString + 11 + i, (unsigned)pRozkaz->cString[10 + i] });
					if ((vehicle != nullptr) && (vehicle->Mechanik != nullptr)) {
						vehicle->Mechanik->PutCommand({ pRozkaz->cString + 9, static_cast<std::size_t>(i) }, pRozkaz->fPar[0], pRozkaz->fPar[1], nullptr,
						stopExt); // floaty są z przodu
						WriteLog("AI command: " + std::string(pRozkaz->cString + 9, i));
					}
				}
				break;
			case 4: // badanie zajętości toru
			{
				CommLog(Now() + " " + to_string(pRozkaz->iComm) + " " + std::string(pRozkaz->cString + 1, (unsigned)(pRozkaz->cString[0])) + " rcvd");

				auto* track = simulation::Paths.find(std::string(pRozkaz->cString + 1, (unsigned)(pRozkaz->cString[0])));
				if ((track != nullptr) && (track->IsEmpty())) {
					multiplayer::WyslijWolny(track->name());
				}
			} break;
			case 5: // ustawienie parametrów
			{
				CommLog(Now() + " " + to_string(pRozkaz->iComm) + " params " + to_string(*pRozkaz->iPar) + " rcvd");
				if (*pRozkaz->iPar == 0) // sprawdzenie czasu
					if (*pRozkaz->iPar & 1) // ustawienie czasu
					{
						double t = pRozkaz->fPar[1];
						simulation::Time.data().wDay = std::floor(t); // niby nie powinno być dnia, ale...
						if (Global.fMoveLight >= 0)
							Global.fMoveLight = t; // trzeba by deklinację Słońca przeliczyć
						simulation::Time.data().wHour = std::floor(24 * t) - 24.0 * simulation::Time.data().wDay;
						simulation::Time.data().wMinute = std::floor(60 * 24 * t) - 60.0 * (24.0 * simulation::Time.data().wDay + simulation::Time.data().wHour);
						simulation::Time.data().wSecond =
						std::floor(60 * 60 * 24 * t) - 60.0 * (60.0 * (24.0 * simulation::Time.data().wDay + simulation::Time.data().wHour) + simulation::Time.data().wMinute);
					}
				if (*pRozkaz->iPar & 2) { // ustawienie flag zapauzowania
					Global.iPause = pRozkaz->fPar[2]; // zakładamy, że wysyłający wie, co robi
				}
			} break;
			case 6: // pobranie parametrów ruchu pojazdu
				if (Global.iMultiplayer) {
					// Ra 2014-12: to ma działać również dla pojazdów bez obsady
					CommLog(Now() + " " + to_string(pRozkaz->iComm) + " " + std::string{ pRozkaz->cString + 1, (unsigned)(pRozkaz->cString[0]) } + " rcvd");
					if (pRozkaz->cString[0]) {
						// jeśli długość nazwy jest niezerowa szukamy pierwszego pojazdu o takiej nazwie i odsyłamy parametry ramką #7
						auto* vehicle = (pRozkaz->cString[1] == '*' ? simulation::Vehicles.find(Global.asHumanCtrlVehicle) :
						                                              simulation::Vehicles.find(std::string{ pRozkaz->cString + 1, (unsigned)pRozkaz->cString[0] }));
						if (vehicle != nullptr) {
							multiplayer::WyslijNamiary(vehicle); // wysłanie informacji o pojeździe
						}
					} else {
						// dla pustego wysyłamy ramki 6 z nazwami pojazdów AI (jeśli potrzebne wszystkie, to rozpoznać np. "*")
						simulation::Vehicles.DynamicList();
					}
				}
				break;
			case 8: // ponowne wysłanie informacji o zajętych odcinkach toru
				CommLog(Now() + " " + to_string(pRozkaz->iComm) + " all busy track" + " rcvd");
				simulation::Paths.TrackBusyList();
				break;
			case 9: // ponowne wysłanie informacji o zajętych odcinkach izolowanych
				CommLog(Now() + " " + to_string(pRozkaz->iComm) + " all busy isolated" + " rcvd");
				simulation::Paths.IsolatedBusyList();
				break;
			case 10: // badanie zajętości jednego odcinka izolowanego
				CommLog(Now() + " " + to_string(pRozkaz->iComm) + " " + std::string(pRozkaz->cString + 1, (unsigned)(pRozkaz->cString[0])) + " rcvd");
				simulation::Paths.IsolatedBusy(std::string(pRozkaz->cString + 1, (unsigned)(pRozkaz->cString[0])));
				break;
			case 11: // ustawienie parametrów ruchu pojazdu
				//    Ground.IsolatedBusy(AnsiString(pRozkaz->cString+1,(unsigned)(pRozkaz->cString[0])));
				break;
			case 12: // skrocona ramka parametrow pojazdow AI (wszystkich!!)
				CommLog(Now() + " " + to_string(pRozkaz->iComm) + " obsadzone" + " rcvd");
				multiplayer::WyslijObsadzone();
				//    Ground.IsolatedBusy(AnsiString(pRozkaz->cString+1,(unsigned)(pRozkaz->cString[0])));
				break;
			case 13: // ramka uszkodzenia i innych stanow pojazdu, np. wylaczenie CA, wlaczenie recznego itd.
				CommLog(Now() + " " + to_string(pRozkaz->iComm) + " " + std::string(pRozkaz->cString + 1, (unsigned)(pRozkaz->cString[0])) + " rcvd");
				if (pRozkaz->cString[1]) // jeśli długość nazwy jest niezerowa
				{ // szukamy pierwszego pojazdu o takiej nazwie i odsyłamy parametry ramką #13
					auto* lookup = (pRozkaz->cString[2] == '*' ? simulation::Vehicles.find(Global.asHumanCtrlVehicle) : // nazwa pojazdu użytkownika
					                simulation::Vehicles.find(std::string(pRozkaz->cString + 2, (unsigned)pRozkaz->cString[1]))); // nazwa pojazdu
					if (lookup == nullptr) {
						break;
					} // nothing found, nothing to do
					auto* d{ lookup };
					while (d != nullptr) {
						d->Damage(pRozkaz->cString[0]);
						d = d->Next(); // pozostałe też
					}
					d = lookup->Prev();
					while (d != nullptr) {
						d->Damage(pRozkaz->cString[0]);
						d = d->Prev(); // w drugą stronę też
					}
					multiplayer::WyslijUszkodzenia(lookup->asName, lookup->MoverParameters->EngDmgFlag); // zwrot informacji o pojeździe
				}
				break;
			default:
				break;
		}
};

//---------------------------------------------------------------------------

void TWorld::CreateE3D(std::string const& Path, bool Dynamic)
{ // rekurencyjna generowanie plików E3D

	std::string last; // zmienne używane w rekurencji
	TTrack* trk{ nullptr };
	double at{ 0.0 };
	double shift{ 0.0 };

#ifdef _WIN32

	std::string searchpattern("*.*");

	::WIN32_FIND_DATA filedata;
	::HANDLE search = ::FindFirstFile((Path + searchpattern).c_str(), &filedata);

	if (search == INVALID_HANDLE_VALUE) {
		return;
	} // if nothing to do, bail out

	do {
		std::string filename = filedata.cFileName;

		if (filedata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			// launch recursive search for sub-directories...
			if (filename == ".") {
				continue;
			}
			if (filename == "..") {
				continue;
			}
			CreateE3D(Path + filename + "/", Dynamic);
		} else {
			// process the file
			if (filename.size() < 4) {
				continue;
			}
			std::string const filetype = ToLower(filename.substr(filename.size() - 4, 4));
			if (filetype == ".mmd") {
				if (false == Dynamic) {
					continue;
				}

				// konwersja pojazdów będzie ułomna, bo nie poustawiają się animacje na submodelach określonych w MMD
				if (last != Path) { // utworzenie toru dla danego pojazdu
					last = Path;
					trk = TTrack::Create400m(1, shift);
					shift += 10.0; // następny tor będzie deczko dalej, aby nie zabić FPS
					at = 400.0;
				}
				auto* dynamic = new TDynamicObject();

				at -= dynamic->Init("", Path.substr(8, Path.size() - 9), // skip leading "dynamic/" and trailing slash
				"none", filename.substr(0, filename.size() - 4), trk, at, "nobody", 0.0, "none", 0.0, "", false, "");

				// po wczytaniu CHK zrobić pętlę po ładunkach, aby każdy z nich skonwertować
				cParser loadparser(dynamic->MoverParameters->LoadAccepted); // typy ładunków
				std::string loadname;
				loadparser.getTokens(1, true, ",");
				loadparser >> loadname;
				while (loadname != "") {
					if ((true == FileExists(Path + loadname + ".t3d")) && (false == FileExists(Path + loadname + ".e3d"))) {
						// a nie ma jeszcze odpowiednika binarnego
						at -= dynamic->Init("", Path.substr(8, Path.size() - 9), // skip leading "dynamic/" and trailing slash
						"none", filename.substr(0, filename.size() - 4), trk, at, "nobody", 0.0, "none", 1.0, loadname, false, "");
					}

					loadname = "";
					loadparser.getTokens(1, true, ",");
					loadparser >> loadname;
				}

				if (dynamic->iCabs) { // jeśli ma jakąkolwiek kabinę
					delete Train;
					Train = new TTrain();
					if (dynamic->iCabs & 0x1) {
						dynamic->MoverParameters->ActiveCab = 1;
						Train->Init(dynamic, true);
					}
					if (dynamic->iCabs & 0x4) {
						dynamic->MoverParameters->ActiveCab = -1;
						Train->Init(dynamic, true);
					}
					if (dynamic->iCabs & 0x2) {
						dynamic->MoverParameters->ActiveCab = 0;
						Train->Init(dynamic, true);
					}
				}
				// z powrotem defaultowa sciezka do tekstur
				Global.asCurrentTexturePath = (szTexturePath);
			} else if (filetype == ".t3d") {
				// z modelami jest prościej
				Global.asCurrentTexturePath = Path;
				TModelsManager::GetModel(Path + filename, false);
			}
		}

	} while (::FindNextFile(search, &filedata));

	// all done, clean up
	::FindClose(search);

#endif
};

//---------------------------------------------------------------------------
// passes specified sound to all vehicles within range as a radio message broadcasted on specified channel
void TWorld::radio_message(sound_source* Message, int const Channel)
{
	if (Train != nullptr) {
		Train->radio_message(Message, Channel);
	}
}

void TWorld::CabChange(TDynamicObject* old, TDynamicObject* now)
{ // ewentualna zmiana kabiny użytkownikowi
	if (Train)
		if (Train->Dynamic() == old)
			Global.changeDynObj = now; // uruchomienie protezy
};

void TWorld::ChangeDynamic()
{
	// Ra: to nie może być tak robione, to zbytnia proteza jest
	if (Train->Dynamic()->Mechanik) {
		// AI może sobie samo pójść
		if (false == Train->Dynamic()->Mechanik->AIControllFlag) {
			// tylko jeśli ręcznie prowadzony
			// jeśli prowadzi AI, to mu nie robimy dywersji!
			Train->Dynamic()->MoverParameters->CabDeactivisation();
			Train->Dynamic()->Controller = AIdriver;
			Train->Dynamic()->MoverParameters->ActiveCab = 0;
			Train->Dynamic()->MoverParameters->BrakeLevelSet(Train->Dynamic()->MoverParameters->Handle->GetPos(bh_NP)); //rozwala sterowanie hamulcem GF 04-2016
			Train->Dynamic()->MechInside = false;
		}
	}
	TDynamicObject* temp = Global.changeDynObj;
	Train->Dynamic()->bDisplayCab = false;
	Train->Dynamic()->ABuSetModelShake({});

	if (Train->Dynamic()->Mechanik) // AI może sobie samo pójść
		if (false == Train->Dynamic()->Mechanik->AIControllFlag) {
			// tylko jeśli ręcznie prowadzony
			// przsunięcie obiektu zarządzającego
			Train->Dynamic()->Mechanik->MoveTo(temp);
		}

	Train->DynamicSet(temp);
	Controlled = temp;
	mvControlled = Controlled->ControlledFind()->MoverParameters;
	Global.asHumanCtrlVehicle = Train->Dynamic()->name();
	if (Train->Dynamic()->Mechanik) // AI może sobie samo pójść
		if (!Train->Dynamic()->Mechanik->AIControllFlag) // tylko jeśli ręcznie prowadzony
		{
			Train->Dynamic()->MoverParameters->LimPipePress = Controlled->MoverParameters->PipePress;
			Train->Dynamic()->MoverParameters->CabActivisation(); // załączenie rozrządu (wirtualne kabiny)
			Train->Dynamic()->Controller = Humandriver;
			Train->Dynamic()->MechInside = true;
		}
	Train->InitializeCab(Train->Dynamic()->MoverParameters->CabNo, Train->Dynamic()->asBaseDir + Train->Dynamic()->MoverParameters->TypeName + ".mmd");
	if (!FreeFlyModeFlag) {
		Train->Dynamic()->bDisplayCab = true;
		Train->Dynamic()->ABuSetModelShake({}); // zerowanie przesunięcia przed powrotem?
		Train->MechStop();
		FollowView(); // na pozycję mecha
	}
	Global.changeDynObj = NULL;
}

void TWorld::ToggleDaylight()
{
	Global.FakeLight = !Global.FakeLight;

	if (Global.FakeLight) {
		// for fake daylight enter fixed hour
		Environment.time(10, 30, 0);
	} else {
		// local clock based calculation
		Environment.time();
	}
}

// calculates current season of the year based on set simulation date
void TWorld::compute_season(int const Yearday) const
{
	using dayseasonpair = std::pair<int, std::string>;

	std::vector<dayseasonpair> seasonsequence{ { 65, "winter" }, { 158, "spring" }, { 252, "summer" }, { 341, "autumn" }, { 366, "winter" } };
	auto const lookup = std::lower_bound(
	std::begin(seasonsequence), std::end(seasonsequence), clamp(Yearday, 1, seasonsequence.back().first), [](dayseasonpair const& Left, const int Right) { return Left.first < Right; });

	Global.Season = lookup->second + ":";
	// season can affect the weather so if it changes, re-calculate weather as well
	compute_weather();
}

// calculates current weather
void TWorld::compute_weather() const
{
	Global.Weather = (Global.Overcast < 0.25 ? "clear:" : Global.Overcast < 1.0 ? "cloudy:" : (Global.Season != "winter:" ? "rain:" : "snow:"));
}

void world_environment::init()
{
	//	m_skydome.init();
	m_sun.init();
	m_moon.init();
	m_stars.init();
	m_clouds.Init();
}

void world_environment::update()
{
	// move celestial bodies...
	m_sun.update();
	m_moon.update();
	// ...determine source of key light and adjust global state accordingly...
	// diffuse (sun) intensity goes down after twilight, and reaches minimum 18 degrees below horizon
	float twilightfactor = clamp(-m_sun.getAngle(), 0.0f, 18.0f) / 18.0f;
	// NOTE: sun light receives extra padding to prevent moon from kicking in too soon
	auto const sunlightlevel = m_sun.getIntensity() + 0.05f * (1.f - twilightfactor);
	auto const moonlightlevel = m_moon.getIntensity() * 0.65f; // scaled down by arbitrary factor, it's pretty bright otherwise
	float keylightintensity;
	glm::vec3 keylightcolor;
	if (moonlightlevel > sunlightlevel) {
		// rare situations when the moon is brighter than the sun, typically at night
		Global.SunAngle = m_moon.getAngle();
		Global.DayLight.position = m_moon.getDirection();
		Global.DayLight.direction = -1.0f * m_moon.getDirection();
		keylightintensity = moonlightlevel;
		// if the moon is up, it overrides the twilight
		twilightfactor = 0.0f;
		keylightcolor = glm::vec3(255.0f / 255.0f, 242.0f / 255.0f, 202.0f / 255.0f);
	} else {
		// regular situation with sun as the key light
		Global.SunAngle = m_sun.getAngle();
		Global.DayLight.position = m_sun.getDirection();
		Global.DayLight.direction = -1.0f * m_sun.getDirection();
		keylightintensity = sunlightlevel;
		// include 'golden hour' effect in twilight lighting
		float const duskfactor = 1.0f - clamp(Global.SunAngle, 0.0f, 18.0f) / 18.0f;
		keylightcolor = interpolate(glm::vec3(255.0f / 255.0f, 242.0f / 255.0f, 231.0f / 255.0f), glm::vec3(235.0f / 255.0f, 140.0f / 255.0f, 36.0f / 255.0f), duskfactor);
	}
	// ...update skydome to match the current sun position as well...
	m_skydome.SetOvercastFactor(Global.Overcast);
	m_skydome.Update(m_sun.getDirection());
	// ...retrieve current sky colour and brightness...
	auto const skydomecolour = m_skydome.GetAverageColor();
	auto const skydomehsv = colors::RGBtoHSV(skydomecolour);
	// sun strength is reduced by overcast level
	keylightintensity *= (1.0f - std::min(1.f, Global.Overcast) * 0.65f);

	// intensity combines intensity of the sun and the light reflected by the sky dome
	// it'd be more technically correct to have just the intensity of the sun here,
	// but whether it'd _look_ better is something to be tested
	auto const intensity = std::min(1.15f * (0.05f + keylightintensity + skydomehsv.z), 1.25f);
	// the impact of sun component is reduced proportionally to overcast level, as overcast increases role of ambient light
	auto const diffuselevel = interpolate(keylightintensity, intensity * (1.0f - twilightfactor), 1.0f - std::min(1.f, Global.Overcast) * 0.75f);
	// ...update light colours and intensity.
	keylightcolor = keylightcolor * diffuselevel;
	Global.DayLight.diffuse = glm::vec4(keylightcolor, Global.DayLight.diffuse.a);
	Global.DayLight.specular = glm::vec4(keylightcolor * 0.85f, diffuselevel);

	// tonal impact of skydome color is inversely proportional to how high the sun is above the horizon
	// (this is pure conjecture, aimed more to 'look right' than be accurate)
	float const ambienttone = clamp(1.0f - (Global.SunAngle / 90.0f), 0.0f, 1.0f);
	Global.DayLight.ambient[0] = interpolate(skydomehsv.z, skydomecolour.r, ambienttone);
	Global.DayLight.ambient[1] = interpolate(skydomehsv.z, skydomecolour.g, ambienttone);
	Global.DayLight.ambient[2] = interpolate(skydomehsv.z, skydomecolour.b, ambienttone);

	Global.fLuminance = intensity;

	// update the fog. setting it to match the average colour of the sky dome is cheap
	// but quite effective way to make the distant items blend with background better
	// NOTE: base brightness calculation provides scaled up value, so we bring it back to 'real' one here
	Global.FogColor = m_skydome.GetAverageHorizonColor();
}

void world_environment::time(int const Hour, int const Minute, int const Second)
{
	m_sun.setTime(Hour, Minute, Second);
}
