#include "RenderPass.hpp"

RP_Tex::RP_Tex()
    : mTexShader{"shaders/texture_vertex.glsl",
                 "shaders/texture_fragment.glsl"} {
  struct TexVert {
    glm::vec3 aPos;      // -1 to 1
    glm::vec2 aTexCoord; // 0 to 1
  };
  TexVert texVerts[3]{
      {glm::vec3(-1.0, 3.0, 0.0f), glm::vec2(0.0, 2.0)},  // upper left
      {glm::vec3(-1.0, -1.0, 0.0f), glm::vec2(0.0, 0.0)}, // bottom left
      {glm::vec3(3.0, -1.0, 0.0f), glm::vec2(2.0, 0.0)}}; // bottom right
  mVBO.bufferData(sizeof(texVerts), &texVerts[0], GL_STATIC_DRAW);

  mVAO.bindVertexArray();
  mVAO.vertexAttribPointer(mVBO, 0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5,
                           (void *)0);
  mVAO.vertexAttribPointer(mVBO, 1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5,
                           (void *)(sizeof(float) * 3));
  mVAO.unbind();
};
void RP_Tex::draw(const RP_FBO &FBO) const {
  mTexShader.useProgram();
  glUniform1i(mTexShader.getUniformLocation("uTexture"), 0);
  glActiveTexture(GL_TEXTURE0);
  FBO.bindTexture(GL_TEXTURE_2D);
  mVAO.bindVertexArray();
  glDrawArrays(GL_TRIANGLES, 0, 3);
  mVAO.unbind();
  glUseProgram(0);
};
