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
#define _USE_MATH_DEFINES
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <cstdio>
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
#include <bitset>
#include <chrono>
#include <optional>
#include <filesystem>

#ifdef NDEBUG
#define EU07_BUILD_STATIC
#endif

#include "glad/glad.h"

#include "GL/glu.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#ifndef GLFW_TRUE
#define GLFW_FALSE 0
#define GLFW_TRUE 1
#define glfwGetKeyName(a, b) ("")
#define glfwFocusWindow(w)
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
#include <glm/gtx/string_cast.hpp>

int const null_handle = 0;

#include "openglmatrixstack.h"
#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)
#define glDebug(x) if (GLAD_GL_GREMEDY_string_marker) glStringMarkerGREMEDY(0, __FILE__ ":" STRINGIZE(__LINE__) ": " x);
#include "openglcolor.h"

// imgui.h comes with its own operator new which gets wrecked by dbg_new, so we temporarily disable the latter
#ifdef DBG_NEW
#pragma push_macro("new")
#undef new
#include "imgui.h"
#pragma pop_macro("new")
#else
#include "imgui.h"
#endif
