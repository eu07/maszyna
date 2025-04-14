#include "ddspp/ddspp.h"
#include "nvtexture.h"

#include <sstream>

bool NvTexture::LoadDDS(std::string &data, int size_bias) {
  ddspp::Descriptor desc{};
  if (ddspp::decode_header(reinterpret_cast<uint8_t *>(data.data()), desc) !=
      ddspp::Success)
    return false;
  std::stringstream ss(data, std::ios::in | std::ios::binary);
  // auto sd = opengl_texture::deserialize_ddsd(ss);
  // int blksize = 16;
  // m_bc = true;
  m_has_alpha = false;
  switch (desc.format) {
    case ddspp::DXGIFormat::BC1_UNORM:
    case ddspp::DXGIFormat::BC1_UNORM_SRGB:
      m_format =
          m_srgb ? nvrhi::Format::BC1_UNORM_SRGB : nvrhi::Format::BC1_UNORM;
      break;

    case ddspp::DXGIFormat::BC2_UNORM:
    case ddspp::DXGIFormat::BC2_UNORM_SRGB:
      m_format =
          m_srgb ? nvrhi::Format::BC2_UNORM_SRGB : nvrhi::Format::BC2_UNORM;
      break;

    case ddspp::DXGIFormat::BC3_UNORM:
    case ddspp::DXGIFormat::BC3_UNORM_SRGB:
      m_format =
          m_srgb ? nvrhi::Format::BC3_UNORM_SRGB : nvrhi::Format::BC3_UNORM;
      break;

    case ddspp::DXGIFormat::BC4_UNORM:
      m_format = nvrhi::Format::BC4_UNORM;
      break;

    case ddspp::DXGIFormat::BC4_SNORM:
      m_format = nvrhi::Format::BC4_SNORM;
      break;

    case ddspp::DXGIFormat::BC5_UNORM:
      m_format = nvrhi::Format::BC5_UNORM;
      break;

    case ddspp::DXGIFormat::BC5_SNORM:
      m_format = nvrhi::Format::BC5_SNORM;
      break;

    case ddspp::DXGIFormat::BC6H_SF16:
      m_format =  nvrhi::Format::BC6H_SFLOAT;
      break;

    case ddspp::DXGIFormat::BC6H_UF16:
      m_format =  nvrhi::Format::BC6H_UFLOAT;
      break;

    case ddspp::DXGIFormat::BC7_UNORM:
    case ddspp::DXGIFormat::BC7_UNORM_SRGB:
      m_format =
          m_srgb ? nvrhi::Format::BC7_UNORM_SRGB : nvrhi::Format::BC7_UNORM;
      break;

    default:
      return false;
  }
  //
  m_width = desc.width;
  m_height = desc.height;

  struct Bc2Block {
    uint64_t alpha;
    uint16_t color0;
    uint16_t color1;
    uint32_t pixels;
    [[nodiscard]] bool HasTransparency() const {
      return alpha != static_cast<uint64_t>(-1);
    }
  };

  static_assert(sizeof(Bc2Block) == 16);

  struct Bc3Block {
    uint16_t alpha;
    uint16_t pixels0;
    uint16_t pixels1;
    uint16_t pixels2;
    uint64_t color_block;
    [[nodiscard]] bool HasTransparency() const {
      return alpha != static_cast<uint16_t>(-1);
    }
  };

  struct Bc7Block {
    uint8_t data[16];
    [[nodiscard]] bool HasTransparency() const {
      int mode = 0;
      if (!data[0]) return false;
      for (; !(data[0] & (1 << (7 - mode))); ++mode)
        ;
      return mode > 3;
    }
  };

  static_assert(sizeof(Bc3Block) == 16);
  for (int mip = 0; mip < desc.numMips; ++mip) {
    int width = std::max(m_width >> mip, 1);
    int height = std::max(m_height >> mip, 1);
    int blkx = (width + desc.blockWidth - 1) / desc.blockWidth;
    int blky = (height + desc.blockHeight - 1) / desc.blockHeight;
    auto &[pitch, mip_data] = m_data.emplace_back();
    pitch = ddspp::get_row_pitch(desc, mip);
    mip_data.resize(pitch * blky, '\0');
    ss.seekg(desc.headerSize + ddspp::get_offset(desc, mip, 0));
    ss.read(mip_data.data(), mip_data.size());
    if (!mip) {
      switch (desc.format) {
        case ddspp::DXGIFormat::BC2_UNORM:
        case ddspp::DXGIFormat::BC2_UNORM_SRGB:
          for (size_t j = 0; !m_has_alpha && j < blkx * blky; ++j) {
            const Bc2Block &block =
                reinterpret_cast<const Bc2Block *>(data.data())[j];
            if (block.HasTransparency()) m_has_alpha = true;
          }
          break;
        case ddspp::DXGIFormat::BC3_UNORM:
        case ddspp::DXGIFormat::BC3_UNORM_SRGB:
          for (size_t j = 0; !m_has_alpha && j < blkx * blky; ++j) {
            const Bc3Block &block =
                reinterpret_cast<const Bc3Block *>(data.data())[j];
            if (block.HasTransparency()) m_has_alpha = true;
          }
          break;
        case ddspp::DXGIFormat::BC7_UNORM:
        case ddspp::DXGIFormat::BC7_UNORM_SRGB:
          for (size_t j = 0; !m_has_alpha && j < blkx * blky; ++j) {
            const Bc7Block &block =
                reinterpret_cast<const Bc7Block *>(data.data())[j];
            if (block.HasTransparency()) m_has_alpha = true;
          }
          break;
        default:
          break;
      }
    }
  }
  return true;
}