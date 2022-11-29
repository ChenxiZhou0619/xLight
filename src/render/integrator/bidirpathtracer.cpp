#include <core/render-core/film.h>
#include <core/render-core/info.h>
#include <core/render-core/integrator.h>
#include <core/task/task.h>
#include <spdlog/spdlog.h>
#include <tbb/tbb.h>

#include <chrono>

template <typename Type> class ScopeAssignment {
public:
  ScopeAssignment(Type *_target = nullptr, Type _value = Type())
      : target(_target) {
    if (_target) {
      backup = *target;
      *target = _value;
    }
  }

  ~ScopeAssignment() {
    if (target) {
      *target = backup;
    }
  }

  ScopeAssignment(const ScopeAssignment &) = delete;
  ScopeAssignment &operator=(const ScopeAssignment &) = delete;
  ScopeAssignment &operator=(ScopeAssignment &&other) {
    if (target)
      *target = backup;
    target = other.target;
    backup = other.backup;
    other.target = nullptr;
    return *this;
  }

private:
  Type *target, backup;
};

class BidirectionalPathTracer : public Integrator {
private:
  enum class TransportMode { Radiance, Importance };

  int random_walk(const Scene &scene, Sampler *sampler, int max_depth,
                  TransportMode mode, Ray3f ray, SpectrumRGB beta, float pdf,
                  std::vector<PathVertex> *path) const {
    int bounces = 0;

    //* pdf_fwd represents the pdf of sampling the current vertex (in
    //* solid-angle measure) by previous vertex
    //* pdf_rev represents the pdf of sampling the previous vertex (in
    //* solid-angle measure) by the current vertex
    float pdf_fwd = pdf, pdf_rev = .0f;

    while (true) {
      auto sits = scene.intersectWithSurface(ray);
      //* If escape the scene, just terminate
      // TODO When environment in consideration, this should be expand
      if (!sits->shape)
        break;

      if (sits->shape) {
        ++bounces;
        //* When bounces == max_depth, the random walk should terminate
        if (bounces > max_depth)
          break;
        //* vertex -> sits
        //* prev   -> previous vertex
        PathVertex &vertex = (*path)[bounces], &prev = (*path)[bounces - 1];
        //* create the current vertex
        //* beta is the weight when the path arrived this vertex
        //* pdf is the pdf_fwd (the pdf of sampling this vertex on previous
        //* vertex with respect to solid-angle)
        //* prev is the previous path vertex
        vertex = PathVertex::create_surface(sits, beta, pdf_fwd, prev);

        //* sample a direction of for random walk (in solid-angle measure)
        auto scatter_info = sits->sampleScatter(sampler->next2D());
        //* update the ray
        ray = sits->scatterRay(scene, scatter_info.wo);
        //* update the pdf_fwd and pdf_prev
        pdf_fwd = scatter_info.pdf;
        pdf_rev = sits->pdfScatter(scatter_info.wo, sits->wi);
        beta *= scatter_info.weight;
        if (pdf_fwd == FINF) {
          (*path)[bounces].delta = true;
          pdf_fwd = pdf_rev = 0;
        }
        prev.pdf_rev = vertex.convert_pdf(pdf_rev, prev);
        if (beta.isZero())
          break;
      }
    }
    return bounces;
  }

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

    //? What is the pdf_fwd of the light vertex
    (*lightpath)[0] = PathVertex::create_light(light_info);

    //* beta = Le(x, dir) * abscos(<light_normal, dir>) / (pdf_pos * pdf_dir)
    SpectrumRGB beta = light_info.Le *
                       std::abs(dot(light_info.normal, dir_world)) /
                       (pdf_pos * pdf_dir);

