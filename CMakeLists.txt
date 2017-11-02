cmake_minimum_required(VERSION 3.0)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMake_modules/")
include(PrecompiledHeader)
#include(cotire)
set(DEPS_DIR ${DEPS_DIR} "${CMAKE_SOURCE_DIR}/ref")
project("eu07")

set(CMAKE_CXX_STANDARD 14)
include_directories("." "Console" "McZapkie")
file(GLOB HEADERS "*.h" "Console/*.h" "McZapkie/*.h")

set(SOURCES
"Texture.cpp"
"TextureDDS.cpp"
"Timer.cpp"
"Track.cpp"
"Traction.cpp"
"TractionPower.cpp"
"Train.cpp"
"TrkFoll.cpp"
"VBO.cpp"
"World.cpp"
"AirCoupler.cpp"
"AnimModel.cpp"
"Button.cpp"
"Camera.cpp"
"Driver.cpp"
"dumb3d.cpp"
"DynObj.cpp"
"EU07.cpp"
"Event.cpp"
"EvLaunch.cpp"
"Float3d.cpp"
"Gauge.cpp"
"Globals.cpp"
"Logs.cpp"
"McZapkie/friction.cpp"
"McZapkie/hamulce.cpp"
"McZapkie/mctools.cpp"
"McZapkie/Mover.cpp"
"McZapkie/Oerlikon_ESt.cpp"
"MdlMngr.cpp"
"MemCell.cpp"
"Model3d.cpp"
"mtable.cpp"
"parser.cpp"
"renderer.cpp"
"PyInt.cpp"
"ResourceManager.cpp"
"sn_utils.cpp"
"Segment.cpp"
"sky.cpp"
"sun.cpp"
"stars.cpp"
"lightarray.cpp"
"skydome.cpp"
"sound.cpp"
"Spring.cpp"
"shader.cpp"
"frustum.cpp"
"uilayer.cpp"
"openglmatrixstack.cpp"
"moon.cpp"
"command.cpp"
"keyboardinput.cpp"
"gamepadinput.cpp"
"openglgeometrybank.cpp"
"mouseinput.cpp"
"translation.cpp"
"material.cpp"
"lua.cpp"
"stdafx.cpp"
"uart.cpp"
"messaging.cpp"
"scene.cpp"
"scenenode.cpp"
"simulation.cpp"
"vertex.cpp"
)

set (ARCH "x86")

