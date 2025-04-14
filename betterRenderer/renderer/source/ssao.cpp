#include "ssao.h"

#include <nvrhi/nvrhi.h>
#include <nvrhi/utils.h>

#include <glm/glm.hpp>

#include "XeGTAO.h"
#include "gbuffer.h"
#include "nvrenderer/nvrenderer.h"
#include "nvrendererbackend.h"

NvSsao::NvSsao(NvRenderer* renderer)
    : m_backend(renderer->m_backend.get()),
      m_gbuffer(renderer->m_gbuffer.get()),
      m_width(0),
      m_height(0) {}

void NvSsao::Init() {
  using namespace nvrhi;
  m_CSPrefilterDepths16x16 = m_backend->CreateShader(
      "gtao_CSPrefilterDepths16x16", ShaderType::Compute);
  m_CSGTAOLow = m_backend->CreateShader("gtao_CSGTAOLow", ShaderType::Compute);
  m_CSGTAOMedium =
      m_backend->CreateShader("gtao_CSGTAOMedium", ShaderType::Compute);
  m_CSGTAOHigh =
      m_backend->CreateShader("gtao_CSGTAOHigh", ShaderType::Compute);
  m_CSGTAOUltra =
      m_backend->CreateShader("gtao_CSGTAOUltra", ShaderType::Compute);
  m_CSDenoisePass =
      m_backend->CreateShader("gtao_CSDenoisePass", ShaderType::Compute);
  m_CSDenoiseLastPass =
      m_backend->CreateShader("gtao_CSDenoiseLastPass", ShaderType::Compute);

  m_width = m_gbuffer->m_framebuffer->getFramebufferInfo().width;
  m_height = m_gbuffer->m_framebuffer->getFramebufferInfo().height;

  m_constantBuffer = m_backend->GetDevice()->createBuffer(
      nvrhi::utils::CreateStaticConstantBufferDesc(
          sizeof(XeGTAO::GTAOConstants), "XeGTAO Constants")
          .setInitialState(ResourceStates::ConstantBuffer)
          .setKeepInitialState(true));

  m_workingDepths = m_backend->GetDevice()->createTexture(
      nvrhi::TextureDesc()
          .setDebugName("XeGTAO Working Depths")
          .setWidth(m_width)
          .setHeight(m_height)
          .setIsUAV(true)
          .setFormat(Format::R32_FLOAT)
          .setMipLevels(XE_GTAO_DEPTH_MIP_LEVELS)
          .setInitialState(ResourceStates::ShaderResource)
          .setKeepInitialState(true));

  m_workingEdges = m_backend->GetDevice()->createTexture(
      nvrhi::TextureDesc()
          .setDebugName("XeGTAO Working Edges")
          .setWidth(m_width)
          .setHeight(m_height)
          .setIsUAV(true)
          .setFormat(Format::R8_UNORM)
          .setInitialState(ResourceStates::ShaderResource)
          .setKeepInitialState(true));

  m_debugImage = m_backend->GetDevice()->createTexture(
      nvrhi::TextureDesc()
          .setDebugName("XeGTAO Debug Image")
          .setWidth(m_width)
          .setHeight(m_height)
          .setIsUAV(true)
          .setFormat(Format::R11G11B10_FLOAT)
          .setInitialState(ResourceStates::ShaderResource)
          .setKeepInitialState(true));

  m_workingAOTerm = m_backend->GetDevice()->createTexture(
      nvrhi::TextureDesc()
          .setDebugName("XeGTAO Working AO Term A")
          .setWidth(m_width)
          .setHeight(m_height)
          .setIsUAV(true)
          .setFormat(Format::R8_UINT)
          .setInitialState(ResourceStates::ShaderResource)
          .setKeepInitialState(true));

  m_workingAOTermPong = m_backend->GetDevice()->createTexture(
      nvrhi::TextureDesc()
          .setDebugName("XeGTAO Working AO Term B")
          .setWidth(m_width)
          .setHeight(m_height)
          .setIsUAV(true)
          .setFormat(Format::R8_UINT)
          .setInitialState(ResourceStates::ShaderResource)
          .setKeepInitialState(true));

  m_outputAO = m_backend->GetDevice()->createTexture(
      nvrhi::TextureDesc()
          .setDebugName("XeGTAO Output AO")
          .setWidth(m_width)
          .setHeight(m_height)
          .setIsUAV(true)
          .setFormat(Format::R8_UNORM)
          .setIsTypeless(true)
          .setInitialState(ResourceStates::ShaderResource)
          .setKeepInitialState(true));

  SamplerHandle sampler_point = m_SamplerPoint = m_backend->GetDevice()->createSampler(
      nvrhi::SamplerDesc()
          .setAllAddressModes(SamplerAddressMode::Clamp)
          .setAllFilters(false));

  {
    BindingLayoutHandle blPrefilterDepths;
    BindingSetDesc bsDescPrefilter =
        BindingSetDesc()
            .addItem(BindingSetItem::ConstantBuffer(0, m_constantBuffer))
            .addItem(BindingSetItem::Texture_SRV(0, m_gbuffer->m_gbuffer_depth))
            .addItem(BindingSetItem::Sampler(0, sampler_point));
    for (int i = 0; i < XE_GTAO_DEPTH_MIP_LEVELS; ++i) {
      bsDescPrefilter.addItem(
          BindingSetItem::Texture_UAV(i, m_workingDepths, Format::UNKNOWN,
                                      TextureSubresourceSet(i, 1, 0, 1)));
    }
    utils::CreateBindingSetAndLayout(m_backend->GetDevice(),
                                     ShaderType::Compute, 0, bsDescPrefilter,
                                     blPrefilterDepths, m_BSPrefilterDepths);
    m_PSOPrefilterDepths = m_backend->GetDevice()->createComputePipeline(
        ComputePipelineDesc()
            .setComputeShader(m_CSPrefilterDepths16x16)
            .addBindingLayout(blPrefilterDepths));
  }

  {
    BindingLayoutHandle blGTAO;
    utils::CreateBindingSetAndLayout(
        m_backend->GetDevice(), ShaderType::Compute, 0,
        BindingSetDesc()
            .addItem(BindingSetItem::ConstantBuffer(0, m_constantBuffer))
            .addItem(BindingSetItem ::Texture_SRV(0, m_workingDepths))
            .addItem(
                BindingSetItem::Texture_SRV(1, m_gbuffer->m_gbuffer_normal))
            .addItem(BindingSetItem::Texture_UAV(0, m_workingAOTerm))
            .addItem(BindingSetItem::Texture_UAV(1, m_workingEdges))
            .addItem(BindingSetItem::Texture_UAV(2, m_debugImage))
            .addItem(BindingSetItem::Sampler(0, sampler_point)),
        blGTAO, m_BSGTAO);
    m_PSOGTAO = m_backend->GetDevice()->createComputePipeline(
        ComputePipelineDesc()
            .setComputeShader(m_CSGTAOHigh)
            .addBindingLayout(blGTAO));
  }

  {
    BindingLayoutHandle blDenoise;
    m_BLDenoise = m_backend->GetDevice()->createBindingLayout(
        BindingLayoutDesc()
            .setVisibility(nvrhi::ShaderType::Compute)
            .addItem(BindingLayoutItem::ConstantBuffer(0))
            .addItem(BindingLayoutItem::Texture_SRV(0))
            .addItem(BindingLayoutItem::Texture_SRV(1))
            .addItem(BindingLayoutItem::Texture_UAV(0))
            .addItem(BindingLayoutItem::Texture_UAV(1))
            .addItem(BindingLayoutItem::Texture_UAV(2))
            .addItem(BindingLayoutItem::Sampler(0)));
    utils::CreateBindingSetAndLayout(
        m_backend->GetDevice(), ShaderType::Compute, 0,
        BindingSetDesc()
            .addItem(BindingSetItem::ConstantBuffer(0, m_constantBuffer))
            .addItem(BindingSetItem::Texture_SRV(1, m_workingEdges))
            .addItem(BindingSetItem::Texture_UAV(2, m_debugImage))
            .addItem(BindingSetItem::Sampler(0, sampler_point)),
        blDenoise, m_BSDenoise);
    m_PSODenoise = m_backend->GetDevice()->createComputePipeline(
        ComputePipelineDesc()
            .setComputeShader(m_CSDenoisePass)
            //.addBindingLayout(blDenoise)
            .addBindingLayout(m_BLDenoise));
    m_PSODenoiseLastPass = m_backend->GetDevice()->createComputePipeline(
        ComputePipelineDesc()
            .setComputeShader(m_CSDenoiseLastPass)
            //.addBindingLayout(blDenoise)
            .addBindingLayout(m_BLDenoise));
  }
}

