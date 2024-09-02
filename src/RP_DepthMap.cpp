#include "Mesh.hpp"
#include "RenderPass.hpp"
#include "Shader.hpp"
#include "gl.hpp"
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <stdio.h>

RP_DepthMap::RP_DepthMap(GLuint textureSize)
    : mDepthShader{"shaders/depth_map_vertex.glsl",
                   "shaders/depth_map_fragment.glsl"},
      mTextureSize{textureSize} {
  mTexture.bindTexture(GL_TEXTURE_2D);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, mTextureSize,
               mTextureSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE,
                  GL_COMPARE_REF_TO_TEXTURE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
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
};

glm::mat4 RP_DepthMap::getProjection() const {
  // return glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);
  float scale = 5.0f;
  return glm::ortho(-scale, scale, -scale, scale, .01f, 60.0f);
}

void RP_DepthMap::begin() {
  glGetIntegerv(GL_VIEWPORT, &gVP[0]);
  glGetBooleanv(GL_DEPTH_TEST, &gDepthTest);
  glGetBooleanv(GL_CULL_FACE, &gCullFace);
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glViewport(0, 0, mTextureSize, mTextureSize);
  mFBO.bindFramebuffer(GL_DRAW_FRAMEBUFFER);
  glClear(GL_DEPTH_BUFFER_BIT);
  mDepthShader.useProgram();
};

void RP_DepthMap::setMVP(const glm::mat4 &uMVP) {
  mDepthShader.uniformMatrix4fv("uMVP", GL_FALSE, uMVP);
}

void RP_DepthMap::end() {
  glUseProgram(0);
  mFBO.unbindFramebuffer(GL_DRAW_FRAMEBUFFER);
  glViewport(gVP[0], gVP[1], gVP[2], gVP[3]);
  if (gDepthTest == GL_FALSE)
    glDisable(GL_DEPTH_TEST);
  if (gCullFace == GL_TRUE)
    glEnable(GL_CULL_FACE);
};

const RP_FBO &RP_DepthMap::getFBO() const { return mFBO; };