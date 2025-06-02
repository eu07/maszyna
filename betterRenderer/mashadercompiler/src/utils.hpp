#pragma once

#include <yaml-cpp/yaml.h>

#include <string>

inline std::wstring ToWide(const std::string& str) {
  std::wstring wstr{};
  wstr.resize(std::mbstowcs(nullptr, str.c_str(), static_cast<uint64_t>(-1)));
  std::mbstowcs(wstr.data(), str.c_str(), wstr.size());
  return wstr;
}

inline std::string ToNarrow(const std::wstring& wstr) {
  std::string str{};
  str.resize(std::wcstombs(nullptr, wstr.c_str(), static_cast<uint64_t>(-1)));
  std::wcstombs(str.data(), wstr.c_str(), str.size());
  return str;
}

template <typename KeyType>
YAML::Node TemplateOverride(const KeyType& key, const YAML::Node& container,
                            const YAML::Node& templates) {
  YAML::Node local = container[key];
  YAML::Node use_template = container["use_template"];
  if (!local.IsDefined() && use_template.IsDefined() &&
      templates[use_template.as<std::string_view>()].IsDefined()) {
    return templates[use_template.as<std::string_view>()][key];
  }
  return local;
}
