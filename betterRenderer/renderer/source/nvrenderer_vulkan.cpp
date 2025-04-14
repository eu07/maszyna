#include "nvrenderer_vulkan.h"

#include <GLFW/glfw3.h>
#include <fmt/format.h>

#include "Globals.h"
#include "Logs.h"
#include "config.h"
#include "nvrenderer/nvrenderer.h"

#if LIBMANUL_WITH_VULKAN

#include <nvrhi/validation.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#include "nvrendererbackend.h"

#define MA_ERROR_LOG(fmt_str, ...) ErrorLog(fmt::format(fmt_str, __VA_ARGS__))
#define MA_INFO_LOG(fmt_str, ...) WriteLog(fmt::format(fmt_str, __VA_ARGS__))
#define CHECK(...)                                                \
  if (!(__VA_ARGS__)) {                                           \
    MA_ERROR_LOG("Initialization failure at {:s}", #__VA_ARGS__); \
    return false;                                                 \
  }

namespace {
std::vector<const char*> stringSetToVector(
    const std::unordered_set<std::string>& set) {
  std::vector<const char*> ret;
  for (const auto& s : set) {
    ret.push_back(s.c_str());
  }

  return ret;
}

template <typename T>
std::vector<T> setToVector(const std::unordered_set<T>& set) {
  std::vector<T> ret;
  for (const auto& s : set) {
    ret.push_back(s);
  }

  return ret;
}

VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugCallback(
    VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType,
    uint64_t obj, size_t location, int32_t code, const char* layerPrefix,
    // ReSharper disable once CppParameterMayBeConstPtrOrRef
    const char* msg, void* userData) {
  // auto manager = static_cast<const NvRendererBackend_Vulkan*>(userData);

  // if (manager) {
  //   const auto& ignored =
  //       manager->m_DeviceParams.ignoredVulkanValidationMessageLocations;
  //   const auto found = std::find(ignored.begin(), ignored.end(), location);
  //   if (found != ignored.end()) return VK_FALSE;
  // }
  if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
    MA_ERROR_LOG("[Vulkan: location={:#x} code={:d}, layerPrefix='{:s}'] {:s}",
                 location, code, layerPrefix, msg);
    // throw std::runtime_error("");
  } else {
    MA_INFO_LOG("[Vulkan: location={:#x} code={:d}, layerPrefix='{:s}'] {:s}",
                location, code, layerPrefix, msg);
  }

  return VK_FALSE;
}
}  // namespace

bool NvRenderer::InitForVulkan(GLFWwindow* Window) {
  // Create d3d device
  const auto Backend = std::make_shared<NvRendererBackend_Vulkan>();
  m_backend = Backend;

  Backend->m_Window = Window;
  Backend->m_renderer = this;

  Backend->m_BackBufferWidth = Global.window_size.x;
  Backend->m_BackBufferHeight = Global.window_size.y;
  //glfwGetWindowSize(Window, &Backend->m_BackBufferWidth,
  //                       &Backend->m_BackBufferHeight);
  Backend->m_EnableComputeQueue = true;
  Backend->m_EnableCopyQueue = true;
  Backend->m_EnableNvrhiValidationLayer = false;
  Backend->m_VsyncEnabled = Global.VSync;
  Backend->m_SwapChainBufferCount = 3;
  Backend->m_EnableDebugRuntime = Global.gfx_gldebug;

  if (!Backend->CreateInstance()) return false;
  if (!Backend->CreateDevice()) return false;
  if (!Backend->CreateSwapChain()) return false;

  Backend->m_BackBufferWidth = 0;
  Backend->m_BackBufferHeight = 0;

  Backend->UpdateWindowSize();
  // if (!Backend->CreateDevice()) {
  //   return false;
  // }
  //
  // if (!Backend->CreateSwapChain(Window)) {
  //  return false;
  //}
  //
  // if (!Backend->CreateRenderTarget()) {
  //  return false;
  //}

  return true;
}

nvrhi::IDevice* NvRendererBackend_Vulkan::GetDevice() {
  return m_EnableNvrhiValidationLayer ? m_ValidationLayer : m_NvrhiDevice;
}

std::string NvRendererBackend_Vulkan::LoadShaderBlob(
    const std::string& filename, const std::string& entrypoint) {
  return {};
}

int NvRendererBackend_Vulkan::GetCurrentBackBufferIndex() {
  return m_SwapChainIndex;
}

int NvRendererBackend_Vulkan::GetBackBufferCount() {
  return static_cast<uint32_t>(m_SwapChainImages.size());
}

nvrhi::TextureHandle NvRendererBackend_Vulkan::GetBackBuffer(int index) {
  return m_SwapChainImages[index].rhiHandle;
}

