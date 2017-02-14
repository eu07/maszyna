#include "stdafx.h"

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
		::GetSystemTime(&t);
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