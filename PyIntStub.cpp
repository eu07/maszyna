/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "PyInt.h"

bool python_taskqueue::init()
{
	return false;
}

void python_taskqueue::exit()
{
}

bool python_taskqueue::insert(python_taskqueue::task_request const &Task)
{
	return false;
}

bool python_taskqueue::run_file(std::string const &File, std::string const &Path)
{
	return false;
}

void python_taskqueue::acquire_lock()
{
}

void python_taskqueue::release_lock()
{
}

void python_taskqueue::update()
{
}
