//---------------------------------------------------------------------------
#ifndef SoundH
#define SoundH

#include <stack>
#undef EOF
#include <objbase.h>
//#include <initguid.h>
//#include <commdlg.h>
#include <mmreg.h>
#include <dsound.h>
//#include "resource.h"
#include "WavRead.h"

typedef LPDIRECTSOUNDBUFFER PSound;

//inline __fastcall Playing(PSound Sound)
//{

//}

class TSoundContainer
{
public:
 int Concurrent;
 int Oldest;
 char Name[80];
 LPDIRECTSOUNDBUFFER DSBuffer;
 TSoundContainer *Next;
 std::stack< LPDIRECTSOUNDBUFFER > DSBuffers;
 LPDIRECTSOUNDBUFFER __fastcall GetUnique(LPDIRECTSOUND pDS);
 __fastcall TSoundContainer(LPDIRECTSOUND pDS, char *Directory, char *strFileName, int NConcurrent);
 __fastcall ~TSoundContainer();
};


class TSoundsManager
{
 static LPDIRECTSOUND       pDS;
 static LPDIRECTSOUNDNOTIFY pDSNotify;
// static char Directory[80];
 static int Count;
 static TSoundContainer *First;
public:
// __fastcall TSoundsManager(HWND hWnd);
// static void __fastcall Init(HWND hWnd, char *NDirectory);
 static void __fastcall Init(HWND hWnd);
 __fastcall ~TSoundsManager();
 static __fastcall Free();
 static void __fastcall Init(char *Name, int Concurrent);
 static TSoundContainer* __fastcall LoadFromFile(char *Name,int Concurrent);
 static void __fastcall LoadSounds(char *Directory);
 static LPDIRECTSOUNDBUFFER __fastcall GetFromName(char *Name);
 static void __fastcall RestoreAll();
};

//---------------------------------------------------------------------------
#endif
