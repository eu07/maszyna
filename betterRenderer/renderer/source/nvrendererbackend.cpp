#include "nvrendererbackend.h"

#include "Logs.h"
#include <fmt/format.h>

void NvRendererMessageCallback::message(nvrhi::MessageSeverity severity,
                                        const char *messageText) {
  switch (severity) {
    case nvrhi::MessageSeverity::Info:
    case nvrhi::MessageSeverity::Warning:
      WriteLog(messageText);
      break;
    case nvrhi::MessageSeverity::Error:
      ErrorLog(messageText);
      //__debugbreak();
      break;
    case nvrhi::MessageSeverity::Fatal:
      ErrorLog(messageText);
      //__debugbreak();
      break;
  }
}

void NvRendererBackend::Init() {
  LoadShaderBlobs();
  for (int i = 0; i < GetBackBufferCount(); ++i) {
    m_framebuffers.emplace_back(GetDevice()->createFramebuffer(
        nvrhi::FramebufferDesc().addColorAttachment(
            nvrhi::FramebufferAttachment().setTexture(GetBackBuffer(i)))));
  }
}

nvrhi::FramebufferHandle NvRendererBackend::GetCurrentFramebuffer() {
  return m_framebuffers[GetCurrentBackBufferIndex()];
}

nvrhi::ShaderHandle NvRendererBackend::CreateShader(const std::string &name,
                                                    nvrhi::ShaderType type) {
  if (auto blob_found = m_shader_blobs.find(name);
      blob_found != m_shader_blobs.end()) {
    const auto &blob = blob_found->second;
    nvrhi::ShaderDesc shader_desc{};
    shader_desc.shaderType = type;
    shader_desc.debugName = name;
    shader_desc.entryName = blob.m_entrypoint;
    return GetDevice()->createShader(shader_desc, blob.m_blob.data(),
                                     blob.m_blob.size());
  }
  return nullptr;
}

nvrhi::ShaderHandle NvRendererBackend::CreateMaterialShader(
    const std::string &name, NvRenderer::RenderPassType pass) {
  if (auto blob_found = m_material_blobs.find(name);
      blob_found != m_material_blobs.end()) {
    const auto &blob = blob_found->second[static_cast<size_t>(pass)];
    WriteLog(name);
    WriteLog(blob.m_entrypoint);
    WriteLog(fmt::format("{:d}", static_cast<int>(pass)));
    if (blob.m_blob.empty()) return nullptr;
    nvrhi::ShaderDesc shader_desc{};
    shader_desc.shaderType = nvrhi::ShaderType::Pixel;
    shader_desc.debugName = name;
    shader_desc.entryName = blob.m_entrypoint;
    return GetDevice()->createShader(shader_desc, blob.m_blob.data(),
                                     blob.m_blob.size());
  }
  return nullptr;
}

void NvRendererBackend::AddMaterialShaderBlob(std::string_view material,
                                              std::string_view pass,
                                              std::string blob,
                                              const std::string &entrypoint) {
  const static std::unordered_map<std::string_view, NvRenderer::RenderPassType>
      material_passes{{"deferred", NvRenderer::RenderPassType::Deferred},
                      {"forward", NvRenderer::RenderPassType::Forward},
                      {"cubemap", NvRenderer::RenderPassType::CubeMap}};
  m_material_blobs[std::string(material)][static_cast<size_t>(
      material_passes.at(pass))] = {entrypoint, blob};
}

void NvRendererBackend::AddShaderBlob(std::string_view shader, std::string blob,
                                      const std::string &entrypoint) {
  m_shader_blobs[std::string(shader)] = {entrypoint, blob};
}
