#pragma once

namespace RendererEnums {

enum class RenderPassType {
  Deferred,
  Forward,
  CubeMap,
  ShadowMap,
  Num,

  RendererWarmUp

};

enum class DrawType : uint8_t { Model, InstancedModel, Num };

}
