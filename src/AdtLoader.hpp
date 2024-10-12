#pragma once

#include <string>
#include <vector>

#include "WowLoader.hpp"

struct MCNKData {
  MCNKHeader header;
  MCVTHeader mcvt; // heights
  MCNRHeader mcnr; // normals
};

struct AdtData {
  MVERHeader mver;
  MHDRHeader mhdr;
  std::vector<MCNKData> mcnk;
};

struct MCNKTexData {
  std::vector<MCLYHeader> mcly;
  MCSHHeader mcsh;
  std::vector<unsigned char> mcal;
};

struct AdtTexData {
  MVERHeader mver;
  MAMPHeader mamp;
  std::vector<MDIDHeader> mdid;
  std::vector<MHIDHeader> mhid;
  std::vector<MCNKTexData> mcnk;
};

struct MCNKObjData {
  std::vector<uint32_t> mcrd; // mddf entry
  std::vector<uint32_t> mcrw; // modf entry
};

struct AdtObjData {
  MVERHeader mver;
  std::vector<MDDFHeader> mddf;
  std::vector<MODFHeader> modf;
  std::vector<MCNKObjData> mcnk;
};

AdtData LoadAdt(const std::string &path);

AdtTexData LoadAdtTex(const std::string &path);

AdtObjData LoadAdtObj(const std::string &path);