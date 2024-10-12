#pragma once

#include <bitset>
#include <stdint.h>

inline constexpr uint32_t ChunkType(const char *t) {
  return t[0] << 24 | t[1] << 16 | t[2] << 8 | t[3];
}

inline std::string ChunkTypeToString(uint32_t c) {
  char out[5];
  out[0] = c >> 24 & 0xFF;
  out[1] = c >> 16 & 0xFF;
  out[2] = c >> 8 & 0xFF;
  out[3] = c & 0xFF;
  out[4] = '\0';
  return std::string(out);
}

enum ChunkType : uint32_t {
  // WDT Chunks
  wdt_MVER = ChunkType("MVER"),
  wdt_MPHD = ChunkType("MPHD"),
  wdt_MAIN = ChunkType("MAIN"),
  wdt_MAID = ChunkType("MAID"),
  wdt_MWMO = ChunkType("MWMO"),
  wdt_MODF = ChunkType("MODF"),

  // Optional WMO Chunks
  //   MLIQ
  //   MFOG
  //   MOPV
  //   MOPR
  //   MOPT
  //   MOCV
  //   MDAL

  // WMO Chunks
  wmo_MVER = ChunkType("MVER"),
  wmo_MOHD = ChunkType("MOHD"),
  wmo_MOTX = ChunkType("MOTX"),
  wmo_MFOG = ChunkType("MFOG"),
  wmo_MOMT = ChunkType("MOMT"),
  wmo_MOPV = ChunkType("MOPV"),
  wmo_MOPT = ChunkType("MOPT"),
  wmo_MOPR = ChunkType("MOPR"),
  wmo_MOGN = ChunkType("MOGN"),
  wmo_MOGI = ChunkType("MOGI"),
  wmo_MODS = ChunkType("MODS"),
  wmo_MODI = ChunkType("MODI"),
  wmo_MODN = ChunkType("MODN"),
  wmo_MODD = ChunkType("MODD"),
  wmo_GFID = ChunkType("GFID"),
  wmo_MLIQ = ChunkType("MLIQ"),
  wmo_MOCV = ChunkType("MOCV"),
  wmo_MDAL = ChunkType("MDAL"),
  wmo_MOGP = ChunkType("MOGP"),
  wmo_MOVI = ChunkType("MOVI"),
  wmo_MOVT = ChunkType("MOVT"),
  wmo_MOTV = ChunkType("MOTV"),
  wmo_MONR = ChunkType("MONR"),
  wmo_MOBA = ChunkType("MOBA"),
  wmo_MOPY = ChunkType("MOPY"),

  // ADT Root Chunks
  adt_MCNK = ChunkType("MCNK"),
  adt_MHDR = ChunkType("MHDR"),
  adt_MH20 = ChunkType("MH20"),
  adt_MVER = ChunkType("MVER"),

  // Root MCNK Chunks
  adt_MCVT = ChunkType("MCVT"), // vertices
  adt_MCNR = ChunkType("MCNR"), // normals
  adt_MCSE = ChunkType("MCSE"), // sound emitter
  adt_MCCV = ChunkType("MCCV"), // vertex shading
  adt_MCBB = ChunkType("MCBB"), // blend batches

  // ADT Tex Chunks
  // MCNK
  // MVER
  adt_MAMP = ChunkType("MAMP"),
  adt_MDID = ChunkType("MDID"),
  adt_MHID = ChunkType("MHID"),
  // adt_MTEX = ChunkType("MTEX"),
  // adt_MTXP = ChunkType("MTXP"),

  // Tex MCNK Chunk
  adt_MCLY = ChunkType("MCLY"),
  adt_MCAL = ChunkType("MCAL"),
  adt_MCSH = ChunkType("MCSH"),

  // Adt Obj Chunk
  // MVER
  adt_MMDX = ChunkType("MMDX"),
  adt_MMID = ChunkType("MMID"),
  adt_MWMO = ChunkType("MWMO"),
  adt_MWID = ChunkType("MWID"),
  adt_MDDF = ChunkType("MDDF"),
  adt_MODF = ChunkType("MODF"),
  adt_MWDS = ChunkType("MWDS"),
  adt_MLDD = ChunkType("MLDD"),
  adt_MLDX = ChunkType("MLDX"),
  adt_MLMD = ChunkType("MLMD"),
  adt_MLMX = ChunkType("MLMX"),

  // Obj MCNK Chunk
  adt_MCRW = ChunkType("MCRW"),
  adt_MCRD = ChunkType("MCRD"),

};

struct ChunkHeader {
  uint32_t tag;
  uint32_t size;
};

struct MVERHeader {
  uint32_t version;
};

