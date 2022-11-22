#pragma once
#include <core/render-core/info.h>
#include <core/shape/shape.h>
#include <embree3/rtcore.h>

#include <memory>
#include <optional>
#include <stack>
#include <variant>
#include <vector>

#include "core/render-core/medium.h"

using Intersection = std::variant<ShapeIntersection, MediumIntersection>;
class Scene {
 public:
  Scene();

  ~Scene() = default;

  void addShape(std::shared_ptr<ShapeInterface> shape);

  void addEmitter(std::shared_ptr<Emitter> emitter);

  void postProcess();

  std::optional<ShapeIntersection> intersect(const Ray3f &ray) const;

  bool occlude(const Ray3f &ray) const;

  void setEnvMap(std::shared_ptr<Emitter> _environment) {
    environment = _environment;
  }

  bool hasEnvironment() const { return environment != nullptr; }

  std::shared_ptr<Medium> getEnvMedium() const { return envMedium; }

  void setEnvMedium(std::shared_ptr<Medium> medium) { envMedium = medium; }

  float pdfEmitter(std::shared_ptr<Emitter> emitter) const;

  std::shared_ptr<Emitter> getEnvEmitter() const { return environment; }

 private:
  //* Embree
  RTCDevice device;
  RTCScene scene;

  int shapeCount = 0;
  std::shared_ptr<Emitter> environment;
  std::vector<std::shared_ptr<ShapeInterface>> shapes;
  std::shared_ptr<Medium> envMedium;

  std::vector<std::shared_ptr<Emitter>> emitters;
  Distrib1D<std::shared_ptr<Emitter>> lightDistrib;

 public:
  std::shared_ptr<SurfaceIntersectionInfo> intersectWithSurface(
      const Ray3f &ray) const;
  LightSourceInfo sampleLightSource(const IntersectionInfo &info,
                                    Sampler *sampler) const;
  LightSourceInfo sampleLightSource(Sampler *sampler) const;
};