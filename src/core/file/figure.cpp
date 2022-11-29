#include "figure.h"

#include <core/geometry/point.h>
#include <core/render-core/spectrum.h>
#include <string.h>

#include <iostream>
#include <memory>
#include <vector>

#include "image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define TINYEXR_IMPLEMENTATION
#include <tinyexr/tinyexr.h>

Figure::Figure(int _width, int _height)
    : width(_width), height(_height), channels(3) {
  // allocate the space
  data = new PixelValue[width * height];
}

Figure::Figure(const std::string &filename) {
  bool isHdr = stbi_is_hdr(filename.c_str());
  stbi_info(filename.c_str(), &width, &height, &channels);
  assert(channels == 3);

  // allocate the space
  data = new PixelValue[width * height];

  // load the image
  if (isHdr) {
    float *tmp = stbi_loadf(filename.c_str(), &width, &height, &channels, 0);
    assert(tmp);
    memcpy(data, tmp, sizeof(float) * 3 * width * height);
    stbi_image_free(tmp);
  } else {
    u_int8_t *tmp = stbi_load(filename.c_str(), &width, &height, &channels, 0);
    assert(tmp);
    for (int x = 0; x < width; ++x) {
      for (int y = 0; y < height; ++y) {
        int offset = x + y * width;
        for (int channel = 0; channel < 3; ++channel) {
          data[offset][channel] = (float)tmp[offset * 3 + channel] / 255;
        }
      }
    }
    stbi_image_free(tmp);
  }
}

Figure::~Figure() {
  // free the space
  delete[] data;
}

void Figure::saveAsPng(const std::string &filename) const {
  char *raw = new char[width * height * 3]();

  auto clamp = [](float f) { return std::min(int(f * 255), 255); };

  for (int x = 0; x < width; ++x) {
    for (int y = 0; y < height; ++y) {
      int offset = x + y * width;
      for (int channel = 0; channel < 3; ++channel) {
        raw[3 * offset + channel] = clamp(data[offset][channel]);
      }
    }
  }

  stbi_write_png(filename.c_str(), width, height, 3, raw, 0);
  delete[] raw;
  return;
}

void Figure::saveAsHdr(const std::string &filename) const {
  stbi_write_hdr(filename.c_str(), width, height, 3, (float *)data);
}

void Figure::saveAsExr(const std::string &filename) const {
  EXRHeader header;
  InitEXRHeader(&header);

  EXRImage image;
  InitEXRImage(&image);

  image.num_channels = 3;

  std::vector<float> images[3];
  images[0].resize(width * height);
  images[1].resize(width * height);
  images[2].resize(width * height);

  for (int i = 0; i < width * height; ++i) {
    images[0][i] = data[i][0];
    images[1][i] = data[i][1];
    images[2][i] = data[i][2];
  }

  float *image_ptr[3];
  image_ptr[0] = &(images[2].at(0));
  image_ptr[1] = &(images[1].at(0));
  image_ptr[2] = &(images[0].at(0));

  image.images = (unsigned char **)image_ptr;
  image.width = width;
  image.height = height;

  header.num_channels = 3;
  header.channels =
      (EXRChannelInfo *)malloc(sizeof(EXRChannelInfo) * header.num_channels);
  strncpy(header.channels[0].name, "B", 255);
  header.channels[0].name[strlen("B")] = '\0';
  strncpy(header.channels[1].name, "G", 255);
  header.channels[1].name[strlen("G")] = '\0';
  strncpy(header.channels[2].name, "R", 255);
  header.channels[2].name[strlen("R")] = '\0';

  header.pixel_types = (int *)malloc(sizeof(int) * header.num_channels);
  header.requested_pixel_types =
      (int *)malloc(sizeof(int) * header.num_channels);
  for (int i = 0; i < header.num_channels; ++i) {
    header.pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;
    header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_HALF;
  }

  const char *err;
  int ret = SaveEXRImageToFile(&image, &header, filename.c_str(), &err);
  if (ret != TINYEXR_SUCCESS) {
    fprintf(stderr, "Save EXR err: %s\n", err);
  }

  printf("Saved exr file. [ %s ] \n", filename.c_str());

  free(header.channels);
  free(header.pixel_types);
  free(header.requested_pixel_types);
}

