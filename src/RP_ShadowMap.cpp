#include "Mesh.hpp"
#include "RenderPass.hpp"
#include "Shader.hpp"
#include "gl.hpp"
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <stdio.h>

RP_ShadowMap::RP_ShadowMap(const RP_VBO &VBO, const RP_EBO &EBO,
                           GLuint numElements, GLuint textureSize)
    : mDepthShader{"shaders/shadow_map_vertex.glsl",
                   "shaders/shadow_map_fragment.glsl"},
      mNumElements{numElements}, mTextureSize{textureSize} {
  mTexture.bindTexture(GL_TEXTURE_2D);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, mTextureSize,
               mTextureSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  glm::vec4 borderColor{0.0};
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, &borderColor[0]);

  mFBO.bindFramebuffer(GL_DRAW_FRAMEBUFFER);
  mTexture.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                GL_TEXTURE_2D, 0);
  GLenum Status = mFBO.checkFramebufferStatus(GL_DRAW_FRAMEBUFFER);
  if (Status != GL_FRAMEBUFFER_COMPLETE) {
    printf("FB error, status: 0x%x\n", Status);
  }
  mFBO.unbindFramebuffer(GL_DRAW_FRAMEBUFFER);

  mVAO.bindVertexArray();
  mVAO.vertexAttribPointer(VBO, 0, 3, GL_FLOAT, GL_FALSE,
                           sizeof(MeshVertexBuffer), (GLvoid *)0);
  EBO.bindBuffer();
  mVAO.unbind();
  EBO.unbind();
};

glm::mat4 RP_ShadowMap::getProjection() const {
  return glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);
}

void RP_ShadowMap::draw(const glm::mat4 &uMVP) const {
  glm::ivec4 vp{};
  GLboolean gDepthTest, gCullFace;
  glGetIntegerv(GL_VIEWPORT, &vp[0]);
  glGetBooleanv(GL_DEPTH_TEST, &gDepthTest);
  glGetBooleanv(GL_CULL_FACE, &gCullFace);
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glViewport(0, 0, mTextureSize, mTextureSize);
  mFBO.bindFramebuffer(GL_DRAW_FRAMEBUFFER);
  glClear(GL_DEPTH_BUFFER_BIT);
  mDepthShader.useProgram();
  mDepthShader.uniformMatrix4fv("uMVP", GL_FALSE, uMVP);
  mVAO.bindVertexArray();
  glDrawElements(GL_TRIANGLES, mNumElements, GL_UNSIGNED_INT, 0);
  mVAO.unbind();
  mFBO.unbindFramebuffer(GL_DRAW_FRAMEBUFFER);
  glUseProgram(0);
  glViewport(vp[0], vp[1], vp[2], vp[3]);
  if (gDepthTest == GL_FALSE)
    glDisable(GL_DEPTH_TEST);
  if (gCullFace == GL_TRUE)
    glEnable(GL_CULL_FACE);
};

const RP_FBO &RP_ShadowMap::getFBO() const { return mFBO; };