
#include "stdafx.h"
#include "stars.h"
#include "globals.h"
#include "MdlMngr.h"

//////////////////////////////////////////////////////////////////////////////////////////
// cStars -- simple starfield model, simulating appearance of starry sky

void
cStars::init() {

    m_stars = TModelsManager::GetModel( "models\\skydome_stars.t3d", false );
}
