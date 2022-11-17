#pragma once

#include <core/file/figure.h>
#include <core/geometry/geometry.h>
#include <core/render-core/spectrum.h>

/**
 * @brief This class response for tile abstruction etc.
 * 
 */
struct FilmPixel{
    FilmPixel() : value(0), weight(.0f) { }
    SpectrumRGB value;
    float weight;
};

class FilmTile {
public:
    FilmTile() = delete;
    FilmTile(int _tile_size, Point2i _tile_offset)
        : tile_size(_tile_size), tile_offset(_tile_offset) 
    {
        data = new FilmPixel[tile_size * tile_size];
    }
    ~FilmTile() {
        delete [] data;
    }

    Point2i pixel_location(Point2i offset) {
        return tile_offset + offset;
    }
    void add_sample(Point2i pixel, SpectrumRGB _value, float _weight)
    {
        auto [i, j] = pixel - tile_offset;
        int offset = i + j * tile_size;
        auto& film_pixel = data[offset];
        film_pixel.value += _value;
        film_pixel.weight += _weight;
    }

    FilmPixel get_sample(Point2i pixel) const {
        auto [i, j] = pixel - tile_offset;
        int offset = i + j * tile_size;
        return data[offset];
    }
private:
    int     tile_size;
    Point2i tile_offset;
    FilmPixel *data = nullptr;

};

class Film {
public:
    int     tile_size;
    
    Film() = delete;
    Film(Point2i _film_size, int _tile_size = 32) 
        : film_size(_film_size), tile_size(_tile_size) 
    { 
        figure = std::make_shared<Figure>(film_size.x, film_size.y);
    }
    Point2i tile_range() const {
        assert(film_size.x % tile_size == 0 && film_size.y % tile_size == 0);
        return {film_size.x / tile_size, film_size.y / tile_size};
    }

    std::shared_ptr<FilmTile> get_tile(Point2i tile_index) const {
        auto [x, y] = tile_index;
        std::shared_ptr<FilmTile> tile = std::make_shared<FilmTile>(
            tile_size, Point2i{x * tile_size, y * tile_size}
        );
        return tile;
    }

    void fill_tile(std::shared_ptr<FilmTile> tile) {
        for (int i = 0; i < tile_size; ++i) {
            for (int j = 0; j < tile_size; ++j) {
                auto pixel = tile->pixel_location({i, j});
                auto [value, weight] = tile->get_sample(pixel);
                value /= weight;
                float rgb[] = {value.r(), value.g(), value.b()};
                figure->setPixel(rgb, pixel);
            }
        }
    }
    void save_film(const std::string & file_name) const {
        figure->saveAsPng(file_name);
    }

private:
    Point2i film_size;
    std::shared_ptr<Figure> figure = nullptr;
};