// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define _USE_MATH_DEFINES
#include <cmath>
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
#ifdef _WINDOWS
#include "targetver.h"
#define NOMINMAX
#include <windows.h>
#include <shlobj.h>
#undef NOMINMAX
#endif
#ifdef __linux__
#include <sys/stat.h>
#endif
// stl
#include <cstddef>
#include <cstdlib>
#include <cassert>
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

#ifdef EU07_BUILD_STATIC
#define GLEW_STATIC
#else
#ifdef _WINDOWS
#define GLFW_DLL
#endif // _windows
#endif // build_static
#include "GL/glew.h"
#ifdef _WINDOWS
#include "GL/wglew.h"
#endif
#define GLFW_INCLUDE_GLU

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/string_cast.hpp>

#include "openglmatrixstack.h"
#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)
#define glDebug(x) if (GLEW_GREMEDY_string_marker) glStringMarkerGREMEDY(0, __FILE__ ":" STRINGIZE(__LINE__) ": " x);
