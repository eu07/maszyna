#include "nvtexture.h"
#include "nvrenderer/nvrenderer.h"

#include <Globals.h>
#include <PyInt.h>
#include <application.h>
#include <dictionary.h>
#include <fmt/format.h>
#include <utilities.h>

#include "Logs.h"
// #include "Texture.h"
#include "nvrendererbackend.h"
#include "stbi/stb_image.h"
#define STB_IMAGE_RESIZE2_IMPLEMENTATION
#include "stbi/stb_image_resize2.h"
#undef STB_IMAGE_RESIZE2_IMPLEMENTATION

uint64_t NvTexture::s_change_counter = 0;

void NvTexture::Load(int size_bias) {
  if (m_sz_texture->is_stub()) {
    m_width = 2;
    m_height = 2;
    auto &[pitch, data] = m_data.emplace_back();
    pitch = m_width * 4;
    m_format = nvrhi::Format::RGBA8_UNORM;
    data.resize(pitch * m_height, '\0');
    return;
  }
  std::string buf{};
  {
    std::ifstream file(fmt::format("{}{}", m_sz_texture->get_name().data(),
                                   m_sz_texture->get_type().data()),
                       std::ios::binary | std::ios::ate);
    buf.resize(file.tellg(), '\0');
    file.seekg(0);
    file.read(buf.data(), buf.size());
  };
  stbi_set_flip_vertically_on_load(true);
  if (LoadDDS(buf, size_bias)) {
  } else if (int channels; stbi_uc *source = stbi_load_from_memory(
                               reinterpret_cast<stbi_uc const *>(buf.data()),
                               buf.size(), &m_width, &m_height, &channels, 4)) {
    int mipcount =
        static_cast<int>(floor(log2(std::max(m_width, m_height)))) + 1;

    m_has_alpha = channels == 4;

    m_format =
        m_srgb ? nvrhi::Format::SRGBA8_UNORM : nvrhi::Format::RGBA8_UNORM;
    int width = m_width, prev_width = m_width, height = m_height,
        prev_height = m_height;
    for (int i = 0; i < mipcount; ++i) {
      auto &[pitch, data] = m_data.emplace_back();
      pitch = width * 4;
      data.resize(pitch * height, '\0');
      if (!i) {
        memcpy(data.data(), source, data.size());
      } else {
        stbir_resize(m_data[i - 1].second.data(), prev_width, prev_height,
                     prev_width * 4, data.data(), width, height, width * 4,
                     STBIR_4CHANNEL,
                     m_srgb ? STBIR_TYPE_UINT8_SRGB : STBIR_TYPE_UINT8,
                     STBIR_EDGE_WRAP, STBIR_FILTER_MITCHELL);
      }
      prev_width = std::exchange(width, std::max(1, width >> 1));
      prev_height = std::exchange(height, std::max(1, height >> 1));
    }

    stbi_image_free(source);
  } else if (m_sz_texture->get_type() == ".tga") {
    std::stringstream ss(buf, std::ios::in | std::ios::binary);
    // Read the header of the TGA, compare it with the known headers for
    // compressed and uncompressed TGAs
    unsigned char tgaheader[18];
    ss.read((char *)tgaheader, sizeof(unsigned char) * 18);

    while (tgaheader[0] > 0) {
      --tgaheader[0];

      unsigned char temp;
      ss.read((char *)&temp, sizeof(unsigned char));
    }

    m_width = tgaheader[13] * 256 + tgaheader[12];
    m_height = tgaheader[15] * 256 + tgaheader[14];
    m_format =
        m_srgb ? nvrhi::Format::SRGBA8_UNORM : nvrhi::Format::RGBA8_UNORM;
    int const bytesperpixel = tgaheader[16] / 8;

    // check whether width, height an BitsPerPixel are valid
    if ((m_width <= 0) || (m_height <= 0) ||
        ((bytesperpixel != 1) && (bytesperpixel != 3) &&
         (bytesperpixel != 4))) {
      return;
    }

    m_has_alpha = false;

    auto &[pitch, data] = m_data.emplace_back();
    pitch = m_width * 4;
    // allocate the data buffer
    int const datasize = pitch * m_height;
    data.resize(datasize);

    // call the appropriate loader-routine
    if (tgaheader[2] == 2) {
      // uncompressed TGA
      // rgb or greyscale image, expand to bgra
      unsigned char buffer[4] = {255, 255, 255,
                                 255};  // alpha channel will be white

      unsigned int *datapointer = (unsigned int *)&data[0];
      unsigned int *bufferpointer = (unsigned int *)&buffer[0];

      int const pixelcount = m_width * m_height;

      for (int i = 0; i < pixelcount; ++i) {
        ss.read((char *)&buffer[0], sizeof(unsigned char) * bytesperpixel);
        if (bytesperpixel == 1) {
          // expand greyscale data
          buffer[1] = buffer[0];
          buffer[2] = buffer[0];
        }
        std::swap(buffer[0], buffer[2]);
        // copy all four values in one operation
        (*datapointer) = (*bufferpointer);
        ++datapointer;
      }
    } else if (tgaheader[2] == 10) {
      // compressed TGA
      int currentpixel = 0;

      unsigned char buffer[4] = {255, 255, 255, 255};
      const int pixelcount = m_width * m_height;

      unsigned int *datapointer = (unsigned int *)&data[0];
      unsigned int *bufferpointer = (unsigned int *)&buffer[0];

      do {
        unsigned char chunkheader = 0;

        ss.read((char *)&chunkheader, sizeof(unsigned char));

        if ((chunkheader & 0x80) == 0) {
          // if the high bit is not set, it means it is the number of RAW color
          // packets, plus 1
          for (int i = 0; i <= chunkheader; ++i) {
            ss.read((char *)&buffer[0], bytesperpixel);
            m_has_alpha |= bytesperpixel == 4 && buffer[3] != 255;

            if (bytesperpixel == 1) {
              // expand greyscale data
              buffer[1] = buffer[0];
              buffer[2] = buffer[0];
            }
            std::swap(buffer[0], buffer[2]);
            // copy all four values in one operation
            (*datapointer) = (*bufferpointer);

            ++datapointer;
            ++currentpixel;
          }
        } else {
          // rle chunk, the color supplied afterwards is reapeated header + 1
          // times (not including the highest bit)
          chunkheader &= ~0x80;
          // read the current color
          ss.read((char *)&buffer[0], bytesperpixel);
          m_has_alpha |= bytesperpixel == 4 && buffer[3] != 255;

          if (bytesperpixel == 1) {
            // expand greyscale data
            buffer[1] = buffer[0];
            buffer[2] = buffer[0];
          }
          std::swap(buffer[0], buffer[2]);
          // copy the color into the image data as many times as dictated
          for (int i = 0; i <= chunkheader; ++i) {
            (*datapointer) = (*bufferpointer);
            ++datapointer;
            ++currentpixel;
          }
        }

      } while (currentpixel < pixelcount);
    }
  }
  int max_size = Global.iMaxTextureSize >> size_bias;
  while (m_data.size() > 1) {
    if (m_width <= max_size && m_height <= max_size) {
      break;
    }
    m_width = std::max(m_width >> 1, 1);
    m_height = std::max(m_height >> 1, 1);
    m_data.erase(m_data.begin());
  }
}

