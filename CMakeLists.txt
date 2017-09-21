cmake_minimum_required(VERSION 3.0)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMake_modules/")
project("eu07++ng")

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
"Ground.cpp"
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
"uart.cpp"
)

if (WIN32)
	add_definitions(-DHAVE_ROUND) # to make pymath to not redefine round
	set(SOURCES ${SOURCES} "windows.cpp" "Console.cpp" "Console/LPT.cpp" "Console/PoKeys55.cpp" "wavread.cpp")
endif()

if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
	set(SOURCES ${SOURCES} "eu07.rc")
	set(SOURCES ${SOURCES} "eu07.ico")
endif()

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
	# /wd4996: disable "deprecation" warnings
	# /wd4244: disable warnings for conversion with possible loss of data
	set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/wd4996 /wd4244")
	set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/LARGEADDRESSAWARE")
endif()

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

if (UNIX)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined -g")
endif()
