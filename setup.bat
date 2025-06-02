@echo off
echo Preparing vcpkg packages
cd ref\vcpkg
call bootstrap-vcpkg.bat
vcpkg install directx-dxc:x64-windows
@echo ON
