cmake_minimum_required(VERSION 3.0)
if(POLICY CMP0058)
	cmake_policy(SET CMP0058 NEW)
endif()
if(POLICY CMP0074)
	cmake_policy(SET CMP0074 NEW)
endif()
if(POLICY CMP0069)
	cmake_policy(SET CMP0069 NEW)
endif()

include(CheckCXXSymbolExists)

set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMake_modules/")

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
"ref/glad/include"
"ref/dds-ktx/include"
"${CMAKE_BINARY_DIR}/geninc")

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
	set (CMAKE_FIND_FRAMEWORK LAST)
endif()

option(WITH_OPENGL_MODERN "Compile with OpenGL modern renderer" ON)
option(WITH_OPENGL_LEGACY "Compile with OpenGL legacy renderer" ON)
option(WITH_PYTHON "Compile with python2" ON)
option(WITH_UART "Compile with libserialport" ON)
option(WITH_LUA "Compile with lua scripting support" ON)
option(WITH_OPENVR "Compile with OpenVR" ON)
option(WITH_ZMQ "Compile with cppzmq" OFF)
option(WITH_CRASHPAD "Compile with crashpad" OFF)
option(USE_LTO "Use link-time optimization" OFF)

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
"driverhints.cpp"
"dumb3d.cpp"
"DynObj.cpp"
"EU07.cpp"
"export_e3d_standalone.cpp"
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
"nullrenderer.cpp"
"renderer.cpp"
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
"ref/dds-ktx/src/dds-ktx.c"

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

set(SOURCES ${SOURCES} "imgui/imgui_impl_opengl3.cpp")
set(SOURCES ${SOURCES} "imgui/imgui_impl_opengl2.cpp")
set_source_files_properties("imgui/imgui_impl_opengl3.cpp" PROPERTIES SKIP_PRECOMPILE_HEADERS TRUE)
set_source_files_properties("imgui/imgui_impl_opengl2.cpp" PROPERTIES SKIP_PRECOMPILE_HEADERS TRUE)

if (WITH_CRASHPAD)
	add_definitions(-DWITH_CRASHPAD)
	set(SOURCES ${SOURCES} "crashreporter.cpp")
	if (WIN32)
		include_directories("ref/crashpad/include" "ref/crashpad/include/mini_chromium")
	else()
		include_directories("ref/crashpad/crashpad" "ref/crashpad/crashpad/third_party/mini_chromium/mini_chromium")
	endif()
endif()

if (WITH_PYTHON)
	add_definitions(-DWITH_PYTHON)
	set(SOURCES ${SOURCES} "PyInt.cpp")
else()
	set(SOURCES ${SOURCES} "PyIntStub.cpp")
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

if (WITH_LUA)
	add_definitions(-DWITH_LUA)
	set(SOURCES ${SOURCES} "lua.cpp")
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

set(SOURCES ${SOURCES} "extras/piped_proc.cpp" "widgets/cameraview_extcam.cpp")

find_package(Git)

if (GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
	execute_process(
	  COMMAND ${GIT_EXECUTABLE} describe --match=DUMMY --always --dirty
	  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	  OUTPUT_VARIABLE GIT_HASH
	  OUTPUT_STRIP_TRAILING_WHITESPACE
	)

    execute_process(
	  COMMAND ${GIT_EXECUTABLE} show -s --format=%ci
	  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	  OUTPUT_VARIABLE SRC_DATE
	  OUTPUT_STRIP_TRAILING_WHITESPACE
	)

    execute_process(
	  COMMAND ${GIT_EXECUTABLE} show -s --format=%cs
	  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	  OUTPUT_VARIABLE SRC_SHORT_DATE
	  OUTPUT_STRIP_TRAILING_WHITESPACE
	)
else()
	string(TIMESTAMP SRC_SHORT_DATE %Y-%m-%d UTC)
	string(TIMESTAMP SRC_DATE UTC)
	set(GIT_HASH "(unknown)")
endif()

configure_file("${CMAKE_SOURCE_DIR}/version_info.h.in" "${CMAKE_BINARY_DIR}/geninc/version_info.h")

set (PREFIX "")

set(OPENVR_LIBRARY "openvr_api")
set(OPENVR_INCLUDE_DIR "")

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
	set(OPENVR_INCLUDE_DIR "${DEPS_DIR}/openvr/include")
	
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
	set(OPENVR_LIBRARY "${DEPS_DIR}/openvr/lib/${ARCH}/openvr_api.lib")
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
	# override cmake defaults
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO " /MD /Zi /O2 /DNDEBUG")
	set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO " /DEBUG /INCREMENTAL:NO /OPT:REF /OPT:ICF")

	# /wd4996: disable "deprecation" warnings
	# /wd4244: disable warnings for conversion with possible loss of data
	# /wd5033: disable because it is all over Python headers
	set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/wd4996 /wd4244 /wd5033")
	set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/LARGEADDRESSAWARE")
