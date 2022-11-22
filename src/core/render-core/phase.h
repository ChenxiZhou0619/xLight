#pragma once
#include "core/math/math.h"
#include "core/math/warp.h"
#include "core/utils/configurable.h"
#include "sampler.h"

struct PhaseQueryRecord {
  Point3f scatterPoint;

  Vector3f wi, wo;

  Frame localFrame;

  float pdf;

  bool isDelta = false;

  PhaseQueryRecord() = default;

  PhaseQueryRecord(Point3f from, Vector3f _wi) {
    scatterPoint = from;
    localFrame = Frame{_wi};
    wi = localFrame.toLocal(_wi);
  }

  PhaseQueryRecord(Point3f from, Vector3f _wi, Vector3f _wo) {
    scatterPoint = from;
    localFrame = Frame{_wi};
    wi = localFrame.toLocal(_wi);
    wo = localFrame.toLocal(_wo);
  }
};

class PhaseFunction : public Configurable {
 public:
  PhaseFunction() = default;

  PhaseFunction(const rapidjson::Value &_value) {}

  virtual ~PhaseFunction() = default;

  virtual SpectrumRGB evaluate(const PhaseQueryRecord &pRec) const = 0;

  virtual float pdf(const PhaseQueryRecord &pRec) const = 0;

  virtual SpectrumRGB sample(PhaseQueryRecord *pRec, Point2f sample) const = 0;
};