#include <core/math/warp.h>
#include <core/render-core/camera.h>
#include <core/render-core/film.h>
#include <core/render-core/info.h>
#include <core/render-core/integrator.h>
#include <core/task/task.h>
#include <tbb/tbb.h>

#include <chrono>
#include <mutex>

class LightTracer : public FilmIntegrator {
protected:
  int generate_lightpath(const Scene &scene, Sampler *sampler, int max_depth,
                         std::vector<PathVertex> *lightpath) const {
    if (max_depth == 0)
      return 0;
    auto light_info = scene.sampleLightSource(sampler);

    if (light_info.lightType == LightSourceInfo::LightType::Environment) {
      std::cout << "Unhandle situation!\n";
      std::exit(1);
    }

    Frame light_local{light_info.normal};
    Vector3f dir_local = Warp::squareToCosineHemisphere(sampler->next2D()),
             dir_world = light_local.toWorld(dir_local);

    float pdf_pos = light_info.pdf,
          pdf_dir = Warp::squareToCosineHemispherePdf(dir_local);

    if (pdf_pos == 0 || pdf_dir == 0)
      return 0;

    // todo
    (*lightpath)[0] = PathVertex::create_light(light_info);

    SpectrumRGB beta = light_info.Le *
                       std::abs(dot(light_info.normal, dir_world)) /
                       (pdf_pos * pdf_dir);

    //* random walk
    int bounces = 0;
    float pdf_fwd = pdf_dir, pdf_rev = .0f;
    Ray3f ray{light_info.position, dir_world};
    while (true) {
      auto sits = scene.intersectWithSurface(ray);
      if (sits->shape) {
        ++bounces;
        if (bounces >= max_depth)
          break;
        (*lightpath)[bounces] = PathVertex::create_surface(
            sits, beta, pdf_fwd, (*lightpath)[bounces - 1]);
        auto scatter_info = sits->sampleScatter(sampler->next2D());
        ray = sits->scatterRay(scene, scatter_info.wo);
        pdf_fwd = scatter_info.pdf;
        beta *= scatter_info.weight;
        if (pdf_fwd == FINF) {
          (*lightpath)[bounces].delta = true;
        }
        if (beta.isZero())
          break;
      } else
        break;
    }
    return bounces + 1;
  }

  SpectrumRGB connect_camera(const Scene &scene, std::shared_ptr<Camera> camera,
                             Point2i resolution, const PathVertex &vertex,
                             Point2i *pixel) const {
    SpectrumRGB result{.0f};
    switch (vertex.type) {
    case VertexType::CameraVertex:
    case VertexType::MediumVertex:
    case VertexType::LightVertex:
      *pixel = Point2i(-1);
      break;
    case VertexType::SurfaceVertex:
      Point3f vertex_position = vertex.position;
      Vector3f wi;
      float pdf;
      Point2f p_raster;
      // todo no sampling now
      SpectrumRGB we =
          camera->sampleWi(vertex_position, Point2f(), &wi, &pdf, &p_raster);
      Ray3f vis_ray{camera->get_position(), vertex_position};
      if (!scene.occlude(vis_ray) && !vertex.delta) {
        if ((0 <= p_raster.x && p_raster.x < 1) &&
            (0 <= p_raster.y && p_raster.y < 1)) {
          *pixel = Point2i{int(resolution.x * p_raster.x),
                           int(resolution.y * p_raster.y)};
          SpectrumRGB f = vertex.info->evaluateScatter(wi);
          result = vertex.beta * f * (we / pdf);
        }
      }
    }
    return result;
  }

public:
  LightTracer() : max_depth(5) {}

  LightTracer(const rapidjson::Value &_value) {
    max_depth = getInt("maxDepth", _value);
  }

  virtual ~LightTracer() = default;

  virtual SpectrumRGB getLi(const Scene &scene, Ray3f ray,
                            Sampler *sampler) const override {
    auto sits = scene.intersectWithSurface(ray);
    if (auto light = sits->light; light) {
      return sits->evaluateLe();
    }
    return SpectrumRGB{0};
  }

  virtual void render(std::shared_ptr<RenderTask> task) const override {
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
                    SpectrumRGB t2s0 = getLi(*scene, ray, sampler.get());
                    tile->add_sample(p_pixel, t2s0, 1);

                    std::vector<PathVertex> light_path(max_depth + 1);
                    //* generate light subpath
                    int n_lightpath = generate_lightpath(
                        *scene, sampler.get(), max_depth + 1, &light_path);
                    //* connect all light subpath vertex to camera
                    const int t = 1;
                    for (int s = 0; s <= n_lightpath; ++s) {
                      int depth = t + s - 2;
                      //* Ignore the following situations
                      if ((s == 1 && t == 1) || depth < 0 || depth > max_depth)
                        continue;
                      Point2i pixel;
                      SpectrumRGB L_path =
                          connect_camera(*scene, camera, task->film_size,
                                         light_path[s - 1], &pixel);
                      if ((0 <= pixel.x && pixel.x < task->film_size.x) &&
                          (0 <= pixel.y && pixel.y < task->film_size.y))
                        film.add_splat(pixel, L_path, 1.f / task->spp);
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
    printProgress(1);
    for (auto tile : film_tiles) {
      film.fill_tile(tile);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << tfm::format(
        "\nRendering costs : %.2f seconds\n",
        (float)std::chrono::duration_cast<std::chrono::milliseconds>(end -
                                                                     start)
                .count() /
            1000.f);
    film.save_film(task->file_name);
    std::string splat_name = task->file_name + "splat.exr";
    film.save_splat(splat_name);
  }

private:
  int max_depth;
};

REGISTER_CLASS(LightTracer, "light-tracer")
