#pragma once

#include <nvrhi/nvrhi.h>

#include "nvrenderer/nvrenderer.h"
#include "nvrendererbackend.h"

#if LIBMANUL_WITH_D3D12

#include <Windows.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <nvrhi/d3d12.h>

class NvRendererBackend_D3D12 : public NvRendererBackend {
  friend class NvRenderer;

  virtual std::string LoadShaderBlob(const std::string &filename,
                                     const std::string &entrypoint) override;
  virtual int GetCurrentBackBufferIndex() override;
  virtual int GetBackBufferCount() override;
  virtual nvrhi::TextureHandle GetBackBuffer(int index) override;
  virtual bool BeginFrame() override;
  virtual void Present() override;
  virtual bool IsHDRDisplay() const override;
  virtual void UpdateWindowSize() override{}

  bool SetHDRMetaData(float MaxOutputNits = 1000.0f,
                      float MinOutputNits = 0.001f, float MaxCLL = 2000.0f,
                      float MaxFALL = 500.0f);

  // Window handle
  HWND m_hwnd;

  // Direct3D resources
  nvrhi::RefCountPtr<ID3D12Device> m_d3d_device;
  nvrhi::RefCountPtr<ID3D12CommandQueue> m_graphics_queue;
  nvrhi::RefCountPtr<ID3D12CommandQueue> m_aux_graphics_queue;
  nvrhi::RefCountPtr<ID3D12CommandQueue> m_compute_queue;
  nvrhi::RefCountPtr<ID3D12CommandQueue> m_copy_queue;
  nvrhi::RefCountPtr<IDXGIFactory2> m_dxgi_factory;
  nvrhi::RefCountPtr<IDXGIAdapter> m_dxgi_adapter;
  nvrhi::RefCountPtr<IDXGISwapChain4> m_swapchain;
  DXGI_SWAP_CHAIN_DESC1 m_swapchain_desc;
  DXGI_SWAP_CHAIN_FULLSCREEN_DESC m_fullscreen_desc;
  DXGI_OUTPUT_DESC1 m_output_desc;

  bool m_TearingSupported = false;

  std::vector<nvrhi::RefCountPtr<ID3D12Resource>> m_SwapChainBuffers;
  std::vector<nvrhi::TextureHandle> m_RhiSwapChainBuffers;
  nvrhi::RefCountPtr<ID3D12Fence> m_FrameFence;
  std::vector<HANDLE> m_FrameFenceEvents;

  UINT64 m_FrameCount = 1;

  // RHI resources

  // Utility functions
  bool CreateDevice();
  bool CreateSwapChain(GLFWwindow *window);
  bool CreateRenderTarget();
  void ReleaseRenderTarget();
  virtual void LoadShaderBlobs() override;

  nvrhi::DeviceHandle m_device;

 public:
  nvrhi::IDevice *GetDevice() override { return m_device.Get(); }
};

#endif