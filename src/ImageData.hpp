#pragma once

#include <fstream>
#include <memory>
#include <sstream>
#include <stb_image.h>
#include <string>
#include <vector>

enum ImageDataFormat {
  id_RGBA8,
  id_RGB8,
  id_R8,
};

class ImageData {
public:
  ImageData(const std::string &filename, int req_comp = 0);
  ImageData(const unsigned char *data, int width, int height,
            ImageDataFormat texture_format);
  inline void InvertYAxis() {
    for (int j = 0; j < height; j++) {
      for (int i = 0; i < width / 2; i++) {
        int x_mirror = width - 1 - i;
        int idx_a = (j * width + i) * num_channels;
        int idx_b = (j * width + x_mirror) * num_channels;
        for (int k = 0; k < num_channels; k++) {
          std::swap(m_data[idx_a + k], m_data[idx_b + k]);
        }
      }
    }
  }
  inline void InvertXAxis() {
    for (int j = 0; j < height / 2; j++) {
      int y_mirror = height - 1 - j;
      for (int i = 0; i < width; i++) {
        int idx_a = (j * width + i) * num_channels;
        int idx_b = (y_mirror * width + i) * num_channels;
        for (int k = 0; k < num_channels; k++) {
          std::swap(m_data[idx_a + k], m_data[idx_b + k]);
        }
      }
    }
  }
  inline void Rotate180() {
    for (int j = 0; j < height / 2; j++) {
      int y_mirror = height - 1 - j;
      for (int i = 0; i < width; i++) {
        int x_mirror = width - 1 - i;
        int idx_a = (j * width + i) * num_channels;
        int idx_b = (y_mirror * width + x_mirror) * num_channels;
        for (int k = 0; k < num_channels; k++) {
          std::swap(m_data[idx_a + k], m_data[idx_b + k]);
        }
      }
    }
  }
  const unsigned char *get() const { return m_data.data(); };
  int width;
  int height;
  int num_channels;

private:
  std::vector<unsigned char> m_data;
};