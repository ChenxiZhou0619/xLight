#include <core/render-core/integrator.h>
#include <core/task/task.h>
#include <core/scene/scene.h>
#include <chrono>
#include <tbb/tbb.h>
#include <mutex>
#include <core/render-core/film.h>

void Integrator::render(std::shared_ptr<RenderTask> task) const 
{
    auto start = std::chrono::high_resolution_clock::now();
    
    Film film {task->film_size, 32};
    auto [x, y] = film.tile_range();
    
    std::vector<std::shared_ptr<FilmTile>> film_tiles; film_tiles.reserve(x * y);

    int finished_tiles = 0,
        tile_size = film.tile_size;
    double total_tiles = x * y;

    auto integrator = task->integrator;
    auto ori_sampler = task->sampler;
    auto camera = task->camera;
    auto scene = task->scene;

    tbb::parallel_for(
        tbb::blocked_range2d<size_t>(0, x, 0, y), 
        [&](const tbb::blocked_range2d<size_t> &r){
                for (int row = r.rows().begin(); row != r.rows().end(); ++row)
                    for (int col = r.cols().begin(); col != r.cols().end(); ++col){
                        
                        auto tile = film.get_tile({row, col});
                        auto sampler = ori_sampler->clone();
                        
                        for (int i = 0; i < tile_size; ++i)
                            for (int j = 0; j < tile_size; ++j) {
                                Point2i p_pixel = tile->pixel_location({i, j});
                                sampler->startPixel(p_pixel);      
                                for (int spp = 0; spp < task->getSpp(); ++spp) {
                                    Ray3f ray = camera->sampleRayDifferential(
                                        p_pixel, 
                                        task->film_size, 
                                        sampler->getCameraSample()
                                    );
                                    ray.medium = scene->getEnvMedium().get();
                                    SpectrumRGB L = integrator->getLi(*scene, ray, sampler.get());
                                    sampler->nextSample();
                                    tile->add_sample(p_pixel, L, 1.f);
                                }
                            }
                        
                        film_tiles.emplace_back(tile);
                        finished_tiles++;
                        if (finished_tiles % 5 == 0) {
                            printProgress((double)finished_tiles/total_tiles);
                        }
                    }
            }
    );
    printProgress(1);
    for (auto tile : film_tiles) {
        film.fill_tile(tile);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << tfm::format("\nRendering costs : %.2f seconds\n",
        (float)std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.f);
    film.save_film(task->file_name);  

}
