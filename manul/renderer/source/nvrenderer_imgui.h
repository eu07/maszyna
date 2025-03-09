#pragma once

#include "imgui_nvrhi.h"
#include "nvrenderer/nvrenderer.h"

class NvImguiRenderer : public imgui_renderer {
  virtual bool Init() override;
  virtual void Shutdown() override;
  virtual void BeginFrame() override;
  virtual void Render() override;

  std::shared_ptr<class NvRendererBackend> m_backend;
  ImGui_NVRHI m_rhi;

 public:
  NvImguiRenderer(class NvRenderer *renderer);
};