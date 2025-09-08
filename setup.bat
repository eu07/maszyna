@echo off
echo Preparing vcpkg packages
cd ref\vcpkg
call bootstrap-vcpkg.bat
vcpkg install directx-dxc:x64-windows
vcpkg install freetype:x64-windows
cd ..\..
@echo ON
