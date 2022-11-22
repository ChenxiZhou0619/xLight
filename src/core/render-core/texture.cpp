#include "texture.h"

#include "stb/stb_image.h"

SpectrumRGB **TextureLoader::toRGB(const std::string &filepath, int &width,
                                   int &height, int &channels) {
  bool isHdr = stbi_is_hdr(filepath.c_str());
  stbi_info(filepath.c_str(), &width, &height, &channels);
  SpectrumRGB **data = new SpectrumRGB *[width];
  for (int i = 0; i < width; ++i) {
    data[i] = new SpectrumRGB[height]();
  }

  if (isHdr) {
    float *tmp = stbi_loadf(filepath.c_str(), &width, &height, &channels, 0);

    for (int x = 0; x < width; ++x) {
      for (int y = 0; y < height; ++y) {
        int offset = x * 3 + y * width * 3;
        data[x][y] = SpectrumRGB{tmp[offset], tmp[offset + 1], tmp[offset + 2]};
      }
    }
    stbi_image_free(tmp);
  } else {
    unsigned char *tmp =
        stbi_load(filepath.c_str(), &width, &height, &channels, 3);

    for (int x = 0; x < width; ++x) {
      for (int y = 0; y < height; ++y) {
        int offset = x * 3 + y * width * 3;
        data[x][y] = SpectrumRGB{(float)tmp[offset] / 255.f,
                                 (float)tmp[offset + 1] / 255.f,
                                 (float)tmp[offset + 2] / 255.f};
      }
    }
    stbi_image_free(tmp);
  }
  return data;
}