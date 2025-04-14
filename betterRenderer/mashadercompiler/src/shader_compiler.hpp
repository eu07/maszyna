#pragma once

#include <nvrhi/nvrhi.h>
#include <yaml-cpp/yaml.h>

#include <filesystem>

#ifndef _WIN32
#include <WinAdapter.h>
#else
#include <Windows.h>
#endif
#include <dxcapi.h>

template <typename T>
using RefCountPtr = nvrhi::RefCountPtr<T>;

enum class MaterialRenderPass : int { Deferred, Forward, CubeMap, Count };

enum class ShaderPlatform { D3D12, Vulkan };

class MaShaderCompiler {
 public:
  int Run();
  std::filesystem::path m_project_path;
  std::filesystem::path m_output_path;

 private:
  std::filesystem::path m_shader_path;
  bool m_generate_debug = false;
  void ParseOptions();
  void Init();
  YAML::Binary CompileShaderToBlob(
      std::filesystem::path file_name, std::wstring entry_name,
      std::vector<std::tuple<std::wstring, std::wstring>> defines,
      std::string target, ShaderPlatform platform);
  bool CompileUtilityShader(YAML::Node& dest, const YAML::Node& src,
                            const YAML::Node& templates,
                            ShaderPlatform platform);
  bool CompileMaterial(YAML::Node& dest, const YAML::Node& src,
                       const YAML::Node& templates, ShaderPlatform platform);
  bool CompileProject(YAML::Node& dest, const YAML::Node& src,
                      const YAML::Node& templates, ShaderPlatform platform);
  RefCountPtr<IDxcUtils> m_dxc_utils;
  RefCountPtr<IDxcCompiler3> m_dxc_compiler;
  RefCountPtr<IDxcIncludeHandler> m_dxc_include_handler;
};
