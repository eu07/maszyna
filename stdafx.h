// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifdef _MSC_VER
// memory debug functions
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#ifdef _DEBUG
#ifndef DBG_NEW
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#define new DBG_NEW
#endif
#endif  // _DEBUG
#endif
// operating system
#ifdef _WIN32
#include "targetver.h"
#define NOMINMAX
#include <windows.h>
#include <shlobj.h>
#undef NOMINMAX
#include <dbghelp.h>
#include <direct.h>
#include <strsafe.h>
#endif
// stl
#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <array>
#include <vector>
#include <deque>
#include <list>
#include <forward_list>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <tuple>
#include <cctype>
#include <locale>
#include <codecvt>
#include <iterator>
#include <random>
#include <algorithm>
#include <functional>
#include <regex>
#include <limits>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <typeinfo>

#ifdef NDEBUG
#define EU07_BUILD_STATIC
#endif

#ifdef EU07_BUILD_STATIC
#define GLEW_STATIC
#else
#ifdef _WIN32
#define GLFW_DLL
#endif // _windows
#endif // build_static
#ifndef __ANDROID__
#include "GL/glew.h"
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#ifdef _WIN32
#include "GL/wglew.h"
#endif
#define GLFW_INCLUDE_GLU
//m7todo: jest tu bo nie chcia³o mi siê wpychaæ do wszystkich plików
#ifndef __ANDROID__
#include <GLFW/glfw3.h>
#endif

#define GLM_ENABLE_EXPERIMENTAL 
#define GLM_FORCE_CTOR_INIT 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/norm.hpp>

#include "openglmatrixstack.h"

// imgui.h comes with its own operator new which gets wrecked by dbg_new, so we temporarily disable the latter
#ifdef _MSC_VER
#ifdef _DEBUG
#undef new
#endif  // _DEBUG
#endif
#include "imgui.h"
#ifdef _MSC_VER
#ifdef _DEBUG
#define new DBG_NEW
#endif  // _DEBUG
#endif
