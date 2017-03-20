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
#ifdef _WINDOWS
#include "targetver.h"
#define NOMINMAX
#include <windows.h>
#include <shlobj.h>
#undef NOMINMAX
#endif
// stl
#include <cstdlib>
#include <cassert>
#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
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

#include "GL/glew.h"
#ifdef _WINDOWS
#include "GL/wglew.h"
#define GLFW_DLL
#endif
#define GLFW_INCLUDE_GLU
//m7todo: jest tu bo nie chcia³o mi siê wpychaæ do wszystkich plików
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>