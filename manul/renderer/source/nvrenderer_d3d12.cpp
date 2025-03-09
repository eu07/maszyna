#include "nvrenderer_d3d12.h"

#include <fmt/format.h>

#include "Globals.h"
#include "Logs.h"
#include "config.h"
#include "nvrenderer/nvrenderer.h"

#if LIBMANUL_WITH_D3D12

#include <GLFW/glfw3native.h>
#include <nvrhi/validation.h>

#include "nvrendererbackend.h"

#define HR_RETURN(op)                                      \
  hr = (HRESULT)(op);                                      \
  if (FAILED(hr)) {                                        \
    ErrorLog(fmt::format(#op " returned HRESULT={}", hr)); \
    return false;                                          \
  }

bool NvRenderer::InitForD3D12(GLFWwindow *Window) {
  // Create d3d device
  std::shared_ptr<NvRendererBackend_D3D12> Backend =
      std::make_shared<NvRendererBackend_D3D12>();
  m_backend = Backend;

  Backend->m_renderer = this;
  Backend->m_hwnd = glfwGetWin32Window(Window);

  if (!Backend->CreateDevice()) {
    return false;
  }

  if (!Backend->CreateSwapChain(Window)) {
    return false;
  }

  if (!Backend->CreateRenderTarget()) {
    return false;
  }

  return true;
}

std::string NvRendererBackend_D3D12::LoadShaderBlob(
    const std::string &filename, const std::string &entrypoint) {
  std::string path = "shaders/shaders_dxil/" + filename;
  if (entrypoint != "main") path += "_" + entrypoint;
  path += ".bin";
  std::ifstream ifs(path, std::ios::binary | std::ios::ate);
  std::string blob(ifs.tellg(), '\0');
  ifs.seekg(0, std::ios_base::beg);
  ifs.read(blob.data(), blob.size());
  return blob;
}

int NvRendererBackend_D3D12::GetCurrentBackBufferIndex() {
  return m_swapchain->GetCurrentBackBufferIndex();
}

int NvRendererBackend_D3D12::GetBackBufferCount() {
  return m_swapchain_desc.BufferCount;
}

nvrhi::TextureHandle NvRendererBackend_D3D12::GetBackBuffer(int index) {
  return m_RhiSwapChainBuffers[index];
}

bool NvRendererBackend_D3D12::BeginFrame() {
  WaitForSingleObject(
      m_FrameFenceEvents[m_swapchain->GetCurrentBackBufferIndex()], INFINITE);
  return true;
}

bool NvRendererBackend_D3D12::SetHDRMetaData(float MaxOutputNits /*=1000.0f*/,
                                             float MinOutputNits /*=0.001f*/,
                                             float MaxCLL /*=2000.0f*/,
                                             float MaxFALL /*=500.0f*/) {
  HRESULT hr;
  struct DisplayChromaticities {
    float RedX;
    float RedY;
    float GreenX;
    float GreenY;
    float BlueX;
    float BlueY;
    float WhiteX;
    float WhiteY;
  };

  if (!m_swapchain) {
    return false;
  }

  auto hdr_config =
      NvRenderer::Config()->m_display.GetHDRConfig(IsHDRDisplay());

  // Clean the hdr metadata if the display doesn't support HDR
  // if (!m_hdrSupport) {
  //  ThrowIfFailed(
  //      m_swapChain->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_NONE, 0, nullptr));
  //  return;
  //}

  static const DisplayChromaticities DisplayChromaticityList[] = {
      {0.64000f, 0.33000f, 0.30000f, 0.60000f, 0.15000f, 0.06000f, 0.31270f,
       0.32900f},  // Display Gamut Rec709
      {0.70800f, 0.29200f, 0.17000f, 0.79700f, 0.13100f, 0.04600f, 0.31270f,
       0.32900f},  // Display Gamut Rec2020
  };

  // Select the chromaticity based on HDR format of the DWM.
  int selectedChroma = 1;
  // if (true//m_currentSwapChainBitDepth == _16 &&
  //     m_currentSwapChainColorSpace ==
  //     DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709) {
  //   selectedChroma = 0;
  // } else if (m_currentSwapChainBitDepth == _10 &&
  //            m_currentSwapChainColorSpace ==
  //                DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020) {
  //   selectedChroma = 1;
  // } else {
  //   // Reset the metadata since this is not a supported HDR format.
  //   HR_RETURN(
  //       m_swapChain->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_NONE, 0,
  //       nullptr));
  //   return;
  // }

  // Set HDR meta data
  const DisplayChromaticities &Chroma = DisplayChromaticityList[selectedChroma];
  DXGI_HDR_METADATA_HDR10 HDR10MetaData = {};
  HDR10MetaData.RedPrimary[0] = static_cast<UINT16>(Chroma.RedX * 50000.0f);
  HDR10MetaData.RedPrimary[1] = static_cast<UINT16>(Chroma.RedY * 50000.0f);
  HDR10MetaData.GreenPrimary[0] = static_cast<UINT16>(Chroma.GreenX * 50000.0f);
  HDR10MetaData.GreenPrimary[1] = static_cast<UINT16>(Chroma.GreenY * 50000.0f);
  HDR10MetaData.BluePrimary[0] = static_cast<UINT16>(Chroma.BlueX * 50000.0f);
  HDR10MetaData.BluePrimary[1] = static_cast<UINT16>(Chroma.BlueY * 50000.0f);
  HDR10MetaData.WhitePoint[0] = static_cast<UINT16>(Chroma.WhiteX * 50000.0f);
  HDR10MetaData.WhitePoint[1] = static_cast<UINT16>(Chroma.WhiteY * 50000.0f);
  HDR10MetaData.MaxMasteringLuminance =
      static_cast<UINT>(hdr_config.GetMaxOutputNits() * 10000.0f);
  HDR10MetaData.MinMasteringLuminance =
      static_cast<UINT>(hdr_config.GetMinOutputNits() * 10000.0f);
  HDR10MetaData.MaxContentLightLevel = static_cast<UINT16>(MaxCLL);
  HDR10MetaData.MaxFrameAverageLightLevel = static_cast<UINT16>(MaxFALL);
  if (IsHDRDisplay()) {
    HR_RETURN(m_swapchain->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_HDR10,
                                          sizeof(DXGI_HDR_METADATA_HDR10),
                                          &HDR10MetaData))
    HR_RETURN(
        m_swapchain->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020))
  } else {
    HR_RETURN(
        m_swapchain->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709))
  }

  return true;
}

