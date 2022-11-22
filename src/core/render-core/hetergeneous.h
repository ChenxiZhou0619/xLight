#pragma once

#include <openvdb/openvdb.h>

#include "core/render-core/medium.h"

class Hetergeneous : public Medium {
 public:
  Hetergeneous() = default;

  Hetergeneous(const rapidjson::Value &_value) {
    // donothing
    // mPhase = std::make_shared<IsotropicPhase>();
  }

  Hetergeneous(openvdb::FloatGrid::Ptr _density, float scale);

  virtual SpectrumRGB evaluateTr(Point3f start, Point3f end) const override;

  virtual SpectrumRGB Le(const Ray3f &ray) const override {
    return SpectrumRGB(0.0);
  }

  virtual ~Hetergeneous() = default;

  virtual std::shared_ptr<MediumIntersectionInfo> sampleIntersection(
      Ray3f ray, float tBounds, Point2f sample) const override;

  virtual std::shared_ptr<MediumIntersectionInfo>
  sampleIntersectionDeterministic(Ray3f ray, float tBounds,
                                  Point2f sample) const override;

  virtual SpectrumRGB sigmaS(Point3f) const override;

 protected:
  openvdb::FloatGrid::Ptr density;

  SpectrumRGB sigmaTMax{.0f};

  float scale;

  float step = 0.1;  // delete
};