bool NvRendererBackend_Vulkan::BeginFrame() {
  const auto& semaphore = m_PresentSemaphores[m_PresentSemaphoreIndex];

  vk::Result res = vk::Result::eErrorUnknown;

  constexpr int maxAttempts = 3;
  for (int attempt = 0; attempt < maxAttempts; ++attempt) {
    res = m_VulkanDevice.acquireNextImageKHR(
        m_SwapChain,
        std::numeric_limits<uint64_t>::max(),  // timeout
        semaphore, vk::Fence(), &m_SwapChainIndex);

    if (res == vk::Result::eErrorOutOfDateKHR) {
      // BackBufferResizing();
      auto surfaceCaps =
          m_VulkanPhysicalDevice.getSurfaceCapabilitiesKHR(m_WindowSurface);

      m_BackBufferWidth = static_cast<int>(surfaceCaps.currentExtent.width);
      m_BackBufferHeight = static_cast<int>(surfaceCaps.currentExtent.height);

      ResizeSwapChain();
      // BackBufferResized();
    } else
      break;
  }

  if (res == vk::Result::eSuccess) {
    m_NvrhiDevice->queueWaitForSemaphore(nvrhi::CommandQueue::Graphics,
                                         semaphore, 0);
    return true;
  }

  return false;
}
void NvRendererBackend_Vulkan::Present() {
  const auto& semaphore = m_PresentSemaphores[m_PresentSemaphoreIndex];

  m_NvrhiDevice->queueSignalSemaphore(nvrhi::CommandQueue::Graphics, semaphore,
                                      0);

  m_BarrierCommandList->open();  // umm...
  m_BarrierCommandList->close();
  m_NvrhiDevice->executeCommandList(m_BarrierCommandList);

  const vk::PresentInfoKHR info = vk::PresentInfoKHR()
                                      .setWaitSemaphoreCount(1)
                                      .setPWaitSemaphores(&semaphore)
                                      .setSwapchainCount(1)
                                      .setPSwapchains(&m_SwapChain)
                                      .setPImageIndices(&m_SwapChainIndex);

  const vk::Result res = m_PresentQueue.presentKHR(&info);
  assert(res == vk::Result::eSuccess || res == vk::Result::eErrorOutOfDateKHR);

  m_PresentSemaphoreIndex =
      (m_PresentSemaphoreIndex + 1) % m_PresentSemaphores.size();

#ifndef _WIN32
  if (m_VsyncEnabled) {
    m_PresentQueue.waitIdle();
  }
#endif

  while (m_FramesInFlight.size() >= m_MaxFramesInFlight) {
    auto query = m_FramesInFlight.front();
    m_FramesInFlight.pop();

    m_NvrhiDevice->waitEventQuery(query);

    m_QueryPool.push_back(query);
  }

  nvrhi::EventQueryHandle query;
  if (!m_QueryPool.empty()) {
    query = m_QueryPool.back();
    m_QueryPool.pop_back();
  } else {
    query = m_NvrhiDevice->createEventQuery();
  }

  m_NvrhiDevice->resetEventQuery(query);
  m_NvrhiDevice->setEventQuery(query, nvrhi::CommandQueue::Graphics);
  m_FramesInFlight.push(query);
}

bool NvRendererBackend_Vulkan::IsHDRDisplay() const { return false; }

bool NvRendererBackend_Vulkan::CreateInstance() {
  if (m_EnableDebugRuntime) {
    enabledExtensions.instance.insert("VK_EXT_debug_report");
    enabledExtensions.layers.insert("VK_LAYER_KHRONOS_validation");
  }

  const auto vkGetInstanceProcAddr =
      m_dynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>(
          "vkGetInstanceProcAddr");
  VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
  return createInstance();
}

PFN_vkGetDeviceProcAddr NvRendererBackend_Vulkan::GetDeviceProcAddr() {
  return m_dynamicLoader.getProcAddress<PFN_vkGetDeviceProcAddr>(
      "vkGetDeviceProcAddr");
}

bool NvRendererBackend_Vulkan::CreateDevice() {
  if (m_EnableDebugRuntime) {
    installDebugCallback();
  }

  std::vector<std::string> requiredVulkanDeviceExtensions{
      VK_EXT_SAMPLER_FILTER_MINMAX_EXTENSION_NAME};

  // add device extensions requested by the user
  for (const std::string& name : requiredVulkanDeviceExtensions) {
    // enabledExtensions.device.insert(name);
  }
  // for (const std::string& name :
  //     m_DeviceParams.optionalVulkanDeviceExtensions) {
  //  optionalExtensions.device.insert(name);
  //}

  // if (!m_DeviceParams.headlessDevice) {
  //  Need to adjust the swap chain format before creating the device because
  //  it affects physical device selection
  // if (m_DeviceParams.swapChainFormat == nvrhi::Format::SRGBA8_UNORM)
  //  m_DeviceParams.swapChainFormat = nvrhi::Format::SBGRA8_UNORM;
  // else if (m_DeviceParams.swapChainFormat == nvrhi::Format::RGBA8_UNORM)
  //  m_DeviceParams.swapChainFormat = nvrhi::Format::BGRA8_UNORM;

  CHECK(createWindowSurface())
  //}
  CHECK(pickPhysicalDevice())
  CHECK(findQueueFamilies(m_VulkanPhysicalDevice))
  CHECK(createDevice())

  auto vecInstanceExt = stringSetToVector(enabledExtensions.instance);
  auto vecLayers = stringSetToVector(enabledExtensions.layers);
  auto vecDeviceExt = stringSetToVector(enabledExtensions.device);

  nvrhi::vulkan::DeviceDesc deviceDesc;
  deviceDesc.errorCB = GetMessageCallback();
  deviceDesc.instance = m_VulkanInstance;
  deviceDesc.physicalDevice = m_VulkanPhysicalDevice;
  deviceDesc.device = m_VulkanDevice;
  deviceDesc.graphicsQueue = m_GraphicsQueue;
  deviceDesc.graphicsQueueIndex = m_GraphicsQueueFamily;
  if (m_EnableComputeQueue) {
    deviceDesc.computeQueue = m_ComputeQueue;
    deviceDesc.computeQueueIndex = m_ComputeQueueFamily;
  }
  if (m_EnableCopyQueue) {
    deviceDesc.transferQueue = m_TransferQueue;
    deviceDesc.transferQueueIndex = m_TransferQueueFamily;
  }
  deviceDesc.instanceExtensions = vecInstanceExt.data();
  deviceDesc.numInstanceExtensions = vecInstanceExt.size();
  deviceDesc.deviceExtensions = vecDeviceExt.data();
  deviceDesc.numDeviceExtensions = vecDeviceExt.size();
  deviceDesc.bufferDeviceAddressSupported = m_BufferDeviceAddressSupported;

  m_NvrhiDevice = nvrhi::vulkan::createDevice(deviceDesc);

  if (m_EnableNvrhiValidationLayer) {
    m_ValidationLayer = nvrhi::validation::createValidationLayer(m_NvrhiDevice);
  }

  return true;
}

