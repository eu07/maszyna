#include "sky.h"

#include <nvrhi/utils.h>

#include "nvrendererbackend.h"
#include "simulationenvironment.h"
#include "simulationtime.h"

namespace {
static const float PI = 3.14159265358979323846f;
static const float INV_PI = 0.31830988618379067154f;
static const float INV_4PI = 0.25f * INV_PI;
static const float PHASE_ISOTROPIC = INV_4PI;
static const float RAYLEIGH_PHASE_SCALE = (3.f / 16.f) * INV_PI;
static const float g = 0.8f;
static const float gg = g * g;

static const float EARTH_RADIUS = 6371.0e3f;         // km
static const float ATMOSPHERE_THICKNESS = 100.0e3f;  // km
static const float ATMOSPHERE_RADIUS = EARTH_RADIUS + ATMOSPHERE_THICKNESS;
static const float EXPOSURE = -4.0;

// Extraterrestial Solar Irradiance Spectra, units W * m^-2 * nm^-1
// https://www.nrel.gov/grid/solar-resource/spectra.html
static const glm::vec4 sun_spectral_irradiance =
    glm::vec4(1.679, 1.828, 1.986, 1.307);
// Rayleigh scattering coefficient at sea level, units km^-1
// "Rayleigh-scattering calculations for the terrestrial atmosphere"
// by Anthony Bucholtz (1995).
static const glm::vec4 molecular_scattering_coefficient_base =
    glm::vec4(6.605e-6, 1.067e-5, 1.842e-5, 3.156e-5);
// Fog scattering/extinction cross section, units m^2 / molecules
// Mie theory results for IOR of 1.333. Particle size is a log normal
// distribution of mean diameter=15 and std deviation=0.4
static const glm::vec4 fog_scattering_cross_section =
    glm::vec4(5.015e-10, 4.987e-10, 4.966e-10, 4.949e-10);
// Ozone absorption cross section, units m^2 / molecules
// "High spectral resolution ozone absorption cross-sections"
// by V. Gorshelev et al. (2014).
static const glm::vec4 ozone_absorption_cross_section =
    glm::vec4(3.472e-25, 3.914e-25, 1.349e-25, 11.03e-27);
static const glm::mat4x3 M = {
    137.672389239975,   -8.632904716299537,  -1.7181567391931372,
    32.549094028629234, 91.29801417199785,   -12.005406444382531,
    -38.91428392614275, 34.31665471469816,   29.89044807197628,
    8.572844237945445,  -11.103384660054624, 117.47585277566478};
}  // namespace

Sky::Sky(NvRenderer* renderer)
    : m_backend(renderer->GetBackend()), MaResourceRegistry(renderer) {
  m_transmittance_pass = std::make_shared<SkyTransmittancePass>(this);
  m_aerial_lut = std::make_shared<SkyAerialLut>(this);
}

void Sky::Init() {
  InitResourceRegistry();
  m_sky_constants = m_backend->GetDevice()->createBuffer(
      nvrhi::utils::CreateVolatileConstantBufferDesc(sizeof(SkyConstants),
                                                     "Sky constants", 16)
          .setInitialState(nvrhi::ResourceStates::ConstantBuffer)
          .setKeepInitialState(true));
  m_transmittance_pass->Init();
  m_aerial_lut->Init();
  RegisterResource(true, "sky_aerial_lut", m_aerial_lut->m_lut,
                   nvrhi::ResourceType::Texture_SRV);
}

void Sky::Render(nvrhi::ICommandList* command_list,
                 const glm::dmat4& projection, const glm::dmat4& view) {
  UpdateConstants(command_list);
  m_transmittance_pass->Render(command_list);
  m_aerial_lut->Render(command_list, projection, view);
}

float Sky::GetOzoneMean() const {
  float month = glm::saturate(simulation::Time.year_day() / 366.f) * 12.f + .5f;
  float month_fract = glm::fract(month);
  int month_l = glm::floor(month);
  static std::array<float, 12> ozone_means{347.f, 370.f, 381.f, 384.f,
                                           372.f, 352.f, 333.f, 317.f,
                                           298.f, 285.f, 290.f, 315.f};
  return glm::lerp(ozone_means[month_l], ozone_means[(month_l + 1) % 12],
                   month_fract);
}

