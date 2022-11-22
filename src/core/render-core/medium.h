#pragma once
#include "core/math/math.h"
#include "core/math/warp.h"
#include "core/utils/configurable.h"
#include "phase.h"
#include "sampler.h"

class Scene;
struct LightSourceInfo;
struct MediumIntersectionInfo;

struct MediumSampleRecord {
  float pathLength;

  float pdf{1.f};

  bool isValid{false};

  SpectrumRGB transmittance{1.f};

  SpectrumRGB sigmaS;

  SpectrumRGB sigmaA;

  SpectrumRGB albedo;

  const Medium *medium = nullptr;
};

class Medium : public Configurable {
 public:
  Medium() = default;
  Medium(const rapidjson::Value &_value) {}
  virtual ~Medium() = default;

  virtual SpectrumRGB evaluateTr(Point3f start, Point3f end) const = 0;

  virtual SpectrumRGB Le(const Ray3f &ray) const = 0;

  virtual SpectrumRGB sigmaS(Point3f p) const = 0;

  SpectrumRGB evaluatePhase(const PhaseQueryRecord &pRec) const {
    return mPhase->evaluate(pRec);
  }

  float pdfPhase(const PhaseQueryRecord &pRec) const {
    return mPhase->pdf(pRec);
  }

  SpectrumRGB samplePhase(PhaseQueryRecord *pRec, Point2f sample) const {
    return mPhase->sample(pRec, sample);
  }

  void setPhase(std::shared_ptr<PhaseFunction> phase) { this->mPhase = phase; }

  virtual std::shared_ptr<MediumIntersectionInfo> sampleIntersection(
      Ray3f ray, float tBounds, Point2f sample) const = 0;

  virtual std::shared_ptr<MediumIntersectionInfo>
  sampleIntersectionDeterministic(Ray3f ray, float tBounds,
                                  Point2f sample) const = 0;

  std::shared_ptr<MediumIntersectionInfo> sampleIntersectionEquiAngular(
      Ray3f ray, float tBounds, Point2f sample,
      const LightSourceInfo &info) const;

 protected:
  std::shared_ptr<PhaseFunction> mPhase;
};

struct MediumIntersection {
  float distance;

  Point3f scatterPoint;

  Vector3f forwardDirection;

  Frame forwardFrame;

  std::shared_ptr<Medium> medium;

  SpectrumRGB sigmaS, sigmaA;

  SpectrumRGB albedo;
};
