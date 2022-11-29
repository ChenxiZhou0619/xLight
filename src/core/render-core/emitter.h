#pragma once
#include <optional>

#include "core/mesh/mesh.h"
#include "core/render-core/texture.h"
#include "core/utils/configurable.h"

class Emitter;
class ShapeInterface;
struct LightSourceInfo;
struct SurfaceIntersectionInfo;
struct IntersectionInfo;

struct PointQueryRecord {
  Point3f p;
  Normal3f normal;
  float pdf;
  // TODO delete this
  const Mesh *mesh;
  const Emitter *emitter;

  std::shared_ptr<ShapeInterface> shape;
};

struct EmitterQueryRecord {
  PointQueryRecord pRec;
  Ray3f ray;

  EmitterQueryRecord() = default;
  EmitterQueryRecord(const PointQueryRecord &_pRec, const Ray3f &_ray)
      : pRec(_pRec), ray(_ray) {}

  const Emitter *getEmitter() const {
    if (pRec.mesh == nullptr)
      return pRec.emitter;
    return pRec.mesh->getEmitter();
  }
};

struct DirectIlluminationRecord {
  enum class EmitterType {
    EArea = 0,
    EEnvironment,
    ESpot,
  } emitter_type;
  Ray3f shadow_ray;
  SpectrumRGB energy;
  float pdf;
  bool isDelta = false;
};

struct EmitterHitInfo {
  float dist;
  Point3f hitpoint;
  Normal3f normal;
  Vector3f dir;
};

// TODO, setTexture method, and maybe Emitter holds a mesh / entity is a good
// choice
class Emitter : public Configurable {
public:
  Emitter() = default;
  Emitter(const rapidjson::Value &_value);
  virtual ~Emitter() = default;

  virtual void initialize() {
    // do nothing
  }

  //* return the LeWeight and pdf with respect to the destination
  virtual std::pair<SpectrumRGB, float> evaluate(const LightSourceInfo &info,
                                                 Point3f destination) const = 0;

  virtual SpectrumRGB
  evaluate(const SurfaceIntersectionInfo &itsInfo) const = 0;

  virtual float pdf(const SurfaceIntersectionInfo &info) const = 0;

  virtual LightSourceInfo sampleLightSource(const IntersectionInfo &info,
                                            Point3f sample) const = 0;

  virtual LightSourceInfo sampleLightSource(Point3f sample) const = 0;

  virtual void setTexture(Texture *envmap) = 0;

  virtual float pdf(const EmitterHitInfo &info) const = 0;

  virtual void pdf_le(Ray3f ray, Normal3f light_normal, float *pdf_pos,
                      float *pdf_dir) const = 0;

  std::weak_ptr<ShapeInterface> shape;
};