#include "RPWowTerrain.hpp"

#include <array>
#include <glm/glm.hpp>
#include <unordered_map>

#include "AdtLoader.hpp"
#include "BlpLoader.hpp"
#include "GpuTexture.hpp"
#include "WdtLoader.hpp"
#include "WowItemLookup.hpp"

std::string GetWowItemPath(uint32_t id) {
  std::string path = GetWowItem(id);
  size_t worldPos = path.find("world/maps/");
  if (worldPos != path.npos) {
    path.replace(worldPos, 6, "");
  }
  size_t spacePos = path.find(" ");
  while (spacePos != path.npos) {
    path.replace(spacePos, 1, "");
    spacePos = path.find(" ");
  }
  return "/home/nick/wow.export/" + path;
}

void RPWowTerrain::LoadTerrainAdt(
    const std::string &wdt_path, const std::string &adt_name,
    MultiTextureArray &multi_texture_array,
    std::unordered_map<uint32_t, uint32_t> &wow_file_data_id_texture_map) {
  std::vector<int> texture_slots{};
  WdtData wd{LoadWdt(wdt_path)};
  uint i = 0;
  for (; i < 64 * 64; i++) {
    std::string root_file_path{GetWowItem(wd.maid[i].rootADT)};
    if (root_file_path.size() == 0)
      continue;
    if (root_file_path.find(adt_name) != root_file_path.npos)
      break;
  }
  AdtData ad{LoadAdt(GetWowItemPath(wd.maid[i].rootADT))};
  AdtTexData atd{LoadAdtTex(GetWowItemPath(wd.maid[i].tex0ADT))};
  AdtObjData aod0{LoadAdtObj(GetWowItemPath(wd.maid[i].obj0ADT))};
  AdtObjData aod1{LoadAdtObj(GetWowItemPath(wd.maid[i].obj1ADT))};

  // atd.mcnk[i].mcly[j].flags.*
  // animation_enabled
  // overbright
  // use_alpha_map

  m_corner_position[0] = ad.mcnk[0].header.position[0];
  m_corner_position[1] = ad.mcnk[0].header.position[1] - 533.3333;

  // load m_textures from atd.mdid[i]
  GpuTexParameters terrain_gtp{{GL_TEXTURE_WRAP_S, GL_REPEAT},
                               {GL_TEXTURE_WRAP_T, GL_REPEAT},
                               {GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR},
                               {GL_TEXTURE_MAG_FILTER, GL_LINEAR}};
  for (MDIDHeader h : atd.mdid) {
    auto r = wow_file_data_id_texture_map.find(h.file_data_id);
    if (r != wow_file_data_id_texture_map.end()) {
      texture_slots.push_back((*r).second);
      continue;
    }
    std::string path = GetWowItemPath(h.file_data_id);
    printf("Loading %s...\n", path.c_str());
    ImageData image{LoadBlp(path)};
    texture_slots.push_back(multi_texture_array.PushImage(image, terrain_gtp));
    wow_file_data_id_texture_map.insert({h.file_data_id, texture_slots.back()});
  }

  // build heightmap (DOUBLE PACKED)
  struct HeightNormalMap {
    float height;
    uint32_t packed_normal;
  };
  std::vector<HeightNormalMap> heightmap;
  for (size_t i = 0; i < 256; i++) {
    const MCNKData &d{ad.mcnk[i]};
    float base_height = ad.mcnk[i].header.position[2];
    for (size_t j = 0; j < (9 * 9 + 8 * 8); j++) {
      const int8_t *inormal = d.mcnr.entries[j].normal;
      glm::vec4 normal{float(inormal[0]) / 127.0, float(inormal[1]) / 127.0,
                       float(inormal[2]) / 127.0, 0.0};
      uint32_t packed_normal = glm::packSnorm4x8(normal);
      heightmap.emplace_back(
          (HeightNormalMap){base_height + d.mcvt.height[j], packed_normal});
    }
  }
  m_ssbo_heightmap.BufferData(heightmap.size() * sizeof(HeightNormalMap),
                              &heightmap[0], GL_STATIC_DRAW);
  m_ssbo_heightmap.Unbind();

  typedef std::array<uint32_t, 4> TextureSlotMap;
  // build texture slot map
  // atd.mcnk appears to be column wise, but we convert back to row wise
  std::vector<uint32_t> holes{};
  holes.reserve(2 * 256);
  std::vector<TextureSlotMap> slotmaps{};
  std::vector<TextureSlotMap> alpha_slotmaps{};
  std::vector<ImageData> alpha_images{};
  std::vector<ImageData> shadow_images{};
  for (size_t row = 0; row < 16; row++) {
    for (size_t col = 0; col < 16; col++) {
      size_t i = col * 16 + row;

      // holes
      holes.push_back(ad.mcnk[i].header.high_res_holes_lower);
      holes.push_back(ad.mcnk[i].header.high_res_holes_upper);

      std::vector<unsigned char> shadow_texture_data{};
      shadow_texture_data.reserve(64 * 64);
      for (size_t ii = 0; ii < 64; ii++) {
        for (size_t jj = 0; jj < 64; jj++) {
          unsigned char bitvalue =
              atd.mcnk[i].mcsh.shadow_map.test(jj * 64 + ii);
          shadow_texture_data.push_back(bitvalue * 255);
        }
      }
      shadow_images.emplace_back(
          ImageData(&shadow_texture_data[0], 64, 64, id_R8));

      // texture slot maps
      TextureSlotMap slotmap{};
      for (size_t j = 0; j < atd.mcnk[i].mcly.size(); j++) {
        slotmap[j] = texture_slots[atd.mcnk[i].mcly[j].textureId];
      }
      slotmaps.push_back(slotmap);

      // alpha slot maps
      TextureSlotMap alpha_slotmap{};
      alpha_slotmap[0] = atd.mcnk[i].mcly.size();
      size_t kAlphaSize = 64;
      for (size_t j = 1; j < atd.mcnk[i].mcly.size(); j++) {

        std::vector<uint8_t> buff(kAlphaSize * kAlphaSize);
        size_t offset = atd.mcnk[i].mcly[j].offsetInMCAL;
        for (size_t row = 0; row < kAlphaSize; row++) {
          for (size_t col = 0; col < kAlphaSize; col++) {
            size_t buff_index = row * kAlphaSize + col;
            size_t col_major_index = col * kAlphaSize + row;
            size_t mcal_index = col_major_index / 2 + offset;
            uint8_t alpha = atd.mcnk[i].mcal[mcal_index];
            if (col_major_index % 2 == 1) {
              alpha = alpha >> 4;
            }
            buff[buff_index] = (alpha & 0xF) << 4; // scale up to 8 bit
          }
        }
        alpha_images.emplace_back(
            ImageData(&buff[0], kAlphaSize, kAlphaSize, id_R8));
        alpha_slotmap[j] = alpha_images.size() - 1;
      }
      alpha_slotmaps.push_back(alpha_slotmap);
    }
  }
  m_ssbo_holes.BufferData(holes.size() * sizeof(holes[0]), &holes[0],
                          GL_STATIC_DRAW);
  m_ssbo_holes.Unbind();

  m_ssbo_texture_slots.BufferData(slotmaps.size() * sizeof(slotmaps[0]),
                                  &slotmaps[0], GL_STATIC_DRAW);
  m_ssbo_texture_slots.Unbind();

  TextureArray alpha_texture_array{64, 64, 1, (int)alpha_images.size()};
  for (auto &i : alpha_images) {
    alpha_texture_array.PushImage(i);
  }
  alpha_texture_array.GetTexture().GenerateMipmap(GL_TEXTURE_2D_ARRAY);
  m_texture_array.push_back(std::move(alpha_texture_array));

  TextureArray shadow_texture_array{64, 64, 1, (int)shadow_images.size()};
  for (auto &i : shadow_images) {
    shadow_texture_array.PushImage(i);
  }
  shadow_texture_array.GetTexture().GenerateMipmap(GL_TEXTURE_2D_ARRAY);
  m_texture_array.push_back(std::move(shadow_texture_array));

  m_ssbo_alpha_slots.BufferData(alpha_slotmaps.size() *
                                    sizeof(alpha_slotmaps[0]),
                                &alpha_slotmaps[0], GL_STATIC_DRAW);
  m_ssbo_alpha_slots.Unbind();

  m_vao.BindVertexArray();
  m_vao.Unbind();
}

RPWowTerrain::RPWowTerrain(
    const std::string &wdt_path, const std::string &adt_name,
    MultiTextureArray &multi_texture_array,
    std::unordered_map<uint32_t, uint32_t> &wow_file_data_id_texture_map) {
  LoadTerrainAdt(wdt_path, adt_name, multi_texture_array,
                 wow_file_data_id_texture_map);
};
void RPWowTerrain::DrawVertices() const {
  constexpr size_t kBoxVerts = 10;
  constexpr size_t kBoxesPerBlock = 8 * 8;
  constexpr size_t kBlocksPerChunk = 16 * 16;
  constexpr size_t kBlockVerts = kBoxVerts * kBoxesPerBlock;
  constexpr size_t kElements = kBlockVerts * kBlocksPerChunk;

  m_vao.BindVertexArray();
  glDrawArrays(GL_TRIANGLE_STRIP, 0, kElements);
  m_vao.Unbind();
};
