/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "Sound.h"
#include "Globals.h"
#include "Logs.h"
#include "usefull.h"
#include "mczapkie/mctools.h"
#include "wavread.h"

#define SAFE_RELEASE(p)     \
    {                       \
        if (p)              \
        {                   \
            (p)->Release(); \
            (p) = NULL;     \
        }                   \
    }

LPDIRECTSOUND TSoundsManager::pDS;
LPDIRECTSOUNDNOTIFY TSoundsManager::pDSNotify;

int TSoundsManager::Count = 0;
TSoundContainer *TSoundsManager::First = NULL;

TSoundContainer::TSoundContainer(LPDIRECTSOUND pDS, std::string const &Directory, std::string const &Filename, int NConcurrent)
{ // wczytanie pliku dźwiękowego
    int hr = 111;
    DSBuffer = nullptr; // na początek, gdyby uruchomić dźwięków się nie udało
    Concurrent = NConcurrent;
    Oldest = 0;
    m_name = Directory + Filename;

    // Create a new wave file class
    std::shared_ptr<CWaveSoundRead> pWaveSoundRead = std::make_shared<CWaveSoundRead>();

    // Load the wave file
    if( ( FAILED( pWaveSoundRead->Open( m_name ) ) )
     && ( FAILED( pWaveSoundRead->Open( Filename ) ) ) ) {
        ErrorLog( "Missed sound: " + Filename );
        return;
    }

    m_name = ToLower( Filename );

    // Set up the direct sound buffer, and only request the flags needed
    // since each requires some overhead and limits if the buffer can
    // be hardware accelerated
    DSBUFFERDESC dsbd;
    ZeroMemory(&dsbd, sizeof(DSBUFFERDESC));
    dsbd.dwSize = sizeof(DSBUFFERDESC);
    dsbd.dwFlags = DSBCAPS_STATIC | DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY;
    if (!Global::bInactivePause) // jeśli przełączony w tło ma nadal działać
        dsbd.dwFlags |= DSBCAPS_GLOBALFOCUS; // to dźwięki mają być również słyszalne
    dsbd.dwBufferBytes = pWaveSoundRead->m_ckIn.cksize;
    dsbd.lpwfxFormat = pWaveSoundRead->m_pwfx;
    fSamplingRate = pWaveSoundRead->m_pwfx->nSamplesPerSec;
    iBitsPerSample = pWaveSoundRead->m_pwfx->wBitsPerSample;

    //    pDSBuffer= (LPDIRECTSOUNDBUFFER*) malloc(Concurrent*sizeof(LPDIRECTSOUNDBUFFER));
    //    for (int i=0; i<Concurrent; i++)
    //      pDSBuffer[i]= NULL;

    // Create the static DirectSound buffer
    if (FAILED(hr = pDS->CreateSoundBuffer(&dsbd, &(DSBuffer), NULL)))
        // if (FAILED(pDS->CreateSoundBuffer(&dsbd,&(DSBuffer),NULL)))
        return;

    // Remember how big the buffer is
    DWORD dwBufferBytes;

    dwBufferBytes = dsbd.dwBufferBytes;

    //----------------------------------------------------------

    BYTE *pbWavData; // Pointer to actual wav data
    UINT cbWavSize; // Size of data
    VOID *pbData = NULL;
    VOID *pbData2 = NULL;
    DWORD dwLength;
    DWORD dwLength2;

    // The size of wave data is in pWaveFileSound->m_ckIn
    INT nWaveFileSize = pWaveSoundRead->m_ckIn.cksize;

    // Allocate that buffer.
    pbWavData = new BYTE[nWaveFileSize];
    if (NULL == pbWavData)
        return; // E_OUTOFMEMORY;

    // if (FAILED(hr=pWaveSoundRead->Read( nWaveFileSize,pbWavData,&cbWavSize)))
	if( FAILED( hr = pWaveSoundRead->Read( nWaveFileSize, pbWavData, &cbWavSize ) ) ) {
		delete[] pbWavData;
		return;
	}

    // Reset the file to the beginning
    pWaveSoundRead->Reset();

    // Lock the buffer down
    // if (FAILED(hr=DSBuffer->Lock(0,dwBufferBytes,&pbData,&dwLength,&pbData2,&dwLength2,0)))
	if( FAILED( hr = DSBuffer->Lock( 0, dwBufferBytes, &pbData, &dwLength, &pbData2, &dwLength2, 0L ) ) ) {
		delete[] pbWavData;
		return;
	}

    // Copy the memory to it.
    memcpy(pbData, pbWavData, dwBufferBytes);

    // Unlock the buffer, we don't need it anymore.
    DSBuffer->Unlock(pbData, dwBufferBytes, NULL, 0);
    pbData = NULL;

    // We dont need the wav file data buffer anymore, so delete it
    delete[] pbWavData;

    DSBuffers.push(DSBuffer);
};
TSoundContainer::~TSoundContainer()
{
    while (!DSBuffers.empty())
    {
        SAFE_RELEASE(DSBuffers.top());
        DSBuffers.pop();
    }
    SafeDelete(Next);
};

