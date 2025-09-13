#pragma once

// Including SDKDDKVer.h defines the highest available Windows platform.

// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.

#include <winsdkver.h>
#ifndef _WIN32_WINNT
//#define _WIN32_WINNT  0x0600     // _WIN32_WINNT_VISTA
#define _WIN32_WINNT  0x0601     // _WIN32_WINNT_WINXP
#endif

#include <SDKDDKVer.h>