void NvTexture::set_components_hint(int format_hint) {
  m_srgb = format_hint == GL_SRGB || format_hint == GL_SRGB8 ||
           format_hint == GL_SRGB_ALPHA || format_hint == GL_SRGB8_ALPHA8;
}

void NvTexture::make_from_memory(size_t width, size_t height,
                                 const uint8_t *data) {
  m_data.clear();
  m_rhi_texture = nullptr;
  auto &[pitch, slice] = m_data.emplace_back();
  m_format = nvrhi::Format::RGBA8_UNORM;
  pitch = 4 * width;
  slice.resize(pitch * height);
  memcpy(slice.data(), data, slice.size());
}

void NvTexture::update_from_memory(size_t width, size_t height,
                                   const uint8_t *data) {
  auto renderer = dynamic_cast<NvRenderer *>(GfxRenderer.get());
  if (!renderer) return;
  auto backend = renderer->GetBackend();
  if (width != m_width || height != m_height ||
      m_format != nvrhi::Format::RGBA8_UNORM || !m_rhi_texture) {
    m_data.clear();
    m_width = width;
    m_height = height;
    auto &[pitch, slice] = m_data.emplace_back();
    m_format = nvrhi::Format::RGBA8_UNORM;
    pitch = 4 * width;
    slice.resize(pitch * height);
    m_rhi_texture = backend->GetDevice()->createTexture(
        nvrhi::TextureDesc()
            .setDebugName(std::string(m_sz_texture->get_name()))
            .setWidth(m_width)
            .setHeight(m_height)
            .setMipLevels(m_data.size())
            .setFormat(m_format)
            .setInitialState(nvrhi::ResourceStates::ShaderResource)
            .setKeepInitialState(true));
    m_last_change = ++s_change_counter;
  }
  auto &slice = std::get<1>(m_data.front());
  memcpy(slice.data(), data, slice.size());
  nvrhi::CommandListHandle command_list =
      backend->GetDevice()->createCommandList(
          nvrhi::CommandListParameters()
              .setQueueType(nvrhi::CommandQueue::Graphics)
              .setEnableImmediateExecution(false));
  command_list->open();
  for (int mip = 0; mip < m_data.size(); ++mip) {
    const auto &[pitch, data] = m_data[mip];
    command_list->writeTexture(m_rhi_texture, 0, mip, data.data(), pitch);
  }
  command_list->close();
  backend->GetDevice()->executeCommandList(command_list,
                                           nvrhi::CommandQueue::Graphics);
}

