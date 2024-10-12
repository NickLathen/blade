#include <cassert>
#include <fstream>
#include <sstream>

#include "AdtLoader.hpp"

AdtData LoadAdt(const std::string &path) {
  AdtData result;
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
    case adt_MVER: {
      file_stream.read((char *)&result.mver, sizeof(result.mver));
      assert(result.mver.version == 18);
      break;
    }
    case adt_MHDR: {
      file_stream.read((char *)&result.mhdr, sizeof(result.mhdr));
      break;
    }
    case adt_MCNK: {
      MCNKData mcnk_data;
      uint32_t end_of_mcnk = file_stream.tellg() + std::streampos(header.size);
      file_stream.read((char *)&mcnk_data.header, sizeof(mcnk_data.header));
      while (file_stream.tellg() < end_of_mcnk) {
        ChunkHeader subheader;
        file_stream.read((char *)&subheader, sizeof(subheader));
        switch (subheader.tag) {
        case adt_MCVT: {
          file_stream.read((char *)&mcnk_data.mcvt, sizeof(mcnk_data.mcvt));
          break;
        }
        case adt_MCNR: {
          file_stream.read((char *)&mcnk_data.mcnr, sizeof(mcnk_data.mcnr));
          break;
        }
        case adt_MCSE: {
          // skip sound emitter
          file_stream.seekg(subheader.size, std::ios_base::seekdir::_S_cur);
          break;
        }
        default: {
          printf("unsupported mcnk type=%s\n",
                 ChunkTypeToString(subheader.tag).c_str());
          file_stream.seekg(subheader.size,
                            std::ios_base::seekdir::_S_cur); // skip
        }
        }
      }
      result.mcnk.push_back(std::move(mcnk_data));
      break;
    }
    default: {
      printf("unsupported chunk type=%s\n",
             ChunkTypeToString(header.tag).c_str());
      file_stream.seekg(header.size, std::ios_base::seekdir::_S_cur); // skip
    }
    }
  }
  return result;
}

AdtTexData LoadAdtTex(const std::string &path) {
  AdtTexData result;
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
    case adt_MVER: {
      file_stream.read((char *)&result.mver, sizeof(result.mver));
      assert(result.mver.version == 18);
      break;
    }
    case adt_MAMP: {
      file_stream.read((char *)&result.mamp, sizeof(result.mamp));
      break;
    }
    case adt_MDID: {
      size_t size = header.size / sizeof(MDIDHeader);
      MDIDHeader mdid[size];
      file_stream.read((char *)mdid, header.size);
      result.mdid.insert(result.mdid.end(), mdid, mdid + size);
      break;
    }
    case adt_MHID: {
      size_t size = header.size / sizeof(MHIDHeader);
      MHIDHeader mhid[size];
      file_stream.read((char *)mhid, header.size);
      result.mhid.insert(result.mhid.end(), mhid, mhid + size);
      break;
    }
    case adt_MCNK: {
      uint32_t end_of_mcnk = file_stream.tellg() + std::streampos(header.size);
      MCNKTexData chunk_data;
      while (file_stream.tellg() < end_of_mcnk) {
        ChunkHeader subheader;
        file_stream.read((char *)&subheader, sizeof(subheader));
        switch (subheader.tag) {
        case adt_MCLY: {
          size_t size = subheader.size / sizeof(MCLYHeader);
          MCLYHeader mcly[size];
          file_stream.read((char *)mcly, subheader.size);
          chunk_data.mcly.insert(chunk_data.mcly.end(), mcly, mcly + size);
          break;
        }
        case adt_MCSH: {
          file_stream.read((char *)&chunk_data.mcsh, sizeof(chunk_data.mcsh));
          break;
        }
        case adt_MCAL: {
          size_t size = subheader.size;
          unsigned char mcal[size];
          file_stream.read((char *)mcal, size);
          chunk_data.mcal.insert(chunk_data.mcal.end(), mcal, mcal + size);
          break;
        }
        default: {
          printf("unsupported mcnk type=%s\n",
                 ChunkTypeToString(subheader.tag).c_str());
          file_stream.seekg(subheader.size,
                            std::ios_base::seekdir::_S_cur); // skip
        }
        }
      }
      result.mcnk.push_back(std::move(chunk_data));
      break;
    }
    default: {
      throw std::runtime_error("Unsupported chunk type");
    }
    }
  }
  return result;
};

AdtObjData LoadAdtObj(const std::string &path) {
  AdtObjData result;
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
    case adt_MVER: {
      file_stream.read((char *)&result.mver, sizeof(result.mver));
      assert(result.mver.version == 18);
      break;
    }
    case adt_MDDF: {
      if (header.size > 0) {
        size_t size = header.size / sizeof(MDDFHeader);
        MDDFHeader mddf[size];
        file_stream.read((char *)mddf, header.size);
        result.mddf.insert(result.mddf.end(), mddf, mddf + size);
      }
      break;
    }
    case adt_MODF: {
      if (header.size > 0) {
        size_t size = header.size / sizeof(MODFHeader);
        MODFHeader modf[size];
        file_stream.read((char *)modf, header.size);
        result.modf.insert(result.modf.end(), modf, modf + size);
      }
      break;
    }
    case adt_MLDD:
    case adt_MLDX:
    case adt_MLMD:
    case adt_MLMX: {
      // skip lod_object_extents
      // skip lod_object_defs
      // skip lod_doodad_refs
      // skip lod_doodad_extents
      file_stream.seekg(header.size, std::ios_base::seekdir::_S_cur);
      break;
    }
    case adt_MCNK: {
      uint32_t end_of_mcnk = file_stream.tellg() + std::streampos(header.size);
      MCNKObjData chunk_data;
      while (file_stream.tellg() < end_of_mcnk) {
        ChunkHeader subheader;
        file_stream.read((char *)&subheader, sizeof(subheader));
        switch (subheader.tag) {
        case adt_MCRW: {
          size_t size = subheader.size / sizeof(uint32_t);
          uint32_t out[size];
          file_stream.read((char *)out, subheader.size);
          chunk_data.mcrw.insert(chunk_data.mcrw.end(), out, out + size);
          break;
        }
        case adt_MCRD: {
          size_t size = subheader.size / sizeof(uint32_t);
          uint32_t out[size];
          file_stream.read((char *)out, subheader.size);
          chunk_data.mcrd.insert(chunk_data.mcrd.end(), out, out + size);
          break;
        }
        default: {
          printf("Unsupported mcnk type=%s\n",
                 ChunkTypeToString(subheader.tag).c_str());
          file_stream.seekg(subheader.size, std::ios_base::seekdir::_S_cur);
        }
        }
      }
      result.mcnk.push_back(std::move(chunk_data));
      break;
    }
    default: {
      printf("Unsupported chunk type=%s\n",
             ChunkTypeToString(header.tag).c_str());
      file_stream.seekg(header.size, std::ios_base::seekdir::_S_cur);
    }
    }
  }
  return result;
};