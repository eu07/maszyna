/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx.h"
#include "piped_proc.h"
#include "Logs.h"

#ifdef __unix__
piped_proc::piped_proc(std::string cmd, bool write)
{
	file = popen(cmd.c_str(), write ? "w" : "r");
}

piped_proc::~piped_proc()
{
	if (file)
		pclose(file);
}

size_t piped_proc::read(unsigned char *buf, size_t len)
{
	if (!file)
		return 0;

	return fread(buf, 1, len, file);
}

size_t piped_proc::write(unsigned char *buf, size_t len)
{
	if (!file)
		return 0;

	return fwrite(buf, 1, len, file);
}
#elif _WIN32
piped_proc::piped_proc(std::string cmd, bool write)
{
	PROCESS_INFORMATION process;
	STARTUPINFO siStartInfo;
	SECURITY_ATTRIBUTES saAttr;

	memset(&process, 0, sizeof(PROCESS_INFORMATION));
	memset(&siStartInfo, 0, sizeof(STARTUPINFO));
	memset(&saAttr, 0, sizeof(SECURITY_ATTRIBUTES));

	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	if (!CreatePipe(&pipe_rd, &pipe_wr, &saAttr, 0)) {
		ErrorLog("piped_proc: CreatePipe failed!");
		return;
	}

	if (!SetHandleInformation(write ? pipe_wr : pipe_rd, HANDLE_FLAG_INHERIT, 0)) {
		ErrorLog("piped_proc: SetHandleInformation failed!");
		return;
	}

	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
	if (!write)
		siStartInfo.hStdOutput = pipe_wr;
	else
		siStartInfo.hStdInput = pipe_rd;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

	if (!CreateProcessA(NULL, (char*)cmd.c_str(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &siStartInfo, &process)) {
		ErrorLog("piped_proc: CreateProcess failed!");
		return;
	}

	proc_h = process.hProcess;
	if (process.hThread)
		CloseHandle(process.hThread);
}

piped_proc::~piped_proc()
{
	if (pipe_wr)
		CloseHandle(pipe_wr);
	if (pipe_rd)
		CloseHandle(pipe_rd);
	if (proc_h)
		CloseHandle(proc_h);
}

size_t piped_proc::read(unsigned char *buf, size_t len)
{
	if (!pipe_rd)
		return 0;

	DWORD read = 0;
	BOOL ret = ReadFile(pipe_rd, buf, len, &read, NULL);

	return read;
}

size_t piped_proc::write(unsigned char *buf, size_t len)
{
	if (!pipe_wr)
		return 0;

	DWORD wrote = 0;
	BOOL ret = WriteFile(pipe_wr, buf, len, &wrote, NULL);

	return wrote;
}
#endif
