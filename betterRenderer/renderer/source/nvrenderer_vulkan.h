#pragma once

#include <nvrhi/nvrhi.h>

#include "nvrenderer/nvrenderer.h"
#include "nvrendererbackend.h"

#if LIBMANUL_WITH_VULKAN

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <nvrhi/vulkan.h>

#include <vulkan/vulkan.hpp>

class NvRendererBackend_Vulkan : public NvRendererBackend {
  friend class NvRenderer;

  nvrhi::IDevice *GetDevice() override;
  std::string LoadShaderBlob(const std::string &filename,
                             const std::string &entrypoint) override;
  virtual int GetCurrentBackBufferIndex() override;
  virtual int GetBackBufferCount() override;
  virtual nvrhi::TextureHandle GetBackBuffer(int index) override;
  virtual bool BeginFrame() override;
  virtual void Present() override;
  virtual void UpdateWindowSize() override;
  bool IsHDRDisplay() const override;

  // bool SetHDRMetaData(float MaxOutputNits = 1000.0f,
  //                     float MinOutputNits = 0.001f, float MaxCLL = 2000.0f,
  //                     float MaxFALL = 500.0f);

  //// Window handle
  // HWND m_hwnd;
  //
  //// Direct3D resources
  // nvrhi::RefCountPtr<ID3D12Device> m_d3d_device;
  // nvrhi::RefCountPtr<ID3D12CommandQueue> m_graphics_queue;
  // nvrhi::RefCountPtr<ID3D12CommandQueue> m_aux_graphics_queue;
  // nvrhi::RefCountPtr<ID3D12CommandQueue> m_compute_queue;
  // nvrhi::RefCountPtr<ID3D12CommandQueue> m_copy_queue;
  // nvrhi::RefCountPtr<IDXGIFactory2> m_dxgi_factory;
  // nvrhi::RefCountPtr<IDXGIAdapter> m_dxgi_adapter;
  // nvrhi::RefCountPtr<IDXGISwapChain4> m_swapchain;
  // DXGI_SWAP_CHAIN_DESC1 m_swapchain_desc;
  // DXGI_SWAP_CHAIN_FULLSCREEN_DESC m_fullscreen_desc;
  // DXGI_OUTPUT_DESC1 m_output_desc;

  // bool m_TearingSupported = false;
  //
  // std::vector<nvrhi::RefCountPtr<ID3D12Resource>> m_SwapChainBuffers;
  // std::vector<nvrhi::TextureHandle> m_RhiSwapChainBuffers;
  // nvrhi::RefCountPtr<ID3D12Fence> m_FrameFence;
  // std::vector<HANDLE> m_FrameFenceEvents;

  // UINT64 m_FrameCount = 1;

  // RHI resources

  // Utility functions
  // bool CreateDevice();
  // bool CreateSwapChain(GLFWwindow *window);
  // bool CreateRenderTarget();
  // void ReleaseRenderTarget();
  virtual void LoadShaderBlobs() override;

 public:
  bool CreateInstance();
  bool CreateDevice();
  bool CreateSwapChain();
  void ResizeSwapChain();
  [[nodiscard]] PFN_vkGetDeviceProcAddr GetDeviceProcAddr();

 private:
  int32_t m_BackBufferWidth;
  int32_t m_BackBufferHeight;
  bool m_VsyncEnabled;
  bool m_EnableNvrhiValidationLayer;
  bool m_EnableDebugRuntime = false;
  bool createInstance();
  bool createWindowSurface();
  void installDebugCallback();
  bool pickPhysicalDevice();
  bool findQueueFamilies(vk::PhysicalDevice physicalDevice);
  bool createDevice();
  bool createSwapChain();
  void destroySwapChain();

  struct VulkanExtensionSet {
    std::unordered_set<std::string> instance;
    std::unordered_set<std::string> layers;
    std::unordered_set<std::string> device;
  };

  // minimal set of required extensions
  VulkanExtensionSet enabledExtensions = {
      // instance
      {
          VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
          VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME,
      },
      // layers
      {},
      // device
      {
          VK_KHR_MAINTENANCE1_EXTENSION_NAME,
          VK_EXT_SAMPLER_FILTER_MINMAX_EXTENSION_NAME,
      },
  };

  // optional extensions
  VulkanExtensionSet optionalExtensions = {
      // instance
      {VK_EXT_DEBUG_UTILS_EXTENSION_NAME},
      // layers
      {},
      // device
      {VK_EXT_DEBUG_MARKER_EXTENSION_NAME,
       VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
       VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
       VK_NV_MESH_SHADER_EXTENSION_NAME,
       VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME,
       VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
       VK_KHR_MAINTENANCE_4_EXTENSION_NAME,
       VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME},
  };

  std::string m_RendererString;

  vk::DynamicLoader m_dynamicLoader;

  vk::Instance m_VulkanInstance;
  vk::DebugReportCallbackEXT m_DebugReportCallback;

  vk::SurfaceKHR m_WindowSurface;

  vk::PhysicalDevice m_VulkanPhysicalDevice;

  vk::Device m_VulkanDevice;
  vk::Queue m_GraphicsQueue;
  vk::Queue m_ComputeQueue;
  vk::Queue m_TransferQueue;
  vk::Queue m_PresentQueue;

  int m_GraphicsQueueFamily = -1;
  int m_ComputeQueueFamily = -1;
  int m_TransferQueueFamily = -1;
  int m_PresentQueueFamily = -1;

  int m_SwapChainBufferCount;

  vk::SurfaceFormatKHR m_SwapChainFormat;
  vk::SwapchainKHR m_SwapChain;

  struct SwapChainImage {
    vk::Image image;
    nvrhi::TextureHandle rhiHandle;
  };

  std::vector<SwapChainImage> m_SwapChainImages;
  uint32_t m_SwapChainIndex = static_cast<uint32_t>(-1);
  int m_MaxFramesInFlight = 2;

  nvrhi::vulkan::DeviceHandle m_NvrhiDevice;
  nvrhi::DeviceHandle m_ValidationLayer;

  nvrhi::CommandListHandle m_BarrierCommandList;
  std::vector<vk::Semaphore> m_PresentSemaphores;
  uint32_t m_PresentSemaphoreIndex = 0;

  std::queue<nvrhi::EventQueryHandle> m_FramesInFlight;
  std::vector<nvrhi::EventQueryHandle> m_QueryPool;

  GLFWwindow *m_Window;

  bool m_EnableComputeQueue;
  bool m_EnableCopyQueue;
  bool m_SwapChainMutableFormatSupported = false;
  bool m_BufferDeviceAddressSupported = false;
};

#endif