void NvRendererBackend_D3D12::Present() {
  // if (!m_windowVisible)
  // 	return;

  auto bufferIndex = m_swapchain->GetCurrentBackBufferIndex();

  UINT presentFlags = 0;
  if (false /*!vsync_enabled*/ && !Global.bFullScreen && m_TearingSupported)
    presentFlags |= DXGI_PRESENT_ALLOW_TEARING;

  m_swapchain->Present(1, presentFlags);

  m_FrameFence->SetEventOnCompletion(m_FrameCount,
                                     m_FrameFenceEvents[bufferIndex]);
  m_graphics_queue->Signal(m_FrameFence, m_FrameCount);
  m_FrameCount++;
}

bool NvRendererBackend_D3D12::IsHDRDisplay() const {
  return m_output_desc.BitsPerColor > 8 &&
         !NvRenderer::Config()->m_display.m_disable_hdr;
}

bool NvRendererBackend_D3D12::CreateDevice() {
  HRESULT hr;
  HR_RETURN(
      CreateDXGIFactory2(Global.gfx_gldebug ? DXGI_CREATE_FACTORY_DEBUG : 0,
                         IID_PPV_ARGS(&m_dxgi_factory)))

  if (Global.gfx_gldebug) {
    nvrhi::RefCountPtr<ID3D12Debug> pDebug;
    HR_RETURN(D3D12GetDebugInterface(IID_PPV_ARGS(&pDebug)))

    pDebug->EnableDebugLayer();
  }

  WriteLog("Available graphics adapters:");
  for (int i = 0;; ++i) {
    nvrhi::RefCountPtr<IDXGIAdapter> adapter = nullptr;
    if (FAILED(m_dxgi_factory->EnumAdapters(i, &adapter))) break;
    DXGI_ADAPTER_DESC desc{};
    HR_RETURN(adapter->GetDesc(&desc));
    std::string str_desc{};
    str_desc.resize(
        wcstombs(nullptr, desc.Description, std::size(desc.Description)));
    wcstombs(str_desc.data(), desc.Description, str_desc.size());
    WriteLog(fmt::format("- adapter{}: {}", i, str_desc));
  }

  if (FAILED(m_dxgi_factory->EnumAdapters(
          NvRenderer::Config()->m_display.m_adapter_index, &m_dxgi_adapter))) {
    ErrorLog(fmt::format("Invalid adapter index: {}",
                         NvRenderer::Config()->m_display.m_adapter_index));
    return false;
  }

  nvrhi::RefCountPtr<IDXGIOutput> output;
  nvrhi::RefCountPtr<IDXGIOutput6> output6;
  if (FAILED(m_dxgi_adapter->EnumOutputs(
          NvRenderer::Config()->m_display.m_output_index, &output))) {
    ErrorLog(fmt::format("Invalid output index: {}",
                         NvRenderer::Config()->m_display.m_output_index));
  }

  HR_RETURN(output->QueryInterface<IDXGIOutput6>(&output6));
  HR_RETURN(output6->GetDesc1(&m_output_desc));

  // {
  // 	DXGI_ADAPTER_DESC aDesc;
  // 	m_DxgiAdapter->GetDesc(&aDesc);
  //
  // 	m_RendererString = GetAdapterName(aDesc);
  // 	m_IsNvidia = IsNvDeviceID(aDesc.VendorId);
  // }

  HR_RETURN(D3D12CreateDevice(m_dxgi_adapter, D3D_FEATURE_LEVEL_12_0,
                              IID_PPV_ARGS(&m_d3d_device)))

  if (Global.gfx_gldebug) {
    nvrhi::RefCountPtr<ID3D12InfoQueue> pInfoQueue;
    m_d3d_device->QueryInterface(&pInfoQueue);

    if (pInfoQueue) {
      pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
      pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);

      D3D12_MESSAGE_ID disableMessageIDs[] = {
          D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
          D3D12_MESSAGE_ID_COMMAND_LIST_STATIC_DESCRIPTOR_RESOURCE_DIMENSION_MISMATCH,  // descriptor validation doesn't understand acceleration structures
      };

      D3D12_INFO_QUEUE_FILTER filter = {};
      filter.DenyList.pIDList = disableMessageIDs;
      filter.DenyList.NumIDs =
          sizeof(disableMessageIDs) / sizeof(disableMessageIDs[0]);
      pInfoQueue->AddStorageFilterEntries(&filter);
    }
  }

  D3D12_COMMAND_QUEUE_DESC queueDesc;
  ZeroMemory(&queueDesc, sizeof(queueDesc));
  queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
  queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  queueDesc.NodeMask = 1;
  HR_RETURN(m_d3d_device->CreateCommandQueue(&queueDesc,
                                             IID_PPV_ARGS(&m_graphics_queue)))
  m_graphics_queue->SetName(L"Graphics Queue");

  if (true) {
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    HR_RETURN(m_d3d_device->CreateCommandQueue(&queueDesc,
                                               IID_PPV_ARGS(&m_compute_queue)))
    m_compute_queue->SetName(L"Compute Queue");
  }

  if (true) {
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    HR_RETURN(m_d3d_device->CreateCommandQueue(&queueDesc,
                                               IID_PPV_ARGS(&m_copy_queue)))
    m_copy_queue->SetName(L"Copy Queue");
  }

  nvrhi::d3d12::DeviceDesc deviceDesc;
  deviceDesc.errorCB = GetMessageCallback();
  deviceDesc.pDevice = m_d3d_device;
  deviceDesc.pGraphicsCommandQueue = m_graphics_queue;
  deviceDesc.pComputeCommandQueue = m_compute_queue;
  deviceDesc.pCopyCommandQueue = m_copy_queue;

  m_device = nvrhi::d3d12::createDevice(deviceDesc);

  if (Global.gfx_gldebug) {
    m_device = nvrhi::validation::createValidationLayer(m_device);
  }

  return true;
}

