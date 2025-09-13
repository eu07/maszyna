#pragma once

#include <nvrhi/nvrhi.h>

#include "nvrenderer/nvrenderer.h"

struct ShaderDefine {
  std::string m_name;
  std::string m_definition;
};

class NvRendererBackend {
 protected:
  friend class NvRenderer;
  NvRenderer *m_renderer;
  virtual std::string LoadShaderBlob(const std::string &fileName,
                                     const std::string &entryName) = 0;
  virtual bool BeginFrame() = 0;
  virtual void Present() = 0;
  virtual void UpdateWindowSize() = 0;
  virtual int GetCurrentBackBufferIndex() = 0;
  virtual int GetBackBufferCount() = 0;
  virtual nvrhi::TextureHandle GetBackBuffer(int index) = 0;
  class NvRendererMessageCallback *GetMessageCallback() const {
    return m_renderer->m_message_callback.get();
  }
  void AddMaterialShaderBlob(std::string_view material, std::string_view pass,
                             std::string blob, const std::string &entrypoint);
  void AddShaderBlob(std::string_view shader, std::string blob,
                     const std::string &entrypoint);
  virtual void LoadShaderBlobs() {}
  struct ShaderBlob {
    std::string m_entrypoint;
    std::string m_blob;
  };
  std::unordered_map<std::string, ShaderBlob> m_shader_blobs;
  std::unordered_map<
      std::string,
      std::array<ShaderBlob, NvRenderer::Constants::NumMaterialPasses()>>
      m_material_blobs;

 public:
  void Init();
  virtual bool IsHDRDisplay() const { return false; }
  // nvrhi::DeviceHandle m_device;
  virtual nvrhi::IDevice *GetDevice() { return nullptr; };
  std::vector<nvrhi::FramebufferHandle> m_framebuffers;
  nvrhi::FramebufferHandle GetCurrentFramebuffer();
  nvrhi::ShaderHandle CreateShader(const std::string &name,
                                   nvrhi::ShaderType type);
  nvrhi::ShaderHandle CreateMaterialShader(const std::string &name,
                                           NvRenderer::RenderPassType pass);
};

class NvRendererMessageCallback : public nvrhi::IMessageCallback {
  virtual void message(nvrhi::MessageSeverity severity,
                       const char *messageText) override;
};