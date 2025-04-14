#pragma once

#include <Model3d.h>

struct MaTopologyUtils {

  void ConvertTopology(gfx::index_array &Indices, gfx::vertex_array &Vertices,
                       int const Typ) const;

};