struct MHDRHeader {
  enum MHDRFlags {
    mhdr_MFBO = 1,      // contains a MFBO chunk.
    mhdr_northrend = 2, // is set for some northrend ones.
  };
  uint32_t flags;
  uint32_t mcin; // MCIN*, Cata+: obviously gone. probably all offsets gone,
                 // except mh2o(which remains in root file).
  uint32_t mtex; // MTEX*
  uint32_t mmdx; // MMDX*
  uint32_t mmid; // MMID*
  uint32_t mwmo; // MWMO*
  uint32_t mwid; // MWID*
  uint32_t mddf; // MDDF*
  uint32_t modf; // MODF*
  uint32_t mfbo; // MFBO*   this is only set if flags & mhdr_MFBO.
  uint32_t mh2o; // MH2O*
  uint32_t mtxf; // MTXF*
  uint8_t mamp_value; // Cata+, explicit MAMP chunk overrides data
  uint8_t padding[3];
  uint32_t unused[3];
};

struct MCNKHeader {
  struct {
    uint32_t has_mcsh : 1;
    uint32_t impass : 1;
    uint32_t lq_river : 1;
    uint32_t lq_ocean : 1;
    uint32_t lq_magma : 1;
    uint32_t lq_slime : 1;
    uint32_t has_mccv : 1;
    uint32_t unknown_0x80 : 1;
    uint32_t : 7; // not set in 6.2.0.20338
    uint32_t
        do_not_fix_alpha_map : 1; // "fix" alpha maps in MCAL and MCSH (4 bit
                                  // alpha maps are 63*63 instead of 64*64). If
                                  // this flag is not set, the MCAL format *has*
                                  // to be unfixed4444, otherwise
                                  // UnpackAlphaShadowBits will assert.
    uint32_t high_res_holes : 1;  // Since ~5.3 WoW uses full 64-bit to store
                                  // holes for each tile if this flag is set.
    uint32_t : 15;                // not set in 6.2.0.20338
  } flags;

  /*0x004*/ uint32_t IndexX;
  /*0x008*/ uint32_t IndexY;

  /*0x00C*/ uint32_t nLayers; // maximum 4
  /*0x010*/ uint32_t nDoodadRefs;
  uint64_t holes_high_res; // only used with flags.high_res_holes
  /*0x01C*/ uint32_t ofsLayer;
  /*0x020*/ uint32_t ofsRefs;
  /*0x024*/ uint32_t ofsAlpha;
  /*0x028*/ uint32_t sizeAlpha;
  /*0x02C*/ uint32_t ofsShadow; // only with flags.has_mcsh
  /*0x030*/ uint32_t sizeShadow;
  /*0x034*/ uint32_t
      areaid; // in alpha: both zone id and sub zone id, as uint16s.
  /*0x038*/ uint32_t nMapObjRefs;
  /*0x03C*/ uint16_t holes_low_res;
  /*0x03E*/ uint16_t unknown_but_used; // in alpha: padding
  /*0x040*/ std::bitset<32>
      ReallyLowQualityTextureingMap; // "predTex", It is used to determine
                                     // which detail doodads to show. Values
                                     // are an array of two bit unsigned
                                     // integers, naming the layer.
  /*0x050*/ std::bitset<16> noEffectDoodad; // doodads disabled if 1; WoD: may
                                            // be an explicit MCDD chunk
  /*0x058*/ uint32_t ofsSndEmitters;
  /*0x05C*/ uint32_t nSndEmitters; // will be set to 0 in the client if
                                   // ofsSndEmitters doesn't point to MCSE!
  /*0x060*/ uint32_t ofsLiquid;
  /*0x064*/ uint32_t sizeLiquid; // 8 when not used; only read if >8.

  // in alpha, remainder is padding but unused.

  /*0x068*/ float position[3]; // position of upper left corner
  /*0x074*/ uint32_t ofsMCCV;  // only with flags.has_mccv, had uint32_t
                               // textureId; in ObscuR's structure.
  /*0x078*/ uint32_t ofsMCLV;  // introduced in Cataclysm
  /*0x07C*/ uint32_t unused;   // currently unused
  /*0x080*/
};

struct MCVTHeader {            // vertex heights, relative to position
  float height[9 * 9 + 8 * 8]; // Real-time Optimally Adapting Mesh (ROAM)
};

struct MCNREntry {
  int8_t normal[3]; // normalized. X, Z, Y. 127 == 1.0, -127 == -1.0.
};

struct MCNRHeader {
  MCNREntry entries[9 * 9 + 8 * 8];
  uint8_t
      padding[13]; // this data is not included in the MCNR chunk but additional
                   // data which purpose is unknown. 0.5.3.3368 lists this as
                   // padding always 0 112 245  18 0  8 0 0  0 84  245 18 0.
                   // Nobody yet found a different pattern. The data is not
                   // derived from the normals. It also does not seem that the
                   // client reads this data. --Schlumpf (talk) 23:01, 26 July
                   // 2015 (UTC) While stated that this data is not "included in
                   // the MCNR chunk", the chunk-size defined for the MCNR chunk
                   // does cover this data. --Kruithne Feb 2016
                   // ... from Cataclysm only (on LK files and before, MCNR
                   // defined size is 435 and not 448) Mjollna (talk)
};

struct MAMPHeader {
  char fred; // texture_size = 64 / (2^mamp_value). either defined here or in
             // MHDR.mamp_value.
  char padding[3];
};
struct MDIDHeader {
  uint32_t file_data_id;
};