bool NvTexture::IsLoaded() const { return false; }

NvTextureManager::NvTextureManager(NvRenderer *renderer)
    : m_backend(renderer->m_backend.get()) {}

size_t NvTextureManager::FetchTexture(std::string path, int format_hint,
                                      int size_bias, bool unload_on_location) {
  if (contains(path, '|')) {
    path.erase(path.find('|'));  // po | może być nazwa kolejnej tekstury
  }

  std::pair<std::string, std::string> locator;  // resource name, resource type
  std::string traits;

  // discern textures generated by a script
  // TBD: support file: for file resources?
  auto const isgenerated{path.find("make:") == 0};
  auto const isinternalsrc{path.find("internal_src:") == 0};
  // process supplied resource name
  if (isgenerated || isinternalsrc) {
    // generated resource
    // scheme:(user@)path?query

    // remove scheme indicator
    path.erase(0, path.find(':') + 1);
    // TODO: extract traits specification from the query
    // clean up slashes
    erase_leading_slashes(path);
  } else {
    // regular file resource
    // (filepath/)(#)filename.extension(:traits)

    // extract trait specifications
    auto const traitpos = path.rfind(':');
    if (traitpos != std::string::npos) {
      // po dwukropku mogą być podane dodatkowe informacje niebędące nazwą
      // tekstury
      if (path.size() > traitpos + 1) {
        traits = path.substr(traitpos + 1);
      }
      path.erase(traitpos);
    }
    // potentially trim file type indicator since we check for multiple types
    erase_extension(path);
    // clean up slashes
    erase_leading_slashes(path);
    path = ToLower(path);
    // temporary code for legacy assets -- textures with names beginning with #
    // are to be sharpened
    if ((starts_with(path, "#")) || (contains(path, "/#"))) {
      traits += '#';
    }
  }
  // try to locate requested texture in the databank
  // TBD, TODO: include trait specification in the resource id in the databank?

  {
    const std::array<std::string, 3> filenames{
        Global.asCurrentTexturePath + path, path, szTexturePath + path};
    for (const auto &filename : filenames) {
      if (auto found = m_texture_map.find(filename);
          found != m_texture_map.end())
        return found->second;
    }
  }

  // if the lookup fails...
  if (isgenerated) {
    // TODO: verify presence of the generator script
    locator.first = path;
    locator.second = "make:";
  } else if (isinternalsrc) {
    locator.first = path;
    locator.second = "internalsrc:";
  } else {
    // ...for file resources check if it's on disk
    locator = texture_manager::find_on_disk(path);
    if (true == locator.first.empty()) {
      // there's nothing matching in the databank nor on the disk, report
      // failure
      ErrorLog("Bad file: failed to locate texture file \"" + path + "\"",
               logtype::file);
      return 0;
    }
  }

  auto &new_texture = m_texture_cache.emplace_back();
  auto index = m_texture_cache.size();
  m_texture_map.emplace(locator.first, index);

  new_texture = std::make_shared<NvTexture>();

  auto sz_texture = std::make_shared<opengl_texture>();
  new_texture->m_sz_texture = sz_texture;
  sz_texture->is_texstub = isgenerated;
  sz_texture->name = locator.first;
  sz_texture->type = locator.second;
  sz_texture->traits = traits;
  sz_texture->components_hint = format_hint;

  new_texture->set_components_hint(format_hint);

  new_texture->Load(size_bias);
  WriteLog("Created texture object for \"" + locator.first + "\"",
           logtype::texture);

  if (unload_on_location) {
    new_texture->m_gc_slot = m_unloadable_textures.size();
    auto &gc_slot = m_unloadable_textures.emplace_back();
    gc_slot.m_index = index - 1;
  } else {
    new_texture->m_gc_slot = static_cast<size_t>(-1);
  }

  return index;
}

