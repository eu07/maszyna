#include <iostream>

#include "shader_compiler.hpp"

int main(int argc, const char** argv) {
  if (argc != 4) {
    return -1;
  }
#ifdef WIN32
  std::cout << "DXIL path: " << argv[1] << std::endl;
  SetDllDirectoryA(argv[1]);
#endif
  setlocale(LC_ALL, ".UTF8");
  MaShaderCompiler compiler{};
  compiler.m_project_path = argv[2];
  compiler.m_output_path = argv[3];
  return compiler.Run();
}