endif()

set_target_properties( ${PROJECT_NAME}
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/pdb"
	OUTPUT_NAME "${PROJECT_NAME}${PREFIX}_${SRC_SHORT_DATE}"
	DEBUG_POSTFIX "_d"
)

check_cxx_symbol_exists(__GNUC__ "" COMPILER_HAVE_GNUC)
if (COMPILER_HAVE_GNUC)
	set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-fvisibility=hidden")
endif()

if(NOT ${CMAKE_VERSION} VERSION_LESS "3.16.0")
	option(USE_PCH "Use precompiled header" ON)

	if (USE_PCH)
		target_precompile_headers(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/stdafx.h>")
		set_source_files_properties("ref/glad/src/glad.c" "ref/dds-ktx/src/dds-ktx.c" "stb/stb_image.c"
			"imgui/imgui.cpp" "imgui/imgui_demo.cpp" "imgui/imgui_draw.cpp" "imgui/imgui_widgets.cpp" "imgui/imgui_impl_glfw.cpp"
			PROPERTIES SKIP_PRECOMPILE_HEADERS TRUE)
	endif()
endif()

if (USE_LTO)
	set_property(TARGET ${PROJECT_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

if (WITH_OPENVR)
	include_directories(${OPENVR_INCLUDE_DIR})
	target_link_libraries(${PROJECT_NAME} ${OPENVR_LIBRARY})
endif()

if (WITH_CRASHPAD)
	if (WIN32)
		target_link_libraries(${PROJECT_NAME} "${CMAKE_SOURCE_DIR}/ref/crashpad/lib_md/base.lib")
		target_link_libraries(${PROJECT_NAME} "${CMAKE_SOURCE_DIR}/ref/crashpad/lib_md/client.lib")
		target_link_libraries(${PROJECT_NAME} "${CMAKE_SOURCE_DIR}/ref/crashpad/lib_md/util.lib")
	else()
		target_link_libraries(${PROJECT_NAME} "${CMAKE_SOURCE_DIR}/ref/crashpad/crashpad/out/Default/obj/client/libclient.a")
		target_link_libraries(${PROJECT_NAME} "${CMAKE_SOURCE_DIR}/ref/crashpad/crashpad/out/Default/obj/util/libutil.a")
		target_link_libraries(${PROJECT_NAME} "${CMAKE_SOURCE_DIR}/ref/crashpad/crashpad/out/Default/obj/third_party/mini_chromium/mini_chromium/base/libbase.a")
	endif()
endif()

find_package(glfw3 REQUIRED)
target_link_libraries(${PROJECT_NAME} glfw)

if (WITH_PYTHON)
	find_package(PythonLibs 2 REQUIRED)
	include_directories(${PYTHON_INCLUDE_DIRS})
	target_link_libraries(${PROJECT_NAME} ${PYTHON_LIBRARIES})
endif()

if (NOT WIN32)
	find_package(PNG 1.6 REQUIRED)
	target_link_libraries(${PROJECT_NAME} PNG::PNG)
else()
	include_directories(${PNG_INCLUDE_DIRS})
	target_link_libraries(${PROJECT_NAME} ${PNG_LIBRARIES})
endif()

find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} Threads::Threads)

find_package(GLM REQUIRED)
include_directories(${GLM_INCLUDE_DIR})

find_package(OpenAL REQUIRED)
if (TARGET OpenAL::OpenAL)
	target_link_libraries(${PROJECT_NAME} OpenAL::OpenAL)
else()
	include_directories(${OPENAL_INCLUDE_DIR})
	target_link_libraries(${PROJECT_NAME} ${OPENAL_LIBRARY})
endif()

find_package(SndFile REQUIRED)
target_link_libraries(${PROJECT_NAME} SndFile::sndfile)

if (WITH_LUA)
	find_package(LuaJIT REQUIRED)
	include_directories(${LUAJIT_INCLUDE_DIR})
	target_link_libraries(${PROJECT_NAME} ${LUAJIT_LIBRARIES})
endif()

if (WITH_UART)
	find_package(libserialport REQUIRED)
	include_directories(${libserialport_INCLUDE_DIR})
	target_link_libraries(${PROJECT_NAME} ${libserialport_LIBRARY})
endif()

find_package(ASIO REQUIRED)
target_link_libraries(${PROJECT_NAME} asio::asio)

if (WIN32)
	target_link_libraries(${PROJECT_NAME} ws2_32)
endif()

if (WITH_ZMQ)
	find_package(cppzmq REQUIRED)
	include_directories(${cppzmq_INCLUDE_DIR})
	target_link_libraries(${PROJECT_NAME} ${cppzmq_LIBRARY})
endif()
