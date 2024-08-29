#include "Mesh.hpp"
#include "RenderPass.hpp"
#include "Shader.hpp"
#include "gl.hpp"
#include <stdio.h>

RP_Depth::RP_Depth(GLuint VBO, GLuint EBO, GLuint numElements,
                   GLuint textureSize)
    : mDepthShader{"shaders/shadow_map_vertex.glsl",
                   "shaders/shadow_map_fragment.glsl"},
      mVBO{VBO}, mEBO{EBO}, mNumElements{numElements},
      mTextureSize{textureSize} {
  glGenFramebuffers(1, &mDepthFBO);
  glGenTextures(1, &mDepthTexture);
  glBindTexture(GL_TEXTURE_2D, mDepthTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, mTextureSize,
               mTextureSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  glm::vec4 borderColor{0.0, 1.0, 1.0, 1.0};
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, &borderColor[0]);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mDepthFBO);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                         GL_TEXTURE_2D, mDepthTexture, 0);

  GLenum Status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);

  if (Status != GL_FRAMEBUFFER_COMPLETE) {
    printf("FB error, status: 0x%x\n", Status);
  }
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, 0);

  glGenVertexArrays(1, &mDepthVAO);
  glBindVertexArray(mDepthVAO);
  glBindBuffer(GL_ARRAY_BUFFER, mVBO);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertexBuffer),
                        (GLvoid *)0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEBO);
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
};

glm::mat4 RP_Depth::getProjection() {
  return glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);
}

void RP_Depth::draw(const glm::mat4 &uMVP) {
  glm::ivec4 vp{};
  glGetIntegerv(GL_VIEWPORT, &vp[0]);
  glViewport(0, 0, mTextureSize, mTextureSize);
  mDepthShader.useProgram();
  glUniformMatrix4fv(mDepthShader.getUniformLocation("uMVP"), 1, GL_FALSE,
                     glm::value_ptr(uMVP));
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mDepthFBO);
  glClear(GL_DEPTH_BUFFER_BIT);
  glBindVertexArray(mDepthVAO);
  glDrawElements(GL_TRIANGLES, mNumElements, GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glUseProgram(0);
  glViewport(vp[0], vp[1], vp[2], vp[3]);
};

GLuint RP_Depth::getFramebuffer() { return mDepthFBO; };