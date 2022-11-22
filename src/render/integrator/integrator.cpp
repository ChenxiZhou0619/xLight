#include <core/render-core/film.h>
#include <core/render-core/integrator.h>
#include <core/scene/scene.h>
#include <core/task/task.h>
#include <tbb/tbb.h>

#include <chrono>
#include <mutex>

void PixelIntegrator::render(std::shared_ptr<RenderTask> task) const {
  auto start = std::chrono::high_resolution_clock::now();

  Film film{task->film_size, 32};
  auto [x, y] = film.tile_range();

  std::vector<std::shared_ptr<FilmTile>> film_tiles;
  film_tiles.reserve(x * y);

  int finished_tiles = 0, tile_size = film.tile_size;
  double total_tiles = x * y;

  auto integrator = task->integrator;
  auto ori_sampler = task->sampler;
  auto camera = task->camera;
  auto scene = task->scene;

  tbb::parallel_for(
      tbb::blocked_range2d<size_t>(0, x, 0, y),
      [&](const tbb::blocked_range2d<size_t> &r) {
        for (int row = r.rows().begin(); row != r.rows().end(); ++row)
          for (int col = r.cols().begin(); col != r.cols().end(); ++col) {
            auto tile = film.get_tile({row, col});
            auto sampler = ori_sampler->clone();

            for (int i = 0; i < tile_size; ++i)
              for (int j = 0; j < tile_size; ++j) {
                Point2i p_pixel = tile->pixel_location({i, j});
                sampler->startPixel(p_pixel);
                for (int spp = 0; spp < task->getSpp(); ++spp) {
                  Ray3f ray = camera->sampleRayDifferential(
                      p_pixel, task->film_size, sampler->getCameraSample());
                  ray.medium = scene->getEnvMedium().get();
                  SpectrumRGB L = integrator->getLi(*scene, ray, sampler.get());
                  sampler->nextSample();
                  tile->add_sample(p_pixel, L, 1.f);
                }
              }

            film_tiles.emplace_back(tile);
            finished_tiles++;
            if (finished_tiles % 5 == 0) {
              printProgress((double)finished_tiles / total_tiles);
            }
          }
      });
  printProgress(1);
  for (auto tile : film_tiles) {
    film.fill_tile(tile);
  }
  auto end = std::chrono::high_resolution_clock::now();
  std::cout << tfm::format(
      "\nRendering costs : %.2f seconds\n",
      (float)std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
              .count() /
          1000.f);
  film.save_film(task->file_name);
}

void FilmIntegrator::render(std::shared_ptr<RenderTask> task) const {
  auto start = std::chrono::high_resolution_clock::now();

  Film film{task->film_size, 32};
  auto [x, y] = film.tile_range();

  std::vector<std::shared_ptr<FilmTile>> film_tiles;
  film_tiles.reserve(x * y);

  int finished_tiles = 0, tile_size = film.tile_size;
  double total_tiles = x * y;

  auto integrator = task->integrator;
  auto ori_sampler = task->sampler;
  auto camera = task->camera;
  auto scene = task->scene;

  Point3f pinehole = camera->get_position();
  tbb::parallel_for(
      tbb::blocked_range2d<size_t>(0, x, 0, y),
      [&](const tbb::blocked_range2d<size_t> &r) {
        for (int row = r.rows().begin(); row != r.rows().end(); ++row)
          for (int col = r.cols().begin(); col != r.cols().end(); ++col) {
            auto tile = film.get_tile({row, col});
            auto sampler = ori_sampler->clone();

            for (int i = 0; i < tile_size; ++i)
              for (int j = 0; j < tile_size; ++j) {
                Point2i p_pixel = tile->pixel_location({i, j});
                sampler->startPixel(p_pixel);

                for (int spp = 0; spp < task->getSpp(); ++spp) {
                  LightSourceInfo light_info =
                      scene->sampleLightSource(sampler.get());

                  Vector3f dir = normalize(light_info.position - pinehole),
                           dir_local = camera->vec2local(dir);
                  Point3f p_film = camera->local2film(normalize(dir_local));
                  if (0 <= p_film.x && p_film.x < 1 && 0 <= p_film.y &&
                      p_film.y < 1) {
                    Point2i pixel{int(task->film_size.x * p_film.x),
                                  int(task->film_size.y * p_film.y)};
                    //                                            auto [value,
                    //                                            weight] =
                    //                                            light_info.light->evaluate(light_info,
                    //                                            pinehole);
                    SpectrumRGB value = SpectrumRGB{10};
                    auto cos_term = std::abs(dot(dir, light_info.normal));
                    auto invdist2 =
                        1.f / (light_info.position - pinehole).length2();
                    value = value * cos_term * invdist2 / light_info.pdf;
                    film.add_splat(pixel, value, 1.f / (task->spp));
                  }
                  sampler->nextSample();
                }
              }

            film_tiles.emplace_back(tile);
            finished_tiles++;
            if (finished_tiles % 5 == 0) {
              printProgress((double)finished_tiles / total_tiles);
            }
          }
      });

  //        film.save_splat();
}