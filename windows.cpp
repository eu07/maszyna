#include "stdafx.h"
#include "World.h"
#include "usefull.h"

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
extern TWorld World;

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
                World.OnCommandGet( ( multiplayer::DaneRozkaz *)( pDane->lpData ) );
            break;
        }
		case WM_KEYDOWN:
		case WM_KEYUP: {
			if (wParam == VK_INSERT || wParam == VK_DELETE || wParam == VK_HOME || wParam == VK_END ||
				wParam == VK_PRIOR || wParam == VK_NEXT || wParam == VK_SNAPSHOT)
				lParam &= ~0x1ff0000;
			if (wParam == VK_INSERT)
				lParam |= 0x152 << 16;
			else if (wParam == VK_DELETE)
				lParam |= 0x153 << 16;
			else if (wParam == VK_HOME)
				lParam |= 0x147 << 16;
			else if (wParam == VK_END)
				lParam |= 0x14F << 16;
			else if (wParam == VK_PRIOR)
				lParam |= 0x149 << 16;
			else if (wParam == VK_NEXT)
				lParam |= 0x151 << 16;
			else if (wParam == VK_SNAPSHOT)
				lParam |= 0x137 << 16;
			break;
		}
    }
    // pass all messages to DefWindowProc
    return CallWindowProc( BaseWindowProc, Hwnd, uMsg, wParam, lParam );
};
