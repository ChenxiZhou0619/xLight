#pragma once

#include <core/file/figure.h>
#include <core/geometry/geometry.h>
#include <core/render-core/spectrum.h>

#include <mutex>

/**
 * @brief This class response for tile abstruction etc.
 *
 */
struct FilmPixel {
  FilmPixel() : value(0), weight(.0f) {}
  SpectrumRGB value;
  float weight;
  std::mutex splat_lock;
};

class FilmTile {
public:
  FilmTile() = delete;
  FilmTile(int _tile_size, Point2i _tile_offset)
      : tile_size(_tile_size), tile_offset(_tile_offset) {
    data = new FilmPixel[tile_size * tile_size];
  }
  ~FilmTile() { delete[] data; }

  Point2i pixel_location(Point2i offset) { return tile_offset + offset; }
  void add_sample(Point2i pixel, SpectrumRGB _value, float _weight) {
    auto [i, j] = pixel - tile_offset;
    int offset = i + j * tile_size;
    auto &film_pixel = data[offset];
    film_pixel.value += _value;
    film_pixel.weight += _weight;
  }

  std::pair<SpectrumRGB, float> get_sample(Point2i pixel) const {
    auto [i, j] = pixel - tile_offset;
    int offset = i + j * tile_size;
    return {data[offset].value, data[offset].weight};
  }

private:
  int tile_size;
  Point2i tile_offset;
  FilmPixel *data = nullptr;
};

class Film {
public:
  int tile_size;

  Film() = delete;
  Film(Point2i _film_size, int _tile_size = 32)
      : film_size(_film_size), tile_size(_tile_size) {
    eye_figure = std::make_shared<Figure>(film_size.x, film_size.y);
    light_figure = new FilmPixel[film_size.x * film_size.y];
  }
  virtual ~Film() { delete[] light_figure; }
  Point2i tile_range() const {
    int x_range = film_size.x / tile_size + ((film_size.x % tile_size) ? 1 : 0),
        y_range = film_size.y / tile_size + ((film_size.y % tile_size) ? 1 : 0);
    return {x_range, y_range};
  }

  std::shared_ptr<FilmTile> get_tile(Point2i tile_index) const {
    auto [x, y] = tile_index;
    std::shared_ptr<FilmTile> tile = std::make_shared<FilmTile>(
        tile_size, Point2i{x * tile_size, y * tile_size});
    return tile;
  }

  void fill_tile(std::shared_ptr<FilmTile> tile) {
    for (int i = 0; i < tile_size; ++i) {
      for (int j = 0; j < tile_size; ++j) {
        auto pixel = tile->pixel_location({i, j});
        auto [value, weight] = tile->get_sample(pixel);
        value /= weight;
        float rgb[] = {value.r(), value.g(), value.b()};
        eye_figure->setPixel(rgb, pixel);
      }
    }
  }
  void save_film(const std::string &file_name) const {
    // figure->saveAsPng(file_name);
    // figure->saveAsHdr(file_name);
    eye_figure->saveAsExr(file_name);
  }

  // TODO
  void add_splat(Point2i pixel, SpectrumRGB _value, float _weight) const {
    int offset = pixel.x + film_size.x * pixel.y;
    auto &splat_pixel = light_figure[offset];
    splat_pixel.splat_lock.lock();
    splat_pixel.value += _value * _weight;
    splat_pixel.weight += _weight;
    splat_pixel.splat_lock.unlock();
  }

  // todo delete this
  void save_splat(const std::string &filename) const {
    std::shared_ptr<Figure> splat_figure =
        std::make_shared<Figure>(film_size.x, film_size.y);

    for (int i = 0; i < film_size.x; ++i) {
      for (int j = 0; j < film_size.y; ++j) {
        int offset = i + j * film_size.x;

        const auto &splat_pixel = light_figure[offset];
        SpectrumRGB res = splat_pixel.value;
        float rgb[] = {res.r(), res.g(), res.b()};
        float rgb1[] = {0, 0, 0};
        eye_figure->getPixel(rgb1, {i, j});
        rgb[0] += rgb1[0];
        rgb[1] += rgb1[1];
        rgb[2] += rgb1[2];
        splat_figure->setPixel(rgb, {i, j});
      }
    }
    splat_figure->saveAsExr(filename);
  }

private:
  Point2i film_size;
  std::shared_ptr<Figure> eye_figure = nullptr;
  FilmPixel *light_figure = nullptr;
};