#pragma once

#include <core/file/figure.h>
#include <core/geometry/geometry.h>

/**
 * @brief This class response for tile abstruction etc.
 * 
 */
class FilmTile {
public:
    FilmTile() = delete;
    FilmTile(int _tile_size, Point2i _tile_offset)
        : tile_size(_tile_size), tile_offset(_tile_offset) { }

    Point2i pixel_location(Point2i offset) {
        return tile_offset + offset;
    }
private:
    int     tile_size;
    Point2i tile_offset;
};

class Film {
public:
    int     tile_size;
    
    Film() = delete;
    Film(Point2i _film_size, int _tile_size = 32) 
        : film_size(_film_size), tile_size(_tile_size) { }
    Point2i tile_range() const {
        assert(film_size.x % tile_size == 0 && film_size.y % tile_size == 0);
        return {film_size.x / tile_size, film_size.y / tile_size};
    }

    std::shared_ptr<FilmTile> get_tile(Point2i tile_index) const {

    }

private:
    Point2i film_size;
};