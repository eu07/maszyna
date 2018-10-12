#include "stdafx.h"
#include "messaging.h"
#include "utilities.h"

#pragma warning (disable: 4091)
#include <dbghelp.h>

LONG CALLBACK unhandled_handler(::EXCEPTION_POINTERS* e)
{
	auto hDbgHelp = ::LoadLibraryA("dbghelp");
	if (hDbgHelp == nullptr)
		return EXCEPTION_CONTINUE_SEARCH;
	auto pMiniDumpWriteDump = (decltype(&MiniDumpWriteDump))::GetProcAddress(hDbgHelp, "MiniDumpWriteDump");
	if (pMiniDumpWriteDump == nullptr)
		return EXCEPTION_CONTINUE_SEARCH;

	char name[MAX_PATH];
	{
		auto nameEnd = name + ::GetModuleFileNameA(::GetModuleHandleA(0), name, MAX_PATH);
		::SYSTEMTIME t;
		::GetLocalTime(&t);
		wsprintfA(nameEnd - strlen(".exe"),
			"_crashdump_%4d%02d%02d_%02d%02d%02d.dmp",
			t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
	}

	auto hFile = ::CreateFileA(name, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE)
		return EXCEPTION_CONTINUE_SEARCH;

	::MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;
	exceptionInfo.ThreadId = ::GetCurrentThreadId();
	exceptionInfo.ExceptionPointers = e;
	exceptionInfo.ClientPointers = FALSE;

	auto dumped = pMiniDumpWriteDump(
		::GetCurrentProcess(),
		::GetCurrentProcessId(),
		hFile,
		::MINIDUMP_TYPE(::MiniDumpWithIndirectlyReferencedMemory | ::MiniDumpScanMemory),
		e ? &exceptionInfo : nullptr,
		nullptr,
		nullptr);

	::CloseHandle(hFile);

	return EXCEPTION_CONTINUE_SEARCH;
}

HWND Hwnd;
WNDPROC BaseWindowProc;
PCOPYDATASTRUCT pDane;

LRESULT APIENTRY WndProc( HWND hWnd, // handle for this window
                         UINT uMsg, // message for this window
                         WPARAM wParam, // additional message information
                         LPARAM lParam) // additional message information
{
    switch( uMsg ) // check for windows messages
    {
        case WM_COPYDATA: {
            // obsługa danych przesłanych przez program sterujący
            pDane = (PCOPYDATASTRUCT)lParam;
            if( pDane->dwData == MAKE_ID4('E', 'U', '0', '7')) // sygnatura danych
                multiplayer::OnCommandGet( ( multiplayer::DaneRozkaz *)( pDane->lpData ) );
            break;
        }
    }
    // pass all unhandled messages to DefWindowProc
    return CallWindowProc( BaseWindowProc, Hwnd, uMsg, wParam, lParam );
};