glm::vec4 Sky::GetAerosolAbsorptionCrossSection() const {
  switch (m_aerosol_preset) {
    case AerosolPreset::Rural:
      return {5.0393e-23f, 8.0765e-23f, 1.3823e-22f, 2.3383e-22f};
    case AerosolPreset::Urban:
      return {2.8722e-24f, 4.6168e-24f, 7.9706e-24f, 1.3578e-23f};
    default:
      return glm::vec4(0.f);
  }
}

glm::vec4 Sky::GetAerosolScatteringCrossSection() const {
  switch (m_aerosol_preset) {
    case AerosolPreset::Rural:
      return {2.6004e-22f, 2.4844e-22f, 2.8362e-22f, 2.7494e-22f};
    case AerosolPreset::Urban:
      return {1.5908e-22f, 1.7711e-22f, 2.0942e-22f, 2.4033e-22f};
    default:
      return glm::vec4(0.f);
  }
}

float Sky::GetAerosolBaseDensity() const {
  switch (m_aerosol_preset) {
    case AerosolPreset::Rural:
      return 8.544e15f;
    case AerosolPreset::Urban:
      return 1.3681e17f;
    default:
      return 0.f;
  }
}

float Sky::GetAerosolBackgroundDividedByBaseDensity() const {
  switch (m_aerosol_preset) {
    case AerosolPreset::Rural:
      return 2e3f / GetAerosolBaseDensity();
    case AerosolPreset::Urban:
      return 2e3f / GetAerosolBaseDensity();
    default:
      return 0.f;
  }
}

float Sky::GetAerosolHeightScale() const {
  switch (m_aerosol_preset) {
    case AerosolPreset::Rural:
      return .73f;
    case AerosolPreset::Urban:
      return .73f;
    default:
      return 0.f;
  }
}

float Sky::GetFogDensity() const {
  return 3e7f * glm::pow(0.99937f, glm::min(3e7f, m_visibility));
}

void Sky::UpdateConstants(nvrhi::ICommandList* command_list) {
  SkyConstants constants{};
  constants.g_AerosolAbsorptionCrossSection =
      GetAerosolAbsorptionCrossSection();
  constants.g_AerosolScatteringCrossSection =
      GetAerosolScatteringCrossSection();
  constants.g_AerosolHeightScale = GetAerosolHeightScale();
  constants.g_AerosolBaseDensity = GetAerosolBaseDensity();
  constants.g_AerosolBackgroundDividedByBaseDensity =
      GetAerosolBackgroundDividedByBaseDensity();
  constants.g_GroundAlbedo = glm::vec4{m_ground_albedo};
  constants.g_AerosolTurbidity = m_aerosol_turbidity;
  constants.g_OzoneMean = GetOzoneMean();
  constants.g_FogDensity = GetFogDensity();
  constants.g_FogHeightOffset = m_fog_height_offset;
  constants.g_FogHeightScale = m_fog_height_scale;
  command_list->writeBuffer(m_sky_constants, &constants, sizeof(constants));
}

