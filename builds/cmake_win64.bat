if not exist deps_win call %~dp0download_windeps.bat
set DEPS_DIR="%cd%/deps_win"
if not exist build_win64 mkdir build_win64
pushd build_win64
cmake ../.. -A x64 ^
-DGLEW_INCLUDE_DIR=%DEPS_DIR%/glew-2.0.0/include ^
-DGLEW_LIBRARY=%DEPS_DIR%/glew-2.0.0/lib/Release/x64/glew32.lib ^
-DGLFW3_ROOT_PATH=%DEPS_DIR%/glfw-3.2.1.bin.WIN64 ^
-DGLUT_INCLUDE_DIR=%DEPS_DIR%/freeglut/include ^
-DGLUT_glut_LIBRARY=%DEPS_DIR%/freeglut/lib/x64/freeglut.lib ^
-DPNG_PNG_INCLUDE_DIR=%DEPS_DIR%/libpng/include ^
-DPNG_LIBRARY=%DEPS_DIR%/libpng/lib/win64/libpng16.lib ^
-DZLIB_INCLUDE_DIR=%DEPS_DIR%/zlib-1.2.11 ^
-DGLM_ROOT_DIR=%DEPS_DIR%/glm-0.9.8.4 ^
-DOPENAL_INCLUDE_DIR=%DEPS_DIR%/openal/include ^
-DOPENAL_LIBRARY=%DEPS_DIR%/openal/lib/win64/OpenAL32.lib ^
-DLIBSNDFILE_INCLUDE_DIR=%DEPS_DIR%/libsndfile/include ^
-DLIBSNDFILE_LIBRARY=%DEPS_DIR%/libsndfile/lib/win64/libsndfile-1.lib ^
-DLUAJIT_INCLUDE_DIR=%DEPS_DIR%/luajit/include ^
-DLUAJIT_LIBRARIES=%DEPS_DIR%/luajit/lib/win64/lua51.lib ^
-Dlibserialport_INCLUDE_DIR=%DEPS_DIR%/libserialport/include ^
-Dlibserialport_LIBRARY=%DEPS_DIR%/libserialport/lib/win64/libserialport-0.lib
popd