#pragma once

#include <string>

#include "GpuResources.hpp"
#include "GpuTexture.hpp"
#include "TextureArray.hpp"
#include "utils.hpp"

class RPWowTerrain {
public:
  RPWowTerrain(const std::string &wdt_path, const std::string &adt_name);
  NEVER_COPY(RPWowTerrain);
  RPWowTerrain(RPWowTerrain &&other)
      : m_vao{std::move(other.m_vao)},
        m_ssbo_heightmap{std::move(other.m_ssbo_heightmap)},
        m_ssbo_texture_slots{std::move(other.m_ssbo_texture_slots)},
        m_ssbo_alpha_slots{std::move(other.m_ssbo_alpha_slots)},
        m_corner_position{std::move(other.m_corner_position)},
        m_texture{std::move(other.m_texture)} {};
  const GpuTexture &GetBlendTexture() const { return m_texture[0]; };
  const GpuTexture &GetAlphaTexture() const { return m_texture[1]; };
  const GpuSSBO &GetHeightmapSSBO() const { return m_ssbo_heightmap; }
  const GpuSSBO &GetTextureSlotsSSBO() const { return m_ssbo_texture_slots; }
  const GpuSSBO &GetAlphaSlotsSSBO() const { return m_ssbo_alpha_slots; }
  const glm::vec2 &GetCornerPosition() const { return m_corner_position; }
  void DrawVertices() const;

private:
  void LoadTerrainAdt(const std::string &wdt_path, const std::string &adt_name);
  GpuVAO m_vao;
  GpuSSBO m_ssbo_heightmap;     // heights + normals
  GpuSSBO m_ssbo_texture_slots; // map texture slots for each block
  GpuSSBO m_ssbo_alpha_slots;   // map alpha slots for each block
  glm::vec2 m_corner_position{};
  std::vector<GpuTexture> m_texture;
  TextureArray m_blend_array_texture;
};
