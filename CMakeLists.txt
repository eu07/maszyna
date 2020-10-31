cmake_minimum_required(VERSION 3.0)
if(POLICY CMP0058) 
	cmake_policy(SET CMP0058 NEW) 
endif()
if(POLICY CMP0074) 
	cmake_policy(SET CMP0074 NEW) 
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMake_modules/")
include(cotire)
set(DEPS_DIR ${DEPS_DIR} "${CMAKE_SOURCE_DIR}/ref")
project("eu07")

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fsanitize=undefined")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=undefined -Wformat")
#set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -fsanitize=address -fsanitize=undefined")

set(CMAKE_CXX_STANDARD 17)
include_directories("."
"Console"
"McZapkie"
"gl"
"ref/glad/include")

file(GLOB HEADERS "*.h"
"Console/*.h"
"McZapkie/*.h"
"gl/*.h"
"network/*.h"
"vr/*.h"
"network/backend/*.h"
"widgets/*.h"
"launcher/*.h"
"extras/*.h")

if (APPLE)
	set (CMAKE_EXE_LINKER_FLAGS "-pagezero_size 10000 -image_base 100000000")
endif()

option(USE_IMGUI_GL3 "Use OpenGL3+ imgui implementation" ON)
option(WITH_OPENGL_MODERN "Compile with OpenGL modern renderer" ON)
option(WITH_OPENGL_LEGACY "Compile with OpenGL legacy renderer" ON)
option(WITH_UART "Compile with libserialport" ON)
option(WITH_OPENVR "Compile with OpenVR" OFF)
option(WITH_ZMQ "Compile with cppzmq" OFF)
option(USE_VSDEV_CAMERA "Use VS_Dev camera preview implementation" OFF)

set(SOURCES
"Texture.cpp"
"Timer.cpp"
"Track.cpp"
"Traction.cpp"
"TractionPower.cpp"
"Train.cpp"
"TrkFoll.cpp"
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
"frustum.cpp"
"uilayer.cpp"
"openglmatrixstack.cpp"
"moon.cpp"
"command.cpp"
"keyboardinput.cpp"
"gamepadinput.cpp"
"drivermouseinput.cpp"
"translation.cpp"
"material.cpp"
"lua.cpp"
"stdafx.cpp"
"messaging.cpp"
"scene.cpp"
"scenenode.cpp"
"simulation.cpp"
"vertex.cpp"
"audio.cpp"
"audiorenderer.cpp"
"motiontelemetry.cpp"
"utilities.cpp"
"uitranscripts.cpp"
"station.cpp"
"application.cpp"
"simulationtime.cpp"
"sceneeditor.cpp"
"screenshot.cpp"
"driverkeyboardinput.cpp"
"drivermode.cpp"
"driveruilayer.cpp"
"driveruipanels.cpp"
"editorkeyboardinput.cpp"
"editormode.cpp"
"editormouseinput.cpp"
"editoruilayer.cpp"
"editoruipanels.cpp"
"scenarioloadermode.cpp"
"scenenodegroups.cpp"
"simulationenvironment.cpp"
"simulationstateserializer.cpp"
"precipitation.cpp"
"pythonscreenviewer.cpp"
"dictionary.cpp"
"particles.cpp"
"headtrack.cpp"
"headtrack.h"
"ladderlogic.cpp"
"geometrybank.cpp"
"openglcolor.cpp"
"simulationsounds.cpp"
"openglcamera.cpp"

"network/network.cpp"
"network/message.cpp"
"network/manager.cpp"
"network/backend/asio.cpp"

"widgets/vehiclelist.cpp"
"widgets/map.cpp"
"widgets/map_objects.cpp"
"widgets/popup.cpp"
"widgets/time.cpp"
"widgets/vehicleparams.cpp"
"widgets/trainingcard.cpp"
"widgets/perfgraphs.cpp"

"ref/glad/src/glad.c"

"gl/shader.cpp"
"gl/vao.cpp"
"gl/ubo.cpp"
"gl/framebuffer.cpp"
"gl/renderbuffer.cpp"
"gl/postfx.cpp"
"gl/cubemap.cpp"
"gl/glsl_common.cpp"
"gl/buffer.cpp"
"gl/fence.cpp"
"gl/pbo.cpp"
"gl/query.cpp"

"imgui/imgui.cpp"
"imgui/imgui_demo.cpp"
"imgui/imgui_draw.cpp"
"imgui/imgui_widgets.cpp"
"imgui/imgui_impl_glfw.cpp"

"stb/stb_image.c"

"launcher/launchermode.cpp"
"launcher/scenery_list.cpp"
"launcher/launcheruilayer.cpp"
"launcher/vehicle_picker.cpp"
"launcher/keymapper.cpp"
"launcher/textures_scanner.cpp"
"launcher/scenery_scanner.cpp"

"vr/vr_interface.cpp"
)

