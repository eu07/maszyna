/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <stack>
#include <cinttypes>

#ifdef _WIN32
#include <dsound.h>
#else
#define DSBFREQUENCY_MIN 0
#define DSBFREQUENCY_MAX 0
#define DSBSTATUS_PLAYING 0
#define DSBPLAY_LOOPING 0
#define DSBVOLUME_MIN 0
#define DSBVOLUME_MAX 0
struct DSBCAPS
{
	uint16_t dwBufferBytes;
	uint16_t dwSize;
};
struct dummysb
{
	void Stop() {}
	void SetCurrentPosition(int) {}
	void SetVolume(int) {}
	void Play(int, int, int) {}
	void GetStatus(unsigned int *stat) { *stat = 0; }
	void SetPan(int) {}
	void SetFrequency(int) {}
	void GetCaps(DSBCAPS *caps) { caps->dwBufferBytes = 0; }
};
typedef dummysb* LPDIRECTSOUNDBUFFER;
typedef int LPDIRECTSOUNDNOTIFY;
typedef int LPDIRECTSOUND;
typedef int HWND;
#endif

typedef LPDIRECTSOUNDBUFFER PSound;

class TSoundContainer
{
public:
    int Concurrent;
    int Oldest;
    std::string m_name;
    LPDIRECTSOUNDBUFFER DSBuffer;
    float fSamplingRate; // częstotliwość odczytana z pliku
    int iBitsPerSample; // ile bitów na próbkę
    TSoundContainer *Next;
    std::stack<LPDIRECTSOUNDBUFFER> DSBuffers;

    TSoundContainer(LPDIRECTSOUND pDS, std::string const &Directory, std::string const &Filename, int NConcurrent);
    ~TSoundContainer();
    LPDIRECTSOUNDBUFFER GetUnique( LPDIRECTSOUND pDS );
};

class TSoundsManager
{
private:
    static LPDIRECTSOUND pDS;
    static LPDIRECTSOUNDNOTIFY pDSNotify;
    static TSoundContainer *First;
    static int Count;

    static TSoundContainer * LoadFromFile( std::string const &Directory, std::string const &FileName, int Concurrent);

public:
    ~TSoundsManager();
    static void Init( HWND hWnd );
    static void Free();
    static LPDIRECTSOUNDBUFFER GetFromName( std::string const &Name, bool Dynamic, float *fSamplingRate = NULL );
    static void RestoreAll();
};
