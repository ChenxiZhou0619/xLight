#pragma once
#include <variant>

#include "bsdf.h"
#include "camera.h"
#include "core/scene/scene.h"
#include "core/utils/configurable.h"
#include "emitter.h"
#include "medium.h"
#include "sampler.h"

class Scene;
class RenderTask;
class ImageBlock;
class Camera;

#define PBSTR "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"
#define PBWIDTH 60

inline void printProgress(double percentage) {
  int val = (int)(percentage * 100);
  int lpad = (int)(percentage * PBWIDTH);
  int rpad = PBWIDTH - lpad;
  printf("\r%3d%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
  fflush(stdout);
}

class Integrator : public Configurable {
public:
  Integrator() = default;
  Integrator(const rapidjson::Value &_value) {}

  virtual ~Integrator() {}
  virtual SpectrumRGB getLi(const Scene &scene, Ray3f ray,
                            Sampler *sampler) const = 0;

  virtual void render(std::shared_ptr<RenderTask> task) const = 0;
};

class PixelIntegrator : public Integrator {
public:
  PixelIntegrator() = default;
  PixelIntegrator(const rapidjson::Value &_value) {}
  virtual ~PixelIntegrator() {}
  virtual SpectrumRGB getLi(const Scene &scene, Ray3f ray,
                            Sampler *sampler) const override = 0;
  virtual void render(std::shared_ptr<RenderTask> task) const override final;
};

//* structs used for bidir methods

enum class VertexType {
  CameraVertex,
  LightVertex,
  SurfaceVertex,
  MediumVertex
};

struct PathVertex {
  //* Two basic data
  /**
   * CameraVertex : The vertex is t0 vertex
   * LightVertex : Only 1. camera path hit environment(or last vertex) 2. s0
   * vertex
   * SurfaceVertex : The vertex laies on surface
   *
   */
  SpectrumRGB beta;
  VertexType type;
  //* Declare the sampling properties of current node
  bool delta = false;
  float pdf_fwd = 0, pdf_rev = 0;
  //* Used to compute the pdf for connection
  std::shared_ptr<IntersectionInfo> info = nullptr;
  std::shared_ptr<Camera> camera = nullptr;
  std::shared_ptr<Emitter> light = nullptr;
  //* Vertex geometry property
  Vector3f normal; //! All non-surface vertex should init this to zero
  Point3f position;

  PathVertex() = default;

  //* Return whether the vertex is on a surface
  bool on_surface() const { return !normal.isZero(); }

  //* Return whether the vertex is on infinite lightsource
  // TODO fixme
  bool is_inf_light() const { return false; }

  //* This function turn a pdf in solid-angle measure to unit area measure
  float convert_pdf(float pdf_solid_angle, const PathVertex &next) const {
    Vector3f w = next.position - position;
    if (w.length2() == 0)
      return 0;
    float inv_dist2 = 1.f / w.length2();
    if (next.on_surface()) {
      pdf_solid_angle *= std::abs(dot(next.normal, w * std::sqrt(inv_dist2)));
    }
    return pdf_solid_angle * inv_dist2;
  }

  //* Given the precessor node and successor node, compute the pdf of sampling
  //* such path
  float pdf(const Scene &scene, const PathVertex *prev,
            const PathVertex *next) const {
    if (type == VertexType::LightVertex) {
      return pdf_light(scene, next);
    }

    Vector3f wn = next->position - position;
    if (wn.length2() == 0)
      return 0;
    wn = normalize(wn);

    Vector3f wp;
    if (prev) {
      wp = prev->position - position;
      if (wp.length2() == 0)
        return 0;
      wp = normalize(wp);
    }

    float pdf = 0, unused;
    if (type == VertexType::CameraVertex) {
      camera->pdfWe(Ray3f{camera->get_position(), wn}, &unused, &pdf);
    } else if (type == VertexType::SurfaceVertex) {
      pdf = info->pdfScatter(wp, wn);
    } else if (type == VertexType::MediumVertex) {
      std::cout << "No medium now!";
      std::exit(1);
    }
    return convert_pdf(pdf, *next);
  }

  float pdf_light(const Scene &scene, const PathVertex *vertex) const {
    Vector3f w = vertex->position - position;
    float inv_dist2 = 1 / w.length2();
    float pdf;
    w = normalize(w);
    if (is_inf_light()) {
      // TODO
      std::cout << "No implementation for bdpt inf light\n";
      std::exit(1);
    } else {
      float pdf_dir = std::abs(dot(normal, w));
      pdf = pdf_dir * inv_dist2;
    }
    if (vertex->on_surface())
      pdf *= std::abs(dot(vertex->normal, w));
    return pdf;
  }

  //* The vertex is the last vertex on camera path
  float pdf_light_origin(const Scene &scene, const PathVertex *prev) const {
    Vector3f w = prev->position - position;
    if (w.length2() == 0)
      return 0;
    w = normalize(w);
    if (is_inf_light()) {
      std::cout << "No implementation for bdpt inf light\n";
      std::exit(1);
    } else {
      auto shape_ptr = light->shape.lock();
      float pdf_choice = scene.pdfEmitter(light);
      return pdf_choice / shape_ptr->getSurfaceArea();
    }
  }

  static PathVertex create_light(const LightSourceInfo &info) {
    PathVertex res;
    res.type = VertexType::LightVertex;
    res.beta = info.Le;
    res.pdf_fwd = info.pdf;
    res.position = info.position;
    res.normal = info.normal;
    res.light = info.light;
    return res;
  }

  static PathVertex create_surface(std::shared_ptr<IntersectionInfo> info,
                                   SpectrumRGB beta, float pdf_fwd,
                                   const PathVertex &prev) {
    PathVertex res;
    res.type = VertexType::SurfaceVertex;
    res.info = info;
    res.beta = beta;
    res.position = info->position;
    res.normal = ((SurfaceIntersectionInfo *)info.get())->geometryNormal;
    res.pdf_fwd = prev.convert_pdf(pdf_fwd, res);
    return res;
  }

  static PathVertex create_camera(std::shared_ptr<Camera> camera,
                                  const Ray3f &ray, SpectrumRGB beta) {
    PathVertex result;
    result.camera = camera;
    result.beta = beta;
    result.normal = Vector3f(0);
    result.position = camera->get_position();
    return result;
  }

  SpectrumRGB evaluate_le(const Scene &scene, const PathVertex &next) const {
    return info->evaluateLe();
  }
};

class FilmIntegrator : public Integrator {
public:
  FilmIntegrator() = default;
  FilmIntegrator(const rapidjson::Value &_value) {}
  virtual ~FilmIntegrator() = default;
  virtual SpectrumRGB getLi(const Scene &scene, Ray3f ray,
                            Sampler *sampler) const override = 0;
  virtual void render(std::shared_ptr<RenderTask> task) const override;
};
