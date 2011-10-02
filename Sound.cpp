//---------------------------------------------------------------------------

#include    "system.hpp"
#include    "classes.hpp"
#pragma hdrstop

#define STRICT
#include "Sound.h"
#include "Usefull.h"
//#define SAFE_DELETE(p)  { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

char Directory[]= "Sounds\\";

LPDIRECTSOUND       TSoundsManager::pDS;
LPDIRECTSOUNDNOTIFY TSoundsManager::pDSNotify;

int TSoundsManager::Count= 0;
TSoundContainer *TSoundsManager::First= NULL;


__fastcall TSoundContainer::TSoundContainer( LPDIRECTSOUND pDS, char *Directory, char *strFileName, int NConcurrent)
{
    int hr= 111;

    Concurrent= NConcurrent;

    Oldest= 0;
//    strcpy(Name, strFileName);

    strcpy(Name,Directory);
    strcat(Name,strFileName);

    CWaveSoundRead*     pWaveSoundRead;


    // Create a new wave file class
    pWaveSoundRead = new CWaveSoundRead();

    // Load the wave file
    if( FAILED( pWaveSoundRead->Open( Name ) ) )
    {
//        SetFileUI( hDlg, TEXT("Bad wave file.") );
        return;
    }

    strcpy(Name,AnsiString(strFileName).LowerCase().c_str());

    // Set up the direct sound buffer, and only request the flags needed
    // since each requires some overhead and limits if the buffer can
    // be hardware accelerated
    DSBUFFERDESC dsbd;
    ZeroMemory( &dsbd, sizeof(DSBUFFERDESC) );
    dsbd.dwSize        = sizeof(DSBUFFERDESC);
    dsbd.dwFlags       = DSBCAPS_STATIC | DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY;
    dsbd.dwBufferBytes = pWaveSoundRead->m_ckIn.cksize;
    dsbd.lpwfxFormat   = pWaveSoundRead->m_pwfx;

//    pDSBuffer= (LPDIRECTSOUNDBUFFER*) malloc(Concurrent*sizeof(LPDIRECTSOUNDBUFFER));
//    for (int i=0; i<Concurrent; i++)
  //      pDSBuffer[i]= NULL;

    // Create the static DirectSound buffer
    if( FAILED( hr = pDS->CreateSoundBuffer( &dsbd, &(DSBuffer), NULL ) ) )
        return;

    // Remember how big the buffer is
    DWORD dwBufferBytes;

    dwBufferBytes = dsbd.dwBufferBytes;


    //----------------------------------------------------------


    BYTE*   pbWavData; // Pointer to actual wav data
    UINT    cbWavSize; // Size of data
    VOID*   pbData  = NULL;
    VOID*   pbData2 = NULL;
    DWORD   dwLength;
    DWORD   dwLength2;

    // The size of wave data is in pWaveFileSound->m_ckIn
    INT nWaveFileSize = pWaveSoundRead->m_ckIn.cksize;

    // Allocate that buffer.
    pbWavData = new BYTE[ nWaveFileSize ];
    if( NULL == pbWavData )
        return ;//E_OUTOFMEMORY;

    if( FAILED( hr = pWaveSoundRead->Read( nWaveFileSize,
                                           pbWavData,
                                           &cbWavSize ) ) )
        return ;

    // Reset the file to the beginning
    pWaveSoundRead->Reset();

    // Lock the buffer down
    if( FAILED( hr = DSBuffer->Lock( 0, dwBufferBytes, &pbData, &dwLength,
                                   &pbData2, &dwLength2, 0L ) ) )
        return ;

    // Copy the memory to it.
    memcpy( pbData, pbWavData, dwBufferBytes );

    // Unlock the buffer, we don't need it anymore.
    DSBuffer->Unlock( pbData, dwBufferBytes, NULL, 0 );
    pbData = NULL;

    // We dont need the wav file data buffer anymore, so delete it
    SafeDeleteArray( pbWavData );

    SafeDelete( pWaveSoundRead );

    DSBuffers.push(DSBuffer);

/*
    for (int i=1; i<Concurrent; i++)
    {
        if( FAILED( hr= pDS->DuplicateSoundBuffer(pDSBuffer[0],&(pDSBuffer[i]))))
        {
            Concurrent= i;
            break;
        };


    };*/

};
__fastcall TSoundContainer::~TSoundContainer()
{
//    for (int i=Concurrent-1; i>=0; i--)
//        SAFE_RELEASE( pDSBuffer[i] );
//    free(pDSBuffer);
    while (!DSBuffers.empty())
    {
        SAFE_RELEASE(DSBuffers.top());
        DSBuffers.pop();
    }
    SafeDelete( Next );
};

LPDIRECTSOUNDBUFFER __fastcall TSoundContainer::GetUnique(LPDIRECTSOUND pDS)
{

    LPDIRECTSOUNDBUFFER buff;
    pDS->DuplicateSoundBuffer(DSBuffer,&buff);
    DSBuffers.push(buff);
    return DSBuffers.top();
};



__fastcall TSoundsManager::~TSoundsManager()
{

    Free();
};

__fastcall TSoundsManager::Free()
{
    SafeDelete( First );
    SAFE_RELEASE( pDS );
};


void __fastcall TSoundsManager::LoadFromFile(char *Name,int Concurrent)
{
 TSoundContainer *Tmp=First;
 First=new TSoundContainer(pDS, Directory, Name, Concurrent);
 First->Next=Tmp;
 Count++;
};