void NvSsao::Render(nvrhi::ICommandList* command_list,
                    const glm::mat4& projection, size_t frame_index) {
  command_list->beginMarker("XeGTAO");

  XeGTAO::GTAOConstants constants{};
  XeGTAO::GTAOSettings settings{};
  settings.QualityLevel = 2;
  settings.Radius = 10.f;
  settings.ThinOccluderCompensation = .7f;
  XeGTAO::GTAOUpdateConstants(constants, m_width, m_height, settings,
                              glm::value_ptr(projection), false, frame_index);
  command_list->writeBuffer(m_constantBuffer, &constants, sizeof(constants));

  {
    command_list->beginMarker("Prefilter Depth");
    nvrhi::ComputeState state;
    state.setPipeline(m_PSOPrefilterDepths);
    state.addBindingSet(m_BSPrefilterDepths);

    command_list->setComputeState(state);
    command_list->dispatch((m_width + 16 - 1) / 16, (m_height + 16 - 1) / 16,
                           1);
    command_list->endMarker();
  }

  {
    command_list->beginMarker("Main Pass");
    nvrhi::ComputeState state;
    state.setPipeline(m_PSOGTAO);
    state.addBindingSet(m_BSGTAO);

    command_list->setComputeState(state);
    command_list->dispatch(
        (m_width + XE_GTAO_NUMTHREADS_X - 1) / XE_GTAO_NUMTHREADS_X,
        (m_height + XE_GTAO_NUMTHREADS_Y - 1) / XE_GTAO_NUMTHREADS_Y, 1);
    command_list->endMarker();
  }

  {
    command_list->beginMarker("Denoise Pass");
    int num_passes = 4;
    nvrhi::ITexture* ping = m_workingAOTerm;
    nvrhi::ITexture* pong = m_workingAOTermPong;
    for (int i = 0; i < num_passes; ++i) {
      nvrhi::ComputeState state;
      bool last_pass = i == num_passes - 1;
      auto bindingSet = m_backend->GetDevice()->createBindingSet(
          nvrhi::BindingSetDesc()
              .addItem(
                  nvrhi::BindingSetItem::ConstantBuffer(0, m_constantBuffer))
              .addItem(nvrhi::BindingSetItem::Texture_SRV(0, ping))
              .addItem(nvrhi::BindingSetItem::Texture_SRV(
                  1, m_gbuffer->m_gbuffer_normal))
              .addItem(nvrhi::BindingSetItem::Texture_UAV(
                  0, last_pass ? m_outputAO.Get() : pong,
                  nvrhi::Format::R8_UINT))
              .addItem(nvrhi::BindingSetItem::Texture_UAV(1, m_workingEdges))
              .addItem(nvrhi::BindingSetItem::Texture_UAV(2, m_debugImage))
              .addItem(nvrhi::BindingSetItem::Sampler(0, m_SamplerPoint)),
          m_BLDenoise);
      state.setPipeline(last_pass ? m_PSODenoiseLastPass : m_PSODenoise);
      // state.addBindingSet(m_BSDenoise);
      state.addBindingSet(bindingSet);

      command_list->setComputeState(state);
      command_list->dispatch(
          (m_width + (XE_GTAO_NUMTHREADS_X * 2) - 1) /
              (XE_GTAO_NUMTHREADS_X * 2),
          (m_height + XE_GTAO_NUMTHREADS_Y - 1) / XE_GTAO_NUMTHREADS_Y, 1);
      std::swap(ping, pong);
    }
    command_list->endMarker();

    command_list->setTextureState(m_outputAO, nvrhi::AllSubresources,
                                  nvrhi::ResourceStates::ShaderResource);
  }
  command_list->endMarker();
}