bool NvRendererBackend_D3D12::CreateSwapChain(GLFWwindow *window) {
  HRESULT hr;
  ZeroMemory(&m_swapchain_desc, sizeof(m_swapchain_desc));

  if (Global.bFullScreen) {
    const GLFWvidmode *mode = glfwGetVideoMode(glfwGetWindowMonitor(window));
    m_swapchain_desc.Width = mode->width;
    m_swapchain_desc.Height = mode->height;
  } else {
    int fbWidth = 0, fbHeight = 0;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    m_swapchain_desc.Width = fbWidth;
    m_swapchain_desc.Height = fbHeight;
  }

  m_swapchain_desc.BufferCount = 3;
  m_swapchain_desc.BufferUsage =
      DXGI_USAGE_SHADER_INPUT | DXGI_USAGE_RENDER_TARGET_OUTPUT;
  m_swapchain_desc.SampleDesc.Count = 1;
  m_swapchain_desc.SampleDesc.Quality = 0;
  m_swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  m_swapchain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
  m_swapchain_desc.Format = IsHDRDisplay() ? DXGI_FORMAT_R10G10B10A2_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM;

  nvrhi::RefCountPtr<IDXGIFactory5> pDxgiFactory5;
  if (SUCCEEDED(m_dxgi_factory->QueryInterface(IID_PPV_ARGS(&pDxgiFactory5)))) {
    BOOL supported = 0;
    if (SUCCEEDED(pDxgiFactory5->CheckFeatureSupport(
            DXGI_FEATURE_PRESENT_ALLOW_TEARING, &supported, sizeof(supported))))
      m_TearingSupported = (supported != 0);
  }

  if (m_TearingSupported) {
    m_swapchain_desc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
  }

  m_fullscreen_desc = {};
  m_fullscreen_desc.RefreshRate.Numerator = 0;
  m_fullscreen_desc.RefreshRate.Denominator = 1;
  m_fullscreen_desc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
  m_fullscreen_desc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
  m_fullscreen_desc.Windowed = !Global.bFullScreen;

  nvrhi::RefCountPtr<IDXGISwapChain1> pSwapChain1;
  HR_RETURN(m_dxgi_factory->CreateSwapChainForHwnd(
      m_graphics_queue, m_hwnd, &m_swapchain_desc, &m_fullscreen_desc, nullptr,
      &pSwapChain1))

  HR_RETURN(pSwapChain1->QueryInterface(IID_PPV_ARGS(&m_swapchain)))

  if (!CreateRenderTarget()) return false;

  HR_RETURN(m_d3d_device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                                      IID_PPV_ARGS(&m_FrameFence)));

  for (UINT bufferIndex = 0; bufferIndex < m_swapchain_desc.BufferCount;
       bufferIndex++) {
    m_FrameFenceEvents.push_back(CreateEvent(nullptr, false, true, nullptr));
  }
  if (!SetHDRMetaData()) return false;

  return true;
}

