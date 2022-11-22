#pragma once
#include <embree3/rtcore.h>

#include <memory>
#include <stack>

#include "camera.h"
#include "core/accelerate/accel.h"
#include "core/geometry/geometry.h"
#include "core/math/discretepdf.h"
#include "core/mesh/meshLoader.h"
#include "emitter.h"
#include "texture.h"

class Scene {
 public:
  Scene() = default;

  Scene(MeshSet *_meshSetPtr)
      : accelPtr(std::make_unique<Accel>(_meshSetPtr)) {}

  void preprocess();

  bool rayIntersect(const Ray3f &ray) const;

  bool rayIntersect(const Ray3f &ray, RayIntersectionRec &iRec) const;

  // TODO, old method
  void sampleEmitterOnSurface(PointQueryRecord &pRec, Sampler *sampler) const;

  void setEnvMap(Emitter *env_emitter);

  SpectrumRGB evaluateEnvironment(const Ray3f &ray) const;

  float pdfEnvironment(const Ray3f &ray) const;

  float pdfArea(const RayIntersectionRec &i_rec, const Ray3f &ray) const;

  void sampleDirectIllumination(DirectIlluminationRecord *d_rec,
                                Sampler *sampler, Point3f from) const;

  void sampleAttenuatedDirectIllumination(
      DirectIlluminationRecord *d_rec, Sampler *sampler, Point3f from,
      SpectrumRGB *transmittance, const std::stack<Medium *> cur_stack) const;

  SpectrumRGB getTransmittance(const std::stack<Medium *> &medium_stack,
                               Point3f from, Point3f to) const;

 private:
  std::unique_ptr<Accel> accelPtr;

  std::vector<Mesh *> emitterList;

  Distribution1D emitterDistribution;

  float emitterSurfaceArea;

  Emitter *m_env_emitter = nullptr;
};