SpectrumRGB Figure::evaluate(Point2f uv, bool biFilter) const {
  auto float32Spectrum = [](PixelValue v) {
    return SpectrumRGB{v[0], v[1], v[2]};
  };

  if (biFilter) {
    float x = uv.x * width, y = uv.y * height;
    int lx = std::min((int)x, width - 2), ly = std::min((int)y, height - 2);

    float dx = x - lx, dy = y - ly;

    SpectrumRGB lerp1 = float32Spectrum(data[lx + width * ly]) * (1 - dx) *
                        (1 - dy),
                lerp2 =
                    float32Spectrum(data[lx + 1 + width * ly]) * dx * (1 - dy),
                lerp3 = float32Spectrum(data[lx + width * (ly + 1)]) *
                        (1 - dx) * dy,
                lerp4 =
                    float32Spectrum(data[lx + 1 + width * (ly + 1)]) * dx * dy;

    return lerp1 + lerp2 + lerp3 + lerp4;
  }

  int x = uv.x * width, y = uv.y * height;
  return SpectrumRGB{data[x + y * width][0], data[x + y * width][1],
                     data[x + y * width][2]};
}

SpectrumRGB Figure::evaluate(Point2i pixel) const {
  int offset = pixel.x + width * pixel.y;
  return {data[offset][0], data[offset][1], data[offset][2]};
}

void Figure::setPixel(float rgb[3], Point2i pixel) {
  auto [x, y] = pixel;
  int offset = x + y * width;
  if (0 <= x && x < width && 0 <= y && y < height)
    for (int channel = 0; channel < 3; ++channel) {
      data[offset][channel] = rgb[channel];
    }
}

void Figure::getPixel(float *rgb, Point2i pixel) const {
  auto [x, y] = pixel;
  int offset = x + y * width;
  if (0 <= x && x < width && 0 <= y && y < height)
    for (int channel = 0; channel < 3; ++channel) {
      rgb[channel] = data[offset][channel];
    }
}

std::shared_ptr<Figure> Figure::shrinkHalfNearest() const {
  std::shared_ptr<Figure> res = std::make_shared<Figure>(width / 2, height / 2);
  for (int i = 0; i < width - 1; i += 2) {
    for (int j = 0; j < height - 1; j += 2) {
      int offset = i + j * width;
      auto rgb = data[offset];
      res->setPixel(rgb, {i / 2, j / 2});
    }
  }
  return res;
}

std::shared_ptr<Figure> Figure::shrinkHalfBox() const {
  std::shared_ptr<Figure> res = std::make_shared<Figure>(width / 2, height / 2);
  for (int i = 0; i < width - 1; i += 2) {
    for (int j = 0; j < height - 1; j += 2) {
      int offset = i + j * width;
      auto rgb1 = data[offset],
           rgb2 = (i != width - 1) ? data[offset + 1] : rgb1,
           rgb3 = (j != height - 1) ? data[offset + width] : rgb1,
           rgb4 = (i != width - 1 && j != height - 1) ? data[offset + width + 1]
                                                      : rgb1;

      float rgb[3]{0, 0, 0};

      for (int i = 0; i < 3; ++i) {
        rgb[i] = 0.25 * (rgb1[i] + rgb2[i] + rgb3[i] + rgb4[i]);
      }

      res->setPixel(rgb, {i / 2, j / 2});
    }
  }
  return res;
}

void Image::savePNG() const {
  uint8_t *data = new uint8_t[size.x * size.y * channels];
  for (int x = 0; x < size.x; ++x)
    for (int y = 0; y < size.y; ++y) {
      data[(x + y * size.x) * channels + 0] =
          std::min(screen.pixels[x][y][0], 1.f) * 255;
      data[(x + y * size.x) * channels + 1] =
          std::min(screen.pixels[x][y][1], 1.f) * 255;
      data[(x + y * size.x) * channels + 2] =
          std::min(screen.pixels[x][y][2], 1.f) * 255;
    }
  stbi_write_png(filename.c_str(), size.x, size.y, 3, data, 0);
  delete[] data;
}