bool NvRendererBackend_D3D12::CreateRenderTarget() {
  m_SwapChainBuffers.resize(m_swapchain_desc.BufferCount);
  m_RhiSwapChainBuffers.resize(m_swapchain_desc.BufferCount);

  HRESULT hr;

  for (UINT n = 0; n < m_swapchain_desc.BufferCount; n++) {
    HR_RETURN(m_swapchain->GetBuffer(n, IID_PPV_ARGS(&m_SwapChainBuffers[n])))

    nvrhi::TextureDesc textureDesc;
    textureDesc.width = m_swapchain_desc.Width;
    textureDesc.height = m_swapchain_desc.Height;
    textureDesc.sampleCount = m_swapchain_desc.SampleDesc.Count;
    textureDesc.sampleQuality = m_swapchain_desc.SampleDesc.Quality;
    textureDesc.format = IsHDRDisplay() ? nvrhi::Format::R10G10B10A2_UNORM : nvrhi::Format::RGBA8_UNORM;
    textureDesc.debugName = "SwapChainBuffer";
    textureDesc.isRenderTarget = true;
    textureDesc.isUAV = false;
    textureDesc.initialState = nvrhi::ResourceStates::Present;
    textureDesc.keepInitialState = true;

    m_RhiSwapChainBuffers[n] = m_device->createHandleForNativeTexture(
        nvrhi::ObjectTypes::D3D12_Resource,
        nvrhi::Object(m_SwapChainBuffers[n]), textureDesc);
  }

  return true;
}

void NvRendererBackend_D3D12::ReleaseRenderTarget() {
  // Make sure that all frames have finished rendering
  m_device->waitForIdle();

  // Release all in-flight references to the render targets
  m_device->runGarbageCollection();

  // Set the events so that WaitForSingleObject in OneFrame will not hang later
  for (auto e : m_FrameFenceEvents) SetEvent(e);

  // Release the old buffers because ResizeBuffers requires that
  m_RhiSwapChainBuffers.clear();
  m_SwapChainBuffers.clear();
}

#endif