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

RP_Terrain::RP_Terrain()
    : mShader{"shaders/terrain_vertex.glsl", "shaders/terrain_fragment.glsl"} {
  mShader.useProgram();
  mShader.setUniformBlockBinding("uMaterialBlock", muMaterialBlockBinding);
  mShader.uniform1i("uLightDepthTexture", muLightDepthTexture);
  mShader.uniform1i("uDiffuseTexture", muDiffuseTexture);
  glUseProgram(0);

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
  glm::vec2 xRange{-8.0, 8.0};
  glm::vec2 zRange{-8.0, 8.0};
  uint width = 64;
  uint depth = 64;
  float magnitude = 0.5f;
  RP_Terrain_vertex_buffer terrainBuffer[width * depth];
  for (uint i = 0; i < depth; i++) {
    for (uint j = 0; j < width; j++) {
      float xCoord = interpolate(xRange.x, xRange.y, j, 0, width);
      float zCoord = interpolate(zRange.x, zRange.y, i, 0, depth);
      float y = magnitude * (sin(xCoord) + cos(zCoord));
      glm::vec3 aPos{xCoord, y, zCoord};

      float dYdX = magnitude * cos(xCoord);
      float dYdZ = -magnitude * sin(zCoord);
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
  mEBO.bindBuffer();

  mVAO.unbind();
  mVBO.unbind();
  mEBO.unbind();
}
void RP_Terrain::draw(const glm::vec3 &uCameraPos, const Light &light,
                      const glm::mat4 &uMVP, const glm::mat4 &uLightMVP,
                      const glm::mat4 &uModelMatrix, const RP_FBO &FBO) const {
  // set globals
  GLboolean gDepthTest, gCullFace;
  GLint gCullFaceMode, gFrontFace;
  glGetBooleanv(GL_DEPTH_TEST, &gDepthTest);
  glGetBooleanv(GL_CULL_FACE, &gCullFace);
  glGetIntegerv(GL_CULL_FACE_MODE, &gCullFaceMode);
  glGetIntegerv(GL_FRONT_FACE, &gFrontFace);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);

  mShader.useProgram();
  mShader.uniform3fv("uCameraPos", uCameraPos);
  mShader.uniform3fv("uAmbientLightColor", light.uAmbientLightColor);
  mShader.uniform3fv("uLightDir", light.uLightDir);
  mShader.uniform3fv("uLightColor", light.uLightColor);
  mShader.uniform3fv("uLightPos", light.uLightPos);
  mShader.uniformMatrix4fv("uMVP", GL_FALSE, uMVP);
  mShader.uniformMatrix4fv("uLightMVP", GL_FALSE, uLightMVP);
  mShader.uniformMatrix4fv("uModelMatrix", GL_FALSE, uModelMatrix);
  mShader.uniform1f("uSpecularPower", 32.0f);
  mShader.uniform1f("uShininessScale", 2000.0f);
  mShader.uniformMatrix4fv("uMVP", GL_FALSE, uMVP);
  mUBO.bindBufferBase(muMaterialBlockBinding);
  glActiveTexture(GL_TEXTURE0 + muLightDepthTexture);
  FBO.bindTexture(GL_TEXTURE_2D);
  mVAO.bindVertexArray();
  glDrawElements(GL_TRIANGLE_STRIP, mNumElements, GL_UNSIGNED_INT, 0);
  mVAO.unbind();
  glBindTexture(GL_TEXTURE_2D, 0);
  glUseProgram(0);
  if (gDepthTest == GL_FALSE)
    glDisable(GL_DEPTH_TEST);
  if (gCullFace == GL_FALSE)
    glDisable(GL_CULL_FACE);
  glCullFace(gCullFaceMode);
  glFrontFace(gFrontFace);
};
