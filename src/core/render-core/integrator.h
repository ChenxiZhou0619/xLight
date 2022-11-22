#pragma once
#include <variant>

#include "bsdf.h"
#include "core/scene/scene.h"
#include "core/utils/configurable.h"
#include "emitter.h"
#include "medium.h"
#include "sampler.h"

class Scene;
class RenderTask;
class ImageBlock;

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

class FilmIntegrator : public Integrator {
 public:
  FilmIntegrator() = default;
  FilmIntegrator(const rapidjson::Value &_value) {}
  virtual ~FilmIntegrator() = default;
  virtual SpectrumRGB getLi(const Scene &scene, Ray3f ray,
                            Sampler *sampler) const override = 0;
  virtual void render(std::shared_ptr<RenderTask> task) const override;
};
