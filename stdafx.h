// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#ifndef STDAFX_H
#define STDAFX_H

#ifdef __APPLE__
#ifndef __unix__
#define __unix__ 1
#endif
#endif

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
#ifdef _WIN32
#include "targetver.h"
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#undef NOMINMAX
#endif
#ifdef __unix__
#include <sys/stat.h>
#endif
// stl
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
#include <GLFW/glfw3.h>

#ifndef GLFW_TRUE
#define GLFW_FALSE 0
#define GLFW_TRUE 1
#define glfwGetKeyName(a, b) ("")
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
#define glDebug(x) if (GLEW_GREMEDY_string_marker) glStringMarkerGREMEDY(0, __FILE__ ":" STRINGIZE(__LINE__) ": " x);
#include "openglcolor.h"

#ifdef DBG_NEW
#pragma push_macro("new")
#undef new
#include "imgui/imgui.h"
#pragma pop_macro("new")
#else
#include "imgui/imgui.h"
#endif

#endif
