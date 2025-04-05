cmake_minimum_required(VERSION 3.24)

set(CMAKE_CXX_STANDARD 17)

project(MaShaderCompiler)

file(GLOB_RECURSE src "src/*.cpp" "src/*.hpp")

# Confirmed to work with DXC distributed via VCPKG. Your mileage may vary
# TODO Make sure that it works as well for people who do not use VCPKG
find_package(directx-dxc CONFIG REQUIRED)

add_executable(MaShaderCompiler ${src})

target_link_libraries(MaShaderCompiler nvrhi yaml-cpp Microsoft::DirectXShaderCompiler Microsoft::DXIL)

if (WIN32)
    target_link_libraries(MaShaderCompiler delayimp)
    target_link_options(MaShaderCompiler PRIVATE "/DELAYLOAD:dxcompiler.dll")
endif ()

# We later pass the DXIL.dll parent directory as first argument to MaShaderCompiler so that it can sign DX12 shaders
get_target_property(dxil_dll_path Microsoft::DXIL IMPORTED_LOCATION)
get_filename_component(dxil_dll_path ${dxil_dll_path} DIRECTORY)
set_target_properties(MaShaderCompiler PROPERTIES dxil_path "${dxil_dll_path}")
