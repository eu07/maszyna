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
#include <dsound.h>

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
    static bool Init( HWND hWnd );
    static void Free();
    static LPDIRECTSOUNDBUFFER GetFromName( std::string const &Name, bool Dynamic, float *fSamplingRate = NULL );
    static void RestoreAll();
};
