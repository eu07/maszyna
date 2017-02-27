cmake_minimum_required(VERSION 3.0)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMake_modules/")
project("eu07++ng")

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
"wavread.cpp"
"World.cpp"
"AdvSound.cpp"
"AirCoupler.cpp"
"AnimModel.cpp"
"Button.cpp"
"Camera.cpp"
"Console.cpp"
"console/LPT.cpp"
"Console/MWD.cpp"
"console/PoKeys55.cpp"
"Driver.cpp"
"dumb3d.cpp"
"DynObj.cpp"
"EU07.cpp"
"Event.cpp"
"EvLaunch.cpp"
"FadeSound.cpp"
"Float3d.cpp"
"Gauge.cpp"
"Globals.cpp"
"Ground.cpp"
"Logs.cpp"
"mczapkie/friction.cpp"
"mczapkie/hamulce.cpp"
"mczapkie/mctools.cpp"
"mczapkie/Mover.cpp"
"mczapkie/Oerlikon_ESt.cpp"
"MdlMngr.cpp"
"MemCell.cpp"
"Model3d.cpp"
"mtable.cpp"
"parser.cpp"
"renderer.cpp"
"PyInt.cpp"
"RealSound.cpp"
"ResourceManager.cpp"
"sn_utils.cpp"
"Segment.cpp"
"sky.cpp"
"sun.cpp"
"stars.cpp"
"lightarray.cpp"
"skydome.cpp"
"Sound.cpp"
"Spring.cpp"
)

if (WIN32)
	add_definitions(-DHAVE_ROUND) # to make pymath to not redefine round
	set(SOURCES ${SOURCES} "windows.cpp")
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