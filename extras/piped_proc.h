/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#ifdef __unix__
#include <cstdio>
#elif _WIN32
#include "winheaders.h"
#endif
#include <string>

class piped_proc
{
private:
#ifdef __unix__
FILE *file = nullptr;
#elif _WIN32
HANDLE proc_h = nullptr;
HANDLE pipe_rd = nullptr;
HANDLE pipe_wr = nullptr;
#endif

public:
    piped_proc(std::string cmd, bool write = false);
	size_t read(unsigned char *buf, size_t len);
	size_t write(unsigned char *buf, size_t len);
	~piped_proc();
};