    Ray3f ray{light_info.position, dir_world};
    //* Random walk
    //* Initial pdf = pdf_dir
    return random_walk(scene, sampler, max_depth - 1, TransportMode::Importance,
                       ray, beta, pdf_dir, lightpath) +
           1;
  }

  int generate_camerapath(const Scene &scene, Sampler *sampler, int max_depth,
                          std::shared_ptr<Camera> camera, Ray3f ray,
                          std::vector<PathVertex> *camerapath) const {
    if (max_depth == 0)
      return 0;

    float pdf_pos, pdf_dir;
    // TODO create_camera
    (*camerapath)[0] = PathVertex::create_camera(camera, ray, SpectrumRGB{1});
    camera->pdfWe(ray, &pdf_pos, &pdf_dir);

    //* Random walk
    return random_walk(scene, sampler, max_depth - 1, TransportMode::Radiance,
                       ray, SpectrumRGB{1}, pdf_dir, camerapath) +
           1;
  }

  float mis_weight(const Scene &scene, std::vector<PathVertex> &camerapath,
                   std::vector<PathVertex> &lightpath, int t, int s,
                   PathVertex &sampled) const {

    //* misw = pdf(current_strategy) / sum(pdf(all_strategy_with_same_length))
    if (s + t == 2)
      return 1;

    //* All pdf==0 should be consider as delta distribution
    auto remap0 = [](float f) { return f == 0 ? 1 : f; };

    PathVertex *qs = s > 0 ? &lightpath[s - 1] : nullptr,
               *pt = t > 0 ? &camerapath[t - 1] : nullptr,
               *qs_minus = s > 1 ? &lightpath[s - 2] : nullptr,
               *pt_minus = t > 1 ? &camerapath[t - 2] : nullptr;

    ScopeAssignment<PathVertex> a1;
    if (s == 1)
      a1 = {qs, sampled};
    else if (t == 1)
      a1 = {pt, sampled};

    ScopeAssignment<bool> a2, a3;
    if (pt)
      a2 = {&pt->delta, false};
    if (qs)
      a3 = {&qs->delta, false};

    ScopeAssignment<float> a4;
    if (pt) {
      //* pdf_qs_2_pt
      // TODO finish_it
      float pt_pdf_rev = s > 0 ? qs->pdf(scene, qs_minus, pt)
                               : pt->pdf_light_origin(scene, pt_minus);
      a4 = {&pt->pdf_rev, pt_pdf_rev};
    }

    ScopeAssignment<float> a5;
    if (pt_minus) {
      // TODO pt.pdfLight
      float pdf_pt_2_ptminus =
          s > 0 ? pt->pdf(scene, qs, pt_minus) : pt->pdf_light(scene, pt_minus);
      a5 = {&pt_minus->pdf_rev, pdf_pt_2_ptminus};
    }

    ScopeAssignment<float> a6;
    if (qs) {
      a6 = {&qs->pdf_rev, pt->pdf(scene, pt_minus, qs)};
    }

    ScopeAssignment<float> a7;
    if (qs_minus) {
      a7 = {&qs_minus->pdf_rev, qs->pdf(scene, pt, qs_minus)};
    }
    //* misw = 1 / sum_ri
    float sum_ri = 0;

    //* Compute the first part of denominator
    {
      float ri = 1;
      for (int i = t - 1; i > 0; --i) {
        ri *= remap0(camerapath[i].pdf_rev) / remap0(camerapath[i].pdf_fwd);
        if (!camerapath[i].delta && !camerapath[i - 1].delta) {
          sum_ri += ri;
        }
      }
    }
    //* Compute the third part of denominator
    {
      float ri = 1;
      for (int i = s - 1; i > 0; --i) {
        ri *= remap0(lightpath[i].pdf_rev) / remap0(lightpath[i].pdf_fwd);
        if (!lightpath[i].delta && !lightpath[i - 1].delta) {
          sum_ri += ri;
        }
      }
    }

    return 1 / (1 + sum_ri);
  }

  SpectrumRGB connect_subpath(const Scene &scene,
                              std::shared_ptr<Camera> camera,
                              Point2i resolution,
                              std::vector<PathVertex> &lightpath,
                              std::vector<PathVertex> &camerapath, int s, int t,
                              Sampler *sampler, Point2i *pixel) const {
    //* connect the given path (identified by s and t)
    SpectrumRGB L{.0f};

    //* No connection for a camera vertex on infinite light
    if (t > 1 && s != 0 && camerapath[t - 1].type == VertexType::LightVertex)
      return SpectrumRGB{.0f};
    //* For a totally camerapath, if the vertex laies on light source, this
    //* should be consider

    PathVertex sampled;
    if (s == 0) {
      const PathVertex &vertex = camerapath[t - 1];
      if (vertex.light)
        L = vertex.evaluate_le(scene, camerapath[t - 2]) * vertex.beta;
    }
    //* For a lightpath connect to the camera (except the s == 1 situation)
    else if (t == 1 && s != 1) {
      const PathVertex &vertex = lightpath[s - 1];
      //* The vertex is only connectable if is's not a delta distribution
      if (!vertex.delta) {
        //* First, we should sample the camera
        //* But, we just implement the pinhole, so there is no need to sample
        Vector3f wi;
        float pdf;
        Point2f p_raster;
        SpectrumRGB importance =
            camera->sampleWi(vertex.position, Point2f(), &wi, &pdf, &p_raster);
        if (pdf > 0 && !importance.isZero()) {
          Ray3f vis_ray{camera->get_position(), vertex.position};
          sampled =
              PathVertex::create_camera(camera, vis_ray, importance / pdf);
          //* This connection will only contribute when it hit the film
          //* and it was not occlude by the scene
          if (!scene.occlude(vis_ray) && (0 <= p_raster.x && p_raster.x < 1) &&
              (0 <= p_raster.y && p_raster.y < 1)) {
            *pixel = Point2i{int(resolution.x * p_raster.x),
                             int(resolution.y * p_raster.y)};
            SpectrumRGB f = vertex.info->evaluateScatter(wi);
            L = vertex.beta * f * (importance / pdf);
          }
        }
      }
    }
    //* Connect the lightsource to camera path
    else if (s == 1) {
      const PathVertex &vertex = camerapath[t - 1];
      //* This vertex can connect to lightsource only when it's not a delta
      if (!vertex.delta) {
        LightSourceInfo light_info =
            scene.sampleLightSource(*vertex.info, sampler);
        sampled = PathVertex::create_light(light_info);
        sampled.pdf_fwd = sampled.pdf_light_origin(scene, &vertex);
        Ray3f shadow_ray = vertex.info->scatterRay(scene, light_info.position);
        if (!scene.occlude(shadow_ray)) {
          auto light = light_info.light;
          auto [le_weight, pdf] =
              light->evaluate(light_info, vertex.info->position);
          SpectrumRGB f = vertex.info->evaluateScatter(shadow_ray.dir);
          L = vertex.beta * f * le_weight;
          //          spdlog::info("pdf = {}, le = [{}, {}, {}]\n", pdf,
          //          le_weight.r(),
          //                       le_weight.g(), le_weight.b());
        }
      }
    }
    //* All other connect situations
    else {
      const PathVertex &camera_vertex = camerapath[t - 1],
                       &light_vertex = lightpath[s - 1];
      //* The two subpaths can only be connected if they aren't delta
      if (!camera_vertex.delta && !light_vertex.delta) {
        Vector3f camera2light = light_vertex.position - camera_vertex.position;
        float inv_dist2 = 1.f / camera2light.length2();
        camera2light = normalize(camera2light);
        L = camera_vertex.beta *
            camera_vertex.info->evaluateScatter(camera2light) *
            light_vertex.beta *
            light_vertex.info->evaluateScatter(-camera2light) * inv_dist2;
        Ray3f vis_ray{light_vertex.position, camera_vertex.position};
        L *= SpectrumRGB{scene.occlude(vis_ray) ? 0.f : 1.f};
      }
    }
    //* Apply the multiple importance sampling
    float misw = .0f;

    misw = L.isZero() ? 0
                      : mis_weight(scene, camerapath, lightpath, t, s, sampled);
    // ......

    return L * misw;
  }

