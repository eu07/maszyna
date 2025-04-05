
set(LIBMANUL_NAME "manul")

set(LIBMANUL_WITH_DX12 OFF)
set(LIBMANUL_WITH_VULKAN OFF)

if (WIN32)
    set(LIBMANUL_WITH_DX12 ON)
elseif (UNIX)
    set(LIBMANUL_WITH_VULKAN ON)
endif ()

if (LIBMANUL_WITH_VULKAN)
    find_package(Vulkan 1.3.230 REQUIRED)
    include_directories(${Vulkan_INCLUDE_DIRS})
endif ()

# Third-party deps
add_subdirectory("thirdparty/nvrhi")
add_subdirectory("thirdparty/yaml-cpp")
add_subdirectory("thirdparty/fmt")
add_subdirectory("thirdparty/entt")
add_subdirectory("thirdparty/fsr2")

add_subdirectory("mashadercompiler")
add_subdirectory("shaders")

# Source code
file(GLOB_RECURSE src
        "renderer/*.cpp"
        "renderer/*.hpp"
        "renderer/*.h")

add_library(${LIBMANUL_NAME} ${src})

target_include_directories(${LIBMANUL_NAME} PUBLIC
        "renderer/include")

target_include_directories(${LIBMANUL_NAME} PRIVATE
        "renderer/source"
        "${GLOBAL_INCLUDE_DIR}"
        "${IMGUI_DIR}"
        "${GLM_INCLUDE_DIR}")

# Needs more refactoring to get rid of that unfortunately...
target_include_directories(${LIBMANUL_NAME} PRIVATE
        "${GLOBAL_ROOT_DIR}")

# DynObj requires MOVER.h
target_include_directories(${LIBMANUL_NAME} PRIVATE
        "${GLOBAL_ROOT_DIR}/McZapkie")

# OpenAL required by MOVER.h
target_include_directories(${LIBMANUL_NAME} PRIVATE
        "${OPENAL_INCLUDE_DIR}")

# GLuint, GLint typedef'd here
target_include_directories(${LIBMANUL_NAME} PRIVATE
        "${DEPS_DIR}/glad/include")

# Python.h
target_include_directories(${LIBMANUL_NAME} PRIVATE
        "${PYTHON_INCLUDE_DIR}")

target_compile_definitions(${LIBMANUL_NAME} PRIVATE
        GLM_ENABLE_EXPERIMENTAL
        GLM_FORCE_SWIZZLE)

target_compile_definitions(${LIBMANUL_NAME} PRIVATE
        _USE_MATH_DEFINES)

# windows headers included SOMEWHERE
target_compile_definitions(${LIBMANUL_NAME} PRIVATE
        NOMINMAX)

# to avoid global_settings changing size...
target_compile_definitions(${LIBMANUL_NAME} PRIVATE
        ${DEFINITIONS})

# we need to include libserialport stuff to include uart because of
# the setting class having mismatched size across TUs otherwise
target_include_directories(${LIBMANUL_NAME} PRIVATE
        ${libserialport_INCLUDE_DIR} ${LUAJIT_INCLUDE_DIR})

if (LIBMANUL_WITH_DX12)
    target_link_libraries(${LIBMANUL_NAME} nvrhi_d3d12 dxgi.lib)
    target_compile_definitions(${LIBMANUL_NAME} PUBLIC "LIBMANUL_WITH_D3D12=1" "GLFW_EXPOSE_NATIVE_WIN32")
else ()
    target_compile_definitions(${LIBMANUL_NAME} PUBLIC "LIBMANUL_WITH_D3D12=0")
endif ()

if (LIBMANUL_WITH_VULKAN)
    target_link_libraries(${LIBMANUL_NAME} nvrhi_vk Vulkan::Headers)
    target_compile_definitions(${LIBMANUL_NAME} PUBLIC "LIBMANUL_WITH_VULKAN=1" "GLFW_INCLUDE_VULKAN")
else ()
    target_compile_definitions(${LIBMANUL_NAME} PUBLIC "LIBMANUL_WITH_VULKAN=0")
endif ()

## For double-precision vector goodness
#if (WIN32)
#    target_compile_options(${LIBMANUL_NAME} PRIVATE "/arch:AVX2")
#elseif (UNIX)
#    target_compile_options(${LIBMANUL_NAME} PRIVATE "-mavx2")
#endif ()

target_sources(${LIBMANUL_NAME} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/eu07_source/register.cpp")

target_link_libraries(${LIBMANUL_NAME} ${LIBMANUL_NAME}_fsr2 nvrhi yaml-cpp fmt EnTT glfw)
add_dependencies(${LIBMANUL_NAME} ${LIBMANUL_NAME}_shaders)
