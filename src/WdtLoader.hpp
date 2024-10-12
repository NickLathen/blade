#pragma once

#include "WowLoader.hpp"

struct MPHDHeader {
  uint32_t flags;
  // only used >= BFA
  uint32_t lgtFileDataID;
  uint32_t occFileDataID;
  uint32_t fogsFileDataID;
  uint32_t mpvFileDataID;
  uint32_t texFileDataID;
  uint32_t wdlFileDataID;
  uint32_t pd4FileDataID;
};

struct MAINHeader {
  uint32_t has_adt;
  uint32_t async_id; // only set during runtime.
};

struct MAIDHeader {
  uint32_t rootADT;        // reference to fdid of mapname_xx_yy.adt
  uint32_t obj0ADT;        // reference to fdid of mapname_xx_yy_obj0.adt
  uint32_t obj1ADT;        // reference to fdid of mapname_xx_yy_obj1.adt
  uint32_t tex0ADT;        // reference to fdid of mapname_xx_yy_tex0.adt
  uint32_t lodADT;         // reference to fdid of mapname_xx_yy_lod.adt
  uint32_t mapTexture;     // reference to fdid of mapname_xx_yy.blp
  uint32_t mapTextureN;    // reference to fdid of mapname_xx_yy_n.blp
  uint32_t minimapTexture; // reference to fdid of mapxx_yy.blp
};

struct WdtData {
  MVERHeader mver;
  MPHDHeader mphd;
  MAINHeader main[64 * 64];
  MAIDHeader maid[64 * 64];
};

WdtData LoadWdt(const std::string &path);