#pragma once

#include <string>
#include <unordered_map>

#include "GpuResources.hpp"
#include "GpuTexture.hpp"
#include "MultiTextureArray.hpp"
#include "utils.hpp"

class RPWowTerrain {
public:
  RPWowTerrain(
      const std::string &wdt_path, const std::string &adt_name,
      MultiTextureArray &multi_texture_array,
      std::unordered_map<uint32_t, uint32_t> &wow_file_data_id_texture_map);
  NEVER_COPY(RPWowTerrain);
  RPWowTerrain(RPWowTerrain &&other)
      : m_vao{std::move(other.m_vao)},
        m_ssbo_heightmap{std::move(other.m_ssbo_heightmap)},
        m_ssbo_texture_slots{std::move(other.m_ssbo_texture_slots)},
        m_ssbo_alpha_slots{std::move(other.m_ssbo_alpha_slots)},
        m_ssbo_holes{std::move(other.m_ssbo_holes)},
        m_corner_position{std::move(other.m_corner_position)},
        m_texture_array{std::move(other.m_texture_array)} {};
  const GpuTexture &GetAlphaTexture() const {
    return m_texture_array[0].GetTexture();
  };
  const GpuTexture &GetShadowTexture() const {
    return m_texture_array[1].GetTexture();
  };
  const GpuSSBO &GetHeightmapSSBO() const { return m_ssbo_heightmap; }
  const GpuSSBO &GetTextureSlotsSSBO() const { return m_ssbo_texture_slots; }
  const GpuSSBO &GetAlphaSlotsSSBO() const { return m_ssbo_alpha_slots; }
  const GpuSSBO &GetHolesSSBO() const { return m_ssbo_holes; }
  const glm::vec2 &GetCornerPosition() const { return m_corner_position; }
  void DrawVertices() const;

private:
  void LoadTerrainAdt(
      const std::string &wdt_path, const std::string &adt_name,
      MultiTextureArray &multi_texture_array,
      std::unordered_map<uint32_t, uint32_t> &wow_file_data_id_texture_map);
  GpuVAO m_vao;
  GpuSSBO m_ssbo_heightmap;     // heights + normals
  GpuSSBO m_ssbo_texture_slots; // map texture slots for each block
  GpuSSBO m_ssbo_alpha_slots;   // map alpha slots for each block
  GpuSSBO m_ssbo_holes;
  glm::vec2 m_corner_position{};
  std::vector<TextureArray> m_texture_array;
};
