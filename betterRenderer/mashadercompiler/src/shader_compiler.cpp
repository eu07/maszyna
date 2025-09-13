#include "shader_compiler.hpp"

#include <fstream>
#include <iostream>

#include "utils.hpp"

int MaShaderCompiler::Run() {
  ParseOptions();
  Init();
  m_shader_path = m_project_path.parent_path();
  YAML::Node src = YAML::LoadFile((m_project_path).generic_string());

  create_directories(m_output_path);

  {
    YAML::Node dest;
    if (!CompileProject(dest, src["shaders"], src["templates"],
                        ShaderPlatform::D3D12)) {
      return E_FAIL;
    }
    std::ofstream fout(m_output_path / "shaders_dxil.manul");
    fout << dest;
  }

  {
    YAML::Node dest;
    if (!CompileProject(dest, src["shaders"], src["templates"],
                        ShaderPlatform::Vulkan)) {
      return E_FAIL;
    }
    std::ofstream fout(m_output_path / "shaders_spirv.manul");
    fout << dest;
  }

  {
    std::ofstream fout(m_output_path / "project.manul");
    fout << src;
  }

  return S_OK;
}

void MaShaderCompiler::ParseOptions() {}

void MaShaderCompiler::Init() {
  const char *shader_path = getenv("shader_path");
  if (shader_path) {
    m_shader_path = shader_path;
    std::cout << "shader path: " << m_shader_path.generic_string() << std::endl;
  } else {
    m_shader_path = "";
    std::cout << "no custom shader path specified" << std::endl;
  }
  DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_dxc_utils));
  DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_dxc_compiler));
  m_dxc_utils->CreateDefaultIncludeHandler(&m_dxc_include_handler);
}

YAML::Binary MaShaderCompiler::CompileShaderToBlob(
    std::filesystem::path file_name, std::wstring entry_name,
    std::vector<std::tuple<std::wstring, std::wstring>> defines,
    std::string target, ShaderPlatform platform) {
  file_name.replace_extension(".hlsl");
  static std::unordered_map<std::string, std::wstring> targets{
      {"compute", L"cs_6_0"}, {"vertex", L"vs_6_0"},   {"hull", L"hs_6_0"},
      {"domain", L"ds_6_0"},  {"geometry", L"gs_6_0"}, {"pixel", L"ps_6_0"}};

  const wchar_t *target_str;
  if (auto target_found = targets.find(target); target_found != targets.end()) {
    target_str = target_found->second.c_str();
  } else {
    return {};
  }

  RefCountPtr<IDxcBlobEncoding> source_blob;
  if (FAILED(m_dxc_utils->LoadFile(file_name.generic_wstring().c_str(), nullptr,
                                   &source_blob))) {
    return {};
  }
  DxcBuffer Source;
  Source.Ptr = source_blob->GetBufferPointer();
  Source.Size = source_blob->GetBufferSize();
  Source.Encoding = DXC_CP_ACP;

  std::vector<std::wstring> include_paths{
      m_shader_path.generic_wstring(),
      file_name.parent_path().generic_wstring()};

  std::vector<const wchar_t *> args;

  args.emplace_back(L"-HV");
  args.emplace_back(L"2021");
  args.emplace_back(L"-Zpc");

  for (const auto &include_path : include_paths) {
    args.emplace_back(L"-I");
    args.emplace_back(include_path.c_str());
  }

  // if (m_generate_debug) {
  //     args.emplace_back(L"-Od");
  //     args.emplace_back(L"-Zi");
  // } else {
  //     //args.emplace_back(L"-O3");
  // }

  args.emplace_back(L"-Zi");
  args.emplace_back(L"-Qembed_debug");

  std::vector<std::wstring> reg_shifts;

  // Gather SPIRV register shifts once
  static const wchar_t *regShiftArgs[] = {
      L"-fvk-s-shift",
      L"-fvk-t-shift",
      L"-fvk-b-shift",
      L"-fvk-u-shift",
  };

  uint32_t regShifts[] = {128, 0, 256, 384};

  switch (platform) {
    case ShaderPlatform::D3D12:
      args.emplace_back(L"-Gis");
      break;
    case ShaderPlatform::Vulkan:
      args.emplace_back(L"-spirv");
      args.emplace_back(L"-fspv-target-env=vulkan1.2");
      args.emplace_back(L"-fvk-use-dx-layout");
      defines.emplace_back(L"SPIRV", L"1");

      for (uint32_t reg = 0; reg < 4; reg++) {
        for (uint32_t space = 0; space < 8; space++) {
          wchar_t buf[64];

          reg_shifts.emplace_back(regShiftArgs[reg]);

          swprintf(buf, std::size(buf), L"%u", regShifts[reg]);
          reg_shifts.emplace_back(buf);

          swprintf(buf, std::size(buf), L"%u", space);
          reg_shifts.emplace_back(buf);
        }
      }

      for (const std::wstring &arg : reg_shifts) {
        args.emplace_back(arg.c_str());
      }
      break;
  }

  std::vector<DxcDefine> dxc_defines{};
  dxc_defines.reserve(defines.size());
  for (const auto &[name, definition] : defines) {
    auto &define = dxc_defines.emplace_back();
    define.Name = name.c_str();
    define.Value = definition.c_str();
  }

  RefCountPtr<IDxcCompilerArgs> compiler_args;
  m_dxc_utils->BuildArguments(file_name.stem().generic_wstring().c_str(),
                              entry_name.c_str(), target_str, args.data(),
                              args.size(), dxc_defines.data(),
                              dxc_defines.size(), &compiler_args);

  std::vector<LPCWSTR> wargs(
      compiler_args->GetArguments(),
      compiler_args->GetArguments() + compiler_args->GetCount());

  RefCountPtr<IDxcResult> result;
  HRESULT hr = m_dxc_compiler->Compile(
      &Source,                        // Source buffer.
      compiler_args->GetArguments(),  // Array of pointers to arguments.
      compiler_args->GetCount(),      // Number of arguments.
      m_dxc_include_handler,          // User-provided interface to handle
      // #include
      // directives (optional).
      IID_PPV_ARGS(&result)  // Compiler output status, buffer, and errors.
  );

  RefCountPtr<IDxcBlobUtf8> errors;
  result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
  auto errors_length = errors->GetStringLength();

  if (errors && errors_length) {
    std::string message(errors->GetStringPointer());
    std::cout << "Shader compile log: " << file_name.generic_string()
              << std::endl;
    std::cout << message;
    // WriteLog("Shader compile log: " + fileName.generic_string());
    // WriteLog(message);
  }

  if (FAILED(hr)) {
    std::cout << "Shader compile failed: " << file_name.generic_string()
              << std::endl;
    return {};
  }

  RefCountPtr<IDxcBlob> binary_blob;
  result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&binary_blob), nullptr);

  auto buf_start =
      static_cast<unsigned char *>(binary_blob->GetBufferPointer());
  auto buf_end = buf_start + binary_blob->GetBufferSize();
  std::vector<unsigned char> buf{buf_start, buf_end};
  YAML::Binary binary{};
  binary.swap(buf);
  return binary;
}

