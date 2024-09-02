#include "Perlin.hpp"
#include "RenderPass.hpp"

struct RP_Terrain_vertex_buffer {
  glm::vec3 aPos;
  glm::vec3 aNormal;
  glm::vec2 aTexCoords;
};

float interpolate(float min, float max, float value, float minVal,
                  float maxVal) {
  if (value <= minVal)
    return min;
  if (value >= maxVal)
    return max;
  float range = maxVal - minVal;
  float fraction = (value - minVal) / range;
  return min + fraction * (max - min);
}

RP_Terrain::RP_Terrain() {
  BSDFMaterial terrainMaterial{
      .ambientColor = glm::vec3(0.1, 0.1, 0.1),
      .diffuseColor = glm::vec3(.8, 0.3, 0.0),
      .specularColor = glm::vec3(0.5, 0.5, 0.5),
  };
  mUBO.bufferData(sizeof(terrainMaterial), &terrainMaterial, GL_STATIC_DRAW);

  // build grid
  // y = sin(x) + cos(z)
  // dy/dx = cos(x)
  // dy/dz = -sin(z)
  glm::vec2 xRange{-32.0, 32.0};
  glm::vec2 zRange{-32.0, 32.0};
  uint width = 256;
  uint depth = 256;
  RP_Terrain_vertex_buffer terrainBuffer[width * depth];
  for (uint i = 0; i < depth; i++) {
    for (uint j = 0; j < width; j++) {
      float xCoord = interpolate(xRange.x, xRange.y, j, 0, width);
      float zCoord = interpolate(zRange.x, zRange.y, i, 0, depth);
      float epsilon = .0001;
      int seed = 2983798;
      float scale = 1.0f / 16.0f;
      float perlinX = (xCoord + 32.0) * scale * sin(.7);
      float perlinZ = (zCoord + 32.0) * scale * -cos(.7);

      float py = perlinNoise(perlinX, perlinZ, seed);
      float pyx = perlinNoise(perlinX + epsilon, perlinZ, seed);
      float pyz = perlinNoise(perlinX, perlinZ + epsilon, seed);
      float y = 25.0f * py;

      glm::vec3 aPos{xCoord, y - 15.0f, zCoord};

      float dYdX = (pyx - py) / epsilon;
      float dYdZ = -(pyz - py) / epsilon;
      glm::vec3 tangentX{1.0, dYdX, 0.0};
      glm::vec3 tangentZ{0.0, dYdZ, 1.0};
      glm::vec3 aNormal = glm::normalize(glm::cross(tangentZ, tangentX));

      glm::vec2 aTexCoords{0.0, 0.0};

      terrainBuffer[i * depth + j] = (RP_Terrain_vertex_buffer){
          .aPos = aPos, .aNormal = aNormal, .aTexCoords = aTexCoords};
    }
  }
  mVBO.bufferData(sizeof(terrainBuffer), terrainBuffer, GL_STATIC_DRAW);
  uint numStrips = depth - 1;
  uint verticesPerStrip = 2 * width;
  uint degenerateVertices = 2 * (depth - 2);
  uint totalElements = numStrips * verticesPerStrip + degenerateVertices;
  std::vector<GLuint> elementBuffer{};
  elementBuffer.reserve(totalElements);
  for (uint i = 0; i < depth - 1; i++) {
    uint rowOffset = i * width;
    uint nextRowOffset = rowOffset + width;
    for (uint j = 0; j < width; j++) {
      elementBuffer.push_back(rowOffset + j);
      elementBuffer.push_back(nextRowOffset + j);
    }
    if (i < depth - 2) {
      // add degenerate triangles to get back to beginning of next row
      elementBuffer.push_back(nextRowOffset + width - 1);
      elementBuffer.push_back(nextRowOffset);
    }
  }

  mNumElements = elementBuffer.size();
  mEBO.bufferData(mNumElements * sizeof(elementBuffer[0]), &elementBuffer[0],
                  GL_STATIC_DRAW);
  mVAO.bindVertexArray();
  GLint stride = sizeof(RP_Terrain_vertex_buffer);
  mVAO.vertexAttribPointer(mVBO, 0, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid *)0);
  mVAO.vertexAttribPointer(
      mVBO, 1, 3, GL_FLOAT, GL_FALSE, stride,
      (GLvoid *)(offsetof(RP_Terrain_vertex_buffer, aNormal)));
  mVAO.vertexAttribPointer(
      mVBO, 2, 3, GL_FLOAT, GL_FALSE, stride,
      (GLvoid *)(offsetof(RP_Terrain_vertex_buffer, aTexCoords)));
  glVertexAttribI4ui(3, 0, 0, 0, 0); // aMaterialIdx
  mEBO.bindBuffer();

  mVAO.unbind();
  mVBO.unbind();
  mEBO.unbind();
}
void RP_Terrain::drawVertices() const {
  mVAO.bindVertexArray();
  glDrawElements(GL_TRIANGLE_STRIP, mNumElements, GL_UNSIGNED_INT, 0);
  mVAO.unbind();
};