if (WIN32)
	add_definitions(-DHAVE_ROUND) # to make pymath to not redefine round
	add_definitions(-DEU07_BUILD_STATIC) # to make pymath to not redefine round

	set(SOURCES ${SOURCES} "windows.cpp" "Console.cpp" "Console/LPT.cpp" "Console/PoKeys55.cpp")
	set(GLEW_INCLUDE_DIR ${GLEW_INCLUDE_DIR} "${DEPS_DIR}/glew/include/")
	set(GLFW3_INCLUDE_DIR ${GLFW3_INCLUDE_DIR} "${DEPS_DIR}/glfw/include/")
	set(GLUT_INCLUDE_DIR ${GLUT_INCLUDE_DIR} "${DEPS_DIR}/freeglut/include/")
	set(PNG_PNG_INCLUDE_DIR ${PNG_PNG_INCLUDE_DIR} "${DEPS_DIR}/libpng/include/")
	set(ZLIB_INCLUDE_DIR ${ZLIB_INCLUDE_DIR} "${DEPS_DIR}/zlib/")
	set(GLM_INCLUDE_DIR ${GLM_INCLUDE_DIR} "${DEPS_DIR}/glm/")
	set(OPENAL_INCLUDE_DIR ${OPENAL_INCLUDE_DIR} "${DEPS_DIR}/openal/include")
	set(LIBSNDFILE_INCLUDE_DIR ${LIBSNDFILE_INCLUDE_DIR} "${DEPS_DIR}/libsndfile/include")
	set(LUAJIT_INCLUDE_DIR ${LUAJIT_INCLUDE_DIR} "${DEPS_DIR}/luajit/include")
	set(PYTHON_INCLUDE_DIR ${PYTHON_INCLUDE_DIR} "${DEPS_DIR}/python/include")
	set(libserialport_INCLUDE_DIR ${LIBSERIALPORT_INCLUDE_DIR} "${DEPS_DIR}/libserialport/include")
	
	if(${CMAKE_EXE_LINKER_FLAGS} STREQUAL "/machine:x64")
		set (ARCH "x64")
	endif()
	
	
	set(GLEW_LIBRARY ${GLEW_LIBRARY} "${DEPS_DIR}/glew/lib/${ARCH}/glew32s.lib")
	set(GLFW3_LIBRARIES ${GLFW3_LIBRARIES} "${DEPS_DIR}/glfw/lib/${ARCH}/glfw3.lib")
	set(GLUT_glut_LIBRARY ${GLFW3_ROOT_PATH} "${DEPS_DIR}/freeglut/lib/${ARCH}/freeglut.lib")
	set(PNG_LIBRARY ${PNG_LIBRARY} "${DEPS_DIR}/libpng/lib/${ARCH}/libpng16.lib")
	set(OPENAL_LIBRARY ${OPENAL_LIBRARY} "${DEPS_DIR}/openal/lib/${ARCH}/OpenAL32.lib")
	set(LIBSNDFILE_LIBRARY ${LIBSNDFILE_LIBRARY} "${DEPS_DIR}/libsndfile/lib/${ARCH}/libsndfile-1.lib")
	set(LUAJIT_LIBRARIES ${LUAJIT_LIBRARIES} "${DEPS_DIR}/luajit/lib/${ARCH}/lua51.lib")
	set(PYTHON_LIBRARY ${PYTHON_LIBRARY} "${DEPS_DIR}/python/lib/${ARCH}/python27.lib")	
	set(libserialport_LIBRARY ${LIBSERIALPORT_LIBRARY} "${DEPS_DIR}/libserialport/lib/${ARCH}/libserialport-0.lib")	
	
endif()

if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
	set(SOURCES ${SOURCES} "eu07.rc")
	set(SOURCES ${SOURCES} "eu07.ico")
endif()

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
#set_target_properties(${PROJECT_NAME} PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "stdafx.h")
add_precompiled_header(${PROJECT_NAME} stdafx.h FORCEINCLUDE SOURCE_CXX stdafx.cpp)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
	# /wd4996: disable "deprecation" warnings
	# /wd4244: disable warnings for conversion with possible loss of data
	set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/wd4996 /wd4244")
	set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/LARGEADDRESSAWARE")
endif()

string(TIMESTAMP data_exe %y%m%d)

set_target_properties( ${PROJECT_NAME}
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
	OUTPUT_NAME "${PROJECT_NAME}-${ARCH}_${data_exe}"
	DEBUG_POSTFIX "_d"
)

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES})

find_package(GLEW REQUIRED)
include_directories(${GLEW_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${GLEW_LIBRARIES})

find_package(GLFW3 REQUIRED)
include_directories(${GLFW3_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${GLFW3_LIBRARIES})

find_package(GLUT REQUIRED)
include_directories(${GLUT_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${GLUT_LIBRARIES})

find_package(PythonLibs 2 REQUIRED)
include_directories(${PYTHON_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${PYTHON_LIBRARIES})

find_package(PNG REQUIRED)
include_directories(${PNG_INCLUDE_DIRS} ${PNG_PNG_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${PNG_LIBRARIES})
target_link_libraries(${PROJECT_NAME} ${PNG_LIBRARY})

find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} Threads::Threads)

find_package(GLM REQUIRED)
include_directories(${GLM_INCLUDE_DIRS})

find_package(OpenAL REQUIRED)
include_directories(${OPENAL_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${OPENAL_LIBRARY})

find_package(libsndfile REQUIRED)
include_directories(${LIBSNDFILE_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${LIBSNDFILE_LIBRARY})

find_package(LuaJIT REQUIRED)
include_directories(${LUAJIT_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${LUAJIT_LIBRARIES})

find_package(libserialport REQUIRED)
include_directories(${libserialport_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${libserialport_LIBRARY})
