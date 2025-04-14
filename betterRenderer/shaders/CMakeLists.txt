
file(GLOB_RECURSE src "*.hlsl*" "*.h")
list(APPEND src "project.manul")

get_target_property(dxil_path MaSzynaShaderCompiler dxil_path)

add_custom_target(
        ${LIBMANUL_NAME}_shaders
        MaSzynaShaderCompiler "${dxil_path}" "${CMAKE_CURRENT_SOURCE_DIR}/project.manul" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
        DEPENDS ${src})
