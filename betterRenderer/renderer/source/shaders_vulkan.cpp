#include <yaml-cpp/yaml.h>

#include "nvrenderer_vulkan.h"

#if LIBMANUL_WITH_VULKAN

void NvRendererBackend_Vulkan::LoadShaderBlobs() {
  auto root = YAML::LoadFile("shaders/shaders_spirv.manul");

  for (const auto entry : root["materials"]) {
    std::string_view material = entry.first.as<std::string_view>();
    for (const auto shader : entry.second) {
      auto binary = shader.second["binary"].as<YAML::Binary>();
      auto entrypoint = shader.second["entrypoint"].as<std::string>();
      AddMaterialShaderBlob(
          material, shader.first.as<std::string_view>(),
          std::string(binary.data(), binary.data() + binary.size()),
          entrypoint);
    }
  }

  for (const auto shader : root["utility"]) {
    auto binary = shader.second["binary"].as<YAML::Binary>();
    auto entrypoint = shader.second["entrypoint"].as<std::string>();
    AddShaderBlob(shader.first.as<std::string_view>(),
                  std::string(binary.data(), binary.data() + binary.size()),
                  entrypoint);
  }
}

#endif