bool MaShaderCompiler::CompileUtilityShader(YAML::Node &dest,
                                            const YAML::Node &src,
                                            const YAML::Node &templates,
                                            ShaderPlatform platform) {
  std::vector<std::tuple<std::wstring, std::wstring>> definitions{};
  for (const auto definition :
       TemplateOverride("definitions", src, templates)) {
    auto &[name, define] = definitions.emplace_back();
    name = ToWide(definition.first.as<std::string>());
    define = ToWide(definition.second.as<std::string>());
  }
  const std::filesystem::path file_name =
      m_shader_path /
      TemplateOverride("source", src, templates).as<std::string>();
  const auto entry_name =
      TemplateOverride("entrypoint", src, templates).as<std::string>();
  const auto target =
      TemplateOverride("target", src, templates).as<std::string>();
  const auto blob = CompileShaderToBlob(file_name, ToWide(entry_name),
                                        definitions, target, platform);

  if (!blob.size()) {
    return false;
  }

  dest["binary"] = blob;
  dest["entrypoint"] = entry_name;
  return true;
}

bool MaShaderCompiler::CompileMaterial(YAML::Node &dest, const YAML::Node &src,
                                       const YAML::Node &templates,
                                       ShaderPlatform platform) {
  std::vector<std::tuple<std::wstring, std::wstring>> definitions{};
  for (const auto definition :
       TemplateOverride("definitions", src, templates)) {
    auto &[name, define] = definitions.emplace_back();
    name = ToWide(definition.first.as<std::string>());
    define = ToWide(definition.second.as<std::string>());
  }
  const std::filesystem::path file_name =
      m_shader_path /
      TemplateOverride("source", src, templates).as<std::string>();
  const std::string entry_name = "main";
  const std::string target = "pixel";

  for (int i = 0; i < static_cast<int>(MaterialRenderPass::Count); ++i) {
    auto pass = static_cast<MaterialRenderPass>(i);
    const static std::unordered_map<MaterialRenderPass, std::string> pass_names{
        {MaterialRenderPass::Deferred, "deferred"},
        {MaterialRenderPass::Forward, "forward"},
        {MaterialRenderPass::CubeMap, "cubemap"},
    };
    static std::unordered_map<MaterialRenderPass, std::wstring>
        pass_definitions{
            {MaterialRenderPass::Deferred, L"PASS_GBUFFER"},
            {MaterialRenderPass::Forward, L"PASS_FORWARD"},
            {MaterialRenderPass::CubeMap, L"PASS_CUBEMAP"},
        };
    std::vector<std::tuple<std::wstring, std::wstring>> local_definitions{};
    local_definitions.emplace_back(L"PASS", pass_definitions.at(pass));
    local_definitions.insert(local_definitions.end(), definitions.begin(),
                             definitions.end());

    auto blob = CompileShaderToBlob(file_name, ToWide(entry_name),
                                    local_definitions, target, platform);

    if (!blob.size()) {
      return false;
    }

    dest[pass_names.at(pass)]["binary"] = blob;
    dest[pass_names.at(pass)]["entrypoint"] = entry_name;
  }
  return true;
}

bool MaShaderCompiler::CompileProject(YAML::Node &dest, const YAML::Node &src,
                                      const YAML::Node &templates,
                                      ShaderPlatform platform) {
  {
    YAML::Node dest_materials = dest["materials"];
    for (const auto material : src["materials"]) {
      YAML::Node dest_material;
      if (!CompileMaterial(dest_material, material.second, templates, platform))
        return false;
      dest_materials[material.first] = dest_material;
    }
  }
  {
    for (const auto material : src["utility"]) {
      YAML::Node dest_shader;
      if (!CompileUtilityShader(dest_shader, material.second, templates,
                                platform))
        return false;
      dest["utility"][material.first] = dest_shader;
    }
  }
  return true;
}