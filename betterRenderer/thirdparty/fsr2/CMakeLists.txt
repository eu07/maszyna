
set(fsr2_src_dir "${CMAKE_CURRENT_SOURCE_DIR}/src/ffx-fsr2-api")
set(fsr2_shader_dir "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders")
set(fsr2_include_dir "${CMAKE_CURRENT_SOURCE_DIR}/include")

file(GLOB src "${fsr2_src_dir}/*.cpp" "${fsr2_src_dir}/*.h")
file(GLOB_RECURSE src_vk "${fsr2_src_dir}/vk/*.cpp" "${fsr2_src_dir}/vk/*.h")
file(GLOB_RECURSE src_dx12 "${fsr2_src_dir}/dx12/*.cpp" "${fsr2_src_dir}/dx12/*.h")

add_library("${LIBMANUL_NAME}_fsr2" STATIC ${src})

if (LIBMANUL_WITH_VULKAN)
    target_sources("${LIBMANUL_NAME}_fsr2" PRIVATE ${src_vk})
    target_link_libraries("${LIBMANUL_NAME}_fsr2" PRIVATE Vulkan::Headers Vulkan::Vulkan)
endif ()

if (LIBMANUL_WITH_DX12)
    target_sources("${LIBMANUL_NAME}_fsr2" PRIVATE ${src_dx12})
endif ()

target_compile_definitions("${LIBMANUL_NAME}_fsr2" PUBLIC FFX_GCC)

target_include_directories("${LIBMANUL_NAME}_fsr2" PRIVATE ${fsr2_src_dir} ${fsr2_shader_dir} ${fsr2_include_dir})
target_include_directories("${LIBMANUL_NAME}_fsr2" INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src")
