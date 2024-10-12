#include <fstream>
#include <unordered_map>

#include "WowItemLookup.hpp"

std::unordered_map<uint32_t, std::string> kItemMap{};

void InitWowItemLookup() {
  std::ifstream file_stream{"assets/wow/community-listfile.csv"};
  file_stream.seekg(0, std::ios::end);
  std::streamsize file_size = file_stream.tellg();
  file_stream.seekg(0, std::ios::beg);
  uint kBuffSize = 1024;
  char buffer[kBuffSize];
  while (file_stream.tellg() != file_size) {
    file_stream.getline(buffer, kBuffSize, ';');
    uint32_t key = std::stoul(buffer);
    file_stream.getline(buffer, kBuffSize, '\r');
    file_stream.seekg(1, std::ios::seekdir::_S_cur); // skip \n as well
    kItemMap.insert({key, std::string(buffer)});
  }
};

std::string GetWowItem(uint32_t id) {
  const auto &i = kItemMap.find(id);
  if (i != kItemMap.end()) {
    return i->second;
  }
  return "";
};