#include "RenderPass.hpp"

RP_Tex::RP_Tex()
    : mTexShader{"shaders/texture_vertex.glsl",
                 "shaders/texture_fragment.glsl"} {
  glGenVertexArrays(1, &mTexVAO);
  glGenBuffers(1, &mTexVBO);
  glBindVertexArray(mTexVAO);
  glBindBuffer(GL_ARRAY_BUFFER, mTexVBO);
  struct TexVert {
    glm::vec3 aPos;      // -1 to 1
    glm::vec2 aTexCoord; // 0 to 1
  };
  TexVert texVerts[3]{
      {glm::vec3(-1.0, 3.0, 0.0f), glm::vec2(0.0, 2.0)},  // upper left
      {glm::vec3(-1.0, -1.0, 0.0f), glm::vec2(0.0, 0.0)}, // bottom left
      {glm::vec3(3.0, -1.0, 0.0f), glm::vec2(2.0, 0.0)}}; // bottom right
  glBufferData(GL_ARRAY_BUFFER, sizeof(texVerts), &texVerts[0], GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void *)0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5,
                        (void *)(sizeof(float) * 3));
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
};
void RP_Tex::draw(GLuint texture) {
  mTexShader.useProgram();
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);
  glBindVertexArray(mTexVAO);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glUseProgram(0);
};
