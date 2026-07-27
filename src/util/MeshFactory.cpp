#include "util/MeshFactory.h"

MeshData MeshFactory::createCube(const float size, const float textureTiling, const bool mirrorFrontBackU)
{
    const float half = size * 0.5F;
    const float t = textureTiling;
    const float uLow = mirrorFrontBackU ? t : 0.0F;
    const float uHigh = mirrorFrontBackU ? 0.0F : t;

    MeshData data;

    // Each face has its own 4 vertices (position + normal + UV) so texture
    // coordinates and normals can differ correctly per-face; shared corners
    // can't be reused once normals/UVs differ between adjacent faces.
    data.vertices = {
        // Back face (-z), normal (0, 0, -1)
        -half, -half, -half,  0.0F, 0.0F, -1.0F,  uLow,  0.0F,
         half, -half, -half,  0.0F, 0.0F, -1.0F,  uHigh, 0.0F,
         half,  half, -half,  0.0F, 0.0F, -1.0F,  uHigh, t,
        -half,  half, -half,  0.0F, 0.0F, -1.0F,  uLow,  t,

        // Front face (+z), normal (0, 0, 1)
        -half, -half,  half,  0.0F, 0.0F, 1.0F,  uLow,  0.0F,
         half, -half,  half,  0.0F, 0.0F, 1.0F,  uHigh, 0.0F,
         half,  half,  half,  0.0F, 0.0F, 1.0F,  uHigh, t,
        -half,  half,  half,  0.0F, 0.0F, 1.0F,  uLow,  t,

        // Left face (-x), normal (-1, 0, 0)
        -half, -half,  half,  -1.0F, 0.0F, 0.0F,  0.0F, 0.0F,
        -half, -half, -half,  -1.0F, 0.0F, 0.0F,  t,    0.0F,
        -half,  half, -half,  -1.0F, 0.0F, 0.0F,  t,    t,
        -half,  half,  half,  -1.0F, 0.0F, 0.0F,  0.0F, t,

        // Right face (+x), normal (1, 0, 0)
         half, -half, -half,  1.0F, 0.0F, 0.0F,  0.0F, 0.0F,
         half, -half,  half,  1.0F, 0.0F, 0.0F,  t,    0.0F,
         half,  half,  half,  1.0F, 0.0F, 0.0F,  t,    t,
         half,  half, -half,  1.0F, 0.0F, 0.0F,  0.0F, t,

         // Bottom face (-y), normal (0, -1, 0)
         -half, -half,  half,  0.0F, -1.0F, 0.0F,  0.0F, 0.0F,
          half, -half,  half,  0.0F, -1.0F, 0.0F,  t,    0.0F,
          half, -half, -half,  0.0F, -1.0F, 0.0F,  t,    t,
         -half, -half, -half,  0.0F, -1.0F, 0.0F,  0.0F, t,

         // Top face (+y), normal (0, 1, 0)
         -half,  half, -half,  0.0F, 1.0F, 0.0F,  0.0F, 0.0F,
          half,  half, -half,  0.0F, 1.0F, 0.0F,  t,    0.0F,
          half,  half,  half,  0.0F, 1.0F, 0.0F,  t,    t,
         -half,  half,  half,  0.0F, 1.0F, 0.0F,  0.0F, t,
    };

    data.indices.clear();

    for (unsigned int face = 0; face < 6; ++face)
    {
        const unsigned int base = face * 4;

        data.indices.push_back(base + 0);
        data.indices.push_back(base + 1);
        data.indices.push_back(base + 2);

        data.indices.push_back(base + 2);
        data.indices.push_back(base + 3);
        data.indices.push_back(base + 0);
    }

    return data;
}