bool NvTexture::CreateRhiTexture() {
  if (m_rhi_texture) return true;
  m_last_change = ++s_change_counter;
  auto renderer = dynamic_cast<NvRenderer *>(GfxRenderer.get());
  if (!renderer) return false;
  auto backend = renderer->GetBackend();
  m_rhi_texture = renderer->GetBackend()->GetDevice()->createTexture(
      nvrhi::TextureDesc()
          .setDebugName(std::string(m_sz_texture->get_name()))
          .setWidth(m_width)
          .setHeight(m_height)
          .setMipLevels(m_data.size())
          .setFormat(m_format)
          .setInitialState(nvrhi::ResourceStates::CopyDest)
          .setKeepInitialState(true));
  nvrhi::CommandListHandle command_list =
      backend->GetDevice()->createCommandList(
          nvrhi::CommandListParameters()
              .setQueueType(nvrhi::CommandQueue::Copy)
              .setEnableImmediateExecution(false));
  command_list->open();
  for (int mip = 0; mip < m_data.size(); ++mip) {
    const auto &[pitch, data] = m_data[mip];
    command_list->writeTexture(m_rhi_texture, 0, mip, data.data(), pitch);
  }
  command_list->close();
  backend->GetDevice()->executeCommandList(command_list,
                                           nvrhi::CommandQueue::Copy);
  if (m_sz_texture->get_type() == "make:") {
    auto const components{Split(std::string(m_sz_texture->get_name()), '?')};

    auto dictionary = std::make_shared<dictionary_source>(components.back());

    auto rt = std::make_shared<python_rt>();
    rt->shared_tex = this;

    if (!Application.request(
            {ToLower(components.front()), dictionary, rt})) /*__debugbreak()*/
      ;
  }
  return true;
}

bool NvTextureManager::IsValidHandle(size_t handle) {
  return handle > 0 && handle <= m_texture_cache.size();
}

NvTexture *NvTextureManager::GetTexture(size_t handle) {
  if (!handle || handle > m_texture_cache.size()) {
    return nullptr;
  }
  return m_texture_cache[handle - 1].get();
}

nvrhi::ITexture *NvTextureManager::GetRhiTexture(
    size_t handle, nvrhi::ICommandList *command_list) {
  if (!IsValidHandle(handle)) {
    return nullptr;
  }
  const auto &texture = m_texture_cache[handle - 1];
  // if (!texture.m_rhi_texture) {
  //   texture.m_rhi_texture = m_backend->GetDevice()->createTexture(
  //       nvrhi::TextureDesc()
  //           .setDebugName(texture.m_sz_texture.name)
  //           .setWidth(texture.m_width)
  //           .setHeight(texture.m_height)
  //           .setMipLevels(texture.m_mipcount)
  //           .setFormat(texture.m_format)
  //           .setInitialState(nvrhi::ResourceStates::ShaderResource)
  //           .setKeepInitialState(true));
  //
  //   for (int mip = 0; mip < texture.m_mipcount; ++mip) {
  //     const auto &[pitch, data] = texture.m_data[mip];
  //     command_list->writeTexture(texture.m_rhi_texture, 0, mip, data.data(),
  //                                pitch);
  //   }
  // }
  texture->CreateRhiTexture();
  return texture->m_rhi_texture;
}

TextureTraitFlags NvTextureManager::GetTraits(size_t handle) {
  if (!IsValidHandle(handle)) {
    return 0;
  }
  return m_texture_cache[handle - 1]->GetTraits();
}

void NvTextureManager::UpdateLastUse(size_t handle,
                                     const glm::dvec3 &location) {
  if (!IsValidHandle(handle)) {
    return;
  }
  const auto &texture = m_texture_cache[handle - 1];
  if (texture->m_gc_slot < m_unloadable_textures.size()) {
    m_unloadable_textures[texture->m_gc_slot].m_last_position_requested =
        location;
  }
}

nvrhi::SamplerHandle NvTextureManager::GetSamplerForTraits(
    TextureTraitFlags traits, NvRenderer::RenderPassType pass) {
  switch (pass) {
    case NvRenderer::RenderPassType::CubeMap:
    case NvRenderer::RenderPassType::ShadowMap:
      traits[MaTextureTraits_Sharpen] = false;
      traits[MaTextureTraits_NoAnisotropy] = true;
      traits[MaTextureTraits_NoMipBias] = true;
      break;
  }
  auto &sampler = m_samplers[traits];
  if (!sampler) {
    nvrhi::SamplerDesc desc =
        nvrhi::SamplerDesc()
            .setAddressU(traits[MaTextureTraits_ClampS]
                             ? nvrhi::SamplerAddressMode::Clamp
                             : nvrhi::SamplerAddressMode::Wrap)
            .setAddressV(traits[MaTextureTraits_ClampT]
                             ? nvrhi::SamplerAddressMode::Clamp
                             : nvrhi::SamplerAddressMode::Wrap)
            .setAllFilters(true)
            .setMipFilter(false)
            .setMaxAnisotropy(traits[MaTextureTraits_NoAnisotropy] ? 0.f : 16.f)
            .setMipBias(traits[MaTextureTraits_NoMipBias] ? 0.f : -1.76f);
    sampler = m_backend->GetDevice()->createSampler(desc);
  }
  return sampler;
}

void NvTextureManager::Cleanup(const glm::dvec3 &location) {
  for (const auto &slot : m_unloadable_textures) {
    if (glm::distance2(slot.m_last_position_requested, location) >
        Global.BaseDrawRange * Global.BaseDrawRange) {
      m_texture_cache[slot.m_index]->m_rhi_texture = nullptr;
    }
  }
}