if (USE_IMGUI_GL3)
	set(SOURCES ${SOURCES} "imgui/imgui_impl_opengl3.cpp")
else()
	add_definitions(-DEU07_USEIMGUIIMPLOPENGL2)
	set(SOURCES ${SOURCES} "imgui/imgui_impl_opengl2.cpp")
endif()

if (WITH_UART)
	add_definitions(-DWITH_UART)
	set(SOURCES ${SOURCES} "uart.cpp")
endif()

if (WITH_ZMQ)
	add_definitions(-DWITH_ZMQ)
	set(SOURCES ${SOURCES} "zmq_input.cpp")
endif()

if (WITH_OPENVR)
        add_definitions(-DWITH_OPENVR)
        set(SOURCES ${SOURCES} "vr/openvr_imp.cpp")
endif()

if (WITH_OPENGL_MODERN)
        set(SOURCES ${SOURCES}
            "opengl33geometrybank.cpp"
            "opengl33light.cpp"
            "opengl33skydome.cpp"
            "opengl33precipitation.cpp"
            "opengl33particles.cpp"
            "opengl33renderer.cpp")
        add_definitions(-DWITH_OPENGL_MODERN)
endif()

if (WITH_OPENGL_LEGACY)
        set(SOURCES ${SOURCES}
            "openglgeometrybank.cpp"
            "opengllight.cpp"
            "openglskydome.cpp"
            "openglprecipitation.cpp"
            "openglparticles.cpp"
            "openglrenderer.cpp")
        add_definitions(-DWITH_OPENGL_LEGACY)
endif()

if (USE_VSDEV_CAMERA)
	add_definitions(-DUSE_VSDEV_CAMERA)
	set(SOURCES ${SOURCES} "extras/VS_Dev.cpp" "widgets/cameraview_vsdev.cpp")
else()
	add_definitions(-DUSE_EXTCAM_CAMERA)
	set(SOURCES ${SOURCES} "extras/piped_proc.cpp" "widgets/cameraview_extcam.cpp")
endif()

find_package(Git REQUIRED)

string(TIMESTAMP BUILD_DATE %y%m%d)

