if not exist deps_win call %~dp0download_windeps.bat
set DEPS_DIR="%cd%/deps_win"
if not exist build_win32 mkdir build_win32
pushd build_win32
cmake ../.. -T v140_xp ^
-DGLEW_INCLUDE_DIR=%DEPS_DIR%/glew-2.0.0/include ^
-DGLEW_LIBRARY=%DEPS_DIR%/glew-2.0.0/lib/Release/Win32/glew32.lib ^
-DGLFW3_ROOT_PATH=%DEPS_DIR%/glfw-3.2.1.bin.WIN32 ^
-DGLUT_INCLUDE_DIR=%DEPS_DIR%/freeglut/include ^
-DGLUT_glut_LIBRARY=%DEPS_DIR%/freeglut/lib/freeglut.lib ^
-DPNG_PNG_INCLUDE_DIR=%DEPS_DIR%/libpng/include ^
-DPNG_LIBRARY=%DEPS_DIR%/libpng/lib/win32/libpng16.lib ^
-DZLIB_INCLUDE_DIR=%DEPS_DIR%/zlib-1.2.11 ^
-DGLM_ROOT_DIR=%DEPS_DIR%/glm-0.9.8.4 ^
-DOPENAL_INCLUDE_DIR=%DEPS_DIR%/openal/include ^
-DOPENAL_LIBRARY=%DEPS_DIR%/openal/lib/win32/OpenAL32.lib ^
-DLIBSNDFILE_INCLUDE_DIR=%DEPS_DIR%/libsndfile/include ^
-DLIBSNDFILE_LIBRARY=%DEPS_DIR%/libsndfile/lib/win32/libsndfile-1.lib ^
-DLUAJIT_INCLUDE_DIR=%DEPS_DIR%/luajit/include ^
-DLUAJIT_LIBRARIES=%DEPS_DIR%/luajit/lib/win32/lua51.lib ^
-Dlibserialport_INCLUDE_DIR=%DEPS_DIR%/libserialport/include ^
-Dlibserialport_LIBRARY=%DEPS_DIR%/libserialport/lib/win32/libserialport-0.lib
popd