#include "nvrenderer/nvrenderer.h"

#include "fullscreenpass.h"

#include "nvrendererbackend.h"

FullScreenPass::FullScreenPass(NvRendererBackend* backend)
    : m_backend(backend) {}

nvrhi::IShader* FullScreenPass::GetVertexShader() {
  static nvrhi::ShaderHandle vertex_shader = nullptr;
  if (!vertex_shader)
    vertex_shader = m_backend->CreateShader("fx_vertex", nvrhi::ShaderType::Vertex);
  return vertex_shader;
}

nvrhi::IInputLayout* FullScreenPass::GetInputLayout() {
  static nvrhi::InputLayoutHandle input_layout = nullptr;
  if (!input_layout) {
    nvrhi::VertexAttributeDesc attribute_desc =
        nvrhi::VertexAttributeDesc()
            .setName("Position")
            .setElementStride(sizeof(glm::vec2))
            .setOffset(0)
            .setFormat(nvrhi::Format::RG32_FLOAT)
            .setBufferIndex(0)
            .setIsInstanced(false);
    input_layout = m_backend->GetDevice()->createInputLayout(&attribute_desc, 1,
                                                          GetVertexShader());
  }
  return input_layout;
}

nvrhi::IBuffer* FullScreenPass::GetVertexBuffer() {
  static nvrhi::BufferHandle vertex_buffer = nullptr;
  if (!vertex_buffer) {
    glm::vec2 vertices[]{{0.f, 0.f}, {1.f, 0.f}, {0.f, 1.f}, {1.f, 1.f}};
    vertex_buffer = m_backend->GetDevice()->createBuffer(
        nvrhi::BufferDesc()
            .setIsVertexBuffer(true)
            .setByteSize(sizeof(vertices))
            .setInitialState(nvrhi::ResourceStates::VertexBuffer)
            .setKeepInitialState(true)
            .setCpuAccess(nvrhi::CpuAccessMode::Write));
    void* map = m_backend->GetDevice()->mapBuffer(vertex_buffer,
                                               nvrhi::CpuAccessMode::Write);
    memcpy(map, vertices, sizeof(vertices));
    m_backend->GetDevice()->unmapBuffer(vertex_buffer);
  }
  return vertex_buffer;
}

void FullScreenPass::Init() { CreatePipeline(); }

void FullScreenPass::CreatePipeline() {
  nvrhi::GraphicsPipelineDesc pipeline_desc;
  CreatePipelineDesc(pipeline_desc);
  m_pipeline = m_backend->GetDevice()->createGraphicsPipeline(pipeline_desc,
                                                           GetFramebuffer());
}

void FullScreenPass::CreatePipelineDesc(
    nvrhi::GraphicsPipelineDesc& pipeline_desc) {
  pipeline_desc.setVertexShader(GetVertexShader())
      .setInputLayout(GetInputLayout())
      .setPrimType(nvrhi::PrimitiveType::TriangleStrip)
      .setRenderState(
          nvrhi::RenderState()
              .setBlendState(nvrhi::BlendState().setRenderTarget(
                  0, nvrhi::BlendState::RenderTarget().disableBlend()))
              .setRasterState(nvrhi::RasterState()
                                  .setCullNone()
                                  .disableScissor()
                                  .disableDepthClip()
                                  .setFillSolid())
              .setDepthStencilState(nvrhi::DepthStencilState()
                                        .disableDepthTest()
                                        .disableDepthWrite()
                                        .disableStencil()));
}

nvrhi::IFramebuffer* FullScreenPass::GetFramebuffer() { return m_backend->GetCurrentFramebuffer(); }

void FullScreenPass::InitState(nvrhi::GraphicsState& render_state) {
  nvrhi::IFramebuffer* framebuffer = GetFramebuffer();
  render_state.setFramebuffer(framebuffer);
  render_state.setViewport(nvrhi::ViewportState().addViewportAndScissorRect(
      framebuffer->getFramebufferInfo().getViewport()));
  render_state.setPipeline(m_pipeline);
  render_state.addVertexBuffer(
      nvrhi::VertexBufferBinding().setSlot(0).setBuffer(GetVertexBuffer()));
}

void FullScreenPass::Draw(nvrhi::ICommandList* command_list) {
  command_list->draw(nvrhi::DrawArguments().setVertexCount(4));
}
