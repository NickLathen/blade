#include <cassert>
#include <fstream>
#include <sstream>

#include "WdtLoader.hpp"

WdtData LoadWdt(const std::string &path) {
  WdtData result;
  std::ifstream file_stream{path, std::ios::binary};
  if (!file_stream.is_open()) {
    throw std::runtime_error("Could not open file: " + path);
  }
  file_stream.seekg(0, std::ios::end);
  std::streamsize file_size = file_stream.tellg();
  file_stream.seekg(0, std::ios::beg);
  while (file_stream.tellg() < file_size) {
    ChunkHeader header;
    file_stream.read((char *)&header, sizeof(header));
    switch (header.tag) {
    case wdt_MVER: {
      file_stream.read((char *)&result.mver, sizeof(result.mver));
      assert(result.mver.version == 18);
      break;
    }
    case wdt_MPHD: {
      file_stream.read((char *)&result.mphd, sizeof(result.mphd));
      break;
    }
    case wdt_MAIN: {
      file_stream.read((char *)&result.main, sizeof(result.main));
      break;
    }
    case wdt_MAID: {
      file_stream.read((char *)&result.maid, sizeof(result.maid));
      break;
    }
    default: {
      throw std::runtime_error("Unsupported chunk type");
    }
    }
  }
  return result;
};