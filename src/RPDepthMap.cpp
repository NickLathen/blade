#include "Mesh.hpp"
#include "RenderPass.hpp"
#include "Shader.hpp"
#include "gl.hpp"
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <stdio.h>

RPDepthMap::RPDepthMap(GLuint texture_size) : m_texture_size{texture_size} {
  m_texture.BindTexture(GL_TEXTURE_2D);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, m_texture_size,
               m_texture_size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE,
                  GL_COMPARE_REF_TO_TEXTURE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  glm::vec4 border_color{0.0};
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, &border_color[0]);

  m_fbo.BindFramebuffer(GL_DRAW_FRAMEBUFFER);
  m_texture.FramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                 GL_TEXTURE_2D, 0);
  GLenum status = m_fbo.CheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    printf("FB error, status: 0x%x\n", status);
  }
  m_fbo.UnbindFramebuffer(GL_DRAW_FRAMEBUFFER);
};

glm::mat4 RPDepthMap::GetProjection(float fov, float near, float far) const {
  return glm::perspective(glm::radians(fov), 1.0f, near, far);
  // float scale = 100.0f;
  // return glm::ortho(-scale, scale, -scale, scale, .01f, 250.0f);
}

void RPDepthMap::Begin() {
  m_fbo.BindFramebuffer(GL_DRAW_FRAMEBUFFER);
  glClear(GL_DEPTH_BUFFER_BIT);
  glGetIntegerv(GL_VIEWPORT, &g_vp[0]);
  glGetBooleanv(GL_DEPTH_TEST, &g_depth_test);
  glGetBooleanv(GL_CULL_FACE, &g_cull_face);
  glGetIntegerv(GL_CULL_FACE_MODE, &g_cull_face_mode);
  glGetIntegerv(GL_FRONT_FACE, &g_front_face);
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glCullFace(GL_FRONT);
  glFrontFace(GL_CCW);
  glViewport(0, 0, m_texture_size, m_texture_size);
};

void RPDepthMap::End() {
  m_fbo.UnbindFramebuffer(GL_DRAW_FRAMEBUFFER);
  glViewport(g_vp[0], g_vp[1], g_vp[2], g_vp[3]);
  if (g_depth_test == GL_FALSE)
    glDisable(GL_DEPTH_TEST);
  if (g_cull_face == GL_FALSE)
    glDisable(GL_CULL_FACE);
  glCullFace(g_cull_face_mode);
  glFrontFace(g_front_face);
};

const RPTexture &RPDepthMap::GetTexture() const { return m_texture; };