void __fastcall TSoundsManager::LoadSounds(char *Directory)
{
 WIN32_FIND_DATA FindFileData;
 HANDLE handle=FindFirstFile("Sounds\\*.wav",&FindFileData);
 if (handle!=INVALID_HANDLE_VALUE)
  do
  {
   LoadFromFile(FindFileData.cFileName,1);
  } while (FindNextFile(handle, &FindFileData));
 FindClose(handle);
};

LPDIRECTSOUNDBUFFER __fastcall TSoundsManager::GetFromName(char *Name)
{
    TSoundContainer *Next= First;
    DWORD dwStatus;

    for (int i=0; i<Count; i++)
    {
        if (strcmp(Name,Next->Name)==0)
        {
            return ( Next->GetUnique(pDS) ); 
//            DSBuffers.
        /*
            Next->pDSBuffer[Next->Oldest]->Stop();
            Next->pDSBuffer[Next->Oldest]->SetCurrentPosition(0);
            if (Next->Oldest<Next->Concurrent-1)
            {
                Next->Oldest++;
                return (Next->pDSBuffer[Next->Oldest-1]);
            }
            else
            {
                Next->Oldest= 0;
                return (Next->pDSBuffer[Next->Concurrent-1]);
            };

/*            for (int j=0; j<Next->Concurrent; j++)
            {

                Next->pDSBuffer[j]->GetStatus(&dwStatus);
                if ((dwStatus & DSBSTATUS_PLAYING) != DSBSTATUS_PLAYING)
                    return (Next->pDSBuffer[j]);
            }                                   */
        }
        else
            Next= Next->Next;
    };
    Error("Cannot find sound "+AnsiString(Name));
    return (NULL);
};

void __fastcall TSoundsManager::RestoreAll()
{
    TSoundContainer *Next= First;

    HRESULT hr;

    DWORD dwStatus;

    for (int i=0; i<Count; i++)
    {


        if( FAILED( hr = Next->DSBuffer->GetStatus( &dwStatus ) ) );
    //        return hr;

        if( dwStatus & DSBSTATUS_BUFFERLOST )
        {
            // Since the app could have just been activated, then
            // DirectSound may not be giving us control yet, so
            // the restoring the buffer may fail.
            // If it does, sleep until DirectSound gives us control.
            do
            {
                hr = Next->DSBuffer->Restore();
                if( hr == DSERR_BUFFERLOST )
                    Sleep( 10 );
            }
            while( hr = Next->DSBuffer->Restore() );

            char *Name= Next->Name;
            int cc= Next->Concurrent;
//            delete Next;
  //          Next= new TSoundContainer(pDS, Directory, Name, cc);

//            if( FAILED( hr = FillBuffer() ) );

    //            return hr;
        };
        Next= Next->Next;
    };

};


//void __fastcall TSoundsManager::Init(char *Name, int Concurrent)
//__fastcall TSoundsManager::TSoundsManager(HWND hWnd)
//void __fastcall TSoundsManager::Init(HWND hWnd, char *NDirectory)

void __fastcall TSoundsManager::Init(HWND hWnd)
{

    First= NULL;
    Count= 0;
    pDS= NULL;
    pDSNotify= NULL;

    HRESULT  hr=222;
    LPDIRECTSOUNDBUFFER pDSBPrimary = NULL;

//    strcpy(Directory, NDirectory);

    // Create IDirectSound using the primary sound device
    hr = DirectSoundCreate( NULL, &pDS, NULL );
    if( hr!=DS_OK )
    {

if (hr==DSERR_ALLOCATED)
    return;
if (hr==DSERR_INVALIDPARAM)
    return;
if (hr==DSERR_NOAGGREGATION)
    return;
if (hr==DSERR_NODRIVER)
    return;
if (hr==DSERR_OUTOFMEMORY)
    return;

    hr=0;
    };
//    return ;

    // Set coop level to DSSCL_PRIORITY
//    if( FAILED( hr = pDS->SetCooperativeLevel( hWnd, DSSCL_PRIORITY ) ) );
  //      return ;
    pDS->SetCooperativeLevel( hWnd, DSSCL_PRIORITY );

    // Get the primary buffer
    DSBUFFERDESC dsbd;
    ZeroMemory( &dsbd, sizeof(DSBUFFERDESC) );
    dsbd.dwSize        = sizeof(DSBUFFERDESC);
    dsbd.dwFlags       = DSBCAPS_PRIMARYBUFFER;
    dsbd.dwBufferBytes = 0;
    dsbd.lpwfxFormat   = NULL;

    if( FAILED( hr = pDS->CreateSoundBuffer( &dsbd, &pDSBPrimary, NULL ) ) )
        return ;

    // Set primary buffer format to 22kHz and 16-bit output.
    WAVEFORMATEX wfx;
    ZeroMemory( &wfx, sizeof(WAVEFORMATEX) );
    wfx.wFormatTag      = WAVE_FORMAT_PCM;
    wfx.nChannels       = 2;
    wfx.nSamplesPerSec  = 44100;
    wfx.wBitsPerSample  = 16;
    wfx.nBlockAlign     = wfx.wBitsPerSample / 8 * wfx.nChannels;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    if( FAILED( hr = pDSBPrimary->SetFormat(&wfx) ) )
        return ;

    SAFE_RELEASE( pDSBPrimary );

};


//---------------------------------------------------------------------------
#pragma package(smart_init)