LPDIRECTSOUNDBUFFER TSoundContainer::GetUnique(LPDIRECTSOUND pDS)
{
    if (!DSBuffer)
        return NULL;
    // jeśli się dobrze zainicjowało
    LPDIRECTSOUNDBUFFER buff = nullptr;
    pDS->DuplicateSoundBuffer(DSBuffer, &buff);
    if (buff)
        DSBuffers.push(buff);
    return DSBuffers.top();
};

TSoundsManager::~TSoundsManager()
{
    Free();
};

void TSoundsManager::Free()
{
    SafeDelete(First);
    SAFE_RELEASE(pDS);
};

TSoundContainer * TSoundsManager::LoadFromFile( std::string const &Dir, std::string const &Filename, int Concurrent)
{
    TSoundContainer *tmp = First;
    First = new TSoundContainer(pDS, Dir, Filename, Concurrent);
    First->Next = tmp;
    Count++;
    return First; // albo NULL, jak nie wyjdzie (na razie zawsze wychodzi)
};

LPDIRECTSOUNDBUFFER TSoundsManager::GetFromName(std::string const &Name, bool Dynamic, float *fSamplingRate)
{ // wyszukanie dźwięku w pamięci albo wczytanie z pliku
    std::string name{ ToLower(Name) };
    std::string file;
    if (Dynamic)
    { // próba wczytania z katalogu pojazdu
        file = Global::asCurrentDynamicPath + name;
        if (FileExists(file))
            name = file; // nowa nazwa
        else
            Dynamic = false; // wczytanie z "sounds/"
    }
    TSoundContainer *Next = First;
    for (int i = 0; i < Count; i++)
    {
        if( name == Next->m_name ) {

            if (fSamplingRate)
                *fSamplingRate = Next->fSamplingRate; // częstotliwość
            return (Next->GetUnique(pDS));
        }
        else
            Next = Next->Next;
    };
    if (Dynamic) // wczytanie z katalogu pojazdu
        Next = LoadFromFile("", name, 1);
    else
        Next = LoadFromFile(szSoundPath, name, 1);
    if (Next)
    { //
        if (fSamplingRate)
            *fSamplingRate = Next->fSamplingRate; // częstotliwość
        return Next->GetUnique(pDS);
    }
    ErrorLog("Missed sound: " + Name );
    return (nullptr);
};

void TSoundsManager::RestoreAll()
{
    TSoundContainer *Next = First;

    HRESULT hr;

    DWORD dwStatus = 0;

    for (int i = 0; i < Count; i++)
    {
        if (dwStatus & DSBSTATUS_BUFFERLOST)
        {
            // Since the app could have just been activated, then
            // DirectSound may not be giving us control yet, so
            // the restoring the buffer may fail.
            // If it does, sleep until DirectSound gives us control.
            do
            {
                hr = Next->DSBuffer->Restore();
                if (hr == DSERR_BUFFERLOST)
                    Sleep(10);
            } while ((hr = Next->DSBuffer->Restore()) != 0);

            //          char *Name= Next->Name;
            //          int cc= Next->Concurrent;
            //          delete Next;
            //          Next= new TSoundContainer(pDS, Directory, Name, cc);
            //          if( FAILED( hr = FillBuffer() ) );
            //           return hr;
        };
        Next = Next->Next;
    };
};

bool TSoundsManager::Init(HWND hWnd) {

    First = nullptr;
    Count = 0;
    pDS = nullptr;
    pDSNotify = nullptr;

    // Create IDirectSound using the primary sound device
    auto hr = ::DirectSoundCreate(NULL, &pDS, NULL);
    if (hr != DS_OK) {

        return false;
    };

    pDS->SetCooperativeLevel(hWnd, DSSCL_NORMAL);

    return true;
};