bool NvRendererBackend_Vulkan::CreateSwapChain() {
  CHECK(createSwapChain())

  m_BarrierCommandList = m_NvrhiDevice->createCommandList();

  m_PresentSemaphores.reserve(m_MaxFramesInFlight + 1);
  for (uint32_t i = 0; i < m_MaxFramesInFlight + 1; ++i) {
    m_PresentSemaphores.push_back(
        m_VulkanDevice.createSemaphore(vk::SemaphoreCreateInfo()));
  }

  return true;
}
void NvRendererBackend_Vulkan::ResizeSwapChain() {
  if (m_VulkanDevice) {
    destroySwapChain();
    createSwapChain();
  }
}

void NvRendererBackend_Vulkan::UpdateWindowSize() {
  int width;
  int height;
  glfwGetWindowSize(m_Window, &width, &height);

  // if (width == 0 || height == 0) {
  //   // window is minimized
  //   m_windowVisible = false;
  //   return;
  // }
  //
  // m_windowVisible = true;

  if (m_BackBufferWidth != width || m_BackBufferHeight != height) {
    // window is not minimized, and the size has changed

    // BackBufferResizing();

    m_BackBufferWidth = width;
    m_BackBufferHeight = height;
    // m_DeviceParams.vsyncEnabled = m_RequestedVSync;

    ResizeSwapChain();

    // BackBufferResized();
  }

  // m_DeviceParams.vsyncEnabled = m_RequestedVSync;
}

bool NvRendererBackend_Vulkan::createInstance() {
  if (!glfwVulkanSupported()) {
    MA_ERROR_LOG(
        "{:s}",
        "GLFW reports that Vulkan is not supported. Perhaps missing a call to "
        "glfwInit()?");
    return false;
  }

  // add any extensions required by GLFW
  uint32_t glfwExtCount;
  const char** glfwExt = glfwGetRequiredInstanceExtensions(&glfwExtCount);
  assert(glfwExt);

  for (uint32_t i = 0; i < glfwExtCount; i++) {
    enabledExtensions.instance.insert(std::string(glfwExt[i]));
  }

  // add instance extensions requested by the user
  // for (const std::string& name :
  //     m_DeviceParams.requiredVulkanInstanceExtensions) {
  //  enabledExtensions.instance.insert(name);
  //}
  // for (const std::string& name :
  //     m_DeviceParams.optionalVulkanInstanceExtensions) {
  //  optionalExtensions.instance.insert(name);
  //}

  // add layers requested by the user
  // for (const std::string& name : m_DeviceParams.requiredVulkanLayers) {
  //  enabledExtensions.layers.insert(name);
  //}
  // for (const std::string& name : m_DeviceParams.optionalVulkanLayers) {
  //  optionalExtensions.layers.insert(name);
  //}

  std::unordered_set<std::string> requiredExtensions =
      enabledExtensions.instance;

  // figure out which optional extensions are supported
  for (const auto& instanceExt : vk::enumerateInstanceExtensionProperties()) {
    const std::string name = instanceExt.extensionName;
    if (optionalExtensions.instance.find(name) !=
        optionalExtensions.instance.end()) {
      enabledExtensions.instance.insert(name);
    }

    requiredExtensions.erase(name);
  }

  if (!requiredExtensions.empty()) {
    std::stringstream ss;
    ss << "Cannot create a Vulkan instance because the following required "
          "extension(s) are not supported:";
    for (const auto& ext : requiredExtensions) ss << std::endl << "  - " << ext;

    MA_ERROR_LOG("{:s}", ss.str().c_str());
    return false;
  }

  MA_INFO_LOG("{:s}", "Enabled Vulkan instance extensions:");
  for (const auto& ext : enabledExtensions.instance) {
    MA_INFO_LOG("    {:s}", ext.c_str());
  }

  std::unordered_set<std::string> requiredLayers = enabledExtensions.layers;

  for (const auto& layer : vk::enumerateInstanceLayerProperties()) {
    const std::string name = layer.layerName;
    if (optionalExtensions.layers.find(name) !=
        optionalExtensions.layers.end()) {
      enabledExtensions.layers.insert(name);
    }

    requiredLayers.erase(name);
  }

  if (!requiredLayers.empty()) {
    std::stringstream ss;
    ss << "Cannot create a Vulkan instance because the following required "
          "layer(s) are not supported:";
    for (const auto& ext : requiredLayers) ss << std::endl << "  - " << ext;

    MA_ERROR_LOG("{:s}", ss.str().c_str());
    return false;
  }

  MA_INFO_LOG("{:s}", "Enabled Vulkan layers:");
  for (const auto& layer : enabledExtensions.layers) {
    MA_INFO_LOG("    {:s}", layer.c_str());
  }

  auto instanceExtVec = stringSetToVector(enabledExtensions.instance);
  auto layerVec = stringSetToVector(enabledExtensions.layers);

  auto applicationInfo = vk::ApplicationInfo();

  // Query the Vulkan API version supported on the system to make sure we use at
  // least 1.3 when that's present.
  vk::Result res = vk::enumerateInstanceVersion(&applicationInfo.apiVersion);

  if (res != vk::Result::eSuccess) {
    MA_ERROR_LOG("Call to vkEnumerateInstanceVersion failed, error code = {:s}",
                 nvrhi::vulkan::resultToString(static_cast<VkResult>(res)));
    return false;
  }

  constexpr uint32_t minimumVulkanVersion = VK_MAKE_API_VERSION(0, 1, 3, 0);

  // Check if the Vulkan API version is sufficient.
  if (applicationInfo.apiVersion < minimumVulkanVersion) {
    MA_ERROR_LOG(
        "The Vulkan API version supported on the system ({:d}.{:d}.{:d}) is "
        "too low, "
        "at least {:d}.{:d}.{:d} is required.",
        VK_API_VERSION_MAJOR(applicationInfo.apiVersion),
        VK_API_VERSION_MINOR(applicationInfo.apiVersion),
        VK_API_VERSION_PATCH(applicationInfo.apiVersion),
        VK_API_VERSION_MAJOR(minimumVulkanVersion),
        VK_API_VERSION_MINOR(minimumVulkanVersion),
        VK_API_VERSION_PATCH(minimumVulkanVersion));
    return false;
  }

  // Spec says: A non-zero variant indicates the API is a variant of the Vulkan
  // API and applications will typically need to be modified to run against it.
  if (VK_API_VERSION_VARIANT(applicationInfo.apiVersion) != 0) {
    MA_ERROR_LOG(
        "The Vulkan API supported on the system uses an unexpected variant: "
        "{:d}.",
        VK_API_VERSION_VARIANT(applicationInfo.apiVersion));
    return false;
  }

  // Create the vulkan instance
  vk::InstanceCreateInfo info =
      vk::InstanceCreateInfo()
          .setEnabledLayerCount(static_cast<uint32_t>(layerVec.size()))
          .setPpEnabledLayerNames(layerVec.data())
          .setEnabledExtensionCount(
              static_cast<uint32_t>(instanceExtVec.size()))
          .setPpEnabledExtensionNames(instanceExtVec.data())
          .setPApplicationInfo(&applicationInfo);

  res = vk::createInstance(&info, nullptr, &m_VulkanInstance);
  if (res != vk::Result::eSuccess) {
    MA_ERROR_LOG("Failed to create a Vulkan instance, error code = {:s}",
                 nvrhi::vulkan::resultToString(static_cast<VkResult>(res)));
    return false;
  }

  VULKAN_HPP_DEFAULT_DISPATCHER.init(m_VulkanInstance);

  return true;
}

