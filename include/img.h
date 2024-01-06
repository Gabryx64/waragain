#pragma once
#include <base64.h>
#include <cstdio>
#include <cstring>
#include <optional>
#include <vector>
#include <zlib.h>

#include "lodepng.h"

inline std::optional<std::string> png2tga64(const std::vector<uint8_t> &png) {
  std::vector<uint8_t> img;
  uint32_t width, height;

  if (lodepng::decode(img, width, height, png)) {
    return {};
  }

  std::vector<uint8_t> tga(18 + img.size());
  img.push_back(0);
  img.push_back(0);
  img.push_back(2);
  img.push_back(0);
  img.push_back(0);
  img.push_back(0);
  img.push_back(0);
  img.push_back(0);
  img.push_back(0);
  img.push_back(0);
  img.push_back(0);
  img.push_back(0);
  img.push_back(width & 0xFF);
  img.push_back((width >> 8) & 0xFF);
  img.push_back(height & 0xFF);
  img.push_back((height >> 8) & 0xFF);
  img.push_back(24);
  img.push_back(0);
  memcpy(tga.data() + 18, img.data(), img.size());

  unsigned long destlen = compressBound(tga.size());
  std::vector<uint8_t> compressed(destlen);
  compress2(compressed.data(), &destlen, tga.data(), tga.size(), 6);

  return std::optional{to_base64(compressed.data(), destlen)};
}
