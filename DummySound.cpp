/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include <string>
#include "Sound.h"

TSoundContainer::TSoundContainer(LPDIRECTSOUND pDS, std::string const &Directory, std::string const &Filename, int NConcurrent)
{
};

TSoundContainer::~TSoundContainer()
{
};

LPDIRECTSOUNDBUFFER TSoundContainer::GetUnique(LPDIRECTSOUND pDS)
{
	return new dummysb();
};

TSoundsManager::~TSoundsManager()
{
};

void TSoundsManager::Free()
{
};

TSoundContainer * TSoundsManager::LoadFromFile( std::string const &Dir, std::string const &Filename, int Concurrent)
{
    return new TSoundContainer(0, Dir, Filename, Concurrent);
};

LPDIRECTSOUNDBUFFER TSoundsManager::GetFromName(std::string const &Name, bool Dynamic, float *fSamplingRate)
{
    return new dummysb();
};

void TSoundsManager::RestoreAll()
{
};

void TSoundsManager::Init(HWND hWnd)
{
};