bool NvRendererBackend_Vulkan::createWindowSurface() {
  if (const VkResult res = glfwCreateWindowSurface(
          m_VulkanInstance, m_Window, nullptr,
          reinterpret_cast<VkSurfaceKHR*>(&m_WindowSurface));
      res != VK_SUCCESS) {
    MA_ERROR_LOG("Failed to create a GLFW window surface, error code = {:s}",
                 nvrhi::vulkan::resultToString(res));
    return false;
  }

  return true;
}

void NvRendererBackend_Vulkan::installDebugCallback() {
  auto info = vk::DebugReportCallbackCreateInfoEXT()
                  .setFlags(vk::DebugReportFlagBitsEXT::eError |
                            vk::DebugReportFlagBitsEXT::eWarning |
                            vk::DebugReportFlagBitsEXT::eInformation |
                            vk::DebugReportFlagBitsEXT::ePerformanceWarning)
                  .setPfnCallback(&vulkanDebugCallback)
                  .setPUserData(this);

  const vk::Result res = m_VulkanInstance.createDebugReportCallbackEXT(
      &info, nullptr, &m_DebugReportCallback);
  assert(res == vk::Result::eSuccess);
}

bool NvRendererBackend_Vulkan::pickPhysicalDevice() {
  VkFormat requestedFormat =
      nvrhi::vulkan::convertFormat(nvrhi::Format::BGRA8_UNORM);
  vk::Extent2D requestedExtent(m_BackBufferWidth, m_BackBufferHeight);

  auto devices = m_VulkanInstance.enumeratePhysicalDevices();

  int firstDevice = 0;
  int lastDevice = static_cast<int>(devices.size()) - 1;
  // if (m_DeviceParams.adapterIndex >= 0) {
  //   if (m_DeviceParams.adapterIndex > lastDevice) {
  //     log::error("The specified Vulkan physical device %d does not exist.",
  //                m_DeviceParams.adapterIndex);
  //     return false;
  //   }
  //   firstDevice = m_DeviceParams.adapterIndex;
  //   lastDevice = m_DeviceParams.adapterIndex;
  // }

  // Start building an error message in case we cannot find a device.
  std::stringstream errorStream;
  errorStream << "Cannot find a Vulkan device that supports all the required "
                 "extensions and properties.";

  // build a list of GPUs
  std::vector<vk::PhysicalDevice> discreteGPUs;
  std::vector<vk::PhysicalDevice> otherGPUs;
  for (int deviceIndex = firstDevice; deviceIndex <= lastDevice;
       ++deviceIndex) {
    vk::PhysicalDevice const& dev = devices[deviceIndex];
    vk::PhysicalDeviceProperties prop = dev.getProperties();

    errorStream << std::endl << prop.deviceName.data() << ":";

    // check that all required device extensions are present
    std::unordered_set<std::string> requiredExtensions =
        enabledExtensions.device;
    auto deviceExtensions = dev.enumerateDeviceExtensionProperties();
    for (const auto& ext : deviceExtensions) {
      requiredExtensions.erase(std::string(ext.extensionName.data()));
    }

    bool deviceIsGood = true;

    if (!requiredExtensions.empty()) {
      // device is missing one or more required extensions
      for (const auto& ext : requiredExtensions) {
        errorStream << std::endl << "  - missing " << ext;
      }
      deviceIsGood = false;
    }

    auto deviceFeatures = dev.getFeatures();
    if (!deviceFeatures.samplerAnisotropy) {
      // device is a toaster oven
      errorStream << std::endl << "  - does not support samplerAnisotropy";
      deviceIsGood = false;
    }
    if (!deviceFeatures.textureCompressionBC) {
      errorStream << std::endl << "  - does not support textureCompressionBC";
      deviceIsGood = false;
    }

    if (!findQueueFamilies(dev)) {
      // device doesn't have all the queue families we need
      errorStream << std::endl
                  << "  - does not support the necessary queue types";
      deviceIsGood = false;
    }

    if (deviceIsGood && m_WindowSurface) {
      if (bool surfaceSupported =
              dev.getSurfaceSupportKHR(m_PresentQueueFamily, m_WindowSurface);
          !surfaceSupported) {
        errorStream << std::endl << "  - does not support the window surface";
        deviceIsGood = false;
      } else {
        // check that this device supports our intended swap chain creation
        // parameters
        auto surfaceCaps = dev.getSurfaceCapabilitiesKHR(m_WindowSurface);
        auto surfaceFmts = dev.getSurfaceFormatsKHR(m_WindowSurface);

        if (surfaceCaps.minImageCount > m_SwapChainBufferCount ||
            (surfaceCaps.maxImageCount < m_SwapChainBufferCount &&
             surfaceCaps.maxImageCount > 0)) {
          errorStream
              << std::endl
              << "  - cannot support the requested swap chain image count:";
          errorStream << " requested " << m_SwapChainBufferCount
                      << ", available " << surfaceCaps.minImageCount << " - "
                      << surfaceCaps.maxImageCount;
          deviceIsGood = false;
        }

        if (surfaceCaps.minImageExtent.width > requestedExtent.width ||
            surfaceCaps.minImageExtent.height > requestedExtent.height ||
            surfaceCaps.maxImageExtent.width < requestedExtent.width ||
            surfaceCaps.maxImageExtent.height < requestedExtent.height) {
          errorStream << std::endl
                      << "  - cannot support the requested swap chain size:";
          errorStream << " requested " << requestedExtent.width << "x"
                      << requestedExtent.height << ", ";
          errorStream << " available " << surfaceCaps.minImageExtent.width
                      << "x" << surfaceCaps.minImageExtent.height;
          errorStream << " - " << surfaceCaps.maxImageExtent.width << "x"
                      << surfaceCaps.maxImageExtent.height;
          deviceIsGood = false;
        }

        bool surfaceFormatPresent = false;
        for (const vk::SurfaceFormatKHR& surfaceFmt : surfaceFmts) {
          if (surfaceFmt.format == static_cast<vk::Format>(requestedFormat)) {
            surfaceFormatPresent = true;
            break;
          }
        }

        if (!surfaceFormatPresent) {
          // can't create a swap chain using the format requested
          errorStream << std::endl
                      << "  - does not support the requested swap chain format";
          deviceIsGood = false;
        }

        // check that we can present from the graphics queue
        if (uint32_t canPresent = dev.getSurfaceSupportKHR(
                m_GraphicsQueueFamily, m_WindowSurface);
            !canPresent) {
          errorStream << std::endl << "  - cannot present";
          deviceIsGood = false;
        }
      }
    }

    if (!deviceIsGood) continue;

    if (prop.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
      discreteGPUs.push_back(dev);
    } else {
      otherGPUs.push_back(dev);
    }
  }

  // pick the first discrete GPU if it exists, otherwise the first integrated
  // GPU
  if (!discreteGPUs.empty()) {
    m_VulkanPhysicalDevice = discreteGPUs[0];
    return true;
  }

  if (!otherGPUs.empty()) {
    m_VulkanPhysicalDevice = otherGPUs[0];
    return true;
  }

  MA_ERROR_LOG("{:s}", errorStream.str().c_str());

  return false;
}