public:
  BidirectionalPathTracer() : max_depth(5) {}

  BidirectionalPathTracer(const rapidjson::Value &_value) {
    max_depth = getInt("maxDepth", _value);
  }

  virtual ~BidirectionalPathTracer() = default;

  virtual SpectrumRGB getLi(const Scene &scene, Ray3f ray,
                            Sampler *sampler) const override {
    // no implementation
    std::cerr << "BidirectionalPathTracer::getLi not implement!\n";
    std::exit(1);
  }
  virtual void render(std::shared_ptr<RenderTask> task) const override {
    auto start = std::chrono::high_resolution_clock::now();

    Film film{task->film_size, 32};
    auto [x, y] = film.tile_range();

    //    std::vector<std::shared_ptr<FilmTile>> film_tiles;
    //    film_tiles.reserve(x * y);

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
                    SpectrumRGB L{.0f};
                    //* generate light subpath
                    std::vector<PathVertex> light_path(max_depth + 1);
                    int n_lightpath = generate_lightpath(
                        *scene, sampler.get(), max_depth + 1, &light_path);

                    //* generate camera subpath
                    Ray3f ray = camera->sampleRayDifferential(
                        p_pixel, task->film_size, sampler->getCameraSample());
                    std::vector<PathVertex> camera_path(max_depth + 2);
                    int n_camerapath = generate_camerapath(
                        *scene, sampler.get(), max_depth + 2, camera, ray,
                        &camera_path);

                    //* connect all light subpath vertex to camera
                    for (int t = 1; t <= n_camerapath; ++t) {
                      for (int s = 0; s <= n_lightpath; ++s) {
                        int depth = t + s - 2;
                        //* Ignore the following situations
                        if ((s == 1 && t == 1) || depth < 0 ||
                            depth > max_depth)
                          continue;
                        Point2i pixel = p_pixel;
                        SpectrumRGB L_path = connect_subpath(
                            *scene, camera, task->film_size, light_path,
                            camera_path, s, t, sampler.get(), &pixel);
                        if (t == 1) {
                          if ((0 <= pixel.x && pixel.x < task->film_size.x) &&
                              (0 <= pixel.y && pixel.y < task->film_size.y))
                            film.add_splat(pixel, L_path, 1.f / task->spp);
                        } else {
                          L += L_path;
                        }
                      }
                    }
                    sampler->nextSample();
                    //                    tile->add_sample(p_pixel, L, 1);
                    film.add_sample(p_pixel, L, 1);
                  }
                }

              //              film_tiles.emplace_back(tile);
              finished_tiles++;
              if (finished_tiles % 5 == 0) {
                printProgress((double)finished_tiles / total_tiles);
              }
            }
        });
    printProgress(1);
    //    for (auto tile : film_tiles) {
    //      film.fill_tile(tile);
    //    }
    auto end = std::chrono::high_resolution_clock::now();
    auto cost =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << tfm::format("\nRendering costs : %.2f seconds\n",
                             cost.count() / 1000.f);
    // TODO add splat_film and eye_film
    //    film.save_film(task->file_name);
    //    std::string splat_name = task->file_name + "splat.exr";
    // film.save_splat(task->file_name);
    film.save_as(task->file_name, 0);
  }

private:
  int max_depth;
};

REGISTER_CLASS(BidirectionalPathTracer, "bdpt")