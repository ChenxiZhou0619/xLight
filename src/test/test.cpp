#include <core/file/figure.h>
#include <core/render-core/film.h>
#include <render/texture/mipmap.h>
#include <tbb/tbb.h>
#include <core/render-core/sampler.h>

int main() {
    Film film {{1536, 832}, 32};
    int tile_size = film.tile_size;

    auto [x, y] = film.tile_range();
    tbb::parallel_for(
        tbb::blocked_range2d<size_t>(0, x, 0, y),
        [&](const tbb::blocked_range2d<size_t> &r) {
            for(int row = r.rows().begin(); row != r.rows().end(); ++row)
                for (int col = r.cols().begin(); col != r.cols().end(); ++col) {
                    auto tile = film.get_tile({row, col});

                    for (int i = 0; i < tile_size; ++i) {
                        for (int j = 0; j < tile_size; ++j) {
                            Point2i p_pixel = tile->pixel_location({i, j});

                            
                        }
                    }                    
                }
        }
    );

}