void Sky::OnGui() {
  if (!m_gui_active) return;
  if (ImGui::Begin("Sky settings", &m_gui_active,
                   ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::SliderFloat("Visibility", &m_visibility, 10.f, 3e4f);
    ImGui::SliderFloat("Fog Offset", &m_fog_height_offset, 0.f, 10.f);
    ImGui::SliderFloat("Fog Height", &m_fog_height_scale, .01f, 10.f);
    if (ImGui::BeginCombo("Aerosol Type",
                          GetAerosolTypeDesc(m_aerosol_preset))) {
      for (int i = 0; i < static_cast<int>(AerosolPreset::Num); ++i) {
        AerosolPreset preset = static_cast<AerosolPreset>(i);
        if (ImGui::Selectable(GetAerosolTypeDesc(preset),
                              preset == m_aerosol_preset)) {
          m_aerosol_preset = preset;
        }
      }
      ImGui::EndCombo();
    }
    ImGui::SliderFloat("Aerosol Turbidity", &m_aerosol_turbidity, 0., 10.);
    ImGui::SliderFloat("Ground Albedo", &m_ground_albedo, 0., 10.);
    ImGui::End();
  }
}

void Sky::ShowGui() { m_gui_active = true; }

const char* Sky::GetAerosolTypeDesc(AerosolPreset preset) {
  switch (preset) {
    case AerosolPreset::Rural:
      return "Rural";
    case AerosolPreset::Urban:
      return "Urban";
    default:
      return "INVALID";
  }
}

glm::vec4 Sky::GetMolecularScatteringCoefficient(float h) const {
  return molecular_scattering_coefficient_base *
         glm::exp(-0.07771971f * glm::pow(h, 1.16364243f));
}

glm::vec4 Sky::GetMolecularAbsorptionCoefficient(float h) const {
  h += 1e-4f;  // Avoid division by 0
  float t = glm::log(h) - 3.22261f;
  float density = 3.78547397e17f * (1.f / h) * glm::exp(-t * t * 5.55555555f);
  return ozone_absorption_cross_section * GetOzoneMean() * density;
}

glm::vec4 Sky::GetFogScatteringCoefficient(float h) const {
  if (GetFogDensity() > 0.f) {
    return fog_scattering_cross_section * GetFogDensity() *
           glm::min(1.f,
                    glm::exp((-h + m_fog_height_offset) / m_fog_height_scale));
  } else {
    return glm::vec4{0.f};
  }
}

glm::vec3 Sky::CalcSunColor() const {
  glm::vec4 transmittance =
      ComputeTransmittance(-Global.DayLight.direction, 32);
  return simulation::Environment.light_intensity() *
         LinearSrgbFromSpectralSamples(sun_spectral_irradiance *
                                       transmittance) *
         glm::exp(EXPOSURE);
}

glm::vec3 Sky::LinearSrgbFromSpectralSamples(glm::vec4 L) {
  glm::vec3 c = M * L;
  return c.r * glm::vec3(0.4123908f, 0.21263901f, 0.01933082f) +
         c.g * glm::vec3(0.35758434f, 0.71516868f, 0.11919478f) +
         c.b * glm::vec3(0.18048079f, 0.07219232f, 0.95053215f);
}

float Sky::RaySphereIntersection(glm::vec3 ro, glm::vec3 rd, float radius) {
  float b = glm::dot(ro, rd);
  float c = glm::dot(ro, ro) - radius * radius;
  if (c > 0.f && b > 0.f) return -1.f;
  float d = b * b - c;
  if (d < 0.f) return -1.f;
  if (d > b * b) return (-b + glm::sqrt(d));
  return (-b - glm::sqrt(d));
}

float Sky::GetAerosolDensity(float h) const {
  return GetAerosolBaseDensity() * (exp(-h / GetAerosolHeightScale()) +
                                    GetAerosolBackgroundDividedByBaseDensity());
}

void Sky::GetAtmosphereCollisionCoefficients(
    float h, glm::vec4& aerosol_absorption, glm::vec4& aerosol_scattering,
    glm::vec4& molecular_absorption, glm::vec4& molecular_scattering,
    glm::vec4& fog_scattering, glm::vec4& extinction) const {
  h = glm::max(h, 1.e-3f);  // In case height is negative
  h *= 1.e-3f;
  float aerosol_density = GetAerosolDensity(h);
  aerosol_absorption = GetAerosolAbsorptionCrossSection() * aerosol_density *
                       m_aerosol_turbidity;
  aerosol_scattering = GetAerosolScatteringCrossSection() * aerosol_density *
                       m_aerosol_turbidity;
  molecular_absorption = GetMolecularAbsorptionCoefficient(h);
  molecular_scattering = GetMolecularScatteringCoefficient(h);
  fog_scattering = GetFogScatteringCoefficient(h);
  extinction = aerosol_absorption + aerosol_scattering + molecular_absorption +
               molecular_scattering + fog_scattering;
}

glm::vec4 Sky::ComputeTransmittance(const glm::vec3& ray_dir, int steps) const {
  glm::vec3 ray_origin{0.f, EARTH_RADIUS + Global.pCamera.Pos.y, 0.f};
  glm::vec4 transmittance{1.f};
  float t_max = std::numeric_limits<float>::max();
  float atmos_dist =
      RaySphereIntersection(ray_origin, ray_dir, ATMOSPHERE_RADIUS);

  if (RaySphereIntersection(ray_origin, ray_dir, EARTH_RADIUS) >= 0.0) {
    return glm::vec4(0.f, 0.f, 0.f, 0.f);
  }

  t_max = glm::min(t_max, atmos_dist);
  float dt = t_max / float(steps);

  for (int i = 0; i < steps; ++i) {
    float t = (float(i) + 0.5) * dt;
    glm::vec3 x_t = ray_origin + ray_dir * t;

    float distance_to_earth_center = glm::length(x_t);
    float altitude = distance_to_earth_center - EARTH_RADIUS;

    glm::vec4 aerosol_absorption, aerosol_scattering;
    glm::vec4 molecular_absorption, molecular_scattering;
    glm::vec4 fog_scattering;
    glm::vec4 extinction;
    GetAtmosphereCollisionCoefficients(
        altitude, aerosol_absorption, aerosol_scattering, molecular_absorption,
        molecular_scattering, fog_scattering, extinction);

    glm::vec4 step_transmittance = glm::exp(-dt * extinction);
    transmittance *= step_transmittance;
  }
  return transmittance;
}

void SkyTransmittancePass::Init() {
  m_pixel_shader =
      m_backend->CreateShader("sky_transmittance", nvrhi::ShaderType::Pixel);
  m_output = m_backend->GetDevice()->createTexture(
      nvrhi::TextureDesc()
          .setFormat(nvrhi::Format::RGBA8_UNORM)
          .setWidth(128)
          .setHeight(128)
          .setIsRenderTarget(true)
          .setInitialState(nvrhi::ResourceStates::ShaderResource)
          .setKeepInitialState(true));
  nvrhi::utils::CreateBindingSetAndLayout(
      m_backend->GetDevice(), nvrhi::ShaderType::Pixel, 0,
      nvrhi::BindingSetDesc().addItem(
          nvrhi::BindingSetItem::ConstantBuffer(13, m_sky->m_sky_constants)),
      m_binding_layout, m_binding_set);
  m_framebuffer = m_backend->GetDevice()->createFramebuffer(
      nvrhi::FramebufferDesc().addColorAttachment(m_output));
  FullScreenPass::Init();
}

void SkyTransmittancePass::CreatePipelineDesc(
    nvrhi::GraphicsPipelineDesc& pipeline_desc) {
  FullScreenPass::CreatePipelineDesc(pipeline_desc);
  pipeline_desc.setPixelShader(m_pixel_shader);
  pipeline_desc.addBindingLayout(m_binding_layout);
}

void SkyTransmittancePass::Render(nvrhi::ICommandList* command_list) {
  command_list->beginMarker("Sky transmittance");
  nvrhi::GraphicsState graphics_state;
  InitState(graphics_state);
  graphics_state.addBindingSet(m_binding_set);
  command_list->setGraphicsState(graphics_state);
  Draw(command_list);
  command_list->endMarker();
}

nvrhi::IFramebuffer* SkyTransmittancePass::GetFramebuffer() {
  return m_framebuffer;
}

void SkyAerialLut::Init() {
  m_sky_width = 128;
  m_sky_height = 512;
  m_lut_width = 64;
  m_lut_height = 256;
  m_lut_slices = 32;
  m_constant_buffer = m_sky->m_backend->GetDevice()->createBuffer(
      nvrhi::utils::CreateVolatileConstantBufferDesc(
          sizeof(DispatchConstants), "Sky Aerial LUT Dispatch Constants", 16));
  m_lut = m_sky->m_backend->GetDevice()->createTexture(
      nvrhi::TextureDesc()
          .setDimension(nvrhi::TextureDimension::Texture2DArray)
          .setWidth(m_lut_width)
          .setHeight(m_lut_height)
          .setArraySize(m_lut_slices)
          .setFormat(nvrhi::Format::RGBA16_FLOAT)
          .setIsUAV(true)
          .setInitialState(nvrhi::ResourceStates::Common)
          .setKeepInitialState(true)
          .setDebugName("Sky Aerial LUT"));
  m_sky_texture = m_sky->m_backend->GetDevice()->createTexture(
      nvrhi::TextureDesc()
          .setWidth(m_sky_width)
          .setHeight(m_sky_height)
          .setFormat(nvrhi::Format::RGBA16_FLOAT)
          .setIsUAV(true)
          .setInitialState(nvrhi::ResourceStates::Common)
          .setKeepInitialState(true)
          .setDebugName("Sky Texture"));
  auto shader_lut = m_sky->m_backend->CreateShader("sky_aerial_lut",
                                                   nvrhi::ShaderType::Compute);
  auto shader_sky =
      m_sky->m_backend->CreateShader("sky", nvrhi::ShaderType::Compute);
  auto sampler = m_sky->m_backend->GetDevice()->createSampler(
      nvrhi::SamplerDesc()
          .setAllAddressModes(nvrhi::SamplerAddressMode::ClampToEdge)
          .setAllFilters(true));
  nvrhi::BindingLayoutHandle binding_layout_lut;
  nvrhi::BindingLayoutHandle binding_layout_sky;
  nvrhi::utils::CreateBindingSetAndLayout(
      m_sky->m_backend->GetDevice(), nvrhi::ShaderType::Compute, 0,
      nvrhi::BindingSetDesc()
          .addItem(nvrhi::BindingSetItem::ConstantBuffer(0, m_constant_buffer))
          .addItem(
              nvrhi::BindingSetItem::ConstantBuffer(13, m_sky->m_sky_constants))
          .addItem(nvrhi::BindingSetItem::Texture_UAV(0, m_lut))
          .addItem(nvrhi::BindingSetItem::Texture_SRV(
              13, m_sky->m_transmittance_pass->m_output))
          .addItem(nvrhi::BindingSetItem::Sampler(13, sampler)),
      binding_layout_lut, m_bindings_lut);
  nvrhi::utils::CreateBindingSetAndLayout(
      m_sky->m_backend->GetDevice(), nvrhi::ShaderType::Compute, 0,
      nvrhi::BindingSetDesc()
          .addItem(nvrhi::BindingSetItem::ConstantBuffer(0, m_constant_buffer))
          .addItem(
              nvrhi::BindingSetItem::ConstantBuffer(13, m_sky->m_sky_constants))
          .addItem(nvrhi::BindingSetItem::Texture_UAV(1, m_sky_texture))
          .addItem(nvrhi::BindingSetItem::Texture_SRV(
              13, m_sky->m_transmittance_pass->m_output))
          .addItem(nvrhi::BindingSetItem::Sampler(13, sampler)),
      binding_layout_sky, m_bindings_sky);
  m_pso_lut = m_sky->m_backend->GetDevice()->createComputePipeline(
      nvrhi::ComputePipelineDesc()
          .addBindingLayout(binding_layout_lut)
          .setComputeShader(shader_lut));
  m_pso_sky = m_sky->m_backend->GetDevice()->createComputePipeline(
      nvrhi::ComputePipelineDesc()
          .addBindingLayout(binding_layout_sky)
          .setComputeShader(shader_sky));
}

void SkyAerialLut::Render(nvrhi::ICommandList* command_list,
                          const glm::dmat4& projection,
                          const glm::dmat4& view) {
  {
    DispatchConstants constants{};
    constants.g_InverseView = static_cast<glm::mat3>(glm::inverse(view));
    constants.g_InverseProjection = glm::inverse(projection);
    constants.g_SunDir = -Global.DayLight.direction;
    constants.g_Altitude = Global.pCamera.Pos.y;
    constants.g_MaxDepth = Global.BaseDrawRange * Global.fDistanceFactor;
    command_list->writeBuffer(m_constant_buffer, &constants, sizeof(constants));
  }

  command_list->setComputeState(
      nvrhi::ComputeState().setPipeline(m_pso_lut).addBindingSet(
          m_bindings_lut));
  command_list->dispatch((m_lut_width + 7) / 8, (m_lut_height + 7) / 8, 1);

  command_list->setComputeState(
      nvrhi::ComputeState().setPipeline(m_pso_sky).addBindingSet(
          m_bindings_sky));
  command_list->dispatch((m_sky_width + 7) / 8, (m_sky_height + 7) / 8, 1);
}