struct MHIDHeader {
  uint32_t file_data_id;
};

struct MCLYFlags {
  uint32_t animation_rotation : 3; // each tick is 45°
  uint32_t animation_speed : 3;
  uint32_t animation_enabled : 1;
  uint32_t overbright : 1;    // This will make the texture way brighter. Used
                              // for lava to make it "glow".
  uint32_t use_alpha_map : 1; // set for every layer after the first
  uint32_t alpha_map_compressed : 1;    // see MCAL chunk description
  uint32_t use_cube_map_reflection : 1; // This makes the layer behave like its
                                        // a reflection of the skybox. See below
  uint32_t unknown_0x800 : 1;  // WoD?+ if either of 0x800 or 0x1000 is set,
                               // texture effects' texture_scale is applied
  uint32_t unknown_0x1000 : 1; // WoD?+ see 0x800
  uint32_t : 19;
};

struct MCLYHeader {
  uint32_t textureId;
  MCLYFlags flags;
  uint32_t offsetInMCAL;
  uint32_t effectId; // 0xFFFFFFFF for none, in alpha: uint16_t + padding
};

struct MCSHHeader {
  std::bitset<64 * 64> shadow_map;
  // or 63x63 with the last column&row&cell auto-filled as detailed in MCAL.
};

enum MDDFFlags : uint16_t {
  mddf_biodome = 1, // this sets internal flags to | 0x800 (WDOODADDEF.var0xC)
  mddf_shrubbery =
      2, // the actual meaning of these is unknown to me. maybe biodome is for
         // really big M2s. 6.0.1.18179 seems not to check  for this flag
  mddf_unk_4 = 0x4, // Legion+ᵘ
  mddf_unk_8 = 0x8, // Legion+ᵘ
  mddf_unk_10 =
      0x10, // Shadowlands+ᵘ observed in Shadowlands 9.2.7, sets flag 0x4 on the
            // PVS Doodad. May have been added earlier like the rest.
  SMDoodadDefFlag_liquidKnown = 0x20, // Legion+ᵘ
  mddf_entry_is_filedata_id =
      0x40,             // Legion+ᵘ nameId is a file data id to directly load
  mddf_unk_100 = 0x100, // Legion+ᵘ
  mddf_accept_proj_textures = 0x1000, // Legion+ᵘ
};

struct MDDFHeader {
  uint32_t nameId; // references an entry in the MMID chunk, specifying the
                   // model to use. if flag mddf_entry_is_filedata_id is set, a
                   // file data id instead, ignoring MMID.
  uint32_t uniqueId; // this ID should be unique for all ADTs currently loaded.
                     // Best, they are unique for the whole map. Blizzard has
                     // these unique for the whole game.
  float position[3]; // This is relative to a corner of the map. Subtract 17066
                     // from the non vertical values and you should start to see
                     // something that makes sense. You'll then likely have to
                     // negate one of the non vertical values in whatever
                     // coordinate system you're using to finally move it into
                     // place.
  float rotation[3]; // degrees. This is not the same coordinate system
                     // orientation like the ADT itself! (see history.)
  uint16_t scale;    // 1024 is the default size equaling 1.0f.
  MDDFFlags flags;   // values from enum MDDFFlags.
};

enum MODFFlags : uint16_t {
  modf_destroyable =
      0x1, // set for destroyable buildings like the tower in DeathknightStart.
           // This makes it a server-controllable game object.
  modf_use_lod =
      0x2, // WoD(?)+: also load _LOD1.WMO for use dependent on distance
  modf_unk_has_scale = 0x4, // Legion+: if this flag is set then use scale =
                            // scale / 1024, otherwise scale is 1.0
  modf_entry_is_filedata_id =
      0x8, // Legion+: nameId is a file data id to directly load
           // //SMMapObjDef::FLAG_FILEDATAID
  modf_use_sets_from_mwds =
      0x80 // Shadowlands+: if set, doodad set indexes of which to load should
           // be taken from MWDS chunk
};
struct MODFHeader {
  uint32_t nameId;   // references an entry in the MWID chunk, specifying the
                     // model to use.
  uint32_t uniqueId; // this ID should be unique for all ADTs currently loaded.
                     // Best, they are unique for the whole map.
  float position[3];
  float rotation[3]; // same as in MDDF.
  float extents[6];  // position plus the transformed wmo bounding box. used for
                     // defining if they are rendered as well as collision.
  MODFFlags flags;   // values from enum MODFFlags.
  uint16_t doodadSet; // which WMO doodad set is used. Traditionally references
                      // WMO#MODS_chunk, if modf_use_sets_from_mwds is set,
                      // references #MWDR_.28Shadowlands.2B.29
  uint16_t nameSet;   // which WMO name set is used. Used for renaming goldshire
                      // inn to northshire inn while using the same model.
  uint16_t scale;     // Legion+: scale, 1024 means 1 (same as MDDF). Padding in
                      // 0.5.3 alpha.
};