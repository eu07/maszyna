#include <yaml-cpp/yaml.h>

#include "nvrenderer_d3d12.h"

#if LIBMANUL_WITH_D3D12

void NvRendererBackend_D3D12::LoadShaderBlobs() {
  auto root = YAML::LoadFile("shaders/shaders_dxil.manul");

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