bool NvRendererBackend_Vulkan::findQueueFamilies(
    vk::PhysicalDevice physicalDevice) {
  auto props = physicalDevice.getQueueFamilyProperties();

  for (int i = 0; i < static_cast<int>(props.size()); i++) {
    const auto& queueFamily = props[i];

    if (m_GraphicsQueueFamily == -1) {
      if (queueFamily.queueCount > 0 &&
          (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)) {
        m_GraphicsQueueFamily = i;
      }
    }

    if (m_ComputeQueueFamily == -1) {
      if (queueFamily.queueCount > 0 &&
          (queueFamily.queueFlags & vk::QueueFlagBits::eCompute) &&
          !(queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)) {
        m_ComputeQueueFamily = i;
      }
    }

    if (m_TransferQueueFamily == -1) {
      if (queueFamily.queueCount > 0 &&
          (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer) &&
          !(queueFamily.queueFlags & vk::QueueFlagBits::eCompute) &&
          !(queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)) {
        m_TransferQueueFamily = i;
      }
    }

    if (m_PresentQueueFamily == -1) {
      if (queueFamily.queueCount > 0 &&
          glfwGetPhysicalDevicePresentationSupport(m_VulkanInstance,
                                                   physicalDevice, i)) {
        m_PresentQueueFamily = i;
      }
    }
  }

  if (m_GraphicsQueueFamily == -1 || m_PresentQueueFamily == -1 ||
      (m_ComputeQueueFamily == -1 && m_EnableComputeQueue) ||
      (m_TransferQueueFamily == -1 && m_EnableCopyQueue)) {
    return false;
  }

  return true;
}
bool NvRendererBackend_Vulkan::createDevice() {
  // figure out which optional extensions are supported
  auto deviceExtensions =
      m_VulkanPhysicalDevice.enumerateDeviceExtensionProperties();
  for (const auto& ext : deviceExtensions) {
    const std::string name = ext.extensionName;
    if (optionalExtensions.device.find(name) !=
        optionalExtensions.device.end()) {
      enabledExtensions.device.insert(name);
    }

    // if (m_DeviceParams.enableRayTracingExtensions &&
    //     m_RayTracingExtensions.find(name) != m_RayTracingExtensions.end()) {
    //   enabledExtensions.device.insert(name);
    // }
  }

  // if (!m_DeviceParams.headlessDevice) {
  enabledExtensions.device.insert(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
  //}

  const vk::PhysicalDeviceProperties physicalDeviceProperties =
      m_VulkanPhysicalDevice.getProperties();
  m_RendererString = std::string(physicalDeviceProperties.deviceName.data());

  bool accelStructSupported = false;
  bool rayPipelineSupported = false;
  bool rayQuerySupported = false;
  bool meshletsSupported = false;
  bool vrsSupported = false;
  bool synchronization2Supported = false;
  bool maintenance4Supported = false;

  MA_INFO_LOG("{:s}", "Enabled Vulkan device extensions:");
  for (const auto& ext : enabledExtensions.device) {
    MA_INFO_LOG("    {:s}", ext.c_str());

    if (ext == VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME)
      accelStructSupported = true;
    else if (ext == VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME)
      rayPipelineSupported = true;
    else if (ext == VK_KHR_RAY_QUERY_EXTENSION_NAME)
      rayQuerySupported = true;
    else if (ext == VK_NV_MESH_SHADER_EXTENSION_NAME)
      meshletsSupported = true;
    else if (ext == VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME)
      vrsSupported = true;
    else if (ext == VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME)
      synchronization2Supported = true;
    else if (ext == VK_KHR_MAINTENANCE_4_EXTENSION_NAME)
      maintenance4Supported = true;
    else if (ext == VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME)
      m_SwapChainMutableFormatSupported = true;
  }

#define APPEND_EXTENSION(condition, desc) \
  if (condition) {                        \
    (desc).pNext = pNext;                 \
    pNext = &(desc);                      \
  }  // NOLINT(cppcoreguidelines-macro-usage)
  void* pNext = nullptr;

  vk::PhysicalDeviceFeatures2 physicalDeviceFeatures2;
  // Determine support for Buffer Device Address, the Vulkan 1.2 way
  auto bufferDeviceAddressFeatures =
      vk::PhysicalDeviceBufferDeviceAddressFeatures();
  // Determine support for maintenance4
  auto maintenance4Features = vk::PhysicalDeviceMaintenance4Features();

  // Put the user-provided extension structure at the end of the chain
  // pNext = m_DeviceParams.physicalDeviceFeatures2Extensions;
  APPEND_EXTENSION(true, bufferDeviceAddressFeatures);
  APPEND_EXTENSION(maintenance4Supported, maintenance4Features);

  physicalDeviceFeatures2.pNext = pNext;
  m_VulkanPhysicalDevice.getFeatures2(&physicalDeviceFeatures2);

  std::unordered_set<int> uniqueQueueFamilies = {m_GraphicsQueueFamily};

  // if (!m_DeviceParams.headlessDevice)
  uniqueQueueFamilies.insert(m_PresentQueueFamily);

  if (m_EnableComputeQueue) uniqueQueueFamilies.insert(m_ComputeQueueFamily);

  if (m_EnableCopyQueue) uniqueQueueFamilies.insert(m_TransferQueueFamily);

  float priority = 1.f;
  std::vector<vk::DeviceQueueCreateInfo> queueDesc;
  queueDesc.reserve(uniqueQueueFamilies.size());
  for (int queueFamily : uniqueQueueFamilies) {
    queueDesc.push_back(vk::DeviceQueueCreateInfo()
                            .setQueueFamilyIndex(queueFamily)
                            .setQueueCount(1)
                            .setPQueuePriorities(&priority));
  }

  auto accelStructFeatures =
      vk::PhysicalDeviceAccelerationStructureFeaturesKHR()
          .setAccelerationStructure(true);
  auto rayPipelineFeatures = vk::PhysicalDeviceRayTracingPipelineFeaturesKHR()
                                 .setRayTracingPipeline(true)
                                 .setRayTraversalPrimitiveCulling(true);
  auto rayQueryFeatures =
      vk::PhysicalDeviceRayQueryFeaturesKHR().setRayQuery(true);
  auto meshletFeatures = vk::PhysicalDeviceMeshShaderFeaturesNV()
                             .setTaskShader(true)
                             .setMeshShader(true);
  auto vrsFeatures = vk::PhysicalDeviceFragmentShadingRateFeaturesKHR()
                         .setPipelineFragmentShadingRate(true)
                         .setPrimitiveFragmentShadingRate(true)
                         .setAttachmentFragmentShadingRate(true);
  auto vulkan13features =
      vk::PhysicalDeviceVulkan13Features()
          .setSynchronization2(synchronization2Supported)
          .setMaintenance4(maintenance4Features.maintenance4);

  pNext = nullptr;
  APPEND_EXTENSION(accelStructSupported, accelStructFeatures)
  APPEND_EXTENSION(rayPipelineSupported, rayPipelineFeatures)
  APPEND_EXTENSION(rayQuerySupported, rayQueryFeatures)
  APPEND_EXTENSION(meshletsSupported, meshletFeatures)
  APPEND_EXTENSION(vrsSupported, vrsFeatures)
  APPEND_EXTENSION(physicalDeviceProperties.apiVersion >= VK_API_VERSION_1_3,
                   vulkan13features)
  APPEND_EXTENSION(physicalDeviceProperties.apiVersion < VK_API_VERSION_1_3 &&
                       maintenance4Supported,
                   maintenance4Features);
#undef APPEND_EXTENSION

  auto deviceFeatures = vk::PhysicalDeviceFeatures()
                            .setShaderImageGatherExtended(true)
                            .setSamplerAnisotropy(true)
                            .setTessellationShader(true)
                            .setTextureCompressionBC(true)
                            .setGeometryShader(true)
                            .setImageCubeArray(true)
                            .setShaderFloat64(true)
                            .setShaderInt16(true)
                            .setDualSrcBlend(true)
                            .setMultiDrawIndirect(true);

  auto vulkan12features =
      vk::PhysicalDeviceVulkan12Features()
          .setDescriptorIndexing(true)
          .setRuntimeDescriptorArray(true)
          .setDescriptorBindingPartiallyBound(true)
          .setDescriptorBindingVariableDescriptorCount(true)
          .setTimelineSemaphore(true)
          .setShaderSampledImageArrayNonUniformIndexing(true)
          .setSamplerFilterMinmax(true)
          .setShaderFloat16(true)
          .setBufferDeviceAddress(
              bufferDeviceAddressFeatures.bufferDeviceAddress)
          .setPNext(pNext);

  auto layerVec = stringSetToVector(enabledExtensions.layers);
  auto extVec = stringSetToVector(enabledExtensions.device);

  auto deviceDesc =
      vk::DeviceCreateInfo()
          .setPQueueCreateInfos(queueDesc.data())
          .setQueueCreateInfoCount(static_cast<uint32_t>(queueDesc.size()))
          .setPEnabledFeatures(&deviceFeatures)
          .setEnabledExtensionCount(static_cast<uint32_t>(extVec.size()))
          .setPpEnabledExtensionNames(extVec.data())
          .setEnabledLayerCount(static_cast<uint32_t>(layerVec.size()))
          .setPpEnabledLayerNames(layerVec.data())
          .setPNext(&vulkan12features);

  // if (m_DeviceParams.deviceCreateInfoCallback)
  //   m_DeviceParams.deviceCreateInfoCallback(deviceDesc);

  if (const vk::Result res = m_VulkanPhysicalDevice.createDevice(
          &deviceDesc, nullptr, &m_VulkanDevice);
      res != vk::Result::eSuccess) {
    MA_ERROR_LOG("Failed to create a Vulkan physical device, error code = {:s}",
                 nvrhi::vulkan::resultToString(static_cast<VkResult>(res)));
    return false;
  }

  m_VulkanDevice.getQueue(m_GraphicsQueueFamily, 0, &m_GraphicsQueue);
  if (m_EnableComputeQueue)
    m_VulkanDevice.getQueue(m_ComputeQueueFamily, 0, &m_ComputeQueue);
  if (m_EnableCopyQueue)
    m_VulkanDevice.getQueue(m_TransferQueueFamily, 0, &m_TransferQueue);
  // if (!m_DeviceParams.headlessDevice)
  m_VulkanDevice.getQueue(m_PresentQueueFamily, 0, &m_PresentQueue);

  VULKAN_HPP_DEFAULT_DISPATCHER.init(m_VulkanDevice);

  // remember the bufferDeviceAddress feature enablement
  m_BufferDeviceAddressSupported = vulkan12features.bufferDeviceAddress;

  MA_INFO_LOG("Created Vulkan device: {:s}", m_RendererString.c_str());

  return true;
}
bool NvRendererBackend_Vulkan::createSwapChain() {
  destroySwapChain();

  m_SwapChainFormat = {static_cast<vk::Format>(nvrhi::vulkan::convertFormat(
                           nvrhi::Format::BGRA8_UNORM)),
                       vk::ColorSpaceKHR::eSrgbNonlinear};

  vk::Extent2D extent = vk::Extent2D(m_BackBufferWidth, m_BackBufferHeight);

  std::unordered_set<uint32_t> uniqueQueues = {
      static_cast<uint32_t>(m_GraphicsQueueFamily),
      static_cast<uint32_t>(m_PresentQueueFamily)};

  std::vector<uint32_t> queues = setToVector(uniqueQueues);

  const bool enableSwapChainSharing = queues.size() > 1;

  auto desc =
      vk::SwapchainCreateInfoKHR()
          .setSurface(m_WindowSurface)
          .setMinImageCount(m_SwapChainBufferCount)
          .setImageFormat(m_SwapChainFormat.format)
          .setImageColorSpace(m_SwapChainFormat.colorSpace)
          .setImageExtent(extent)
          .setImageArrayLayers(1)
          .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment |
                         vk::ImageUsageFlagBits::eTransferDst |
                         vk::ImageUsageFlagBits::eSampled)
          .setImageSharingMode(enableSwapChainSharing
                                   ? vk::SharingMode::eConcurrent
                                   : vk::SharingMode::eExclusive)
          .setFlags(m_SwapChainMutableFormatSupported
                        ? vk::SwapchainCreateFlagBitsKHR::eMutableFormat
                        : static_cast<vk::SwapchainCreateFlagBitsKHR>(0))
          .setQueueFamilyIndexCount(
              enableSwapChainSharing ? static_cast<uint32_t>(queues.size()) : 0)
          .setPQueueFamilyIndices(enableSwapChainSharing ? queues.data()
                                                         : nullptr)
          .setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity)
          .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
          .setPresentMode(m_VsyncEnabled ? vk::PresentModeKHR::eFifo
                                         : vk::PresentModeKHR::eImmediate)
          .setClipped(true)
          .setOldSwapchain(nullptr);

  std::vector<vk::Format> imageFormats = {m_SwapChainFormat.format};
  switch (m_SwapChainFormat.format) {
    case vk::Format::eR8G8B8A8Unorm:
      imageFormats.push_back(vk::Format::eR8G8B8A8Srgb);
      break;
    case vk::Format::eR8G8B8A8Srgb:
      imageFormats.push_back(vk::Format::eR8G8B8A8Unorm);
      break;
    case vk::Format::eB8G8R8A8Unorm:
      imageFormats.push_back(vk::Format::eB8G8R8A8Srgb);
      break;
    case vk::Format::eB8G8R8A8Srgb:
      imageFormats.push_back(vk::Format::eB8G8R8A8Unorm);
      break;
    default:;
  }

  auto imageFormatListCreateInfo =
      vk::ImageFormatListCreateInfo().setViewFormats(imageFormats);

  if (m_SwapChainMutableFormatSupported)
    desc.pNext = &imageFormatListCreateInfo;

  if (const vk::Result res =
          m_VulkanDevice.createSwapchainKHR(&desc, nullptr, &m_SwapChain);
      res != vk::Result::eSuccess) {
    MA_ERROR_LOG("Failed to create a Vulkan swap chain, error code = {:s}",
                 nvrhi::vulkan::resultToString(static_cast<VkResult>(res)));
    return false;
  }

  struct DisplayChromaticities {
    float RedX;
    float RedY;
    float GreenX;
    float GreenY;
    float BlueX;
    float BlueY;
    float WhiteX;
    float WhiteY;
  } Chromacities{0.70800f, 0.29200f, 0.17000f, 0.79700f,
                 0.13100f, 0.04600f, 0.31270f, 0.32900f};

  //vk::HdrMetadataEXT hdr_metadata{{Chromacities.RedX, Chromacities.RedY},
  //                                {Chromacities.GreenX, Chromacities.GreenY},
  //                                {Chromacities.BlueX, Chromacities.BlueY},
  //                                {Chromacities.WhiteX, Chromacities.WhiteY},
  //                                2500.f,
  //                                .001f,
  //                                2000.f,
  //                                500.f,
  //                                nullptr};
  // m_VulkanDevice.setHdrMetadataEXT(1, &m_SwapChain, &hdr_metadata);

  // retrieve swap chain images
  auto images = m_VulkanDevice.getSwapchainImagesKHR(m_SwapChain);
  for (auto image : images) {
    SwapChainImage sci;
    sci.image = image;

    nvrhi::TextureDesc textureDesc;
    textureDesc.width = m_BackBufferWidth;
    textureDesc.height = m_BackBufferHeight;
    textureDesc.format = nvrhi::Format::BGRA8_UNORM;
    textureDesc.debugName = "Swap chain image";
    textureDesc.initialState = nvrhi::ResourceStates::Present;
    textureDesc.keepInitialState = true;
    textureDesc.isRenderTarget = true;

    sci.rhiHandle = m_NvrhiDevice->createHandleForNativeTexture(
        nvrhi::ObjectTypes::VK_Image, nvrhi::Object(sci.image), textureDesc);
    m_SwapChainImages.push_back(sci);
  }
  m_framebuffers.clear();
  for (int i = 0; i < GetBackBufferCount(); ++i) {
    m_framebuffers.emplace_back(GetDevice()->createFramebuffer(
        nvrhi::FramebufferDesc().addColorAttachment(
            nvrhi::FramebufferAttachment().setTexture(GetBackBuffer(i)))));
  }

  m_SwapChainIndex = 0;

  return true;
}

void NvRendererBackend_Vulkan::destroySwapChain() {
  if (m_VulkanDevice) {
    m_VulkanDevice.waitIdle();
  }

  if (m_SwapChain) {
    m_VulkanDevice.destroySwapchainKHR(m_SwapChain);
    m_SwapChain = nullptr;
  }

  m_SwapChainImages.clear();
}

#endif