execute_process(
  COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_BRANCH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND ${GIT_EXECUTABLE} describe --match=DUMMY --always --dirty
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

configure_file("${CMAKE_SOURCE_DIR}/version_info.h.in" "${CMAKE_SOURCE_DIR}/version_info.h")

set (PREFIX "")

if (WIN32)
	add_definitions(-DHAVE_ROUND) # to make pymath to not redefine round
	add_definitions(-DGLFW_DLL)

	set(SOURCES ${SOURCES} "windows.cpp" "Console.cpp" "Console/LPT.cpp" "Console/PoKeys55.cpp")
	set(GLFW3_INCLUDE_DIR ${GLFW3_INCLUDE_DIR} "${DEPS_DIR}/glfw/include/")
	set(PNG_INCLUDE_DIRS ${PNG_INCLUDE_DIRS} "${DEPS_DIR}/libpng/include/")
	set(ZLIB_INCLUDE_DIR ${ZLIB_INCLUDE_DIR} "${DEPS_DIR}/zlib/")
	set(OPENAL_INCLUDE_DIR ${OPENAL_INCLUDE_DIR} "${DEPS_DIR}/openal/include")
	set(LIBSNDFILE_INCLUDE_DIR ${LIBSNDFILE_INCLUDE_DIR} "${DEPS_DIR}/libsndfile/include")
	set(LUAJIT_INCLUDE_DIR ${LUAJIT_INCLUDE_DIR} "${DEPS_DIR}/luajit/include")
	set(PYTHON_INCLUDE_DIR ${PYTHON_INCLUDE_DIR} "${DEPS_DIR}/python/include")
	set(libserialport_INCLUDE_DIR ${LIBSERIALPORT_INCLUDE_DIR} "${DEPS_DIR}/libserialport/include")
	
	if(${CMAKE_EXE_LINKER_FLAGS} STREQUAL "/machine:x64")
		set (ARCH "x64")
	else()
		set (ARCH "x86")
	endif()

	set (PREFIX "-${ARCH}")

	set(GLFW3_LIBRARIES ${GLFW3_LIBRARIES} "${DEPS_DIR}/glfw/lib/${ARCH}/glfw3dll.lib")
	set(PNG_LIBRARIES ${PNG_LIBRARIES} "${DEPS_DIR}/libpng/lib/${ARCH}/libpng16.lib")
	set(OPENAL_LIBRARY ${OPENAL_LIBRARY} "${DEPS_DIR}/openal/lib/${ARCH}/OpenAL32.lib")
	set(LIBSNDFILE_LIBRARY ${LIBSNDFILE_LIBRARY} "${DEPS_DIR}/libsndfile/lib/${ARCH}/libsndfile-1.lib")
	set(LUAJIT_LIBRARIES ${LUAJIT_LIBRARIES} "${DEPS_DIR}/luajit/lib/${ARCH}/lua51.lib")
	set(PYTHON_LIBRARY ${PYTHON_LIBRARY} "${DEPS_DIR}/python/lib/${ARCH}/python27.lib")
	set(libserialport_LIBRARY ${LIBSERIALPORT_LIBRARY} "${DEPS_DIR}/libserialport/lib/${ARCH}/libserialport-0.lib")
	set(cppzmq_LIBRARY ${cppzmq_LIBRARY} "${DEPS_DIR}/cppzmq/lib/${ARCH}/libzmq.lib")
endif()

set(cppzmq_INCLUDE_DIR ${cppzmq_INCLUDE_DIR} "${DEPS_DIR}/cppzmq/include")
set(GLM_INCLUDE_DIR ${GLM_INCLUDE_DIR} "${DEPS_DIR}/glm/")

if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
	set(SOURCES ${SOURCES} "eu07.rc")
	set(SOURCES ${SOURCES} "eu07.ico")
	add_compile_options("/utf-8")
endif()

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
	# /wd4996: disable "deprecation" warnings
	# /wd4244: disable warnings for conversion with possible loss of data
	set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/wd4996 /wd4244")
	set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/LARGEADDRESSAWARE")
endif()

set_target_properties( ${PROJECT_NAME}
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/pdb"
        OUTPUT_NAME "${PROJECT_NAME}${PREFIX}_${BUILD_DATE}"
	DEBUG_POSTFIX "_d"
)

if (WITH_OPENVR)
        target_link_libraries(${PROJECT_NAME} openvr_api)
endif()

find_package(GLFW3 REQUIRED)
include_directories(${GLFW3_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${GLFW3_LIBRARIES})

find_package(PythonLibs 2 REQUIRED)
include_directories(${PYTHON_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${PYTHON_LIBRARIES})

if (NOT WIN32)
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(PNG libpng16 REQUIRED)
endif()
include_directories(${PNG_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${PNG_LIBRARIES})

find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} Threads::Threads)

find_package(GLM REQUIRED)
include_directories(${GLM_INCLUDE_DIR})

find_package(OpenAL REQUIRED)
include_directories(${OPENAL_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${OPENAL_LIBRARY})

find_package(libsndfile REQUIRED)
include_directories(${LIBSNDFILE_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${LIBSNDFILE_LIBRARY})

find_package(LuaJIT REQUIRED)
include_directories(${LUAJIT_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${LUAJIT_LIBRARIES})

if (WITH_UART)
	find_package(libserialport REQUIRED)
	include_directories(${libserialport_INCLUDE_DIR})
	target_link_libraries(${PROJECT_NAME} ${libserialport_LIBRARY})
endif()

find_package(ASIO REQUIRED)
target_link_libraries(${PROJECT_NAME} ASIO::ASIO)

if (WIN32)
	target_link_libraries(${PROJECT_NAME} ws2_32)
endif()

if (WITH_ZMQ)
	find_package(cppzmq REQUIRED)
	include_directories(${cppzmq_INCLUDE_DIR})
	target_link_libraries(${PROJECT_NAME} ${cppzmq_LIBRARY})
endif()

#target_compile_options(${PROJECT_NAME} PRIVATE -flto)
#target_link_libraries(${PROJECT_NAME} -flto)

#set_target_properties(${PROJECT_NAME} PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "stdafx.h")
#set_target_properties(${PROJECT_NAME} PROPERTIES COTIRE_ADD_UNITY_BUILD FALSE)
#cotire(${PROJECT_NAME})
