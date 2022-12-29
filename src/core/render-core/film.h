#pragma once

#include <core/file/figure.h>
#include <core/geometry/geometry.h>
#include <core/render-core/filter.h>
#include <core/render-core/spectrum.h>

#include <mutex>

/**
 * @brief This class response for tile abstruction etc.
 *
 */
struct FilmPixel {
  FilmPixel() : value(0), weight(.0f) {}
  float weight;
  SpectrumRGB value;
  std::mutex write_lock;
};

class FilmTile {
public:
  FilmTile() = delete;
  FilmTile(int _tile_size, Point2i _tile_offset)
      : tile_size(_tile_size), tile_offset(_tile_offset) {}
  ~FilmTile() {}

  Point2i pixel_location(Point2i offset) { return tile_offset + offset; }

private:
  int tile_size;
  Point2i tile_offset;
};

class Film {
public:
  int tile_size;

  Film() = delete;
  Film(Point2i _film_size, int _tile_size = 32)
      : film_size(_film_size), tile_size(_tile_size) {
    splat_pixels = new FilmPixel[film_size.x * film_size.y];
    film_pixels = new FilmPixel[film_size.x * film_size.y];
  }
  virtual ~Film() {
    delete[] splat_pixels;
    delete[] film_pixels;
  }
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

  void add_splat(Point2i pixel, SpectrumRGB _value, float _weight) const {
    int offset = pixel.x + film_size.x * pixel.y;
    auto &splat_pixel = splat_pixels[offset];
    splat_pixel.write_lock.lock();
    splat_pixel.value += _value * _weight;
    splat_pixel.weight += _weight;
    splat_pixel.write_lock.unlock();
  }

  void add_sample(Point2i pixel_location, SpectrumRGB _value,
                  float _weight) const {
    auto [x, y] = pixel_location;
    float filter_raidus = filter ? filter->radius : 0;
    for (int i = x - filter_raidus; i <= x + filter_raidus; ++i) {
      for (int j = y - filter_raidus; j <= y + filter_raidus; ++j) {
        if (0 <= i && i < film_size.x && 0 <= j && j < film_size.y) {
          int offset = i + j * film_size.x;
          auto &film_pixel = film_pixels[offset];
          Point2f filter_offset{(i - x) / (filter_raidus + 0.5f),
                                (j - y) / (filter_raidus + 0.5f)};
          float filter_weight = filter ? filter->evaluate(filter_offset) : 1;
          film_pixel.write_lock.lock();
          film_pixel.value += _value * _weight * filter_weight;
          film_pixel.weight += _weight * filter_weight;
          film_pixel.write_lock.unlock();
        }
      }
    }
  }

  //* type == 1 exr
  //* type == 2 png
  void save_as(const std::string &film_name, int type) const {
    std::shared_ptr<Figure> final_figure =
        std::make_shared<Figure>(film_size.x, film_size.y);

    //* Fill the figure
    for (int i = 0; i < film_size.x; ++i) {
      for (int j = 0; j < film_size.y; ++j) {
        int offset = i + j * film_size.x;
        const auto &film_pixel = film_pixels[offset];
        SpectrumRGB value = film_pixel.value;
        float weight = film_pixel.weight;
        weight = weight ? weight : 1;
        value = value / weight;
        value += splat_pixels[offset].value;

        float rgb[] = {value.r(), value.g(), value.b()};
        final_figure->setPixel(rgb, {i, j});
      }
    }

    final_figure->saveAsExr(film_name);
  }

  std::shared_ptr<Filter> filter = nullptr;

private:
  Point2i film_size;

  //* new
  FilmPixel *splat_pixels = nullptr;
  FilmPixel *film_pixels = nullptr;
};